#ifndef PROBFD_HEURISTICS_PDBS_CEGAR_CEGAR_H
#define PROBFD_HEURISTICS_PDBS_CEGAR_CEGAR_H

#include "probfd/heuristics/pdbs/evaluators.h"
#include "probfd/heuristics/pdbs/types.h"

#include "probfd/heuristics/pdbs/cegar/flaw.h"

#include "probfd/task_proxy.h"

#include "downward/utils/logging.h"

#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace utils {
class CountdownTimer;
class RandomNumberGenerator;
} // namespace utils

namespace probfd {
namespace heuristics {
namespace pdbs {

class ProbabilityAwarePatternDatabase;
class SubCollectionFinderFactory;

namespace cegar {

class FlawFindingStrategy;

struct CEGARResult {
    std::unique_ptr<ProjectionCollection> projections;
    std::unique_ptr<PPDBCollection> pdbs;

    ~CEGARResult();
};

class CEGAR {
    class PDBInfo;

    mutable utils::LogProxy log;

    // Random number generator
    const std::shared_ptr<utils::RandomNumberGenerator> rng;

    // Flaw finding strategy
    const std::shared_ptr<FlawFindingStrategy> flaw_strategy;

    // behavior defining parameters
    const bool wildcard;
    const int max_pdb_size;
    const int max_collection_size;
    const double max_time;

    const std::vector<int> goals;
    std::unordered_set<int> blacklisted_variables;

    // the pattern collection in form of their pdbs plus stored plans.
    std::vector<PDBInfo> pdb_infos;

    // Takes a variable as key and returns the solutions-entry whose pattern
    // contains said variable. Used for checking if a variable is already
    // included in some pattern as well as for quickly finding the other partner
    // for merging.
    std::unordered_map<int, std::reference_wrapper<PDBInfo>> variable_to_info;

public:
    CEGAR(
        utils::LogProxy log,
        const std::shared_ptr<utils::RandomNumberGenerator>& rng,
        std::shared_ptr<FlawFindingStrategy> flaw_strategy,
        bool wildcard,
        int max_pdb_size,
        int max_collection_size,
        double max_time,
        std::vector<int> goals,
        std::unordered_set<int> blacklisted_variables = {});

    ~CEGAR();

    CEGARResult generate_pdbs(
        ProbabilisticTaskProxy task_proxy,
        FDRSimpleCostFunction& task_cost_function);

private:
    void generate_trivial_solution_collection(
        ProbabilisticTaskProxy task_proxy,
        FDRSimpleCostFunction& task_cost_function,
        int& collection_size,
        utils::CountdownTimer& timer);

    // If a concrete solution was found, returns a pointer to the corresponding
    // info struct, otherwise nullptr.
    PDBInfo* get_flaws(
        ProbabilisticTaskProxy task_proxy,
        std::vector<Flaw>& flaws,
        std::vector<int>& flaw_offsets,
        value_t termination_cost,
        utils::CountdownTimer& timer);

    bool can_add_variable(
        VariableProxy variable,
        const PDBInfo& info,
        int collection_size) const;
    bool can_merge_patterns(
        const PDBInfo& left,
        const PDBInfo& right,
        int collection_size) const;

    void add_variable_to_pattern(
        ProbabilisticTaskProxy task_proxy,
        FDRSimpleCostFunction& task_cost_function,
        PDBInfo& info,
        int var,
        int& collection_size,
        utils::CountdownTimer& timer);
    void merge_patterns(
        ProbabilisticTaskProxy task_proxy,
        FDRSimpleCostFunction& task_cost_function,
        PDBInfo& left,
        PDBInfo& right,
        int& collection_size,
        utils::CountdownTimer& timer);

    void refine(
        ProbabilisticTaskProxy task_proxy,
        FDRSimpleCostFunction& task_cost_function,
        const std::vector<Flaw>& flaws,
        const std::vector<int>& flaw_offsets,
        int& collection_size,
        utils::CountdownTimer& timer);

    void print_collection() const;
};

extern void add_cegar_wildcard_option_to_feature(plugins::Feature& feature);

} // namespace cegar
} // namespace pdbs
} // namespace heuristics
} // namespace probfd

#endif // PDBS_PATTERN_COLLECTION_GENERATOR_CEGAR_H
