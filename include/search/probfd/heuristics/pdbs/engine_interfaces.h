#ifndef MDPS_HEURISTICS_PDBS_ENGINE_INTERFACES_H
#define MDPS_HEURISTICS_PDBS_ENGINE_INTERFACES_H

#include "probfd/heuristics/pdbs/abstract_operator.h"
#include "probfd/heuristics/pdbs/match_tree.h"
#include "probfd/heuristics/pdbs/state_ranking_function.h"

#include "probfd/engine_interfaces/action_id_map.h"
#include "probfd/engine_interfaces/cost_function.h"
#include "probfd/engine_interfaces/state_evaluator.h"
#include "probfd/engine_interfaces/state_id_map.h"
#include "probfd/engine_interfaces/transition_generator.h"

#include "utils/collections.h"

#include <memory>
#include <ranges>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace pdbs {
class PatternDatabase;
}

namespace probfd {
namespace engine_interfaces {
template <>
class StateIDMap<heuristics::pdbs::StateRank> {
public:
    using visited_iterator = std::set<int>::const_iterator;
    using visited_range = std::ranges::subrange<visited_iterator>;

    explicit StateIDMap() = default;

    StateID get_state_id(const heuristics::pdbs::StateRank& state);
    heuristics::pdbs::StateRank get_state(const StateID& id);

    unsigned size() const;

    visited_iterator visited_begin() const;
    visited_iterator visited_end() const;

    visited_range visited() const;

    void clear() { seen.clear(); }

private:
    std::set<int> seen;
};

template <>
class ActionIDMap<const heuristics::pdbs::AbstractOperator*> {
public:
    explicit ActionIDMap(
        const std::vector<heuristics::pdbs::AbstractOperator>& ops);

    ActionID get_action_id(
        const StateID& state_id,
        const heuristics::pdbs::AbstractOperator* action) const;

    const heuristics::pdbs::AbstractOperator*
    get_action(const StateID& state_id, const ActionID& action_id) const;

private:
    const std::vector<heuristics::pdbs::AbstractOperator>& ops_;
};

template <>
class TransitionGenerator<const heuristics::pdbs::AbstractOperator*> {
public:
    explicit TransitionGenerator(
        StateIDMap<heuristics::pdbs::StateRank>& id_map,
        const heuristics::pdbs::MatchTree& aops_gen);

    void generate_applicable_actions(
        const StateID& state,
        std::vector<const heuristics::pdbs::AbstractOperator*>& aops);

    void generate_action_transitions(
        const StateID& state,
        const heuristics::pdbs::AbstractOperator* op,
        Distribution<StateID>& result);

    void generate_all_transitions(
        const StateID& state,
        std::vector<const heuristics::pdbs::AbstractOperator*>& aops,
        std::vector<Distribution<StateID>>& result);

private:
    StateIDMap<heuristics::pdbs::StateRank>& id_map_;
    const heuristics::pdbs::MatchTree& aops_gen_;
};

} // namespace engine_interfaces

namespace heuristics {
namespace pdbs {

using StateRankEvaluator = engine_interfaces::StateEvaluator<StateRank>;
using AbstractCostFunction =
    engine_interfaces::CostFunction<StateRank, const AbstractOperator*>;

class QualitativeResultStore;

class SSPPatternDatabase;
class MaxProbPatternDatabase;

class PDBEvaluator : public StateRankEvaluator {
public:
    explicit PDBEvaluator(const ::pdbs::PatternDatabase& pdb);

    EvaluationResult evaluate(const StateRank& state) const override;

private:
    const ::pdbs::PatternDatabase& pdb;
};

class DeadendPDBEvaluator : public StateRankEvaluator {
public:
    explicit DeadendPDBEvaluator(const ::pdbs::PatternDatabase& pdb);

    EvaluationResult evaluate(const StateRank& state) const override;

private:
    const ::pdbs::PatternDatabase& pdb;
};

class IncrementalPPDBEvaluatorBase : public StateRankEvaluator {
    int left_multiplier;
    int right_multiplier;
    int domain_size;

public:
    explicit IncrementalPPDBEvaluatorBase(
        const StateRankingFunction* mapper,
        int add_var);

protected:
    StateRank to_parent_state(StateRank state) const;
};

template <typename PDBType>
class IncrementalPPDBEvaluator : public IncrementalPPDBEvaluatorBase {
    const PDBType& pdb;

public:
    explicit IncrementalPPDBEvaluator(
        const PDBType& pdb,
        const StateRankingFunction* mapper,
        int add_var);

    EvaluationResult evaluate(const StateRank& state) const override;
};

template <typename PDBType>
class MergeEvaluator : public engine_interfaces::StateEvaluator<StateRank> {
    const StateRankingFunction& mapper;
    const PDBType& left;
    const PDBType& right;

public:
    MergeEvaluator(
        const StateRankingFunction& mapper,
        const PDBType& left,
        const PDBType& right);

protected:
    EvaluationResult evaluate(const StateRank& state) const override;
};

class BaseAbstractCostFunction : public AbstractCostFunction {
protected:
    const std::vector<bool>& goal_state_flags_;
    const value_t value_in_;
    const value_t value_not_in_;

public:
    explicit BaseAbstractCostFunction(
        const std::vector<bool>& goal_state_flags,
        value_t value_in,
        value_t value_not_in)
        : goal_state_flags_(goal_state_flags)
        , value_in_(value_in)
        , value_not_in_(value_not_in)
    {
    }

    TerminationInfo get_termination_info(const StateRank& state) override
    {
        const bool is_contained = goal_state_flags_[state.id];
        return TerminationInfo(
            is_contained,
            is_contained ? value_in_ : value_not_in_);
    }
};

class ZeroCostAbstractCostFunction : public BaseAbstractCostFunction {
public:
    using BaseAbstractCostFunction::BaseAbstractCostFunction;

    value_t get_action_cost(StateID, const AbstractOperator*) override
    {
        return 0;
    }
};

class NormalCostAbstractCostFunction : public BaseAbstractCostFunction {
public:
    using BaseAbstractCostFunction::BaseAbstractCostFunction;

    value_t get_action_cost(StateID, const AbstractOperator* op) override
    {
        return op->cost;
    }
};

} // namespace pdbs
} // namespace heuristics
} // namespace probfd

#endif // __ENGINE_INTERFACES_H__