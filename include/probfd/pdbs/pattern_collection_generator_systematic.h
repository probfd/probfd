#ifndef PROBFD_PDBS_PATTERN_COLLECTION_GENERATOR_SYSTEMATIC_H
#define PROBFD_PDBS_PATTERN_COLLECTION_GENERATOR_SYSTEMATIC_H

#include "probfd/pdbs/pattern_collection_generator.h"
#include "probfd/pdbs/types.h"

#include "downward/utils/hash.h"

#include <cstdlib>
#include <memory>
#include <unordered_set>
#include <vector>

namespace probfd {
class ProbabilisticTaskProxy;
class ProbabilisticTask;
} // namespace probfd

namespace probfd::causal_graph {
class ProbabilisticCausalGraph;
}

namespace probfd::pdbs {

class PatternCollectionGeneratorSystematic : public PatternCollectionGenerator {
    using PatternSet = utils::HashSet<Pattern>;

    const size_t max_pattern_size;
    const bool only_interesting_patterns;
    std::shared_ptr<PatternCollection> patterns;
    PatternSet pattern_set; // Cleared after pattern computation.

public:
    PatternCollectionGeneratorSystematic(
        int pattern_max_size,
        bool only_interesting_patterns,
        utils::Verbosity verbosity);

    PatternCollectionInformation generate(
        const std::shared_ptr<ProbabilisticTask>& task,
        const std::shared_ptr<FDRCostFunction>& task_cost_function) override;

private:
    void enqueue_pattern_if_new(const Pattern& pattern);

    void build_sga_patterns(
        const ProbabilisticTaskProxy& task_proxy,
        const causal_graph::ProbabilisticCausalGraph& cg);
    void build_patterns(const ProbabilisticTaskProxy& task_proxy);
    void build_patterns_naive(const ProbabilisticTaskProxy& task_proxy);
};

} // namespace probfd::pdbs

#endif // PROBFD_PDBS_PATTERN_COLLECTION_GENERATOR_SYSTEMATIC_H
