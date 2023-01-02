#include "probfd/heuristics/pdbs/probabilistic_projection.h"

#include "probfd/analysis_objectives/goal_probability_objective.h"

#include "probfd/task_utils/task_properties.h"

#include <algorithm>
#include <deque>
#include <numeric>
#include <set>

namespace probfd {
namespace heuristics {
namespace pdbs {

namespace {
// Footprint used for detecting duplicate operators.
struct ProgressionOperatorFootprint {
    long long int precondition_hash;
    std::vector<WeightedElement<StateRank>> successors;

    ProgressionOperatorFootprint(
        long long int precondition_hash,
        const AbstractOperator& op)
        : precondition_hash(precondition_hash)
        , successors(op.outcomes.begin(), op.outcomes.end())
    {
        std::sort(successors.begin(), successors.end());
    }

    friend bool operator<(
        const ProgressionOperatorFootprint& a,
        const ProgressionOperatorFootprint& b)
    {
        return std::tie(a.precondition_hash, a.successors) <
               std::tie(b.precondition_hash, b.successors);
    }

    friend bool operator==(
        const ProgressionOperatorFootprint& a,
        const ProgressionOperatorFootprint& b)
    {
        return std::tie(a.precondition_hash, a.successors) ==
               std::tie(b.precondition_hash, b.successors);
    }
};

struct OutcomeInfo {
    StateRank base_effect = StateRank(0);
    std::vector<int> missing_pres;

