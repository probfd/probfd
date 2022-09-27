#include "regrouped_operator_counting_heuristic.h"

#include "../../../global_operator.h"
#include "../../../globals.h"
#include "../../../option_parser.h"
#include "../../../plugin.h"
#include "../../../utils/system.h"
#include "../../../utils/timer.h"
#include "../../analysis_objectives/expected_cost_objective.h"
#include "../../analysis_objectives/goal_probability_objective.h"
#include "../../globals.h"
#include "../../probabilistic_operator.h"

#include <algorithm>
#include <cassert>
#include <memory>

namespace probabilistic {
namespace occupation_measure_heuristic {

namespace {

template <typename T>
std::vector<int> make_explicit(const std::vector<T>& partial_state)
{
    std::vector<int> pre(g_variable_domain.size(), -1);

    for (const auto& [var, val] : partial_state) {
        pre[var] = val;
    }

    return pre;
}

// Explicit precondition values (-1 if no precondition for variable)
std::vector<int> get_precondition_explicit(const ProbabilisticOperator& op)
{
    return make_explicit(op.get_preconditions());
}

// Explicit goal values (-1 if not a goal variable)
std::vector<int> get_goal_explicit()
{
    return make_explicit(g_goal);
}

} // namespace

RegroupedOperatorCountingHeuristic::RegroupedOperatorCountingHeuristic(
    const options::Options& opts)
    : lp_solver_(lp::LPSolverType(opts.get_enum("lpsolver")))
    , is_maxprob(
          std::dynamic_pointer_cast<
              analysis_objectives::GoalProbabilityObjective>(
              g_analysis_objective) != nullptr)
{
    ::verify_no_axioms_no_conditional_effects();

    std::cout << "Initializing regrouped operator counting heuristic..."
              << std::endl;

    utils::Timer timer;

    if (is_maxprob) {
        load_maxprob_lp();
    } else {
        load_expcost_lp();
    }

    std::cout << "Finished ROC LP setup after " << timer << std::endl;
}

EvaluationResult
RegroupedOperatorCountingHeuristic::evaluate(const GlobalState& state) const
{
    if (is_maxprob) {
        for (unsigned i = 0; i < g_goal.size(); ++i) {
            if (state[g_goal[i].first] == g_goal[i].second) {
                lp_solver_.set_constraint_lower_bound(i, -1);
            }
        }
        for (unsigned var = 0; var < g_variable_domain.size(); ++var) {
            lp_solver_.set_constraint_lower_bound(
                constraint_offsets_[var] + state[var],
                -1);
        }
        lp_solver_.solve();
        EvaluationResult res(true, 0.0);
        if (lp_solver_.has_optimal_solution()) {
            const double v = lp_solver_.get_objective_value();
            res = EvaluationResult(v == 0.0, v);
        }
        for (unsigned i = 0; i < g_goal.size(); ++i) {
            if (state[g_goal[i].first] == g_goal[i].second) {
                lp_solver_.set_constraint_lower_bound(i, 0);
            }
        }
        for (unsigned var = 0; var < g_variable_domain.size(); ++var) {
            lp_solver_.set_constraint_lower_bound(
                constraint_offsets_[var] + state[var],
                0);
        }
        return res;
    } else {
        const auto goal_explicit = get_goal_explicit();

        std::vector<int> reset;
        reset.reserve(2 * g_variable_domain.size());

        for (size_t var = 0; var < g_variable_domain.size(); ++var) {
            const int g_val = goal_explicit[var];
            if (g_val == -1) {
                const int idx_state_val = constraint_offsets_[var] + state[var];
                lp_solver_.set_constraint_lower_bound(idx_state_val, -1);
                reset.push_back(idx_state_val);
            } else if (state[var] != goal_explicit[var]) {
                const int idx_state_val = constraint_offsets_[var] + state[var];
                const int idx_g_val = constraint_offsets_[var] + g_val;
                lp_solver_.set_constraint_lower_bound(idx_state_val, -1);
                lp_solver_.set_constraint_lower_bound(idx_g_val, 1);
                reset.push_back(idx_state_val);
                reset.push_back(idx_g_val);
            }
        }

        lp_solver_.solve();
        assert(lp_solver_.has_optimal_solution());
        const double v = -lp_solver_.get_objective_value();
        EvaluationResult res(false, v);

        for (int idx : reset) {
            lp_solver_.set_constraint_lower_bound(idx, 0.0);
        }

        return res;
    }
}

void RegroupedOperatorCountingHeuristic::add_options_to_parser(
    options::OptionParser& parser)
{
    lp::add_lp_solver_option_to_parser(parser);
}

void RegroupedOperatorCountingHeuristic::load_maxprob_lp()
{
    const double inf = lp_solver_.get_infinity();
    std::vector<lp::LPVariable> lp_vars;
    std::vector<lp::LPConstraint> constraints;
    constraint_offsets_.resize(g_variable_domain.size(), g_goal.size());
    for (unsigned var = 1; var < g_variable_domain.size(); ++var) {
        constraint_offsets_[var] =
            constraint_offsets_[var - 1] + g_variable_domain[var - 1];
    }
    constraints.resize(
        g_goal.size() + constraint_offsets_.back() + g_variable_domain.back(),
        lp::LPConstraint(0, inf));
    lp_vars.emplace_back(0, 1, 1);

    std::vector<int> goal(g_variable_domain.size(), -1);
    for (unsigned i = 0; i < g_goal.size(); ++i) {
        goal[g_goal[i].first] = i;
        constraints[i].insert(0, -1);
    }

    std::vector<int> pre(g_variable_domain.size());
    for (unsigned op_num = 0; op_num < g_operators.size(); ++op_num) {
        const ProbabilisticOperator& op = g_operators[op_num];
        int lp_var_start = lp_vars.size();

        std::fill(pre.begin(), pre.end(), -1);
        for (unsigned i = 0; i < op.get_preconditions().size(); ++i) {
            const auto& f = op.get_preconditions()[i];
            pre[f.var] = f.val;
        }

        for (unsigned out_num = 0; out_num < op.num_outcomes(); ++out_num) {
            const GlobalOperator& outcome = *(op[out_num].op);
            const int lp_var = lp_vars.size();
            lp_vars.emplace_back(0, inf, 0);
            for (unsigned i = 0; i < outcome.get_effects().size(); ++i) {
                const auto& eff = outcome.get_effects()[i];
                constraints[constraint_offsets_[eff.var] + eff.val].insert(
                    lp_var,
                    1);
                if (pre[eff.var] != -1) {
                    constraints[constraint_offsets_[eff.var] + pre[eff.var]]
                        .insert(lp_var, -1);
                }
                if (goal[eff.var] != -1) {
                    const int goal_val = g_goal[goal[eff.var]].second;
                    if (goal_val == eff.val) {
                        constraints[goal[eff.var]].insert(lp_var, 1);
                    } else if (pre[eff.var] == goal_val) {
                        constraints[goal[eff.var]].insert(lp_var, -1);
                    }
                }
            }
        }

        for (int i = op.num_outcomes() - 1, lp_var = lp_vars.size() - 1;
             lp_var > lp_var_start;
             --lp_var, --i) {
            constraints.emplace_back(0, 0);
            constraints.back().insert(lp_var_start, 1.0 / op[0].prob);
            constraints.back().insert(lp_var, -1.0 / op[i].prob);
        }
    }

    lp_solver_.load_problem(
        lp::LPObjectiveSense::MAXIMIZE,
        lp_vars,
        constraints);
}

void RegroupedOperatorCountingHeuristic::load_expcost_lp()
{
    const double inf = lp_solver_.get_infinity();
    std::vector<lp::LPVariable> lp_vars;
    std::vector<lp::LPConstraint> constraints;

    // Two constraints for every projection state
    constraint_offsets_.resize(g_variable_domain.size(), 0);
    for (unsigned var = 1; var < g_variable_domain.size(); ++var) {
        constraint_offsets_[var] =
            constraint_offsets_[var - 1] + g_variable_domain[var - 1];
    }

    constraints.resize(
        constraint_offsets_.back() + g_variable_domain.back(),
        lp::LPConstraint(0.0, inf));

    for (const ProbabilisticOperator& op : g_operators) {
        const int cost = op.get_cost();
        const std::vector<int> pre = get_precondition_explicit(op);
        const int lp_var_start = lp_vars.size();

        for (const ProbabilisticOutcome& o : op) {
            const GlobalOperator& outcome = *o.op;
            const int lp_var = lp_vars.size();
            lp_vars.emplace_back(0, inf, cost);

            for (const auto& eff : outcome.get_effects()) {
                const int var = eff.var;
                const int val = eff.val;

                const int offset = constraint_offsets_[var];

                // Always produces / Sometimes produces
                constraints[offset + val].insert(lp_var, 1);

                if (pre[var] != -1) {
                    // Always consumes
                    constraints[offset + pre[var]].insert(lp_var, -1);
                }
            }
        }

        // Regrouping contraints
        for (int lp_var = lp_var_start + 1; lp_var < (int)lp_vars.size();
             ++lp_var) {
            int i = lp_var - lp_var_start;

            lp::LPConstraint& regroup = constraints.emplace_back(0, 0);
            regroup.insert(lp_var_start, 1.0 / op[0].prob);
            regroup.insert(lp_var, -1.0 / op[i].prob);
        }
    }

    lp_solver_.load_problem(
        lp::LPObjectiveSense::MINIMIZE,
        lp_vars,
        constraints);
}

static Plugin<GlobalStateEvaluator> _plugin(
    "hroc",
    options::parse<GlobalStateEvaluator, RegroupedOperatorCountingHeuristic>);

} // namespace occupation_measure_heuristic
} // namespace probabilistic
