#ifndef PROBFD_ENGINES_HEURISTIC_SEARCH_BASE_H
#define PROBFD_ENGINES_HEURISTIC_SEARCH_BASE_H

#include "probfd/engines/heuristic_search_state_information.h"
#include "probfd/engines/utils.h"

#include "probfd/engine_interfaces/new_state_observer.h"
#include "probfd/engine_interfaces/policy_picker.h"
#include "probfd/engine_interfaces/successor_sampler.h"

#include "probfd/policies/map_policy.h"

#include "probfd/utils/graph_visualization.h"

#include "probfd/engine.h"
#include "probfd/progress_report.h"

#include "downward/utils/collections.h"
#include "downward/utils/system.h"

#if defined(EXPENSIVE_STATISTICS)
#include "downward/utils/timer.h"
#endif

#include <cassert>
#include <deque>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <type_traits>
#include <vector>

namespace probfd {
namespace engines {

/// Namespace dedicated to the MDP heuristic search base implementation
namespace heuristic_search {

namespace internal {

/**
 * @brief Base statistics for MDP heuristic search.
 */
struct CoreStatistics {
    unsigned long long backups = 0;
    unsigned long long backed_up_states = 0;
    unsigned long long evaluated_states = 0;
    unsigned long long pruned_states = 0;
    unsigned long long goal_states = 0;
    unsigned long long terminal_states = 0;
    unsigned long long self_loop_states = 0;
    unsigned long long value_changes = 0;
    unsigned long long policy_updates = 0;
};

/**
 * @brief Extended statistics for MDP heuristic search.
 */
struct Statistics : public CoreStatistics {
    unsigned state_info_bytes = 0;
    value_t initial_state_estimate = 0;
    bool initial_state_found_terminal = 0;

    value_t value = 0_vt;
    CoreStatistics before_last_update;

#if defined(EXPENSIVE_STATISTICS)
    utils::Timer update_time = utils::Timer(true);
    utils::Timer policy_selection_time = utils::Timer(true);
#endif

    /**
     * @brief Prints the statistics to the specified output stream.
     */
    void print(std::ostream& out) const
    {
        out << "  Stored " << state_info_bytes << " bytes per state"
            << std::endl;

        out << "  Initial state value estimation: " << initial_state_estimate
            << std::endl;
        out << "  Initial state value found terminal: "
            << initial_state_found_terminal << std::endl;

        out << "  Evaluated state(s): " << evaluated_states << std::endl;
        out << "  Pruned state(s): " << pruned_states << std::endl;
        out << "  Goal state(s): " << goal_states << std::endl;
        out << "  Terminal state(s): " << terminal_states << std::endl;
        out << "  Self-loop state(s): " << self_loop_states << std::endl;
        out << "  Backed up state(s): " << backed_up_states << std::endl;
        out << "  Number of backups: " << backups << std::endl;
        out << "  Number of value changes: " << value_changes << std::endl;
        out << "  Number of policy updates: " << policy_updates << std::endl;

        out << "  Evaluated state(s) until last value change: "
            << before_last_update.evaluated_states << std::endl;
        out << "  Pruned state(s) until last value change: "
            << before_last_update.pruned_states << std::endl;
        out << "  Goal state(s) until last value change: "
            << before_last_update.goal_states << std::endl;
        out << "  Terminal state(s) until last value change: "
            << before_last_update.terminal_states << std::endl;
        out << "  Self-loop state(s) until last value change: "
            << before_last_update.self_loop_states << std::endl;
        out << "  Backed up state(s) until last value change: "
            << before_last_update.backed_up_states << std::endl;
        out << "  Number of backups until last value change: "
            << before_last_update.backups << std::endl;
        out << "  Number of value changes until last value change: "
            << before_last_update.value_changes << std::endl;
        out << "  Number of policy updates until last value change: "
            << before_last_update.policy_updates << std::endl;

#if defined(EXPENSIVE_STATISTICS)
        out << "  Updating time: " << update_time << std::endl;
        out << "  Policy selection time: " << policy_selection_time
            << std::endl;
#endif
    }

