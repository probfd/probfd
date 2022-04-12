#ifndef MDPS_ENGINES_AO_SEARCH_H
#define MDPS_ENGINES_AO_SEARCH_H

#include "../engine_interfaces/dead_end_listener.h"
#include "../storage/per_state_storage.h"
#include "heuristic_search_base.h"

#include <limits>
#include <queue>
#include <string>
#include <type_traits>
#include <vector>

namespace probabilistic {
namespace engines {

/// Namespace dedicated to the AO* family of MDP algorithms.
namespace ao_search {

struct Statistics {
    unsigned long long iterations = 0;

    void print(std::ostream& out) const
    {
        out << "  Iterations: " << iterations << std::endl;
    }

    void register_report(ProgressReport& report)
    {
        report.register_print(
            [this](std::ostream& out) { out << "i=" << iterations; });
    }
};

template <typename StateInfo>
struct PerStateInformation : public StateInfo {
    static constexpr const uint8_t MARK = 1 << StateInfo::BITS;
    static constexpr const uint8_t SOLVED = 2 << StateInfo::BITS;
    static constexpr const uint8_t MASK = 3 << StateInfo::BITS;
    static constexpr const uint8_t BITS = StateInfo::BITS + 2;

    bool is_tip_state() const { return update_order == 0; }
    bool is_marked() const { return this->info & MARK; }
    bool is_solved() const { return this->info & SOLVED; }
    bool is_unflagged() const { return (this->info & MASK) == 0; }

    void mark()
    {
        assert(!is_solved());
        this->info = (this->info & ~MASK) | MARK;
    }
    void unmark() { this->info = (this->info & ~MARK); }
    void set_solved() { this->info = (this->info & ~MASK) | SOLVED; }

    const std::vector<StateID>& get_parents() const { return parents; }
    std::vector<StateID>& get_parents() { return parents; }
    void add_parent(const StateID& s) { parents.push_back(s); }

