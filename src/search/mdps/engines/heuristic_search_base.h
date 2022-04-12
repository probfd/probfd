#ifndef MDPS_ENGINES_HEURISTIC_SEARCH_BASE_H
#define MDPS_ENGINES_HEURISTIC_SEARCH_BASE_H

#include "../engine_interfaces/dead_end_listener.h"
#include "../engine_interfaces/heuristic_search_connector.h"
#include "../engine_interfaces/new_state_handler.h"
#include "../engine_interfaces/policy_picker.h"
#include "../progress_report.h"
#include "../storage/per_state_storage.h"
#include "../utils/graph_visualization.h"
#include "../value_utils.h"
#include "engine.h"
#include "heuristic_search_state_information.h"

#if defined(EXPENSIVE_STATISTICS)
#include "../../utils/timer.h"
#endif

#include <cassert>
#include <deque>
#include <fstream>
#include <iostream>
#include <limits>
#include <type_traits>
#include <vector>

namespace probabilistic {
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
    value_type::value_t initial_state_estimate = 0;
    bool initial_state_found_terminal = 0;

    unsigned long long externally_set_dead_ends = 0;
    unsigned long long dead_end_safe_updates = 0;
    unsigned long long dead_end_safe_updates_states = 0;
    unsigned long long dead_end_safe_updates_dead_ends = 0;

    unsigned long long wrongly_classified_dead_ends = 0;
    unsigned long long safe_updates_non_dead_end_value = 0;

    value_type::value_t value = value_type::zero;
    CoreStatistics before_last_update;

#if defined(EXPENSIVE_STATISTICS)
    utils::Timer update_time;
    utils::Timer policy_selection_time;
#endif

    Statistics()
    {
#if defined(EXPENSIVE_STATISTICS)
        update_time.stop();
        policy_selection_time.stop();
#endif
    }

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