    void jump() { before_last_update = *this; }
};

/**
 * @brief The common base class for MDP heuristic search algorithms.
 *
 * @tparam State - The state type of the underlying MDP model.
 * @tparam Action - The action type of the undelying MDP model.
 * @tparam StateInfoT - The state information container type.
 */
template <typename State, typename Action, typename StateInfoT>
class HeuristicSearchBase : public MDPEngine<State, Action> {
public:
    using StateInfo = StateInfoT;

    static constexpr bool StorePolicy = StateInfo::StorePolicy;
    static constexpr bool UseInterval = StateInfo::UseInterval;

    using EngineValueType = engines::EngineValueType<UseInterval>;

private:
    class StateInfos : public engine_interfaces::StateProperties {
        storage::PerStateStorage<StateInfo> state_infos_;

    public:
        StateInfo& operator[](StateID sid) { return state_infos_[sid]; }
        const StateInfo& operator[](StateID sid) const
        {
            return state_infos_[sid];
        }

        const StateFlags& lookup_state_flags(StateID state_id) override
        {
            return state_infos_[state_id];
        }

        value_t lookup_value(StateID state_id) override
        {
            return state_infos_[state_id].get_value();
        }

        Interval lookup_bounds(StateID state_id) override
        {
            return state_infos_[state_id].get_bounds();
        }
    };

    engine_interfaces::PolicyPicker<State, Action>* policy_chooser_;
    engine_interfaces::NewStateObserver<State>* on_new_state_;

    StateInfos state_infos_;

    Statistics statistics_;

    StateID initial_state_id_ = StateID::undefined;

protected:
    ProgressReport* report_;
    const bool interval_comparison_;

    struct UpdateResult {
        bool value_changed;
        bool policy_changed;
        std::optional<Action> policy_action = std::nullopt;
    };

public:
    HeuristicSearchBase(
        engine_interfaces::PolicyPicker<State, Action>* policy_chooser,
        engine_interfaces::NewStateObserver<State>* new_state_handler,
        ProgressReport* report,
        bool interval_comparison)
        : policy_chooser_(policy_chooser)
        , on_new_state_(new_state_handler)
        , report_(report)
        , interval_comparison_(interval_comparison)
    {
        statistics_.state_info_bytes = sizeof(StateInfo);
    }

    Interval solve(
        MDP<State, Action>& mdp,
        Evaluator<State>& heuristic,
        param_type<State> state,
        double max_time =
            std::numeric_limits<double>::infinity()) final override
    {
        this->initialize_report(mdp, heuristic, state);
        return this->do_solve(mdp, heuristic, state, max_time);
    }

    std::unique_ptr<PartialPolicy<State, Action>> compute_policy(
        MDP<State, Action>& mdp,
        Evaluator<State>& heuristic,
        param_type<State> state,
        double max_time = std::numeric_limits<double>::infinity()) override
    {
        this->solve(mdp, heuristic, state, max_time);

        /*
         * Expand some greedy policy graph, starting from the initial state.
         * Collect optimal actions along the way.
         */

        std::unique_ptr<policies::MapPolicy<State, Action>> policy(
            new policies::MapPolicy<State, Action>(&mdp));

        const StateID initial_state_id = mdp.get_state_id(state);

        std::deque<StateID> queue;
        std::set<StateID> visited;
        queue.push_back(initial_state_id);
        visited.insert(initial_state_id);

        do {
            const StateID state_id = queue.front();
            queue.pop_front();

            std::optional<Action> action;

            if constexpr (StorePolicy) {
                action = this->lookup_policy(state_id);
            } else {
                action = this->lookup_policy(mdp, heuristic, state_id);
            }

            // Terminal states have no policy decision.
            if (!action) {
                continue;
            }

            const Interval bound = this->lookup_bounds(state_id);

            policy->emplace_decision(state_id, *action, bound);

            // Push the successor traps.
            Distribution<StateID> successors;
            mdp.generate_action_transitions(state_id, *action, successors);

            for (const StateID succ_id : successors.support()) {
                if (visited.insert(succ_id).second) {
                    queue.push_back(succ_id);
                }
            }
        } while (!queue.empty());

        return policy;
    }

