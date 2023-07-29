#ifndef PROBFD_HEURISTICS_CARTESIAN_FLAW_GENERATOR_H
#define PROBFD_HEURISTICS_CARTESIAN_FLAW_GENERATOR_H

#include "probfd/heuristics/cartesian/engine_interfaces.h"
#include "probfd/heuristics/cartesian/flaw.h"
#include "probfd/heuristics/cartesian/probabilistic_transition.h"
#include "probfd/heuristics/cartesian/types.h"

#include "probfd/policy_pickers/arbitrary_tiebreaker.h"

#include "probfd/progress_report.h"
#include "probfd/value_type.h"

#include "downward/algorithms/priority_queues.h"

#include <deque>
#include <memory>
#include <optional>
#include <vector>

namespace utils {
class CountdownTimer;
class Timer;
} // namespace utils

namespace probfd {
namespace heuristics {
namespace cartesian {

/**
 * @brief Find flaws in the abstraction.
 */
class FlawGenerator {
public:
    virtual ~FlawGenerator() = default;

    virtual std::optional<Flaw> generate_flaw(
        const ProbabilisticTaskProxy& task_proxy,
        const std::vector<int>& domain_sizes,
        Abstraction& abstraction,
        CartesianCostFunction& cost_function,
        const AbstractState* init_id,
        CartesianHeuristic& heuristic,
        utils::LogProxy& log,
        utils::CountdownTimer& timer) = 0;

    virtual void notify_split() = 0;

    virtual void print_statistics(utils::LogProxy& log) = 0;
};

class FlawGeneratorFactory {
public:
    virtual ~FlawGeneratorFactory() = default;
    virtual std::unique_ptr<FlawGenerator> create_flaw_generator() const = 0;
};

/**
 * @brief Calls topological value iteration to compute the complete optimal
 * value function (for states reachable from the initial state).
 */
std::vector<value_t> compute_distances(
    Abstraction& abstraction,
    CartesianHeuristic& heuristic,
    const std::vector<value_t>& costs);

} // namespace cartesian
} // namespace heuristics
} // namespace probfd

#endif
