#ifndef PROBFD_HEURISTICS_MERGE_AND_SHRINK_SHRINK_FH_H
#define PROBFD_HEURISTICS_MERGE_AND_SHRINK_SHRINK_FH_H

#include "probfd/merge_and_shrink/shrink_bucket_based.h"

#include <vector>

namespace plugins {
class Options;
}

namespace probfd::merge_and_shrink {

/*
  NOTE: In case where we must merge across buckets (i.e. when
  the number of (f, h) pairs is larger than the number of
  permitted abstract states), this shrink strategy will *not* make
  an effort to be at least be h-preserving.

  This could be improved, but not without complicating the code.
  Usually we set the number of abstract states large enough that we
  do not need to merge across buckets. Therefore the complication
  might not be worth the code maintenance cost.
*/
class ShrinkH : public ShrinkBucketBased {
public:
    enum class HighLow { HIGH, LOW };

private:
    const HighLow h_start;

    std::vector<Bucket> ordered_buckets_use_vector(
        const TransitionSystem& ts,
        const Distances& distances,
        int max_h) const;
    std::vector<Bucket> ordered_buckets_use_map(
        const TransitionSystem& ts,
        const Distances& distances) const;

public:
    explicit ShrinkH(const plugins::Options& opts);

    virtual bool requires_liveness() const override { return true; }
    virtual bool requires_goal_distances() const override { return true; }

protected:
    virtual std::string name() const override;
    virtual void
    dump_strategy_specific_options(utils::LogProxy& log) const override;

    virtual std::vector<Bucket> partition_into_buckets(
        const TransitionSystem& ts,
        const Distances& distances) const override;
};

} // namespace probfd::merge_and_shrink

#endif // PROBFD_HEURISTICS_MERGE_AND_SHRINK_SHRINK_FH_H