    void print_statistics(std::ostream& out) const final override
    {
        statistics_.print(out);
        this->print_additional_statistics(out);
    }

    value_t lookup_value(StateID state_id) const
    {
        return get_state_info(state_id).get_value();
    }

    Interval lookup_bounds(StateID state_id) const
    {
        return get_state_info(state_id).get_bounds();
    }

    std::optional<Action> lookup_policy(
        MDP<State, Action>& mdp,
        Evaluator<State>& heuristic,
        StateID state_id)
        requires(!StorePolicy)
    {
        std::vector<Action> opt_aops;
        std::vector<Distribution<StateID>> opt_transitions;
        compute_optimal_transitions(
            mdp,
            heuristic,
            state_id,
            lookup_initialize(mdp, heuristic, state_id),
            opt_aops,
            opt_transitions);

        if (opt_aops.empty()) return std::nullopt;

        return opt_aops[this->policy_chooser_->pick_index(
            mdp,
            state_id,
            std::nullopt,
            opt_aops,
            opt_transitions,
            state_infos_)];
    }

    std::optional<Action> lookup_policy(StateID state_id)
        requires(StorePolicy)
    {
        return get_state_info(state_id).policy;
    }

    /**
     * @brief Resets the heuristic search engine to a clean state.
     *
     * This method is needed by the FRET wrapper engine to restart the heuristic
     * search after traps have been collapsed.
     */
    virtual void reset_search_state() {}

    /**
     * @brief Checks if the state \p state_id is terminal.
     */
    bool is_terminal(StateID state_id) const
    {
        return get_state_info(state_id).is_terminal();
    }

    /**
     * @brief Checks if the state represented by \p state_id is marked as a
     * dead-end.
     */
    bool is_marked_dead_end(StateID state_id) const
    {
        return get_state_info(state_id).is_dead_end();
    }

    /**
     * @brief Checks if the state represented by \p state_id has been visited
     * yet.
     */
    bool was_visited(StateID state_id) const
    {
        return get_state_info(state_id).is_value_initialized();
    }

    /**
     * @brief Clears the currently selected greedy action for the state
     * represented by \p state_id
     */
    void clear_policy(StateID state_id)
    {
        static_assert(StorePolicy, "Policy not stored by algorithm!");

        get_state_info(state_id).clear_policy();
    }

    /**
     * @brief Gets the currently selected greedy action for the state
     * represented by \p state_id
     */
    Action get_policy(StateID state_id)
    {
        static_assert(StorePolicy, "Policy not stored by algorithm!");

        std::optional a = get_state_info(state_id).get_policy();
        assert(a.has_value());
        return *a;
    }

    /**
     * @brief Generates the successor distribution referring to the application
     * of the current greedy action in a state
     *
     * @param[in] state - The id of the source state
     * @param[out] result - The returned successor distribution when applying
     * the current greedy action in the state represented by \p state
     */
    bool apply_policy(
        MDP<State, Action>& mdp,
        Evaluator<State>& heuristic,
        StateID state_id,
        Distribution<StateID>& result)
    {
        static_assert(StorePolicy, "Policy not stored by algorithm!");

        std::optional a = get_state_info(state_id).get_policy();

        if (!a) {
            return async_update(mdp, heuristic, state_id, &result)
                .value_changed;
        }

        mdp.generate_action_transitions(state_id, *a, result);
        return false;
    }

    /**
     * @brief Calls notify_dead_end(StateInfo&) with the respective state info
     * object
     */
    bool notify_dead_end(StateID state_id)
    {
        return notify_dead_end(get_state_info(state_id));
    }

    /**
     * @brief Stores dead-end information in \p state_info. Returns true on
     * change.
     */
    bool notify_dead_end(StateInfo& state_info)
    {
        if (!state_info.is_dead_end()) {
            state_info.set_dead_end();
            state_info.value = EngineValueType(state_info.termination_cost);
            return true;
        }

        return false;
    }

    /**
     * @brief Calls notify_dead_end_ifnot_goal(StateInfo&) for the internal
     * state info object of \p state_id.
     */
    bool notify_dead_end_ifnot_goal(StateID state_id)
    {
        return notify_dead_end_ifnot_goal(get_state_info(state_id));
    }

