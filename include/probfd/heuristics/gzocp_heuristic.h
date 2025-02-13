#ifndef PROBFD_HEURISTICS_GZOCP_HEURISTIC_H
#define PROBFD_HEURISTICS_GZOCP_HEURISTIC_H

#include "probfd/heuristics/task_dependent_heuristic.h"
#include "probfd/task_evaluator_factory.h"

#include <memory>
#include <vector>

// Forward Declarations
namespace utils {
class RandomNumberGenerator;
class LogProxy;
} // namespace utils

namespace probfd::pdbs {
class ProbabilityAwarePatternDatabase;
class PatternCollectionGenerator;
} // namespace probfd::pdbs

namespace probfd::heuristics {

class GZOCPHeuristic : public TaskDependentHeuristic {
public:
    enum OrderingStrategy { RANDOM, SIZE_ASC, SIZE_DESC, INHERIT };

private:
    const value_t termination_cost_;
    const OrderingStrategy ordering_;
    const std::shared_ptr<utils::RandomNumberGenerator> rng_;

    std::vector<pdbs::ProbabilityAwarePatternDatabase> pdbs_;

public:
    explicit GZOCPHeuristic(
        std::shared_ptr<ProbabilisticTask> task,
        std::shared_ptr<FDRCostFunction> task_cost_function,
        utils::LogProxy log,
        std::shared_ptr<pdbs::PatternCollectionGenerator> generator,
        OrderingStrategy order,
        std::shared_ptr<utils::RandomNumberGenerator> rng);

    void print_statistics() const override
    {
        // TODO
    }

protected:
    value_t evaluate(const State& state) const override;
};

class GZOCPHeuristicFactory : public TaskEvaluatorFactory {
    const std::shared_ptr<probfd::pdbs::PatternCollectionGenerator>
        pattern_collection_generator_;
    const GZOCPHeuristic::OrderingStrategy ordering_;
    const int random_seed_;
    const utils::Verbosity verbosity_;

public:
    explicit GZOCPHeuristicFactory(
        std::shared_ptr<probfd::pdbs::PatternCollectionGenerator>
            pattern_collection_generator_,
        GZOCPHeuristic::OrderingStrategy ordering_,
        int random_seed_,
        utils::Verbosity verbosity_);

    std::unique_ptr<FDREvaluator> create_evaluator(
        std::shared_ptr<ProbabilisticTask> task,
        std::shared_ptr<FDRCostFunction> task_cost_function) override;
};

} // namespace probfd::heuristics

#endif // PROBFD_HEURISTICS_GZOCP_HEURISTIC_H
