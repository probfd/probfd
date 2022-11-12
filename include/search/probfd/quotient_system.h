#ifndef MDPS_QUOTIENT_SYSTEM_H
#define MDPS_QUOTIENT_SYSTEM_H

#include "algorithms/segmented_vector.h"

#include "probfd/quotient_system/engine_interfaces.h"
#include "probfd/quotient_system/quotient_system.h"

#include "probfd/probabilistic_operator.h"
#include "probfd/transition_generator.h"

#include <algorithm>
#include <cassert>
#include <memory>
#include <set>
#include <unordered_set>
#include <vector>

namespace probfd {
namespace quotient_system {

template <>
class QuotientSystem<const ProbabilisticOperator*> {
    friend struct const_iterator;

public:
    using Action = const ProbabilisticOperator*;
    using QAction = QuotientAction<Action>;
    using QuotientStateIDIterator = DefaultQuotientSystem<
        const ProbabilisticOperator*>::QuotientStateIDIterator;

    struct const_iterator {
        using iterator_type = std::forward_iterator_tag;
        using value_type = StateID::size_type;
        using difference_type = std::ptrdiff_t;
        using pointer = const StateID::size_type*;
        using reference = const StateID::size_type&;

        explicit const_iterator(
            const QuotientSystem* qs,
            DefaultQuotientSystem<Action>::const_iterator x)
            : qs_(qs)
            , i(x)
        {
        }

        const_iterator& operator++()
        {
            if (qs_->cache_) {
                while (++i.i < qs_->state_infos_.size()) {
                    const StateID::size_type ref =
                        qs_->state_infos_[i.i].states[0];
                    if (ref == i.i) {
                        break;
                    }
                }
            } else {
                ++i;
            }
            return *this;
        }

        const_iterator operator+=(int x)
        {
            const_iterator res = *this;
            while (x-- > 0)
                this->operator++();
            return res;
        }

        bool operator==(const const_iterator& other) const
        {
            return i == other.i;
        }

        bool operator!=(const const_iterator& other) const
        {
            return i != other.i;
        }

        reference operator*() const { return i.i; }

    private:
        const QuotientSystem* qs_;
        DefaultQuotientSystem<Action>::const_iterator i;
    };

    explicit QuotientSystem(
        engine_interfaces::ActionIDMap<Action>* aid,
        engine_interfaces::TransitionGenerator<Action>* transition_gen)
        : cache_(transition_gen->caching_)
        , gen_(transition_gen)
        , fallback_(nullptr)
    {
        if (!cache_) {
            fallback_.reset(
                new DefaultQuotientSystem<Action>(aid, transition_gen));
        } else {
            state_infos_.push_back(QuotientInformation(0));
        }
    }

    unsigned quotient_size(const StateID& state_id) const;

    const_iterator begin() const;
    const_iterator end() const;

    utils::RangeProxy<QuotientStateIDIterator>
    quotient_range(const StateID& state_id) const;

    StateID translate_state_id(const StateID& sid) const;

    void
    generate_applicable_ops(const StateID& sid, std::vector<QAction>& result);

    void generate_successors(
        const StateID&,
        const QAction& a,
        Distribution<StateID>& result);

    void generate_all_successors(
        const StateID& sid,
        std::vector<QAction>& aops,
        std::vector<Distribution<StateID>>& successors);

    QAction get_action(const StateID& sid, const ActionID& aid) const;
    ActionID get_action_id(const StateID& sid, const QAction& a) const;

    Action get_original_action(const StateID&, const QAction& a) const;
    ActionID
    get_original_action_id(const StateID& sid, const ActionID& a) const;

    template <typename Range>
    void build_quotient(Range& range)
    {
        this->build_quotient(range.begin(), range.end());
    }

    template <typename StateIDIterator>
    void build_quotient(StateIDIterator begin, StateIDIterator end)
    {
        if (begin != end) {
            this->build_quotient(begin, end, *begin);
        }
    }

    template <typename StateIDIterator>
    void build_quotient(
        StateIDIterator begin,
        StateIDIterator end,
        const StateID& rid)
    {
        this->build_quotient(
            begin,
            end,
            rid,
            utils::infinite_iterator<std::vector<Action>>());
    }