    /**
     * @brief If no goal state flag was set, calls notify_dead_end(StateInfo&).
     *
     * Returns true if the goal flag was set.
     */
    bool notify_dead_end_ifnot_goal(StateInfo& state_info)
    {
        if (state_info.is_goal_state()) {
            return false;
        }

        notify_dead_end(state_info);

        return true;
    }

    /**
     * @brief Computes the value update for a state and returns whether the
     * value changed.
     *
     * If the policy is stored, the greedy action for s is also updated using
     * the internal policy tiebreaking settings.
     */
    bool async_update(
        MDP<State, Action>& mdp,
        Evaluator<State>& heuristic,
        StateID s)
    {
        if constexpr (!StorePolicy) {
            return compute_value_update(
                mdp,
                heuristic,
                s,
                lookup_initialize(mdp, heuristic, s));
        } else {
            return compute_value_policy_update(mdp, heuristic, s, nullptr)
                .value_changed;
        }
    }

    /**
     * @brief Computes the value and policy update for a state and optionally
     * outputs the new greedy transition.
     *
     * @note Output parameters may be nullptr. Only applicable if the policy is
     * stored.
     *
     * @param[in] s - The state for the value update
     * @param[out] policy_transition - Return address for the new greedy
     * transition.
     */
    UpdateResult async_update(
        MDP<State, Action>& mdp,
        Evaluator<State>& heuristic,
        StateID s,
        Distribution<StateID>* policy_transition)
    {
        return compute_value_policy_update(
            mdp,
            heuristic,
            s,
            policy_transition);
    }

    bool compute_value_update_and_optimal_transitions(
        MDP<State, Action>& mdp,
        Evaluator<State>& heuristic,
        StateID state_id,
        std::vector<Action>& opt_aops,
        std::vector<Distribution<StateID>>& opt_transitions)
    {
        return compute_value_update_and_optimal_transitions(
            mdp,
            heuristic,
            state_id,
            lookup_initialize(mdp, heuristic, state_id),
            opt_aops,
            opt_transitions);
    }

protected:
    /**
     * @brief Solves for the optimal state value of the input state.
     *
     * Called internally after initializing the progress report.
     */
    virtual Interval do_solve(
        MDP<State, Action>& mdp,
        Evaluator<State>& heuristic,
        param_type<State> state,
        double max_time) = 0;

    /**
     * @brief Prints additional statistics to the output stream.
     *
     * Called internally after printing the base heuristic search statistics.
     */
    virtual void print_additional_statistics(std::ostream& out) const = 0;

    /**
     * @brief Sets up internal custom reports of a state in an implementation.
     */
    virtual void setup_custom_reports(param_type<State>) {}

    /**
     * @brief Advances the progress report.
     */
    void print_progress() { this->report_->print(); }

    bool check_interval_comparison() const { return interval_comparison_; }

    /**
     * @brief Get the state info object of a state.
     */
    StateInfo& get_state_info(StateID id) { return state_infos_[id]; }

    /**
     * @brief Get the state info object of a state.
     */
    const StateInfo& get_state_info(StateID id) const
    {
        return state_infos_[id];
    }

    /**
     * @brief Get the state info object of a state, if needed.
     *
     * This method is used as a selection mechanism to obtain the correct
     * state information object for a state. Algorithms like LRTDP may or
     * may not store their algorithm specific state information separately
     * from the base state information stored in this class. This method
     * checks if the provided state info object is the required base state
     * information object by checking for type equality and returns it if that
     * is the case. Otherwise, the base state information object for this state
     * is retrieved and returned.
     */
    template <typename AlgStateInfo>
    StateInfo& get_state_info(const StateID id, AlgStateInfo& info)
    {
        if constexpr (std::is_same_v<AlgStateInfo, StateInfo>) {
            return info;
        } else {
            return get_state_info(id);
        }
    }

