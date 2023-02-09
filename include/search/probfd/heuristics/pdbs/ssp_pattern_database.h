#ifndef MDPS_HEURISTICS_PDBS_EXPCOST_PROJECTION_H
#define MDPS_HEURISTICS_PDBS_EXPCOST_PROJECTION_H

#include "probfd/heuristics/pdbs/abstract_policy.h"
#include "probfd/heuristics/pdbs/probabilistic_pattern_database.h"

#include "probfd/heuristics/constant_evaluator.h"

namespace pdbs {
class PatternDatabase;
}

namespace probfd {
namespace heuristics {
namespace pdbs {

/// Implements a probabilistic pattern database for SSPs.
class SSPPatternDatabase : public ProbabilisticPatternDatabase {
public:
    /**
     * @brief Constructs a pattern database from a given task and pattern, using
     * the specified construction options.
     *
     * @param task_proxy The input task with respect to which the projection is
     * constructed.
     * @param pattern The pattern of the pattern database.
     * @param operator_pruning Whether equivalent operators shall be pruned
     * during construction of the projection.
     * @param heuristic A heuristic used to accelerate the computation of the
     * value table.
     */
    SSPPatternDatabase(
        const ProbabilisticTaskProxy& task_proxy,
        Pattern pattern,
        bool operator_pruning = true,
        const StateRankEvaluator& heuristic =
            ConstantEvaluator<StateRank>(0_vt));

    /**
     * @brief Constructs a pattern database from a given task and the pattern of
     * a given deterministic PDB, using the specified construction options.
     *
     * This constructor makes use of the supplied deterministic PDB by using its
     * induced heuristic to accelerate the value table computation.
     *
     * @param task_proxy The input task with respect to which the projection is
     * constructed.
     * @param pdb A previous deterministic pattern database for the all-outcomes
     * determinization of the given task.
     * @param operator_pruning Whether equivalent operators shall be pruned
     * during construction of the projection.
     */
    SSPPatternDatabase(
        const ProbabilisticTaskProxy& task_proxy,
        const ::pdbs::PatternDatabase& pdb,
        bool operator_pruning = true);

    /**
     * @brief Constructs a pattern database for the pattern of a previous
     * pattern database with one additional variable.
     *
     * This constructor makes use of the supplied PDB by using its
     * induced heuristic to accelerate the value table computation.
     *
     * @param task_proxy The input task with respect to which the projection is
     * constructed.
     * @param pdb A previous pattern database for the given task.
     * @param operator_pruning Whether equivalent operators shall be pruned
     * during construction of the projection.
     */
    SSPPatternDatabase(
        const ProbabilisticTaskProxy& task_proxy,
        const SSPPatternDatabase& pdb,
        int add_var,
        bool operator_pruning = true);

    /**
     * @brief Constructs a pattern database for the union of the two patterns
     * of two previous PDBs.
     *
     * This constructor makes use of the supplied PDBs by using the minimum
     * over their induced heuristics to accelerate the value table computation.
     *
     * \todo One could do even better by exploiting potential additivity of the
     * two patterns.
     *
     * @param task_proxy The input task with respect to which the projection is
     * constructed.
     * @param left A previous pattern database for the given task.
     * @param right A previous pattern database for the given task.
     * @param operator_pruning Whether equivalent operators shall be pruned
     * during construction of the projection.
     */
    SSPPatternDatabase(
        const ProbabilisticTaskProxy& task_proxy,
        const SSPPatternDatabase& left,
        const SSPPatternDatabase& right,
        bool operator_pruning = true);

    /// Get a heuristic evaluation for an input state.
    [[nodiscard]] EvaluationResult evaluate(const State& s) const;

    /// Get a heuristic evaluation for an abstract state given by a state rank.
    [[nodiscard]] EvaluationResult evaluate(StateRank s) const;

    /// Dump the PDB's projection as a dot graph to a specified path with or
    /// without transition labels shown.
    void
    dump_graphviz(const std::string& path, bool transition_labels = true) const;

private:
    void compute_value_table(const StateRankEvaluator& heuristic);

#if !defined(NDEBUG) && defined(USE_LP)
    void verify(
        const engine_interfaces::StateIDMap<StateRank>& state_id_map,
        const std::vector<StateID>& proper_states);
#endif
};

} // namespace pdbs
} // namespace heuristics
} // namespace probfd

#endif // __EXPCOST_PROJECTION_H__