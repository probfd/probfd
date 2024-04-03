#include "probfd/pdbs/cegar/cegar.h"

#include "probfd/pdbs/cegar/flaw.h"
#include "probfd/pdbs/cegar/flaw_finding_strategy.h"

#include "probfd/pdbs/policy_extraction.h"
#include "probfd/pdbs/probability_aware_pattern_database.h"
#include "probfd/pdbs/projection_state_space.h"
#include "probfd/pdbs/utils.h"

#include "probfd/multi_policy.h"
#include "probfd/task_proxy.h"

#include "downward/utils/collections.h"
#include "downward/utils/countdown_timer.h"
#include "downward/utils/logging.h"
#include "downward/utils/math.h"
#include "downward/utils/rng.h"

#include "downward/plugins/plugin.h"

#include <cassert>
#include <ostream>
#include <ranges>
#include <span>
#include <utility>

using namespace std;

namespace probfd::pdbs::cegar {

CEGARResult::~CEGARResult() = default;

/*
 * Implementation notes: The state space needs to be kept to find flaws in the
 * policy. Since it exists anyway, the algorithm is also a producer of
 * projection state spaces, not only of PDBs. Hence, the heap allocation to
 * return it to the user, in case he needs it, e.g. to compute the saturated
 * costs of the PDB.
 */
class CEGAR::PDBInfo {
    std::unique_ptr<ProjectionStateSpace> state_space;
    StateRank initial_state;
    std::unique_ptr<ProbabilityAwarePatternDatabase> pdb;
    std::unique_ptr<ProjectionMultiPolicy> policy;

public:
    PDBInfo(
        ProbabilisticTaskProxy task_proxy,
        StateRankingFunction ranking_function,
        FDRSimpleCostFunction& task_cost_function,
        utils::RandomNumberGenerator& rng,
        bool wildcard,
        utils::CountdownTimer& timer);

    PDBInfo(
        ProbabilisticTaskProxy task_proxy,
        StateRankingFunction ranking_function,
        FDRSimpleCostFunction& task_cost_function,
        utils::RandomNumberGenerator& rng,
        const ProbabilityAwarePatternDatabase& previous,
        int add_var,
        bool wildcard,
        utils::CountdownTimer& timer);

    PDBInfo(
        ProbabilisticTaskProxy task_proxy,
        StateRankingFunction ranking_function,
        FDRSimpleCostFunction& task_cost_function,
        utils::RandomNumberGenerator& rng,
        const ProbabilityAwarePatternDatabase& merge_left,
        const ProbabilityAwarePatternDatabase& merge_right,
        bool wildcard,
        utils::CountdownTimer& timer);

    [[nodiscard]]
    const Pattern& get_pattern() const;

    [[nodiscard]]
    const ProjectionStateSpace& get_mdp() const;

    [[nodiscard]]
    const ProbabilityAwarePatternDatabase& get_pdb() const;

    [[nodiscard]]
    const ProjectionMultiPolicy& get_policy() const;

    [[nodiscard]]
    value_t get_policy_cost(const State& state) const;

    std::unique_ptr<ProjectionStateSpace> extract_state_space();
    std::unique_ptr<ProbabilityAwarePatternDatabase> extract_pdb();

    [[nodiscard]]
    bool is_solvable() const;