    /**
     * @brief Get the state info object of a state, if needed.
     *
     * This method is used as a selection mechanism to obtain the correct
     * state information object for a state. Algorithms like LRTDP may or
     * may not store their algorithm specific state information separately
     * from the base state information stored in this class. This method
     * checks if the provided state info object is the required base state
     * information object by checking for type equality and returns it if that
     * is the case. Otherwise, the base state information object for this state
     * is retrieved and returned.
     */
    template <typename AlgStateInfo>
    const StateInfo& get_state_info(StateID id, const AlgStateInfo& info) const
    {
        if constexpr (std::is_same_v<AlgStateInfo, StateInfo>) {
            return info;
        } else {
            return get_state_info(id);
        }
    }

    StateID sample_state(
        engine_interfaces::SuccessorSampler<Action>& sampler,
        StateID source,
        const Distribution<StateID>& transition)
    {
        return sampler
            .sample(source, this->get_policy(source), transition, state_infos_);
    }

private:
    void initialize_report(
        MDP<State, Action>& mdp,
        Evaluator<State>& heuristic,
        param_type<State> state)
    {
        initial_state_id_ = mdp.get_state_id(state);

        StateInfo& info = get_state_info(initial_state_id_);

        if (!initialize_if_needed(mdp, heuristic, initial_state_id_, info)) {
            return;
        }

        if constexpr (UseInterval) {
            report_->register_bound("v", [&info]() { return info.value; });
        } else {
            report_->register_bound("v", [&info]() {
                return Interval(info.value, INFINITE_VALUE);
            });
        }

        statistics_.value = info.get_value();
        statistics_.before_last_update = statistics_;
        statistics_.initial_state_estimate = info.get_value();
        statistics_.initial_state_found_terminal = info.is_terminal();

        setup_custom_reports(state);
    }

    bool update(StateInfo& state_info, const EngineValueType& other)
    {
        if constexpr (UseInterval) {
            return engines::update(
                state_info.value,
                other,
                interval_comparison_);
        } else {
            return engines::update(state_info.value, other);
        }
    }

    StateInfo& lookup_initialize(
        MDP<State, Action>& mdp,
        Evaluator<State>& heuristic,
        StateID state_id)
    {
        StateInfo& state_info = get_state_info(state_id);
        initialize_if_needed(mdp, heuristic, state_id, state_info);
        return state_info;
    }

    bool initialize_if_needed(
        MDP<State, Action>& mdp,
        Evaluator<State>& heuristic,
        StateID state_id,
        StateInfo& state_info)
    {
        if (!state_info.is_value_initialized()) {
            statistics_.evaluated_states++;

            State state = mdp.get_state(state_id);
            TerminationInfo term = mdp.get_termination_info(state);
            const value_t t_cost = term.get_cost();

            state_info.termination_cost = t_cost;
            if (term.is_goal_state()) {
                state_info.set_goal();
                state_info.value = EngineValueType(t_cost);
                statistics_.goal_states++;
                if (on_new_state_) on_new_state_->notify_goal(state);
                return true;
            }

            EvaluationResult estimate = heuristic.evaluate(state);
            if (estimate.is_unsolvable()) {
                statistics_.pruned_states++;
                notify_dead_end(state_info);
                if (on_new_state_) on_new_state_->notify_dead(state);
            } else {
                state_info.set_on_fringe();

                if constexpr (UseInterval) {
                    state_info.value.lower = estimate.get_estimate();
                } else {
                    state_info.value = estimate.get_estimate();
                }

                if (on_new_state_) on_new_state_->notify_state(state);
            }

            return true;
        }

        return false;
    }

