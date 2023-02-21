#ifndef PROBFD_HEURISTICS_CARTESIAN_TYPES_H
#define PROBFD_HEURISTICS_CARTESIAN_TYPES_H

#include "cegar/types.h"

#include <limits>
#include <memory>
#include <unordered_set>
#include <vector>

namespace cegar {
class CartesianHeuristicFunction;
class RefinementHierarchy;
}

namespace probfd {
namespace heuristics {
namespace cartesian {

struct ProbabilisticTransition;

// Classed reused from classical implementation
using AbstractState = cegar::AbstractState;
using CartesianHeuristicFunction = cegar::CartesianHeuristicFunction;
using RefinementHierarchy = cegar::RefinementHierarchy;

// Typedefs reused from classical implementation
using AbstractStates = cegar::AbstractStates;
using Goals = cegar::Goals;
using NodeID = cegar::NodeID;

// Typdefs adapted from classical implementation
using Loops = std::vector<int>;
using ProbabilisticTransitions = std::vector<ProbabilisticTransition>;

constexpr int UNDEFINED = -1;

} // namespace cartesian
} // namespace heuristics
} // namespace probfd

#endif
