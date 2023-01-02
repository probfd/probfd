#ifndef MDPS_TRANSITION_SAMPLER_VBIASED_SUCCESSOR_SAMPLER_H
#define MDPS_TRANSITION_SAMPLER_VBIASED_SUCCESSOR_SAMPLER_H

#include "probfd/transition_sampler.h"

#include "probfd/utils/distribution_random_sampler.h"

#include <memory>

namespace options {
class Options;
class OptionParser;
} // namespace options

namespace utils {
class RandomNumberGenerator;
}

namespace probfd {

namespace new_state_handlers {
class StoreHeuristic;
}

namespace transition_sampler {

class VBiasedSuccessorSampler : public ProbabilisticOperatorTransitionSampler {
public:
    explicit VBiasedSuccessorSampler(const options::Options&);
    explicit VBiasedSuccessorSampler(
        std::shared_ptr<utils::RandomNumberGenerator> rng);

    static void add_options_to_parser(options::OptionParser& parser);

protected:
    virtual StateID sample(
        const StateID& state,
        OperatorID op,
        const Distribution<StateID>& successors) override;

    std::shared_ptr<new_state_handlers::StoreHeuristic> heuristic_;
    Distribution<StateID> biased_;
    distribution_random_sampler::DistributionRandomSampler sampler_;
};

} // namespace transition_sampler
} // namespace probfd

#endif // __VBIASED_SUCCESSOR_SAMPLER_H__