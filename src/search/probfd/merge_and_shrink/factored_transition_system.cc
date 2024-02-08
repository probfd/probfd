#include "probfd/merge_and_shrink/factored_transition_system.h"

#include "probfd/merge_and_shrink/distances.h"
#include "probfd/merge_and_shrink/labels.h"
#include "probfd/merge_and_shrink/merge_and_shrink_representation.h"
#include "probfd/merge_and_shrink/transition_system.h"
#include "probfd/merge_and_shrink/utils.h"

#include "downward/utils/logging.h"
#include "downward/utils/memory.h"
#include "downward/utils/system.h"

#ifndef NDEBUG
#include "downward/utils/collections.h"
#endif

#include <cassert>

using namespace std;

namespace probfd::merge_and_shrink {

FTSConstIterator::FTSConstIterator(
    const FactoredTransitionSystem& fts,
    bool end)
    : fts(fts)
    , current_index((end ? fts.get_size() : 0))
{
    next_valid_index();
}

void FTSConstIterator::next_valid_index()
{
    while (current_index < fts.get_size() && !fts.is_active(current_index)) {
        ++current_index;
    }
}

void FTSConstIterator::operator++()
{
    ++current_index;
    next_valid_index();
}

FactoredTransitionSystem::FactoredTransitionSystem(
    unique_ptr<Labels> labels,
    vector<unique_ptr<TransitionSystem>>&& transition_systems,
    vector<unique_ptr<MergeAndShrinkRepresentation>>&& mas_representations,
    vector<unique_ptr<Distances>>&& distances,
    const bool compute_liveness,
    const bool compute_goal_distances,
    utils::LogProxy& log)
    : labels(std::move(labels))
    , transition_systems(std::move(transition_systems))
    , mas_representations(std::move(mas_representations))
    , distances(std::move(distances))
    , compute_liveness(compute_liveness)
    , compute_goal_distances(compute_goal_distances)
    , num_active_entries(this->transition_systems.size())
{
    assert(!compute_liveness || compute_goal_distances);
    for (size_t index = 0; index < this->transition_systems.size(); ++index) {
        if (compute_goal_distances) {
            this->distances[index]->compute_distances(compute_liveness, log);
        }
        assert(is_component_valid(index));
    }
}

FactoredTransitionSystem::~FactoredTransitionSystem() = default;

void FactoredTransitionSystem::assert_index_valid(int index) const
{
    assert(utils::in_bounds(index, transition_systems));
    assert(utils::in_bounds(index, mas_representations));
    assert(utils::in_bounds(index, distances));
    if (!(transition_systems[index] && mas_representations[index] &&
          distances[index]) &&
        !(!transition_systems[index] && !mas_representations[index] &&
          !distances[index])) {
        cerr << "Factor at index is in an inconsistent state!" << endl;
        utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
    }
}

bool FactoredTransitionSystem::is_component_valid(int index) const
{
    assert(is_active(index));
    if (compute_liveness && !distances[index]->is_liveness_computed()) {
        return false;
    }
    if (compute_goal_distances &&
        !distances[index]->are_goal_distances_computed()) {
        return false;
    }
    return transition_systems[index]->is_valid(*labels);
}

void FactoredTransitionSystem::assert_all_components_valid() const
{
    for (size_t index = 0; index < transition_systems.size(); ++index) {
        if (transition_systems[index]) {
            assert(is_component_valid(index));
        }
    }
}

void FactoredTransitionSystem::apply_label_mapping(
    const vector<pair<int, vector<int>>>& label_mapping,
    int combinable_index)
{
    assert_all_components_valid();
    for (const auto& entry : label_mapping) {
        assert(entry.first == labels->get_num_total_labels());
        const vector<int>& old_labels = entry.second;
        labels->reduce_labels(old_labels);
    }
    for (size_t i = 0; i < transition_systems.size(); ++i) {
        if (transition_systems[i]) {
            transition_systems[i]->apply_label_reduction(
                *labels,
                label_mapping,
                static_cast<int>(i) != combinable_index);
        }
    }
    assert_all_components_valid();
}

bool FactoredTransitionSystem::apply_abstraction(
    int index,
    const StateEquivalenceRelation& state_equivalence_relation,
    utils::LogProxy& log)
{
    assert(is_component_valid(index));

    int new_num_states = state_equivalence_relation.size();
    if (new_num_states == transition_systems[index]->get_size()) {
        return false;
    }

    vector<int> abstraction_mapping = compute_abstraction_mapping(
        transition_systems[index]->get_size(),
        state_equivalence_relation);

    transition_systems[index]->apply_abstraction(
        state_equivalence_relation,
        abstraction_mapping,
        log);
    if (compute_goal_distances) {
        distances[index]->apply_abstraction(
            state_equivalence_relation,
            compute_liveness,
            log);
    }
    mas_representations[index]->apply_abstraction_to_lookup_table(
        abstraction_mapping);

    /* If distances need to be recomputed, this already happened in the
       Distances object. */
    assert(is_component_valid(index));
    return true;
}

int FactoredTransitionSystem::merge(
    int index1,
    int index2,
    utils::LogProxy& log)
{
    assert(is_component_valid(index1));
    assert(is_component_valid(index2));
    const TransitionSystem& new_ts =
        *transition_systems.emplace_back(TransitionSystem::merge(
            *labels,
            *transition_systems[index1],
            *transition_systems[index2],
            log));
    transition_systems[index1] = nullptr;
    transition_systems[index2] = nullptr;

    distances[index1] = nullptr;
    distances[index2] = nullptr;

    mas_representations.push_back(
        std::make_unique<MergeAndShrinkRepresentationMerge>(
            std::move(mas_representations[index1]),
            std::move(mas_representations[index2])));
    mas_representations[index1] = nullptr;
    mas_representations[index2] = nullptr;

    auto& dist = *distances.emplace_back(std::make_unique<Distances>(new_ts));
    // Restore the invariant that distances are computed.
    if (compute_goal_distances) {
        dist.compute_distances(compute_liveness, log);
    }
    --num_active_entries;

    int new_index = transition_systems.size() - 1;
    assert(is_component_valid(new_index));
    return new_index;
}

pair<unique_ptr<MergeAndShrinkRepresentation>, unique_ptr<Distances>>
FactoredTransitionSystem::extract_factor(int index)
{
    assert(is_component_valid(index));
    return make_pair(
        std::move(mas_representations[index]),
        std::move(distances[index]));
}

void FactoredTransitionSystem::statistics(int index, utils::LogProxy& log) const
{
    if (log.is_at_least_verbose()) {
        assert(is_component_valid(index));
        const TransitionSystem& ts = *transition_systems[index];
        ts.dump_statistics(log);
        const Distances& dist = *distances[index];
        dist.statistics(log);
    }
}

void FactoredTransitionSystem::dump(int index, utils::LogProxy& log) const
{
    if (log.is_at_least_debug()) {
        assert_index_valid(index);
        transition_systems[index]->dump_labels_and_transitions(log);
        mas_representations[index]->dump(log);
    }
}

void FactoredTransitionSystem::dump(utils::LogProxy& log) const
{
    if (log.is_at_least_debug()) {
        for (int index : *this) {
            dump(index, log);
        }
    }
}

bool FactoredTransitionSystem::is_factor_solvable(int index) const
{
    assert(is_component_valid(index));
    return transition_systems[index]->is_solvable(*distances[index]);
}

bool FactoredTransitionSystem::is_factor_trivial(int index) const
{
    assert(is_component_valid(index));
    if (!mas_representations[index]->is_total()) {
        return false;
    }
    const TransitionSystem& ts = *transition_systems[index];
    for (int state = 0; state < ts.get_size(); ++state) {
        if (!ts.is_goal_state(state)) {
            return false;
        }
    }
    return true;
}

bool FactoredTransitionSystem::is_active(int index) const
{
    assert_index_valid(index);
    return transition_systems[index] != nullptr;
}

} // namespace probfd::merge_and_shrink