    [[nodiscard]]
    bool is_goal(StateRank rank) const;
};

CEGAR::PDBInfo::PDBInfo(
    ProbabilisticTaskProxy task_proxy,
    StateRankingFunction ranking_function,
    FDRSimpleCostFunction& task_cost_function,
    utils::RandomNumberGenerator& rng,
    bool wildcard,
    utils::CountdownTimer& timer)
    : state_space(new ProjectionStateSpace(
          task_proxy,
          task_cost_function,
          ranking_function,
          false,
          timer.get_remaining_time()))
    , initial_state(
          ranking_function.get_abstract_rank(task_proxy.get_initial_state()))
    , pdb(new ProbabilityAwarePatternDatabase(
          *state_space,
          std::move(ranking_function),
          initial_state,
          heuristics::ConstantEvaluator<StateRank>(0_vt),
          timer.get_remaining_time()))
    , policy(compute_optimal_projection_policy(
          *state_space,
          pdb->get_value_table(),
          initial_state,
          rng,
          wildcard))
{
}

CEGAR::PDBInfo::PDBInfo(
    ProbabilisticTaskProxy task_proxy,
    StateRankingFunction ranking_function,
    FDRSimpleCostFunction& task_cost_function,
    utils::RandomNumberGenerator& rng,
    const ProbabilityAwarePatternDatabase& previous,
    int add_var,
    bool wildcard,
    utils::CountdownTimer& timer)
    : state_space(new ProjectionStateSpace(
          task_proxy,
          task_cost_function,
          ranking_function,
          false,
          timer.get_remaining_time()))
    , initial_state(
          ranking_function.get_abstract_rank(task_proxy.get_initial_state()))
    , pdb(new ProbabilityAwarePatternDatabase(
          *state_space,
          std::move(ranking_function),
          initial_state,
          previous,
          add_var,
          timer.get_remaining_time()))
    , policy(compute_optimal_projection_policy(
          *state_space,
          pdb->get_value_table(),
          initial_state,
          rng,
          wildcard))
{
}

CEGAR::PDBInfo::PDBInfo(
    ProbabilisticTaskProxy task_proxy,
    StateRankingFunction ranking_function,
    FDRSimpleCostFunction& task_cost_function,
    utils::RandomNumberGenerator& rng,
    const ProbabilityAwarePatternDatabase& left,
    const ProbabilityAwarePatternDatabase& right,
    bool wildcard,
    utils::CountdownTimer& timer)
    : state_space(new ProjectionStateSpace(
          task_proxy,
          task_cost_function,
          ranking_function,
          false,
          timer.get_remaining_time()))
    , initial_state(
          ranking_function.get_abstract_rank(task_proxy.get_initial_state()))
    , pdb(new ProbabilityAwarePatternDatabase(
          *state_space,
          std::move(ranking_function),
          initial_state,
          left,
          right,
          timer.get_remaining_time()))
    , policy(compute_optimal_projection_policy(
          *state_space,
          pdb->get_value_table(),
          initial_state,
          rng,
          wildcard))
{
}

const Pattern& CEGAR::PDBInfo::get_pattern() const
{
    return pdb->get_pattern();
}

const ProjectionStateSpace& CEGAR::PDBInfo::get_mdp() const
{
    assert(state_space);
    return *state_space;
}

const ProbabilityAwarePatternDatabase& CEGAR::PDBInfo::get_pdb() const
{
    assert(pdb);
    return *pdb;
}

const ProjectionMultiPolicy& CEGAR::PDBInfo::get_policy() const
{
    return *policy;
}

value_t CEGAR::PDBInfo::get_policy_cost(const State& state) const
{
    return pdb->lookup_estimate(state);
}

std::unique_ptr<ProjectionStateSpace> CEGAR::PDBInfo::extract_state_space()
{
    return std::move(state_space);
}

std::unique_ptr<ProbabilityAwarePatternDatabase> CEGAR::PDBInfo::extract_pdb()
{
    return std::move(pdb);
}

bool CEGAR::PDBInfo::is_solvable() const
{
    return state_space->is_goal(initial_state) ||
           !policy->get_decisions(initial_state).empty();
}

bool CEGAR::PDBInfo::is_goal(StateRank rank) const
{
    return state_space->is_goal(rank);
}

CEGAR::CEGAR(
    const shared_ptr<utils::RandomNumberGenerator>& arg_rng,
    std::shared_ptr<FlawFindingStrategy> flaw_strategy,
    bool wildcard,
    int arg_max_pdb_size,
    int arg_max_collection_size,
    std::vector<int> goals,
    std::unordered_set<int> blacklisted_variables)
    : rng_(arg_rng)
    , flaw_strategy_(std::move(flaw_strategy))
    , wildcard_(wildcard)
    , max_pdb_size_(arg_max_pdb_size)
    , max_collection_size_(arg_max_collection_size)
    , goals_(std::move(goals))
    , blacklisted_variables_(std::move(blacklisted_variables))
{
}

CEGAR::~CEGAR() = default;

void CEGAR::print_collection(utils::LogProxy log) const
{
    log << "[";

    auto unsolved_infos =
        std::ranges::subrange(pdb_infos_.begin(), unsolved_end);

    if (!unsolved_infos.empty()) {
        log << unsolved_infos.front().get_pattern();
        for (const auto& info : unsolved_infos | std::views::drop(1)) {
            log << ", " << info.get_pattern();
        }
    }

    log << "]" << endl;
}

void CEGAR::generate_trivial_solution_collection(
    ProbabilisticTaskProxy task_proxy,
    FDRSimpleCostFunction& task_cost_function,
    utils::CountdownTimer& timer,
    utils::LogProxy log)
{
    assert(!goals_.empty());

    pdb_infos_.reserve(goals_.size());

    for (int var : goals_) {
        add_pattern_for_var(task_proxy, task_cost_function, var, timer);
    }

    unsolved_end = pdb_infos_.end();
    solved_end = pdb_infos_.end();

    if (log.is_at_least_normal()) {
        log << "CEGAR initial collection: ";
        print_collection(log);

        if (log.is_at_least_verbose()) {
            log << endl;
        }
    }
}

auto CEGAR::get_flaws(
    ProbabilisticTaskProxy task_proxy,
    std::vector<Flaw>& flaws,
    std::vector<int>& flaw_offsets,
    utils::CountdownTimer& timer,
    utils::LogProxy log) -> std::vector<PDBInfo>::iterator
{
    auto info_it = pdb_infos_.begin();
    auto flaw_offset_it = flaw_offsets.begin();

    while (info_it != unsolved_end) {
        auto& info = *info_it;

        // abort if no abstract solution could be found
        // TODO should do this on construction already
        if (!info.is_solvable()) {
            log << "CEGAR: Problem unsolvable" << endl;
            utils::exit_with(utils::ExitCode::SEARCH_UNSOLVABLE);
        }

        // find out if and why the abstract solution
        // would not work for the concrete task.
        // We always start with the initial state.
        const size_t num_flaws_before = flaws.size();
        const bool executable = flaw_strategy_->apply_policy(
            task_proxy,
            info.get_pdb().get_state_ranking_function(),
            info.get_mdp(),
            info.get_policy(),
            blacklisted_variables_,
            flaws,
            timer);

        const size_t new_num_flaws = flaws.size();

        // Check for new flaws
        if (new_num_flaws == num_flaws_before) {
            // Check if policy is executable modulo blacklisting.
            // Even if there are no flaws, there might be goal violations
            // that did not make it into the flaw list.
            if (executable && blacklisted_variables_.empty()) {
                /*
                 * If there are no flaws, this does not guarantee that the
                 * plan is valid in the concrete state space because we might
                 * have ignored variables that have been blacklisted. Hence, the
                 * tests for empty blacklists.
                 */
                flaws.clear();
                return info_it;
            }

            if (info_it != --unsolved_end) {
                auto& moved_info = *unsolved_end;
                // update look-up table
                for (int var : moved_info.get_pattern()) {
                    variable_to_info_[var] = info_it;
                }

                info = std::move(moved_info);
            }

            continue;
        }

        *flaw_offset_it = static_cast<int>(new_num_flaws);

        ++info_it;
        ++flaw_offset_it;
    }

    return unsolved_end;
}

bool CEGAR::can_add_variable_to_pattern(
    const VariablesProxy& variables,
    std::vector<PDBInfo>::iterator info_it,
    int var) const
{
    int pdb_size = info_it->get_pdb().num_states();
    int domain_size = variables[var].get_domain_size();

    if (!utils::is_product_within_limit(pdb_size, domain_size, max_pdb_size_)) {
        return false;
    }

    int added_size = pdb_size * domain_size - pdb_size;
    return collection_size_ + added_size <= max_collection_size_;
}

bool CEGAR::can_merge_patterns(
    std::vector<PDBInfo>::iterator info_it1,
    std::vector<PDBInfo>::iterator info_it2) const
{
    int pdb_size1 = info_it1->get_pdb().num_states();
    int pdb_size2 = info_it2->get_pdb().num_states();

    if (!utils::is_product_within_limit(pdb_size1, pdb_size2, max_pdb_size_)) {
        return false;
    }

    int added_size = pdb_size1 * pdb_size2 - pdb_size1 - pdb_size2;
    return collection_size_ + added_size <= max_collection_size_;
}

void CEGAR::add_pattern_for_var(
    ProbabilisticTaskProxy task_proxy,
    FDRSimpleCostFunction& task_cost_function,
    int var,
    utils::CountdownTimer& timer)
{
    auto info_it = pdb_infos_.emplace(
        pdb_infos_.end(),
        task_proxy,
        StateRankingFunction(task_proxy.get_variables(), {var}),
        task_cost_function,
        *rng_,
        wildcard_,
        timer);

    variable_to_info_[var] = info_it;
    collection_size_ += info_it->get_pdb().num_states();
}

void CEGAR::add_variable_to_pattern(
    ProbabilisticTaskProxy task_proxy,
    FDRSimpleCostFunction& task_cost_function,
    std::vector<PDBInfo>::iterator info_it,
    int var,
    utils::CountdownTimer& timer)
{
    PDBInfo& info = *info_it;

    const auto& pdb = info.get_pdb();

    // update collection size
    collection_size_ -= pdb.num_states();

    // compute new solution
    info = PDBInfo(
        task_proxy,
        StateRankingFunction(
            task_proxy.get_variables(),
            extended_pattern(pdb.get_pattern(), var)),
        task_cost_function,
        *rng_,
        pdb,
        var,
        wildcard_,
        timer);

    // update collection size
    collection_size_ += info.get_pdb().num_states();

    // update look-up table
    variable_to_info_[var] = info_it;
}

void CEGAR::merge_patterns(
    ProbabilisticTaskProxy task_proxy,
    FDRSimpleCostFunction& task_cost_function,
    std::vector<PDBInfo>::iterator info_it1,
    std::vector<PDBInfo>::iterator info_it2,
    utils::CountdownTimer& timer)
{
    // Merge pattern at index2 into pattern at index2
    PDBInfo& solution1 = *info_it1;
    PDBInfo& solution2 = *info_it2;

    const ProbabilityAwarePatternDatabase& pdb1 = solution1.get_pdb();
    const ProbabilityAwarePatternDatabase& pdb2 = solution2.get_pdb();

    // update look-up table
    for (int var : solution2.get_pattern()) {
        variable_to_info_[var] = info_it1;
    }

    // store old pdb sizes
    int pdb_size1 = pdb1.num_states();
    int pdb_size2 = pdb2.num_states();

    // update collection size
    collection_size_ -= pdb_size1;
    collection_size_ -= pdb_size2;

    // compute merge solution
    solution1 = PDBInfo(
        task_proxy,
        StateRankingFunction(
            task_proxy.get_variables(),
            utils::merge_sorted(pdb1.get_pattern(), pdb2.get_pattern())),
        task_cost_function,
        *rng_,
        pdb1,
        pdb2,
        wildcard_,
        timer);

    // update collection size
    collection_size_ += solution1.get_pdb().num_states();

    // fill gap if created
    if (info_it2 != --unsolved_end) {
        auto& moved_from = *unsolved_end;

        // update look-up table
        for (int var : moved_from.get_pattern()) {
            variable_to_info_[var] = info_it2;
        }

        solution2 = std::move(moved_from);
    }

    if (unsolved_end != --solved_end) {
        auto& moved_from = *solved_end;

        // update look-up table
        for (int var : moved_from.get_pattern()) {
            variable_to_info_[var] = unsolved_end;
        }

        *unsolved_end = std::move(moved_from);
    }
}

void CEGAR::refine(
    ProbabilisticTaskProxy task_proxy,
    FDRSimpleCostFunction& task_cost_function,
    const VariablesProxy& variables,
    const std::vector<Flaw>& flaws,
    const std::vector<int>& flaw_offsets,
    utils::CountdownTimer& timer,
    utils::LogProxy log)
{
    assert(!flaws.empty());

    // pick a random flaw
    int random_flaw_index = rng_->random(flaws.size());
    const Flaw& flaw = flaws[random_flaw_index];

    int solution_index = std::distance(
        flaw_offsets.begin(),
        std::ranges::upper_bound(flaw_offsets, random_flaw_index));
    auto solution_it = std::next(pdb_infos_.begin(), solution_index);
    int var = flaw.variable;

    if (log.is_at_least_verbose()) {
        log << "CEGAR: chosen flaw: pattern "
            << pdb_infos_[solution_index].get_pattern();
    }

    if (log.is_at_least_verbose()) {
        log << " with a violated";
        if (flaw.is_precondition) {
            log << " precondition ";
        } else {
            log << " goal ";
        }
        log << "on " << var << endl;
    }

    const auto it = variable_to_info_.find(var);

    if (it != variable_to_info_.end()) {
        // var is already in another pattern of the collection
        auto other_it = it->second;
        assert(other_it != solution_it);

        if (log.is_at_least_verbose()) {
            log << "CEGAR: var" << var << " is already in pattern "
                << other_it->get_pattern() << endl;
        }

        if (can_merge_patterns(solution_it, other_it)) {
            if (log.is_at_least_verbose()) {
                log << "CEGAR: merge the two patterns" << endl;
            }

            merge_patterns(
                task_proxy,
                task_cost_function,
                solution_it,
                other_it,
                timer);
            return;
        }
    } else {
        // var is not yet in the collection
        // Note on precondition violations: var may be a goal variable but
        // nevertheless is added to the pattern causing the flaw and not to
        // a single new pattern.
        if (log.is_at_least_verbose()) {
            log << "CEGAR: var" << var << " is not in the collection yet"
                << endl;
        }

        if (can_add_variable_to_pattern(variables, solution_it, var)) {
            if (log.is_at_least_verbose()) {
                log << "CEGAR: add it to the pattern" << endl;
            }

            add_variable_to_pattern(
                task_proxy,
                task_cost_function,
                solution_it,
                var,
                timer);
            return;
        }
    }

    if (log.is_at_least_verbose()) {
        log << "could not add var/merge pattern containing var "
            << "due to size limits, blacklisting var" << endl;
    }

    blacklisted_variables_.insert(var);
}

CEGARResult CEGAR::generate_pdbs(
    ProbabilisticTaskProxy task_proxy,
    FDRSimpleCostFunction& task_cost_function,
    double max_time,
    utils::LogProxy log)
{
    if (log.is_at_least_normal()) {
        log << "CEGAR options: \n"
            << "  flaw strategy: " << flaw_strategy_->get_name() << "\n"
            << "  max pdb size: " << max_pdb_size_ << "\n"
            << "  max collection size: " << max_collection_size_ << "\n"
            << "  max time: " << max_time << "\n"
            << "  wildcard plans: " << std::boolalpha << wildcard_ << "\n"
            << "  goal variables: " << goals_ << "\n"
            << "  blacklisted variables: " << blacklisted_variables_ << endl;
    }

    if (log.is_at_least_normal()) {
        log << endl;
    }

    utils::CountdownTimer timer(max_time);

    const VariablesProxy variables = task_proxy.get_variables();

    // Start with a solution of the trivial abstraction
    generate_trivial_solution_collection(
        task_proxy,
        task_cost_function,
        timer,
        log);

    const State initial_state = task_proxy.get_initial_state();
    initial_state.unpack();

    std::vector<Flaw> flaws;
    std::vector<int> flaw_offsets(pdb_infos_.size(), 0);
    std::vector<PDBInfo>::iterator solution_it;

    // main loop of the algorithm
    int refinement_counter = 1;

    try {
        for (;;) {
            if (log.is_at_least_verbose()) {
                log << "iteration #" << refinement_counter << endl;
            }

            solution_it =
                get_flaws(task_proxy, flaws, flaw_offsets, timer, log);

            if (flaws.empty()) {
                if (solution_it != unsolved_end) {
                    const auto& info = *solution_it;

                    assert(blacklisted_variables_.empty());

                    if (log.is_at_least_verbose()) {
                        log << "CEGAR: Task solved during computation of "
                               "abstract policies."
                            << endl;
                        log << "CEGAR: Cost of policy: "
                            << info.get_policy_cost(initial_state) << endl;
                    }
                } else {
                    if (log.is_at_least_verbose()) {
                        log << "CEGAR: Flaw list empty."
                            << "No further refinements possible." << endl;
                    }
                }

                break;
            }

            timer.throw_if_expired();

            // if there was a flaw, then refine the abstraction
            // such that said flaw does not occur again
            refine(
                task_proxy,
                task_cost_function,
                variables,
                flaws,
                flaw_offsets,
                timer,
                log);

            ++refinement_counter;
            flaws.clear();

            if (log.is_at_least_verbose()) {
                log << "CEGAR: current collection size: " << collection_size_
                    << endl;
                log << "CEGAR: current collection: ";
                print_collection(log);
            }

            if (log.is_at_least_verbose()) {
                log << endl;
            }
        }
    } catch (utils::TimeoutException&) {
        if (log.is_at_least_normal()) {
            log << "CEGAR: Time limit reached." << endl;
        }
    }

    if (log.is_at_least_normal()) {
        log << endl;
    }

    auto state_spaces =
        std::make_unique<std::vector<std::unique_ptr<ProjectionStateSpace>>>();
    auto pdbs = std::make_unique<PPDBCollection>();

    if (solution_it != unsolved_end) {
        auto& info = *solution_it;
        state_spaces->emplace_back(info.extract_state_space());
        pdbs->emplace_back(info.extract_pdb());
    } else {
        for (auto& info : pdb_infos_) {
            state_spaces->emplace_back(info.extract_state_space());
            pdbs->emplace_back(info.extract_pdb());
        }
    }

    if (log.is_at_least_normal()) {
        log << "CEGAR statistics:\n"
            << "  computation time: " << timer.get_elapsed_time() << "\n"
            << "  number of iterations: " << refinement_counter
            << "\n"
            // << "  final collection: " << *patterns << "\n"
            << "  final collection number of PDBs: " << pdbs->size() << "\n"
            << "  final collection summed PDB sizes: " << collection_size_
            << endl;
    }

    return CEGARResult{std::move(state_spaces), std::move(pdbs)};
}

void add_cegar_wildcard_option_to_feature(plugins::Feature& feature)
{
    feature.add_option<bool>(
        "use_wildcard_policies",
        "if true, compute wildcard plans which are sequences of sets of "
        "operators that induce the same transition; otherwise compute regular "
        "plans which are sequences of single operators",
        "false");
}

} // namespace probfd::pdbs::cegar
