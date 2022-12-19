#include "state_registry.h"

#include "axioms.h"
#include "globals.h"
#include "global_operator.h"
#include "per_state_information.h"

using namespace std;

StateRegistry::StateRegistry(
        const int_packer::IntPacker* state_packer,
        AxiomEvaluator* axiom_evaluator,
        const std::vector<int>& initial_state_data)
    : state_packer(state_packer),
      axiom_evaluator(axiom_evaluator),
      initial_state_data(initial_state_data),
      state_data_pool(get_bins_per_state()),
      registered_states(
          StateIDSemanticHash(state_data_pool, get_bins_per_state()),
          StateIDSemanticEqual(state_data_pool, get_bins_per_state())),
      cached_initial_state(0) {
}

StateRegistry::StateRegistry()
    : StateRegistry(g_state_packer, g_axiom_evaluator, g_initial_state_data)
{}

StateRegistry::~StateRegistry() {
    delete cached_initial_state;
}

const int_packer::IntPacker* StateRegistry::get_state_packer() const
{
    return state_packer;
}

StateID StateRegistry::insert_id_or_pop_state() {
    /*
      Attempt to insert a StateID for the last state of state_data_pool
      if none is present yet. If this fails (another entry for this state
      is present), we have to remove the duplicate entry from the
      state data pool.
    */
    StateID id(state_data_pool.size() - 1);
    pair<int, bool> result = registered_states.insert(id.value);
    bool is_new_entry = result.second;
    if (!is_new_entry) {
        state_data_pool.pop_back();
    }
    assert(registered_states.size() == static_cast<int>(state_data_pool.size()));
    return StateID(result.first);
}

GlobalState StateRegistry::lookup_state(StateID id) const {
    return GlobalState(state_data_pool[id.value], *this, id);
}

const GlobalState &StateRegistry::get_initial_state() {
    if (cached_initial_state == 0) {
        PackedStateBin *buffer = new PackedStateBin[get_bins_per_state()];
        // Avoid garbage values in half-full bins.
        fill_n(buffer, get_bins_per_state(), 0);

        for (size_t i = 0; i < initial_state_data.size(); ++i) {
            state_packer->set(buffer, i, initial_state_data[i]);
        }
        state_data_pool.push_back(buffer);
        // buffer is copied by push_back
        delete[] buffer;
        StateID id = insert_id_or_pop_state();
        cached_initial_state = new GlobalState(lookup_state(id));
    }
    return *cached_initial_state;
}

PackedStateBin* StateRegistry::get_temporary_successor_state(const GlobalState& predecessor, const GlobalOperator& op)
{
    assert(!op.is_axiom());
    state_data_pool.push_back(predecessor.get_packed_buffer());
    PackedStateBin *buffer = state_data_pool[state_data_pool.size() - 1];
    for (size_t i = 0; i < op.get_effects().size(); ++i) {
        const GlobalEffect &effect = op.get_effects()[i];
        if (effect.does_fire(predecessor))
            state_packer->set(buffer, effect.var, effect.val);
    }
    axiom_evaluator->evaluate(buffer, *state_packer);
    return buffer;
}

void StateRegistry::remove_temporary_state()
{
    state_data_pool.pop_back();
}

GlobalState StateRegistry::make_permanent()
{
    StateID id = insert_id_or_pop_state();
    return lookup_state(id);
}

//TODO it would be nice to move the actual state creation (and operator application)
//     out of the StateRegistry. This could for example be done by global functions
//     operating on state buffers (PackedStateBin *).
GlobalState StateRegistry::get_successor_state(const GlobalState &predecessor, const GlobalOperator &op) {
    get_temporary_successor_state(predecessor, op);
    return make_permanent();
}

GlobalState StateRegistry::get_successor_state(const GlobalState &predecessor, const GlobalOperator &op, int var, int val) {
    state_data_pool.push_back(predecessor.get_packed_buffer());
    PackedStateBin *buffer = state_data_pool[state_data_pool.size() - 1];
    for (size_t i = 0; i < op.get_effects().size(); ++i) {
        const GlobalEffect &effect = op.get_effects()[i];
        if (effect.does_fire(predecessor))
            state_packer->set(buffer, effect.var, effect.val);
    }
    axiom_evaluator->evaluate(buffer, *state_packer);
    state_packer->set(buffer, var, val);
    return make_permanent();
}


int StateRegistry::get_bins_per_state() const {
    return state_packer->get_num_bins();
}

int StateRegistry::get_state_size_in_bytes() const {
    return get_bins_per_state() * sizeof(PackedStateBin);
}

void StateRegistry::print_statistics() const {
    cout << "Number of registered states: " << size() << endl;
    registered_states.print_statistics(utils::g_log);
}
