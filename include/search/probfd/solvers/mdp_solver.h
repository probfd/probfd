#ifndef PROBFD_SOLVERS_MDP_SOLVER_H
#define PROBFD_SOLVERS_MDP_SOLVER_H

#include "probfd/engines/engine.h"

#include "probfd/solver_interface.h"

#include "probfd/action_id_map.h"
#include "probfd/engine_interfaces/cost_function.h"
#include "probfd/engine_interfaces/state_evaluator.h"
#include "probfd/progress_report.h"
#include "probfd/state_id_map.h"
#include "probfd/task_proxy.h"
#include "probfd/transition_generator.h"

#include "state_registry.h"

#include <memory>
#include <string>

namespace options {
class Options;
class OptionParser;
} // namespace options

namespace probfd {

/// This namespace contains the solver plugins for various search engines.
namespace solvers {

/**
 * @brief Base interface for MDP solvers.
 */
class MDPSolver : public SolverInterface {
public:
    /**
     * @brief Constructs the MDP solver from the given options.
     */
    explicit MDPSolver(const options::Options& opts);

    /**
     * @brief Factory method that constructs an new MDP engine from the given
     * arguments.
     *
     * @tparam Engine - The engine type to construct.
     */
    template <typename Engine, typename... Args>
    engines::MDPEngine<State, OperatorID>* engine_factory(Args&&... args)
    {
        return new Engine(
            &state_id_map_,
            &action_id_map_,
            &transition_generator_,
            cost_function_,
            std::forward<Args>(args)...);
    }

    /**
     * @brief Factory method a new instance of the encapsulated MDP engine.
     */
    virtual engines::MDPEngineInterface<State>* create_engine() = 0;

    /**
     * @brief Returns the name of the encapsulated MDP engine.
     */
    virtual std::string get_engine_name() const = 0;

    /**
     * @brief Print additional engine statistics to std::cout.
     */
    virtual void print_additional_statistics() const {}

    /**
     * @brief Runs the encapsulated MDP on the global problem.
     */
    virtual void solve() override;

    /**
     * @brief Checks if the MDP engine found a solution.
     */
    virtual bool found_solution() const override { return true; }

    static void add_options_to_parser(options::OptionParser& parser);

protected:
    engine_interfaces::StateIDMap<State>* get_state_id_map()
    {
        return &state_id_map_;
    }

    engine_interfaces::ActionIDMap<OperatorID>* get_action_id_map()
    {
        return &action_id_map_;
    }

    engine_interfaces::CostFunction<State, OperatorID>* get_cost_function()
    {
        return cost_function_;
    }

    engine_interfaces::TransitionGenerator<OperatorID>*
    get_transition_generator()
    {
        return &transition_generator_;
    }

    StateRegistry* get_state_registry() { return &state_registry_; }

    ProgressReport progress_;

private:
    const std::shared_ptr<ProbabilisticTask> task;
    ProbabilisticTaskProxy task_proxy;

    StateRegistry state_registry_;

    engine_interfaces::StateIDMap<State> state_id_map_;
    engine_interfaces::ActionIDMap<OperatorID> action_id_map_;
    engine_interfaces::CostFunction<State, OperatorID>* cost_function_;
    engine_interfaces::TransitionGenerator<OperatorID> transition_generator_;
};

} // namespace solvers
} // namespace probfd

#endif // __MDP_SOLVER_H__