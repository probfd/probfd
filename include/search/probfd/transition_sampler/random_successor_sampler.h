#ifndef MDPS_TRANSITION_SAMPLER_RANDOM_SUCCESSOR_SAMPLER_H
#define MDPS_TRANSITION_SAMPLER_RANDOM_SUCCESSOR_SAMPLER_H

#include "probfd/transition_sampler.h"

#include "probfd/utils/distribution_random_sampler.h"

namespace options {
class Options;
class OptionParser;
} // namespace options

namespace utils {
class RandomNumberGenerator;
}

namespace probfd {
namespace transition_sampler {

class RandomSuccessorSampler : public ProbabilisticOperatorTransitionSampler {
public:
    explicit RandomSuccessorSampler(const options::Options& opts);
    explicit RandomSuccessorSampler(
        std::shared_ptr<utils::RandomNumberGenerator> rng);

    static void add_options_to_parser(options::OptionParser& parser);

protected:
    virtual StateID sample(
        const StateID& state,
        OperatorID op,
        const Distribution<StateID>& successors) override;

    distribution_random_sampler::DistributionRandomSampler sampler_;
};

} // namespace transition_sampler
} // namespace probfd

#endif // __RANDOM_SUCCESSOR_SAMPLER_H__