#ifndef PDBS_ZERO_ONE_PDBS_HEURISTIC_H
#define PDBS_ZERO_ONE_PDBS_HEURISTIC_H

#include "downward/pdbs/pattern_generator.h"
#include "downward/pdbs/zero_one_pdbs.h"

#include "downward/heuristic.h"

namespace pdbs {
class PatternDatabase;

class ZeroOnePDBsHeuristic : public Heuristic {
    ZeroOnePDBs zero_one_pdbs;

protected:
    virtual int compute_heuristic(const State& ancestor_state) override;

public:
    ZeroOnePDBsHeuristic(
        const std::shared_ptr<PatternCollectionGenerator>& patterns,
        const std::shared_ptr<AbstractTask>& transform,
        bool cache_estimates,
        const std::string& name,
        utils::Verbosity verbosity);
};
} // namespace pdbs

#endif
