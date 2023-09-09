#ifndef PDBS_PATTERN_CLIQUES_H
#define PDBS_PATTERN_CLIQUES_H

#include "downward/pdbs/types.h"

#include <memory>
#include <ranges>
#include <vector>

class TaskProxy;

namespace pdbs {
using VariableAdditivity = std::vector<std::vector<bool>>;

extern VariableAdditivity compute_additive_vars(const TaskProxy& task_proxy);

/* Returns true iff the two patterns are additive i.e. there is no operator
   which affects variables in pattern one as well as in pattern two. */
extern bool are_patterns_additive(
    const Pattern& pattern1,
    const Pattern& pattern2,
    const VariableAdditivity& are_additive);

/*
  Computes pattern cliques of the given patterns.
*/
extern std::shared_ptr<std::vector<PatternClique>> compute_pattern_cliques(
    const PatternCollection& patterns,
    const VariableAdditivity& are_additive);

/*
  We compute pattern cliques S with the property that we could
  add the new pattern P to S and still have a pattern clique.

  Ideally, we would like to return all *maximal* cliques S with this
  property (w.r.t. set inclusion), but we don't currently
  guarantee this. (What we guarantee is that all maximal such cliques
  are *included* in the result, but the result could contain
  duplicates or cliques that are subcliques of other cliques in the
  result.)

  We currently implement this as follows:

  * Consider all pattern cliques of the current collection.
  * For each clique S, take the subclique S' that contains
    those patterns that are additive with the new pattern P.
  * Include the subclique S' in the result.

  As an optimization, we actually only include S' in the result if
  it is non-empty. However, this is wrong if *all* subcliques we get
  are empty, so we correct for this case at the end.

  This may include dominated elements and duplicates in the result.
  To avoid this, we could instead use the following algorithm:

  * Let N (= neighbours) be the set of patterns in our current
    collection that are additive with the new pattern P.
  * Let G_N be the compatibility graph of the current collection
    restricted to set N (i.e. drop all non-neighbours and their
    incident edges.)
  * Return the maximal cliques of G_N.

  One nice thing about this alternative algorithm is that we could
  also use it to incrementally compute the new set of pattern cliques
  after adding the new pattern P:

  G_N_cliques = max_cliques(G_N)   // as above
  new_max_cliques = (old_max_cliques \setminus G_N_cliques) \union
                    { clique \union {P} | clique in G_N_cliques}

  That is, the new set of maximal cliques is exactly the set of
  those "old" cliques that we cannot extend by P
  (old_max_cliques \setminus G_N_cliques) and all
  "new" cliques including P.
  */
std::vector<PatternClique> compute_pattern_cliques_with_pattern(
    const std::ranges::input_range auto& patterns,
    const std::vector<PatternClique>& known_pattern_cliques,
    const Pattern& new_pattern,
    const VariableAdditivity& are_additive)
{
    std::vector<PatternClique> cliques_additive_with_pattern;
    for (const PatternClique& known_clique : known_pattern_cliques) {
        // Take all patterns which are additive to new_pattern.
        PatternClique new_clique;
        new_clique.reserve(known_clique.size());
        for (PatternID pattern_id : known_clique) {
            if (are_patterns_additive(
                    new_pattern,
                    patterns[pattern_id],
                    are_additive)) {
                new_clique.push_back(pattern_id);
            }
        }
        if (!new_clique.empty()) {
            cliques_additive_with_pattern.push_back(new_clique);
        }
    }
    if (cliques_additive_with_pattern.empty()) {
        // If nothing was additive with the new variable, then
        // the only clique is the empty set.
        cliques_additive_with_pattern.emplace_back();
    }
    return cliques_additive_with_pattern;
}

} // namespace pdbs

#endif