    template <typename StateIDIterator, typename ActionFilterIterator>
    void build_quotient(
        StateIDIterator begin,
        StateIDIterator end,
        const StateID& rid,
        ActionFilterIterator filter_it)
    {
        assert(std::find(begin, end, rid) != end);

        if (!cache_) {
            fallback_->build_quotient(begin, end, rid, filter_it);
            return;
        }

        QuotientInformation& dest_info = state_infos_[rid];
        std::vector<StateID>& parents = dest_info.parents;
        std::vector<StateID>& states = dest_info.states;
        const unsigned old_qstates = states.size();

        std::unordered_set<StateID> states_set;

        for (auto it = begin; it != end; ++it) {
            const StateID state_id = *it;

            if (state_id == rid) {
                continue;
            }

            QuotientInformation& info = state_infos_[state_id];
            assert(info.states[0] == state_id);

            std::move(
                info.parents.begin(),
                info.parents.end(),
                std::back_inserter(parents));

            // Release Memory
            decltype(info.parents)().swap(info.parents);

            states.insert(states.end(), info.states.begin(), info.states.end());

            assert(utils::contains(states, state_id));
        }

        states_set.insert(states.begin(), states.end());

        for (auto it = begin; it != end; ++it, ++filter_it) {
            const StateID state_id = *it;
            QuotientInformation& info = state_infos_[state_id];
            const int e = (state_id != rid ? info.states.size() : old_qstates);

            for (int i = 0; i < e; ++i) {
                auto state = info.states[i];
                update_cache(*filter_it, gen_->lookup(state), rid, states_set);
                state_infos_[state].states[0] = rid;
            }

            if (state_id != rid) {
                info.states.resize(1);
                info.states[0] = rid;
                info.states.shrink_to_fit();
            }
        }

        auto rem_it = std::remove_if(
            parents.begin(),
            parents.end(),
            [rid, this](StateID parent) {
                return state_infos_[parent].states[0] != rid;
            });

        parents.erase(rem_it, parents.end());
        utils::sort_unique(parents);
        parents.shrink_to_fit();

        for (const StateID& parent : parents) {
            assert(parent != rid);
            assert(state_infos_[parent].states[0] == parent);

            const auto& parent_states = state_infos_[parent].states;

            for (const StateID& parent_state : parent_states) {
                auto& entry = gen_->lookup(parent_state);
                const ActionID* aop = entry.aops;
                const ActionID* aop_end = entry.aops + entry.naops;
                StateID* succ = entry.succs;

                for (; aop != aop_end; ++aop) {
                    auto succ_e = succ + gen_->first_op_[*aop].num_outcomes();
                    for (; succ != succ_e; ++succ) {
                        if (utils::contains(states_set, *succ)) {
                            *succ = rid;
                        }
                        // *succ = state_infos_[*succ].states[0];
                        assert(state_infos_[*succ].states[0] == *succ);
                    }
                }
            }
        }

#ifndef NDEBUG
        {
            const QuotientInformation& qinfo = state_infos_[rid];
            assert(!qinfo.states.empty());
            assert(qinfo.states[0] == rid);
            assert(!utils::contains(qinfo.parents, rid));

            std::vector<StateID> uqs = qinfo.states;
            std::sort(uqs.begin(), uqs.end());
            assert(std::unique(uqs.begin(), uqs.end()) == uqs.end());

            uqs = qinfo.parents;
            std::sort(uqs.begin(), uqs.end());
            assert(std::unique(uqs.begin(), uqs.end()) == uqs.end());
        }

        verify_cache_consistency();
#endif
    }

private:
    struct QuotientInformation {
        std::vector<StateID> parents;
        std::vector<StateID> states;
        explicit QuotientInformation(const StateID& s)
            : states({s})
        {
        }
    };

    void update_cache(
        const std::vector<Action>& exclude,
        engine_interfaces::TransitionGenerator<Action>::CacheEntry& entry,
        const StateID rid,
        const std::unordered_set<StateID>& quotient_states)
    {
        unsigned new_size = 0;
        ActionID* aops_src = entry.aops;
        ActionID* aops_dest = entry.aops;
        StateID* succ_src = entry.succs;
        StateID* succ_dest = entry.succs;

        auto aops_src_end = aops_src + entry.naops;
        for (; aops_src != aops_src_end; ++aops_src) {
            const ProbabilisticOperator* op = gen_->first_op_ + *aops_src;
            if (utils::contains(exclude, op)) {
                continue;
            }

            bool self_loop = true;
            StateID* k = succ_dest;

            auto succ_src_end = succ_src + op->num_outcomes();
            for (; succ_src != succ_src_end; ++succ_src, ++succ_dest) {
                const bool member = utils::contains(quotient_states, *succ_src);
                *succ_dest = member ? rid : *succ_src;
                self_loop = self_loop && (*succ_dest == rid);
                succ_dest = k;
                *aops_dest = *aops_src;
                ++aops_dest;
                ++new_size;
            }
        }

        entry.naops = new_size;
    }

#ifndef NDEBUG
    void verify_cache_consistency();
#endif

    const QuotientInformation* get_infos(const StateID& sid) const;
    engine_interfaces::TransitionGenerator<Action>::CacheEntry&
    lookup(const StateID& sid);
    const engine_interfaces::TransitionGenerator<Action>::CacheEntry&
    lookup(const StateID& sid) const;

    const bool cache_;

    segmented_vector::SegmentedVector<QuotientInformation> state_infos_;
    engine_interfaces::TransitionGenerator<Action>* gen_;

    std::unique_ptr<DefaultQuotientSystem<Action>> fallback_;
};

} // namespace quotient_system
} // namespace probfd

#endif // __QUOTIENT_SYSTEM_H__