    friend bool operator==(const OutcomeInfo& a, const OutcomeInfo& b)
    {
        return std::tie(a.base_effect, a.missing_pres) ==
               std::tie(b.base_effect, b.missing_pres);
    }
};

} // namespace

ProbabilisticProjection::StateRankSpace::StateRankSpace(
    const ProbabilisticTaskProxy& task_proxy,
    const StateRankingFunction& mapper,
    bool operator_pruning)
    : initial_state_(mapper.rank(task_proxy.get_initial_state()))
    , match_tree_(task_proxy, mapper.get_pattern(), mapper)
    , goal_state_flags_(mapper.num_states())
{
    VariablesProxy variables = task_proxy.get_variables();
    ProbabilisticOperatorsProxy operators = task_proxy.get_operators();
    abstract_operators_.reserve(operators.size());

    std::set<ProgressionOperatorFootprint> duplicate_set;

    std::vector<int> pdb_indices(variables.size(), -1);

    for (size_t i = 0; i < mapper.get_pattern().size(); ++i) {
        pdb_indices[mapper.get_pattern()[i]] = i;
    }

    // Generate the abstract operators for each probabilistic operator
    for (const ProbabilisticOperatorProxy& op : operators) {
        const int operator_id = op.get_id();
        const int reward = op.get_reward();

        // Precondition partial state and partial state to enumerate
        // effect values not appearing in precondition
        std::vector<FactPair> local_precondition;

        for (FactProxy fact : op.get_preconditions()) {
            const int pre_var = fact.get_variable().get_id();
            const int pdb_index = pdb_indices[pre_var];

            if (pdb_index != -1) {
                local_precondition.emplace_back(pdb_index, fact.get_value());
            }
        }

        std::vector<FactPair> vars_eff_not_pre;

        // Info about each probabilistic outcome
        Distribution<OutcomeInfo> outcomes;

        // Collect info about the outcomes
        for (const ProbabilisticOutcomeProxy& out : op.get_outcomes()) {
            OutcomeInfo info;

            std::vector<FactPair> local_effect;

            for (ProbabilisticEffectProxy effect : out.get_effects()) {
                FactProxy fact = effect.get_fact();
                const int eff_var = fact.get_variable().get_id();
                const int pdb_index = pdb_indices[eff_var];
                if (pdb_index != -1) {
                    local_effect.emplace_back(pdb_index, fact.get_value());
                }
            }

            for (const auto& [var, val] : local_effect) {
                auto beg = utils::make_transform_iterator(
                    local_precondition.begin(),
                    &FactPair::var);
                auto end = utils::make_transform_iterator(
                    local_precondition.end(),
                    &FactPair::var);
                auto pre_it = utils::find_sorted(beg, end, var);

                int val_change = val;

                if (pre_it == local_precondition.end()) {
                    vars_eff_not_pre.emplace_back(var, 0);
                    info.missing_pres.push_back(var);
                } else {
                    val_change -= pre_it.base->value;
                }

                info.base_effect += mapper.from_fact(var, val_change);
            }

            outcomes.add_unique(std::move(info), out.get_probability());
        }

        utils::sort_unique(vars_eff_not_pre);

        // We enumerate all values for variables that are not part of
        // the precondition but in an effect. Depending on the value of the
        // variable, the value change caused by the abstract operator would be
        // different, hence we generate on operator for each state where
        // enabled.
        auto ran = mapper.partial_assignments(std::move(vars_eff_not_pre));

        for (const std::vector<FactPair>& values : ran) {
            // Generate the progression operator
            AbstractOperator new_op(operator_id, reward);

            for (const auto& [info, prob] : outcomes) {
                const auto& [base_effect, missing_pres] = info;
                auto a = mapper.from_values_partial(missing_pres, values);

                new_op.outcomes.add_unique(base_effect - a, prob);
            }

            // Construct the precondition by merging the original precondition
            // partial state with the partial state for the non-precondition
            // effects of this iteration
            std::vector<FactPair> precondition;
            precondition.reserve(local_precondition.size() + values.size());

            std::merge(
                local_precondition.begin(),
                local_precondition.end(),
                values.begin(),
                values.end(),
                std::back_inserter(precondition));

            if (operator_pruning) {
                // Generate a hash for the precondition to check for duplicates
                const long long int pre_hash =
                    mapper.get_unique_partial_state_id(precondition);
                if (!duplicate_set.emplace(pre_hash, new_op).second) {
                    continue;
                }
            }

            // Now add the progression operators to the match tree
            match_tree_.insert(abstract_operators_.size(), precondition);
            abstract_operators_.emplace_back(std::move(new_op));
        }
    }

    match_tree_.set_first_aop(abstract_operators_.data());

    setup_abstract_goal(task_proxy, mapper);
}

void ProbabilisticProjection::StateRankSpace::setup_abstract_goal(
    const ProbabilisticTaskProxy& task_proxy,
    const StateRankingFunction& mapper)
{
    GoalsProxy task_goals = task_proxy.get_goals();

    std::vector<int> non_goal_vars;
    StateRank base(0);

    // Translate sparse goal into pdb index space
    // and collect non-goal variables aswell.
    const Pattern& variables = mapper.get_pattern();

    const int num_goal_facts = task_goals.size();
    const int num_variables = variables.size();

    for (int v = 0, w = 0; v != static_cast<int>(variables.size());) {
        const int p_var = variables[v];
        const FactProxy goal_fact = task_goals[w];
        const int g_var = goal_fact.get_variable().get_id();

        if (p_var < g_var) {
            non_goal_vars.push_back(v++);
        } else {
            if (p_var == g_var) {
                const int g_val = goal_fact.get_value();
                base.id += mapper.get_multiplier(v++) * g_val;
            }

            if (++w == num_goal_facts) {
                while (v < num_variables) {
                    non_goal_vars.push_back(v++);
                }
                break;
            }
        }
    }

    assert(non_goal_vars.size() != variables.size()); // No goal no fun.

    auto goals = mapper.state_ranks(base, std::move(non_goal_vars));

    for (const auto& g : goals) {
        goal_state_flags_[g.id] = true;
    }
}

bool ProbabilisticProjection::StateRankSpace::is_goal(const StateRank& s) const
{
    return goal_state_flags_[s.id];
}

ProbabilisticProjection::ProbabilisticProjection(
    const ProbabilisticTaskProxy& task_proxy,
    const Pattern& pattern,
    bool operator_pruning,
    value_type::value_t fill)
    : ProbabilisticProjection(
          task_proxy,
          new StateRankingFunction(task_proxy, pattern),
          operator_pruning,
          fill)
{
}

ProbabilisticProjection::ProbabilisticProjection(
    const ProbabilisticTaskProxy& task_proxy,
    StateRankingFunction* mapper,
    bool operator_pruning,
    value_type::value_t fill)
    : state_mapper_(mapper)
    , abstract_state_space_(task_proxy, *state_mapper_, operator_pruning)
    , value_table(state_mapper_->num_states(), fill)
{
    ::task_properties::verify_no_axioms(task_proxy);
    task_properties::verify_no_conditional_effects(task_proxy);
}

std::shared_ptr<StateRankingFunction>
ProbabilisticProjection::get_abstract_state_mapper() const
{
    return state_mapper_;
}

unsigned int ProbabilisticProjection::num_states() const
{
    return state_mapper_->num_states();
}

bool ProbabilisticProjection::is_dead_end(const State& s) const
{
    return is_dead_end(get_abstract_state(s));
}

bool ProbabilisticProjection::is_dead_end(const StateRank& s) const
{
    return utils::contains(dead_ends_, StateID(s.id));
}

bool ProbabilisticProjection::is_goal(const StateRank& s) const
{
    return abstract_state_space_.is_goal(s);
}

value_type::value_t ProbabilisticProjection::lookup(const State& s) const
{
    return lookup(get_abstract_state(s));
}

value_type::value_t ProbabilisticProjection::lookup(const StateRank& s) const
{
    return value_table[s.id];
}

StateRank ProbabilisticProjection::get_abstract_state(const State& s) const
{
    return state_mapper_->rank(s);
}

StateRank
ProbabilisticProjection::get_abstract_state(const std::vector<int>& s) const
{
    return state_mapper_->rank(s);
}

const Pattern& ProbabilisticProjection::get_pattern() const
{
    return state_mapper_->get_pattern();
}

} // namespace pdbs
} // namespace heuristics
} // namespace probfd
