#ifndef PROBFD_HEURISTICS_CARTESIAN_ABSTRACTION_H
#define PROBFD_HEURISTICS_CARTESIAN_ABSTRACTION_H

#include "probfd/heuristics/cartesian/types.h"

#include "probfd/task_proxy.h"

#include "utils/collections.h"

#include <memory>
#include <vector>

namespace utils {
class LogProxy;
}

namespace probfd {
namespace heuristics {
namespace cartesian {

class ProbabilisticTransitionSystem;

/*
  Store the set of AbstractStates, use AbstractSearch to find abstract
  solutions, find flaws, use SplitSelector to select splits in case of
  ambiguities, break spurious solutions and maintain the
  RefinementHierarchy.
*/
class Abstraction {
    const std::unique_ptr<ProbabilisticTransitionSystem> transition_system;
    const State concrete_initial_state;
    const std::vector<FactPair> goal_facts;

    // All (as of yet unsplit) abstract states.
    AbstractStates states;
    // State ID of abstract initial state.
    int init_id;
    // Abstract goal states.
    Goals goals;

    /* DAG with inner nodes for all split states and leaves for all
       current states. */
    std::unique_ptr<RefinementHierarchy> refinement_hierarchy;

    utils::LogProxy& log;

    void initialize_trivial_abstraction(const std::vector<int>& domain_sizes);

public:
    Abstraction(
        const std::shared_ptr<ProbabilisticTask>& task,
        utils::LogProxy& log);
    ~Abstraction();

    Abstraction(const Abstraction&) = delete;

    int get_num_states() const;
    const AbstractState& get_initial_state() const;
    const Goals& get_goals() const;
    const AbstractState& get_state(int state_id) const;
    const ProbabilisticTransitionSystem& get_transition_system() const;
    std::unique_ptr<RefinementHierarchy> extract_refinement_hierarchy();

    /* Needed for CEGAR::separate_facts_unreachable_before_goal(). */
    void mark_all_states_as_goals();

    // Split state into two child states.
    std::pair<int, int>
    refine(const AbstractState& state, int var, const std::vector<int>& wanted);

    void print_statistics() const;
};

} // namespace cartesian
} // namespace heuristics
} // namespace probfd

#endif
