#ifndef MDPS_POLICY_PICKER_H
#define MDPS_POLICY_PICKER_H

#include "probfd/engine_interfaces/policy_picker.h"

#include "probfd/heuristic_search_interfaceable.h"

namespace probfd {
namespace engine_interfaces {

template <>
struct PolicyPicker<OperatorID> : public HeuristicSearchInterfaceable {
public:
    int operator()(
        const StateID& state,
        const ActionID& prev_policy,
        const std::vector<OperatorID>& action_choices,
        const std::vector<Distribution<StateID>>& successors);

protected:
    virtual int pick(
        const StateID& state,
        const ActionID& prev_policy,
        const std::vector<OperatorID>& action_choices,
        const std::vector<Distribution<StateID>>& successors) = 0;
};

} // namespace engine_interfaces

using ProbabilisticOperatorPolicyPicker =
    engine_interfaces::PolicyPicker<OperatorID>;

} // namespace probfd

#endif // __POLICY_PICKER_H__