    bool compute_value_update(
        MDP<State, Action>& mdp,
        Evaluator<State>& heuristic,
        StateID state_id,
        StateInfo& state_info)
    {
#if defined(EXPENSIVE_STATISTICS)
        TimerScope scoped_upd_timer(statistics_.update_time);
#endif
        statistics_.backups++;

        if (state_info.is_terminal()) {
            return false;
        }

        if (state_info.is_on_fringe()) {
            ++statistics_.backed_up_states;
            state_info.removed_from_fringe();
        }

        std::vector<Action> aops;
        std::vector<Distribution<StateID>> transitions;
        mdp.generate_all_transitions(state_id, aops, transitions);

        assert(aops.size() == transitions.size());

        if (aops.empty()) {
            statistics_.terminal_states++;
            const bool result = notify_dead_end(state_info);
            if (result) {
                ++statistics_.value_changes;
                if (state_id == initial_state_id_) {
                    statistics_.jump();
                }
            }
            return result;
        }

        EngineValueType new_value(state_info.termination_cost);

        bool has_only_self_loops = true;
        for (unsigned i = 0; i < aops.size(); ++i) {
            Action& op = aops[i];
            Distribution<StateID>& transition = transitions[i];

            EngineValueType t_value(mdp.get_action_cost(op));
            value_t self_loop = 0_vt;
            bool non_loop = false;

            for (const auto& [succ_id, prob] : transition) {
                if (succ_id == state_id) {
                    self_loop += prob;
                } else {
                    const StateInfo& succ_info =
                        lookup_initialize(mdp, heuristic, succ_id);
                    t_value += prob * succ_info.value;
                    non_loop = true;
                }
            }

            if (non_loop) {
                if (self_loop > 0_vt) {
                    t_value *= 1_vt / (1_vt - self_loop);
                }

                set_min(new_value, t_value);
                has_only_self_loops = false;
            }
        }

        if (has_only_self_loops) {
            statistics_.self_loop_states++;
            return notify_dead_end(state_info);
        }

        if (this->update(state_info, new_value)) {
            ++statistics_.value_changes;
            if (state_id == initial_state_id_) {
                statistics_.jump();
            }
            return true;
        }

        return false;
    }

    EngineValueType compute_non_loop_transitions_and_values(
        MDP<State, Action>& mdp,
        Evaluator<State>& heuristic,
        StateID state_id,
        StateInfo& state_info,
        std::vector<Action>& aops,
        std::vector<Distribution<StateID>>& transitions,
        std::vector<EngineValueType>& values)
    {
        mdp.generate_all_transitions(state_id, aops, transitions);

        assert(aops.size() == transitions.size());

        values.reserve(aops.size());

        EngineValueType best_value(state_info.termination_cost);

        unsigned non_loop_end = 0;
        for (unsigned i = 0; i < aops.size(); ++i) {
            Action& op = aops[i];
            Distribution<StateID>& transition = transitions[i];

            EngineValueType t_value(mdp.get_action_cost(op));
            value_t self_loop = 0_vt;
            bool non_loop = false;

            for (const auto& [succ_id, prob] : transition) {
                if (succ_id == state_id) {
                    self_loop += prob;
                } else {
                    const StateInfo& succ_info =
                        lookup_initialize(mdp, heuristic, succ_id);
                    t_value += prob * succ_info.value;
                    non_loop = true;
                }
            }

            if (non_loop) {
                if (self_loop > 0_vt) {
                    t_value *= 1_vt / (1_vt - self_loop);
                }

                if (non_loop_end != i) {
                    aops[non_loop_end] = std::move(op);
                    transitions[non_loop_end] = std::move(transition);
                }

                ++non_loop_end;

                set_min(best_value, t_value);
                values.push_back(t_value);
            }
        }

        // erase self-loop states
        aops.erase(aops.begin() + non_loop_end, aops.end());
        transitions.erase(
            transitions.begin() + non_loop_end,
            transitions.end());

        return best_value;
    }

    EngineValueType compute_optimal_transitions(
        MDP<State, Action>& mdp,
        Evaluator<State>& heuristic,
        StateID state_id,
        StateInfo& state_info,
        std::vector<Action>& opt_aops,
        std::vector<Distribution<StateID>>& opt_transitions)
    {
        std::vector<EngineValueType> values;
        const EngineValueType best_value =
            compute_non_loop_transitions_and_values(
                mdp,
                heuristic,
                state_id,
                state_info,
                opt_aops,
                opt_transitions,
                values);

        if (opt_aops.empty()) {
            return best_value;
        }

        unsigned optimal_end = 0;
        for (unsigned i = 0; i < opt_aops.size(); ++i) {
            if (is_approx_equal(
                    as_lower_bound(best_value),
                    as_lower_bound(values[i]))) {
                if (optimal_end != i) {
                    opt_aops[optimal_end] = std::move(opt_aops[i]);
                    opt_transitions[optimal_end] =
                        std::move(opt_transitions[i]);
                }

                ++optimal_end;
            }
        }

        // Erase non-optimal transitions
        opt_aops.erase(opt_aops.begin() + optimal_end, opt_aops.end());
        opt_transitions.erase(
            opt_transitions.begin() + optimal_end,
            opt_transitions.end());

        return best_value;
    }

