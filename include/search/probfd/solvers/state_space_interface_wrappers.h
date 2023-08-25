#ifndef MDPS_SOLVERS_STATE_SPACE_INTERFACE_WRAPPERS_H
#define MDPS_SOLVERS_STATE_SPACE_INTERFACE_WRAPPERS_H

#include "probfd/bisimulation/bisimilar_state_space.h"

#include "probfd/algorithms/open_list.h"

#include "probfd/quotients/heuristic_search_interface.h"
#include "probfd/quotients/quotient_system.h"

#include "probfd/open_lists/lifo_open_list.h"

#include <memory>

namespace probfd {
namespace solvers {

template <bool Bisimulation, bool Fret, typename T>
struct Wrapper {
    using type = T;
    type operator()(T t) const { return t; }
};

template <bool Fret, typename Op>
struct translate_action;

template <typename Op>
struct translate_action<true, Op> {
    using type = quotients::QuotientAction<Op>;
};

template <typename Op>
struct translate_action<false, Op> {
    using type = Op;
};

/****************************************************************************/
/* OpenList */

template <>
struct Wrapper<false, true, std::shared_ptr<FDROpenList>> {
    using type = std::shared_ptr<
        algorithms::OpenList<quotients::QuotientAction<OperatorID>>>;

    type operator()(std::shared_ptr<FDROpenList> t) const
    {
        return std::make_shared<quotients::RepresentativeOpenList>(t);
    }
};

template <bool Fret>
struct Wrapper<true, Fret, std::shared_ptr<FDROpenList>> {
    using type = std::shared_ptr<algorithms::OpenList<
        typename translate_action<Fret, bisimulation::QuotientAction>::type>>;

    type operator()(std::shared_ptr<FDROpenList>) const
    {
        return std::make_shared<open_lists::LifoOpenList<
            typename translate_action<Fret, bisimulation::QuotientAction>::
                type>>();
    }
};

} // namespace solvers
} // namespace probfd

#endif // __STATE_SPACE_INTERFACE_WRAPPERS_H__