    int update_order = 0;
    std::vector<StateID> parents;
};

/**
 * @brief Base class for the AO* algorithm family.
 *
 * @tparam StateT - The state type of the underlying MDP.
 * @tparam ActionT - The action type of the underlying MDP.
 * @tparam DualBounds - Determines whether bounded value iteration is performed.
 * @tparam StorePolicy - Determines whether the optimal policy is stored.
 * @tparam StateInfoExtension - The extended state information struct used by
 * the derived algorithm.
 * @tparam Greedy - ?
 */
template <
    typename StateT,
    typename ActionT,
    typename DualBounds,
    typename StorePolicy,
    template <typename>
    class StateInfoExtension,
    bool Greedy>
class AOBase
    : public heuristic_search::HeuristicSearchBase<
          StateT,
          ActionT,
          DualBounds,
          StorePolicy,
          StateInfoExtension> {
public:
    /// The heuristic search base class.
    using HeuristicSearchBase = heuristic_search::HeuristicSearchBase<
        StateT,
        ActionT,
        DualBounds,
        StorePolicy,
        StateInfoExtension>;

    /// The underlying state type.
    using State = typename HeuristicSearchBase::State;

    /// The underlying action type.
    using Action = typename HeuristicSearchBase::Action;

    AOBase(
        StateIDMap<State>* state_id_map,
        ActionIDMap<Action>* action_id_map,
        StateRewardFunction<State>* state_reward_function,
        ActionRewardFunction<Action>* action_reward_function,
        value_utils::IntervalValue reward_bound,
        ApplicableActionsGenerator<Action>* aops_generator,
        TransitionGenerator<Action>* transition_generator,
        StateEvaluator<StateT>* dead_end_eval,
        DeadEndListener<StateT, ActionT>* dead_end_listener,
        PolicyPicker<ActionT>* policy_chooser,
        NewStateHandler<StateT>* new_state_handler,
        StateEvaluator<StateT>* value_init,
        HeuristicSearchConnector* connector,
        ProgressReport* report,
        bool interval_comparison,
        bool stable_policy)
        : HeuristicSearchBase(
              state_id_map,
              action_id_map,
              state_reward_function,
              action_reward_function,
              reward_bound,
              aops_generator,
              transition_generator,
              dead_end_eval,
              dead_end_listener,
              policy_chooser,
              new_state_handler,
              value_init,
              connector,
              report,
              interval_comparison,
              stable_policy)
    {
    }

    virtual void print_statistics(std::ostream& out) const override
    {
        statistics_.print(out);
        HeuristicSearchBase::print_statistics(out);
    }

protected:
    using StateInfo = typename HeuristicSearchBase::StateInfo;

    virtual void setup_custom_reports(const State&) override
    {
        this->statistics_.register_report(*this->report_);
    }

    void backpropagate_tip_value()
    {
        while (!queue_.empty()) {
            auto elem = queue_.top();
            queue_.pop();

            auto& info = get_state_info(elem.second);
            assert(!info.is_goal_state());
            assert(!info.is_terminal() || info.is_solved());
            if (info.is_solved()) {
                // has been handled already
                continue;
            }
            assert(info.is_marked());
            info.unmark();

            bool solved = false;
            bool dead = false;
            bool value_changed = update_value_check_solved(
                IsGreedy(),
                elem.second,
                info,
                solved,
                dead);
            if (solved) {
                mark_solved_push_parents(elem.second, info, dead);
            } else if (value_changed) {
                push_parents_to_queue(info);
            }
        }
    }

    void backpropagate_update_order(const StateID& tip)
    {
        queue_.emplace(get_state_info(tip).update_order, tip);
        while (!queue_.empty()) {
            auto elem = queue_.top();
            queue_.pop();
            auto& info = get_state_info(elem.second);
            if (info.update_order > elem.first) {
                continue;
            }
            auto& parents = info.get_parents();
            auto it = parents.begin();
            while (it != parents.end()) {
                auto& pinfo = get_state_info(*it);
                if (pinfo.is_solved()) {
                    it = parents.erase(it);
                    continue;
                } else if (pinfo.update_order <= elem.first) {
                    pinfo.update_order = elem.first + 1;
                    queue_.emplace(elem.first + 1, *it);
                }
                ++it;
            }
        }
    }

    void initialize_tip_state_value(
        const StateID& state,
        StateInfo& info,
        bool& terminal,
        bool& solved,
        bool& dead,
        bool& value_changed)
    {
        assert(!info.is_solved());
        assert(info.is_tip_state());
        assert(queue_.empty());

        terminal = false;
        solved = false;
        value_changed =
            update_value_check_solved(IsGreedy(), state, info, solved, dead);

        if (info.is_terminal()) {
            terminal = true;
            info.set_solved();
            dead = !info.is_goal_state();
            // std::cout << this->state_id_map_->operator[](state) << " is
            // terminal (" << dead << ")" << std::endl;
            if (dead) {
                this->notify_dead_end(state);
            }
            push_parents_to_queue(info);
            backpropagate_tip_value();
        }

        assert(queue_.empty());
    }

    void push_parents_to_queue(StateInfo& info)
    {
        push_parents_to_queue(IsGreedy(), info);
    }

    void mark_solved_push_parents(
        const StateID& state,
        StateInfo& info,
        const bool dead)
    {
        assert(!info.is_terminal());

        if (dead) {
            assert(!info.is_solved() && !info.is_goal_state());
            notify_dead_end(state);
        }

        info.set_solved();
        push_parents_to_queue(info);
    }

private:
    struct QueueComparator {
        bool operator()(
            const std::pair<int, StateID>& s,
            const std::pair<int, StateID>& t) const
        {
            return s.first > t.first;
        }
    };

    using IsGreedy = std::integral_constant<bool, Greedy>;

    using TopoQueue = std::priority_queue<
        std::pair<int, StateID>,
        std::vector<std::pair<int, StateID>>,
        QueueComparator>;

    void push_parents_to_queue(const std::true_type&, StateInfo& info)
    {
        auto& parents = info.get_parents();
        for (auto it = parents.begin(); it != parents.end(); ++it) {
            auto& pinfo = get_state_info(*it);
            assert(!pinfo.is_dead_end() || pinfo.is_solved());
            if (pinfo.is_unflagged()) {
                pinfo.mark();
                queue_.emplace(pinfo.update_order, *it);
            }
        }
        if (info.is_solved()) {
            typename std::remove_reference<decltype(parents)>::type().swap(
                parents);
        }
    }

    void push_parents_to_queue(const std::false_type&, StateInfo& info)
    {
        auto& parents = info.get_parents();
        for (auto it = parents.begin(); it != parents.end(); ++it) {
            auto& pinfo = get_state_info(*it);
            assert(!pinfo.is_dead_end() || pinfo.is_solved());
            if (info.is_solved()) {
                assert(pinfo.unsolved > 0 || pinfo.is_solved());
                --pinfo.unsolved;
                if (!info.is_dead_end()) {
                    pinfo.alive = 1;
                }
            }
            if (pinfo.is_unflagged()) {
                pinfo.mark();
                queue_.emplace(pinfo.update_order, *it);
            }
        }
        if (info.is_solved()) {
            typename std::remove_reference<decltype(parents)>::type().swap(
                parents);
        }
    }

    bool update_value_check_solved(
        const std::true_type&,
        const StateID& state,
        const StateInfo& info,
        bool& solved,
        bool& dead)
    {
        const bool result =
            this->async_update(state, nullptr, &selected_transition_, nullptr);
        solved = true;
        dead = !selected_transition_.empty() || info.is_dead_end();
        // std::cout << "update " << this->state_id_map_->operator[](state) << "
        // ... [";
        for (auto it = selected_transition_.begin();
             it != selected_transition_.end();
             ++it) {
            // std::cout << " " << this->state_id_map_->operator[](it->first);
            const auto& succ_info = get_state_info(it->first);
            solved = solved && succ_info.is_solved();
            dead = dead && succ_info.is_dead_end();
        }
        // std::cout << " ] => " << solved << "|" << dead << std::endl;
        selected_transition_.clear();
        return result;
    }

    bool update_value_check_solved(
        const std::false_type&,
        const StateID& state,
        const StateInfo& info,
        bool& solved,
        bool& dead)
    {
        solved = info.unsolved == 0;
        dead = solved && info.alive == 0 && !info.is_goal_state();
        const bool vc = this->async_update(state);
        if (value_utils::as_lower_bound(info.value) >=
            this->get_maximal_reward()) {
            solved = true;
            dead = false;
        }
        return vc;
    }

protected:
    std::vector<Action> aops_;
    Distribution<StateID> selected_transition_;

    Statistics statistics_;

private:
    TopoQueue queue_;
};

} // namespace ao_search
} // namespace engines
} // namespace probabilistic

#endif // __AO_SEARCH_H__