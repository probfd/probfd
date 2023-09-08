#include "probfd/heuristics/pdbs/probability_aware_pattern_database.h"
#include "probfd/heuristics/pdbs/projection_state_space.h"
#include "probfd/heuristics/pdbs/utils.h"

#include "probfd/heuristics/abstractions/distances.h"

#include "probfd/task_utils/task_properties.h"

#include "downward/pdbs/pattern_database.h"

#include "downward/utils/collections.h"
#include "downward/utils/countdown_timer.h"

#include <algorithm>

namespace probfd {
namespace heuristics {
namespace pdbs {

using namespace abstractions;

ProbabilityAwarePatternDatabase::ProbabilityAwarePatternDatabase(
    ProbabilisticTaskProxy task_proxy,
    Pattern pattern,
    value_t dead_end_cost)
    : ranking_function_(task_proxy.get_variables(), std::move(pattern))
    , value_table(ranking_function_.num_states(), dead_end_cost)
{
}

ProbabilityAwarePatternDatabase::ProbabilityAwarePatternDatabase(
    StateRankingFunction ranking_function,
    value_t dead_end_cost)
    : ranking_function_(std::move(ranking_function))
    , value_table(ranking_function_.num_states(), dead_end_cost)
{
}

ProbabilityAwarePatternDatabase::ProbabilityAwarePatternDatabase(
    ProbabilisticTaskProxy task_proxy,
    FDRSimpleCostFunction& task_cost_function,
    Pattern pattern,
    const State& initial_state,
    const AbstractionEvaluator& heuristic,
    bool operator_pruning,
    double max_time)
    : ProbabilityAwarePatternDatabase(
          task_proxy,
          std::move(pattern),
          task_cost_function.get_non_goal_termination_cost())
{
    utils::CountdownTimer timer(max_time);
    ProjectionStateSpace mdp(
        task_proxy,
        task_cost_function,
        ranking_function_,
        operator_pruning,
        timer.get_remaining_time());
    compute_value_table(
        mdp,
        ranking_function_.get_abstract_rank(initial_state),
        heuristic,
        value_table,
        timer.get_remaining_time());
}

ProbabilityAwarePatternDatabase::ProbabilityAwarePatternDatabase(
    ProbabilisticTaskProxy task_proxy,
    FDRSimpleCostFunction& task_cost_function,
    Pattern pattern,
    const State& initial_state,
    const AbstractionEvaluator& heuristic,
    std::unique_ptr<ProjectionStateSpace>& mdp,
    bool operator_pruning,
    double max_time)
    : ProbabilityAwarePatternDatabase(
          task_proxy,
          std::move(pattern),
          task_cost_function.get_non_goal_termination_cost())
{
    utils::CountdownTimer timer(max_time);
    auto temp = std::make_unique<ProjectionStateSpace>(
        task_proxy,
        task_cost_function,
        ranking_function_,
        operator_pruning,
        timer.get_remaining_time());
    compute_value_table(
        *temp,
        ranking_function_.get_abstract_rank(initial_state),
        heuristic,
        value_table,
        timer.get_remaining_time());
    mdp = std::move(temp);
}

ProbabilityAwarePatternDatabase::ProbabilityAwarePatternDatabase(
    ProbabilisticTaskProxy task_proxy,
    FDRSimpleCostFunction& task_cost_function,
    const ::pdbs::PatternDatabase& pdb,
    const State& initial_state,
    bool operator_pruning,
    double max_time)
    : ProbabilityAwarePatternDatabase(
          task_proxy,
          task_cost_function,
          pdb.get_pattern(),
          initial_state,
          task_cost_function.get_non_goal_termination_cost() == INFINITE_VALUE
              ? static_cast<const AbstractionEvaluator&>(PDBEvaluator(pdb))
              : static_cast<const AbstractionEvaluator&>(
                    DeadendPDBEvaluator(pdb)),
          operator_pruning,
          max_time)
{
}

ProbabilityAwarePatternDatabase::ProbabilityAwarePatternDatabase(
    ProbabilisticTaskProxy task_proxy,
    FDRSimpleCostFunction& task_cost_function,
    Pattern pattern,
    const State& initial_state,
    const ProbabilityAwarePatternDatabase& pdb,
    bool operator_pruning,
    double max_time)
    : ProbabilityAwarePatternDatabase(
          task_proxy,
          std::move(pattern),
          task_cost_function.get_non_goal_termination_cost())
{
    utils::CountdownTimer timer(max_time);

    ProjectionStateSpace mdp(
        task_proxy,
        task_cost_function,
        ranking_function_,
        operator_pruning,
        timer.get_remaining_time());
    compute_value_table(
        mdp,
        ranking_function_.get_abstract_rank(initial_state),
        IncrementalPPDBEvaluator(ranking_function_, pdb),
        value_table,
        timer.get_remaining_time());
}

ProbabilityAwarePatternDatabase::ProbabilityAwarePatternDatabase(
    ProbabilisticTaskProxy task_proxy,
    FDRSimpleCostFunction& task_cost_function,
    Pattern pattern,
    const State& initial_state,
    const ProbabilityAwarePatternDatabase& pdb,
    std::unique_ptr<ProjectionStateSpace>& mdp,
    bool operator_pruning,
    double max_time)
    : ProbabilityAwarePatternDatabase(
          task_proxy,
          std::move(pattern),
          task_cost_function.get_non_goal_termination_cost())
{
    utils::CountdownTimer timer(max_time);

    auto temp = std::make_unique<ProjectionStateSpace>(
        task_proxy,
        task_cost_function,
        ranking_function_,
        operator_pruning,
        timer.get_remaining_time());
    compute_value_table(
        *temp,
        ranking_function_.get_abstract_rank(initial_state),
        IncrementalPPDBEvaluator(ranking_function_, pdb),
        value_table,
        timer.get_remaining_time());
    mdp = std::move(temp);
}

ProbabilityAwarePatternDatabase::ProbabilityAwarePatternDatabase(
    ProbabilisticTaskProxy task_proxy,
    FDRSimpleCostFunction& task_cost_function,
    Pattern pattern,
    const State& initial_state,
    const ProbabilityAwarePatternDatabase& left,
    const ProbabilityAwarePatternDatabase& right,
    std::unique_ptr<ProjectionStateSpace>& mdp,
    bool operator_pruning,
    double max_time)
    : ProbabilityAwarePatternDatabase(
          task_proxy,
          std::move(pattern),
          task_cost_function.get_non_goal_termination_cost())
{
    utils::CountdownTimer timer(max_time);

    auto temp = std::make_unique<ProjectionStateSpace>(
        task_proxy,
        task_cost_function,
        ranking_function_,
        operator_pruning,
        timer.get_remaining_time());
    compute_value_table(
        *temp,
        ranking_function_.get_abstract_rank(initial_state),
        IncrementalPPDBEvaluator(ranking_function_, left, right),
        value_table,
        timer.get_remaining_time());
    mdp = std::move(temp);
}

ProbabilityAwarePatternDatabase::ProbabilityAwarePatternDatabase(
    ProbabilisticTaskProxy task_proxy,
    FDRSimpleCostFunction& task_cost_function,
    Pattern pattern,
    const State& initial_state,
    const ProbabilityAwarePatternDatabase& left,
    const ProbabilityAwarePatternDatabase& right,
    bool operator_pruning,
    double max_time)
    : ProbabilityAwarePatternDatabase(
          task_proxy,
          std::move(pattern),
          task_cost_function.get_non_goal_termination_cost())
{
    utils::CountdownTimer timer(max_time);

    ProjectionStateSpace mdp(
        task_proxy,
        task_cost_function,
        ranking_function_,
        operator_pruning,
        timer.get_remaining_time());
    compute_value_table(
        mdp,
        ranking_function_.get_abstract_rank(initial_state),
        IncrementalPPDBEvaluator(ranking_function_, left, right),
        value_table,
        timer.get_remaining_time());
}

const Pattern& ProbabilityAwarePatternDatabase::get_pattern() const
{
    return ranking_function_.get_pattern();
}

const StateRankingFunction&
ProbabilityAwarePatternDatabase::get_state_ranking_function() const
{
    return ranking_function_;
}

const std::vector<value_t>&
ProbabilityAwarePatternDatabase::get_value_table() const
{
    return value_table;
}

unsigned int ProbabilityAwarePatternDatabase::num_states() const
{
    return ranking_function_.num_states();
}

value_t ProbabilityAwarePatternDatabase::lookup_estimate(const State& s) const
{
    return lookup_estimate(get_abstract_state(s));
}

value_t
ProbabilityAwarePatternDatabase::lookup_estimate(AbstractStateIndex s) const
{
    return value_table[s];
}

AbstractStateIndex
ProbabilityAwarePatternDatabase::get_abstract_state(const State& s) const
{
    return ranking_function_.get_abstract_rank(s);
}

} // namespace pdbs
} // namespace heuristics
} // namespace probfd