    bool compute_value_update_and_optimal_transitions(
        MDP<State, Action>& mdp,
        Evaluator<State>& heuristic,
        StateID state_id,
        StateInfo& state_info,
        std::vector<Action>& opt_aops,
        std::vector<Distribution<StateID>>& opt_transitions)
    {
#if defined(EXPENSIVE_STATISTICS)
        TimerScope scoped_upd_timer(statistics_.update_time);
#endif
        statistics_.backups++;

        if (state_info.is_terminal()) {
            return false;
        }

        if (state_info.is_on_fringe()) {
            ++statistics_.backed_up_states;
            state_info.removed_from_fringe();
        }

        EngineValueType optimal_value = compute_optimal_transitions(
            mdp,
            heuristic,
            state_id,
            state_info,
            opt_aops,
            opt_transitions);

        if (opt_aops.empty()) {
            statistics_.self_loop_states++;
            return notify_dead_end(state_info);
        }

        if (this->update(state_info, optimal_value)) {
            ++statistics_.value_changes;
            if (state_id == initial_state_id_) {
                statistics_.jump();
            }
            return true;
        }

        return false;
    }

    UpdateResult compute_value_policy_update(
        MDP<State, Action>& mdp,
        Evaluator<State>& heuristic,
        StateID state_id,
        Distribution<StateID>* greedy_transition)
    {
        static_assert(StorePolicy, "Policy not stored by algorithm!");

        std::vector<Action> opt_aops;
        std::vector<Distribution<StateID>> opt_transitions;

        StateInfo& state_info = lookup_initialize(mdp, heuristic, state_id);

        const bool value_changed = compute_value_update_and_optimal_transitions(
            mdp,
            heuristic,
            state_id,
            state_info,
            opt_aops,
            opt_transitions);

        if (opt_aops.empty()) {
            state_info.clear_policy();
            return UpdateResult{value_changed, false};
        }

        auto [policy_changed, action] = compute_policy_update(
            mdp,
            state_id,
            state_info,
            opt_aops,
            opt_transitions,
            greedy_transition);

        return UpdateResult{value_changed, policy_changed, std::move(action)};
    }

    std::pair<bool, Action> compute_policy_update(
        MDP<State, Action>& mdp,
        StateID state_id,
        StateInfo& state_info,
        std::vector<Action>& opt_aops,
        std::vector<Distribution<StateID>>& opt_transitions,
        Distribution<StateID>* greedy_transition)
    {
#if defined(EXPENSIVE_STATISTICS)
        TimerScope scoped(statistics_.policy_selection_time);
#endif

        ++statistics_.policy_updates;

        const int index = this->policy_chooser_->pick_index(
            mdp,
            state_id,
            state_info.get_policy(),
            opt_aops,
            opt_transitions,
            state_infos_);
        assert(utils::in_bounds(index, opt_aops));

        Action& action = opt_aops[index];

        if (greedy_transition != nullptr) {
            (*greedy_transition) = std::move(opt_transitions[index]);
        }

        return {state_info.update_policy(action), std::move(action)};
    }
};

} // namespace internal

template <typename T>
struct NoAdditionalStateData : public T {};

template <
    typename State,
    typename Action,
    bool UseInterval = false,
    bool StorePolicy = false,
    template <typename> class StateInfo = NoAdditionalStateData>
using HeuristicSearchBase = internal::HeuristicSearchBase<
    State,
    Action,
    StateInfo<PerStateBaseInformation<Action, StorePolicy, UseInterval>>>;

} // namespace heuristic_search
} // namespace engines
} // namespace probfd

#endif // __HEURISTIC_SEARCH_BASE_H__