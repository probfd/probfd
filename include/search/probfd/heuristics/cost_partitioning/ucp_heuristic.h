#ifndef PROBFD_HEURISTICS_PDBS_SATURATED_COST_PARTITIONING_UCP_HEURISTIC_H
#define PROBFD_HEURISTICS_PDBS_SATURATED_COST_PARTITIONING_UCP_HEURISTIC_H

#include "probfd/heuristics/task_dependent_heuristic.h"

#include "probfd/heuristics/pdbs/pattern_collection_generator.h"

#include "probfd/evaluator.h"
#include "probfd/types.h"

#include <memory>

namespace plugins {
class Options;
class Feature;
} // namespace plugins

namespace probfd {
namespace heuristics {
namespace pdbs {

class UCPHeuristic : public TaskDependentHeuristic {
    std::vector<ProbabilityAwarePatternDatabase> pdbs;

public:
    explicit UCPHeuristic(const plugins::Options& opts);

    explicit UCPHeuristic(
        std::shared_ptr<ProbabilisticTask> task,
        utils::LogProxy log,
        std::shared_ptr<PatternCollectionGenerator> generator);

    void print_statistics() const override
    {
        // TODO
    }

protected:
    EvaluationResult evaluate(const State& state) const override;

public:
    static void add_options_to_feature(plugins::Feature& feature);
};

} // namespace pdbs
} // namespace heuristics
} // namespace probfd

#endif