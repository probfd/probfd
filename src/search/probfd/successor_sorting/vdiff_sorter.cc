#include "probfd/successor_sorting/vdiff_sorter.h"

#include "probfd/engines/heuristic_search_state_information.h"

#include "option_parser.h"
#include "plugin.h"

namespace probfd {
namespace successor_sorting {

VDiffSorter::VDiffSorter(const options::Options& opts)
    : favor_large_gaps_(opts.get<bool>("prefer_large_gaps") ? -1 : 1)
{
}

void VDiffSorter::add_options_to_parser(options::OptionParser& parser)
{
    parser.add_option<bool>("prefer_large_gaps", "", "true");
}

void VDiffSorter::sort(
    const StateID&,
    const std::vector<OperatorID>&,
    std::vector<Distribution<StateID>>& successors)
{
    std::vector<std::pair<double, unsigned>> k0;
    k0.reserve(successors.size());
    std::vector<std::pair<double, unsigned>> k1;
    for (int i = successors.size() - 1; i >= 0; --i) {
        const auto& t = successors[i].data();
        value_type::value_t sum = 0;
        for (unsigned j = 0; j < t.size(); ++j) {
            const auto& suc = t[j];
            k1.emplace_back(
                std::abs(
                    favor_large_gaps_ *
                    lookup_dual_bounds(suc.element)->error_bound()),
                j);
            sum += suc.probability * k1.back().first;
        }
        k0.emplace_back(sum, i);
        std::sort(k1.begin(), k1.end());
        std::vector<WeightedElement<StateID>> sor;
        sor.reserve(t.size());
        for (unsigned j = 0; j < k1.size(); ++j) {
            sor.push_back(t[k1[j].second]);
        }
        k1.clear();
    }
    std::sort(k0.begin(), k0.end());
    std::vector<Distribution<StateID>> res;
    res.reserve(successors.size());
    for (unsigned i = 0; i < k0.size(); ++i) {
        res.push_back(std::move(successors[k0[i].second]));
    }
    res.swap(successors);
}

static Plugin<ProbabilisticOperatorSuccessorSorting> _plugin(
    "value_gap_sort",
    options::parse<ProbabilisticOperatorSuccessorSorting, VDiffSorter>);

} // namespace successor_sorting
} // namespace probfd