        out << "  Number of dead-end identification runs: "
            << dead_end_safe_updates << std::endl;
        out << "  Dead-end identification runs on states with non dead-end "
               "value: "
            << safe_updates_non_dead_end_value << std::endl;
        out << "  States considered while checking dead-end property: "
            << dead_end_safe_updates_states << std::endl;
        out << "  Wrongly classified dead-ends: "
            << wrongly_classified_dead_ends << std::endl;
        out << "  Dead-ends identified: " << dead_end_safe_updates_dead_ends
            << std::endl;
        out << "  Externally set dead-ends: " << externally_set_dead_ends
            << std::endl
            << std::flush;
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
template <typename StateT, typename ActionT, typename StateInfoT>
class HeuristicSearchBase
    : public MDPEngine<StateT, ActionT>
    , public PerStateInformationLookup {
public:
    using State = StateT;
    using Action = ActionT;
    using StateInfo = StateInfoT;
    using StorePolicy = typename StateInfo::StoresPolicy;
    using DualBounds = typename StateInfo::DualBounds;

    using IncumbentSolution = value_utils::IncumbentSolution<DualBounds>;

    explicit HeuristicSearchBase(
        StateIDMap<State>* state_id_map,
        ActionIDMap<Action>* action_id_map,
        StateRewardFunction<State>* state_reward_function,
        ActionRewardFunction<Action>* action_reward_function,
        value_utils::IntervalValue reward_bound,
        ApplicableActionsGenerator<Action>* aops_generator,
        TransitionGenerator<Action>* transition_generator,
        StateEvaluator<State>* dead_end_eval,
        DeadEndListener<State, Action>* dead_end_listener,
        PolicyPicker<Action>* policy_chooser,
        NewStateHandler<State>* new_state_handler,
        StateEvaluator<State>* value_init,
        HeuristicSearchConnector* connector,
        ProgressReport* report,
        bool interval_comparison,
        bool stable_policy)
        : MDPEngine<State, Action>(
              state_id_map,
              action_id_map,
              state_reward_function,
              action_reward_function,
              reward_bound,
              aops_generator,
              transition_generator)
        , report_(report)
        , interval_comparison_(interval_comparison)
        , stable_policy_(stable_policy)
        , value_initializer_(value_init)
        , policy_chooser_(policy_chooser)
        , on_new_state_(new_state_handler)
        , dead_end_listener_(dead_end_listener)
        , dead_end_eval_(dead_end_eval)
        , dead_end_value_(this->get_minimal_reward())
    {
        statistics_.state_info_bytes = sizeof(StateInfo);
        connector->set_lookup_function(this);
    }

    virtual ~HeuristicSearchBase() = default;

    /**
     * @copydoc MDPEngineInterface<State>::get_result(const State&)
     */
    virtual value_type::value_t get_result(const State& s) override
    {
        const StateInfo& info = state_infos_[this->get_state_id(s)];
        return value_utils::as_upper_bound(info.value);
    }

    /**
     * @copydoc MDPEngineInterface<State>::supports_error_bound()
     */
    virtual bool supports_error_bound() const override
    {
        return DualBounds::value;
    }

    /**
     * @copydoc MDPEngineInterface<State>::get_error()
     */
    virtual value_type::value_t get_error(const State& s) override
    {
        if constexpr (DualBounds::value) {
            const StateInfo& info = state_infos_[this->get_state_id(s)];
            return info.value.error_bound();
        } else {
            return std::numeric_limits<value_type::value_t>::infinity();
        }
    }

    /**
     * @copydoc MDPEngineInterface<State>::print_statistics(std::ostream&)
     */
    virtual void print_statistics(std::ostream& out) const override
    {
        statistics_.print(out);
    }

    /**
     * @copydoc PerStateInformationLookup::lookup(const StateID&)
     */
    virtual const void* lookup(const StateID& state_id) override
    {
        return &state_infos_[state_id];
    }

    /**
     * @brief Checks if the state \p state_id is terminal.
     */
    bool is_terminal(const StateID& state_id)
    {
        return state_infos_[state_id].is_terminal();
    }

    /**
     * @brief Gets the current value of the state represented by \p state_id
     */
    value_type::value_t get_value(const StateID& state_id) const
    {
        return state_infos_[state_id].get_value();
    }

    /**
     * @brief Chekcs if the state represented by \p state_id is marked as a
     * dead-end.
     */
    bool is_marked_dead_end(const StateID& state)
    {
        return state_infos_[state].is_dead_end();
    }

    /**
     * @brief Clears the currently selected greedy action for the state
     * represented by \p state_id
     */
    void clear_policy(const StateID& state_id)
    {
        static_assert(StorePolicy::value, "Policy not stored by algorithm!");

        state_infos_[state_id].set_policy(ActionID::undefined);
    }

    /**
     * @brief Gets the currently selected greedy action for the state
     * represented by \p state_id
     */
    Action get_policy(const StateID& state_id)
    {
        static_assert(StorePolicy::value, "Policy not stored by algorithm!");

        const ActionID aid = state_infos_[state_id].get_policy();
        assert(aid != ActionID::undefined);
        return this->lookup_action(state_id, aid);
    }

    /**
     * @brief Generates the successor distribution referring to the application
     * of the current greedy action in a state
     *
     * @param[in] state - The id of the source state
     * @param[out] result - The returned successor distribution when applying
     * the current greedy action in the state represented by \p state
     */
    bool apply_policy(const StateID& state, Distribution<StateID>& result)
    {
        static_assert(StorePolicy::value, "Policy not stored by algorithm!");

        const StateInfo& info = state_infos_[state];
        if (info.policy == ActionID::undefined) {
            return async_update(state, nullptr, &result, nullptr);
        }

        Action action = this->lookup_action(state, info.policy);
        this->generate_successors(state, action, result);
        return false;
    }

    /**
     * @brief Calls notify_dead_end(const StateID&, StateInfo&) with the
     * respective state info object
     */
    bool notify_dead_end(const StateID& state_id)
    {
        return notify_dead_end(state_id, state_infos_[state_id]);
    }

    /**
     * @brief If \p state_id has not been recognized as a dead-end before,
     * stores this information in \p state_info, notifies the attached
     * dead-end listener of this new dead-end, and returns true.
     * Otherwise returns false.
     */
    bool notify_dead_end(const StateID& state_id, StateInfo& state_info)
    {
        if (!state_info.is_dead_end()) {
            state_info.set_dead_end();
            state_info.value = dead_end_value_;

            if (dead_end_listener_ != nullptr) {
                dead_end_listener_->operator()(state_id);
            }

            return true;
        }

        return false;
    }

    /**
     * @brief Calls notify_dead_end_ifnot_goal(const StateID&, StateInfo&) for
     * the internal state info object of \p state_id.
     */
    bool notify_dead_end_ifnot_goal(const StateID& state_id)
    {
        return notify_dead_end_ifnot_goal(state_id, state_infos_[state_id]);
    }

    /**
     * @brief If \p state_id is not a goal state, calls
     * notify_dead_end(const StateID&, StateInfo&)
     *
     * Returns true if the state is not a goal state.
     */
    bool
    notify_dead_end_ifnot_goal(const StateID& state_id, StateInfo& state_info)
    {
        if (state_info.is_goal_state()) {
            return false;
        }

        notify_dead_end(state_id, state_info);

        return true;
    }

    /**
     * @brief Checks whether the attached dead-end evaluator recognizes the
     * state as a dead-end. If yes and the state has not been recognized before,
     * the state is marked as a dead-end and the dead end listener is notified.
     */
    bool check_dead_end(const StateID& state_id)
    {
        if (dead_end_eval_ != nullptr) {
            State state = this->lookup_state(state_id);
            if (dead_end_eval_->operator()(state)) {
                notify_dead_end(state_id);
                return true;
            }
        }

        return false;
    }

    /**
     * @brief Checks if a dead-end listener is attached.
     */
    bool is_dead_end_learning_enabled() const
    {
        return dead_end_listener_ != nullptr;
    }

    /**
     * @brief Computes the value update for a state and returns whether the
     * value changed.
     *
     * If the policy is stored, the greedy action for s is also updated using
     * the internal policy tiebreaking settings.
     */
    bool async_update(const StateID& s)
    {
        if constexpr (!StorePolicy::value) {
            return compute_value_update(s);
        } else {
            return async_update(s, nullptr, nullptr, nullptr);
        }
    }

    /**
     * @brief Computes the value update for a state and outputs the new greedy
     * action, transition, and whether the policy and value changed. Output
     * parameters may be nullptr.
     *
     * Only applicable if the policy is stored.
     *
     * @param[in] s - The state for the value update
     * @param[out] policy_action - Return address for the new greedy action.
     * @param[out] policy_transition - Return address for the new greedy
     * transition.
     * @param[out] policy_changed - Return address for the policy change flag.
     *
     * @return true - If the value changed.
     * @return false - Otherwise.
     */
    bool async_update(
        const StateID& s,
        ActionID* policy_action,
        Distribution<StateID>* policy_transition,
        bool* policy_changed)
    {
        return async_update(
            s,
            stable_policy_,
            this->policy_chooser_,
            policy_action,
            policy_transition,
            policy_changed);
    }

    /**
     * @brief Computes the value update for a state and outputs the new greedy
     * action, transition, and whether the policy and value changed, where ties
     * between optimal actions are broken by the supplied policy tiebreaker.
     * The policy tiebreaker may not be nullptr.
     *
     * Only applicable if the policy is stored.
     *
     * @param[in] s - The state for the value update
     * @param[out] policy_tiebreaker - A pointer to a function object breaking
     * ties between optimal actions.
     * @param[out] policy_action - Return address for the new greedy action.
     * @param[out] policy_transition - Return address for the new greedy
     * transition.
     * @param[out] policy_changed - Return address for the policy change flag.
     *
     * @return true - If the value changed.
     * @return false - Otherwise.
     */
    template <typename T>
    bool async_update(
        const StateID& s,
        T* policy_tiebreaker,
        ActionID* policy_action,
        Distribution<StateID>* policy_transition,
        bool* policy_changed)
    {
        return async_update(
            s,
            false,
            policy_tiebreaker,
            policy_action,
            policy_transition,
            policy_changed);
    }

protected:
    bool update(StateInfo& state_info, const IncumbentSolution& other)
    {
        if constexpr (DualBounds::value) {
            return value_utils::update(
                state_info.value,
                other,
                interval_comparison_);
        } else {
            return value_utils::update(state_info.value, other);
        }
    }

    /**
     * @brief Initializes the progress report with the given initial state.
     */
    void initialize_report(const State& state)
    {
        initial_state_id_ = this->get_state_id(state);

        static bool initialized = false;
        if (initialized) {
            return;
        }
        initialized = true;

        const StateInfo& info = lookup_initialize(this->get_state_id(state));
        this->add_values_to_report(&info);
        statistics_.value = value_utils::as_upper_bound(info.value);
        statistics_.before_last_update = statistics_;
        statistics_.initial_state_estimate =
            value_utils::as_upper_bound(info.value);
        statistics_.initial_state_found_terminal = info.is_terminal();

        setup_custom_reports(state);
    }

    /**
     * @brief Advances the progress report.
     */
    void report(const StateID) { this->report_->operator()(); }

    /**
     * @brief Sets up internal custom reports of a state in an implementation.
     */
    virtual void setup_custom_reports(const State&) {}

    /**
     * @brief Get the state info storage.
     */
    storage::PerStateStorage<StateInfo>& get_state_info_store()
    {
        return state_infos_;
    }

    /**
     * @brief Get the state info storage.
     */
    const storage::PerStateStorage<StateInfo>& get_state_info_store() const
    {
        return state_infos_;
    }

    /**
     * @brief Get the state info object of a state.
     */
    StateInfo& get_state_info(const StateID& id) { return state_infos_[id]; }

    /**
     * @brief Get the state info object of a state.
     */
    StateInfo& get_state_info(const StateID& id) const
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
    const StateInfo&
    get_state_info(const StateID id, const AlgStateInfo& info) const
    {
        if constexpr (std::is_same_v<AlgStateInfo, StateInfo>) {
            return info;
        } else {
            return get_state_info(id);
        }
    }

    template <typename Info>
    bool do_bounds_disagree(const StateID& state_id, const Info& info)
    {
        if constexpr (DualBounds::value) {
            if constexpr (std::is_same_v<Info, StateInfo>) {
                return interval_comparison_ && !info.value.bounds_equal();
            } else {
                return interval_comparison_ &&
                       !state_infos_[state_id].value.bounds_equal();
            }
        } else {
            return false;
        }
    }

    /**
     * @brief Dumps the search space as a graph.
     *
     * State names are printed as specified by the state-to-string lambda
     * function object.
     *
     * @tparam StateToString - Type of the state-to-string function object.
     * @param file_name - The output file name.
     * @param sstr -The state-to-string lambda function.
     */
    template <typename StateToString>
    void dump_search_space(
        const std::string& file_name,
        const StateToString& sstr = graphviz::DefaultSTS())
    {
        struct ExpansionCondition : public StateEvaluator<State> {
            explicit ExpansionCondition(
                const MDPEngine<State, Action>* hs,
                storage::PerStateStorage<StateInfo>* infos)
                : hs_(hs)
                , infos_(infos)
            {
            }

            EvaluationResult operator()(const State& state) const override
            {
                const StateID sid = hs_->get_state_id(state);
                const StateInfo& info = infos_->operator[](sid);
                return EvaluationResult(info.is_on_fringe(), 0);
            }

            const MDPEngine<State, Action>* hs_;
            storage::PerStateStorage<StateInfo>* infos_;
        };

        ExpansionCondition prune(this, &state_infos_);

        std::ofstream out(file_name);

        graphviz::dump<State>(
            out,
            this->lookup_state(initial_state_id_),
            this->get_state_id_map(),
            this->get_state_reward_function(),
            this->get_applicable_actions_generator(),
            this->get_transition_generator(),
            sstr,
            graphviz::DefaultATS(),
            &prune,
            false);
    }

private:
    void add_values_to_report(const StateInfo* info)
    {
        if constexpr (DualBounds::value) {
            report_->register_value("vl", [info]() {
                return value_utils::as_lower_bound(info->value);
            });
            report_->register_value("vu", [info]() {
                return value_utils::as_upper_bound(info->value);
            });
        } else {
            report_->register_value("v", [info]() {
                return value_utils::as_upper_bound(info->value);
            });
        }
    }

    StateInfo& lookup_initialize(const StateID& state_id)
    {
        StateInfo& info = state_infos_[state_id];
        initialize(state_id, info);
        return info;
    }

    void initialize(const StateID& state_id, StateInfo& state_info)
    {
        if (!state_info.is_value_initialized()) {
            statistics_.evaluated_states++;
            State state = this->lookup_state(state_id);
            auto estimate = this->get_state_reward(state);
            if (estimate) {
                state_info.set_goal();
                state_info.value =
                    IncumbentSolution((value_type::value_t)estimate);
                statistics_.goal_states++;
                if (on_new_state_) on_new_state_->touch_goal(state);
            } else {
                estimate = value_initializer_->operator()(state);
                if (estimate) {
                    statistics_.pruned_states++;
                    notify_dead_end(state_id, state_info);
                    if (on_new_state_) on_new_state_->touch_dead_end(state);
                } else {
                    state_info.set_on_fringe();

                    if constexpr (DualBounds::value) {
                        state_info.value.upper =
                            static_cast<value_type::value_t>(estimate);
                    } else {
                        state_info.value =
                            static_cast<value_type::value_t>(estimate);
                    }

                    if (on_new_state_) on_new_state_->touch(state);
                }
            }
        }
    }

    IncumbentSolution dead_end_value() const { return dead_end_value_; }

    template <typename T>
    bool async_update(
        const StateID& s,
        const bool stable_policy,
        T* policy_tiebreaker,
        ActionID* greedy_action,
        Distribution<StateID>* greedy_transition,
        bool* action_changed)
    {
        static_assert(StorePolicy::value, "Policy not stored by algorithm!");

        if (policy_tiebreaker == nullptr) {
            return compute_value_update(s);
        } else {
            return compute_value_update(
                s,
                stable_policy,
                *policy_tiebreaker,
                greedy_action,
                greedy_transition,
                action_changed);
        }
    }

    bool compute_value_update(const StateID& state_id)
    {
        std::vector<Action> aops;
        std::vector<Distribution<StateID>> transitions;
        IncumbentSolution new_value;
        std::vector<IncumbentSolution> values;

        return compute_value_update(
            state_id,
            lookup_initialize(state_id),
            aops,
            transitions,
            new_value,
            values);
    }

    bool compute_value_update(
        const StateID& state_id,
        StateInfo& state_info,
        std::vector<Action>& aops,
        std::vector<Distribution<StateID>>& transitions,
        IncumbentSolution& new_value,
        std::vector<IncumbentSolution>& values)
    {
#if defined(EXPENSIVE_STATISTICS)
        statistics_.update_time.resume();
#endif
        statistics_.backups++;

        if (state_info.is_terminal()) {
#if defined(EXPENSIVE_STATISTICS)
            statistics_.update_time.stop();
#endif
            return false;
        }

        if (state_info.is_on_fringe()) {
            ++statistics_.backed_up_states;
            state_info.removed_from_fringe();
        }

        this->generate_all_successors(state_id, aops, transitions);
        assert(aops.size() == transitions.size());

        if (aops.empty()) {
            statistics_.terminal_states++;
            bool result = notify_dead_end(state_id, state_info);
#if defined(EXPENSIVE_STATISTICS)
            statistics_.update_time.stop();
#endif
            if (result) {
                ++statistics_.value_changes;
                if (state_id == initial_state_id_) {
                    statistics_.jump();
                }
            }
            return result;
        }

        new_value = IncumbentSolution(this->get_minimal_reward());
        values.reserve(aops.size());

        unsigned non_loop_end = 0;
        for (unsigned i = 0; i < aops.size(); ++i) {
            Action& op = aops[i];
            Distribution<StateID>& transition = transitions[i];

            IncumbentSolution t_value(
                state_info.get_state_reward() +
                this->get_action_reward(state_id, op));
            value_type::value_t self_loop = value_type::zero;
            bool non_loop = false;

            for (const auto& [succ_id, prob] : transition) {
                if (succ_id == state_id) {
                    self_loop += prob;
                } else {
                    const StateInfo& succ_info = lookup_initialize(succ_id);
                    t_value += prob * succ_info.value;
                    non_loop = true;
                }
            }

            if (non_loop) {
                if (self_loop > value_type::zero) {
                    t_value *= value_type::one / (value_type::one - self_loop);
                }

                values.push_back(t_value);
                value_utils::set_max(new_value, t_value);

                if (non_loop_end != i) {
                    aops[non_loop_end] = std::move(op);
                    transitions[non_loop_end] = std::move(transition);
                }

                ++non_loop_end;
            }
        }

        aops.erase(aops.begin() + non_loop_end, aops.end());
        transitions.erase(
            transitions.begin() + non_loop_end,
            transitions.end());

#if defined(EXPENSIVE_STATISTICS)
        statistics_.update_time.stop();
#endif

        if (aops.empty()) {
            statistics_.self_loop_states++;
            return notify_dead_end(state_id, state_info);
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

    template <typename T>
    bool compute_value_update(
        const StateID& state_id,
        bool stable_policy,
        T& choice,
        ActionID* greedy_action,
        Distribution<StateID>* greedy_transition,
        bool* action_changed)
    {
        std::vector<Action> aops;
        std::vector<Distribution<StateID>> transitions;
        IncumbentSolution new_value;
        std::vector<IncumbentSolution> values;

        StateInfo& state_info = lookup_initialize(state_id);

        bool b = compute_value_update(
            state_id,
            state_info,
            aops,
            transitions,
            new_value,
            values);

        if (aops.empty()) {
            state_info.set_policy(ActionID::undefined);
        } else {
            select_policy(
                state_id,
                state_info,
                stable_policy,
                choice,
                greedy_action,
                greedy_transition,
                action_changed,
                aops,
                transitions,
                new_value,
                values);
        }

        return b;
    }

    template <typename T>
    void select_policy(
        const StateID& state_id,
        StateInfo& state_info,
        bool stable,
        T& choice,
        ActionID* greedy_action,
        Distribution<StateID>* greedy_transition,
        bool* action_changed,
        std::vector<Action>& aops,
        std::vector<Distribution<StateID>>& transitions,
        const IncumbentSolution& new_value,
        const std::vector<IncumbentSolution>& values)
    {
#if defined(EXPENSIVE_STATISTICS)
        statistics_.policy_selection_time.resume();
#endif
        auto previous_greedy = state_info.get_policy();

        unsigned optimal_end = 0;
        for (unsigned i = 0; i < aops.size(); ++i) {
            if (value_utils::compare(values[i], new_value) >= 0) {
                if (stable) {
                    const auto aid = this->get_action_id(state_id, aops[i]);
                    if (aid == previous_greedy) {
                        if (action_changed != nullptr) {
                            *action_changed = false;
                        }

                        if (greedy_action != nullptr) {
                            (*greedy_action) = aid;
                        }

                        if (greedy_transition != nullptr) {
                            (*greedy_transition) = std::move(transitions[i]);
                        }

                        return;
                    }
                }

                if (i != optimal_end) {
                    transitions[optimal_end] = std::move(transitions[i]);
                    aops[optimal_end] = std::move(aops[i]);
                }

                ++optimal_end;
            }
        }

        aops.erase(aops.begin() + optimal_end, aops.end());
        transitions.erase(transitions.begin() + optimal_end, transitions.end());

        assert(!aops.empty() && !transitions.empty());

        ++statistics_.policy_updates;

        int index = choice(state_id, previous_greedy, aops, transitions);
        assert(index < 0 || index < static_cast<int>(aops.size()));

        if (index >= 0) {
            const ActionID aid = this->get_action_id(state_id, aops[index]);

            if (action_changed != nullptr) {
                *action_changed = (aid != state_info.get_policy());
            }

            if (greedy_action != nullptr) {
                (*greedy_action) = aid;
            }

            if (greedy_transition != nullptr) {
                (*greedy_transition) = std::move(transitions[index]);
            }

            state_info.set_policy(aid);
        }

#if defined(EXPENSIVE_STATISTICS)
        statistics_.policy_selection_time.stop();
#endif
    }

protected:
    ProgressReport* report_;
    const bool interval_comparison_;
    const bool stable_policy_;

private:
    StateEvaluator<State>* value_initializer_;
    PolicyPicker<Action>* policy_chooser_;
    NewStateHandler<State>* on_new_state_;
    DeadEndListener<State, Action>* dead_end_listener_;
    StateEvaluator<State>* dead_end_eval_;

    const IncumbentSolution dead_end_value_;

    storage::PerStateStorage<StateInfo> state_infos_;

    Statistics statistics_;

    StateID initial_state_id_ = StateID::undefined;
};

} // namespace internal

template <typename T>
struct NoAdditionalStateData : public T {
};

template <
    typename State,
    typename Action,
    typename DualValueBounds = std::false_type,
    typename StorePolicy = std::false_type,
    template <typename> class StateInfo = NoAdditionalStateData>
using HeuristicSearchBase = internal::HeuristicSearchBase<
    State,
    Action,
    StateInfo<PerStateBaseInformation<StorePolicy, DualValueBounds>>>;

} // namespace heuristic_search
} // namespace engines
} // namespace probabilistic

#endif // __HEURISTIC_SEARCH_BASE_H__