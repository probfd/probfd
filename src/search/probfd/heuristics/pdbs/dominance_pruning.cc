#include "probfd/heuristics/pdbs/dominance_pruning.h"

#include "probfd/heuristics/pdbs/probability_aware_pattern_database.h"

#include "downward/utils/countdown_timer.h"
#include "downward/utils/hash.h"

#include <cassert>

namespace probfd {
namespace heuristics {
namespace pdbs {

class Pruner {
    /*
      Algorithm for pruning dominated patterns.

      "patterns" is the vector of patterns used.
      Each pattern is a vector of variable IDs.

      "pattern_cliques" is the vector of pattern cliques.

      The algorithm works by setting a "current pattern collection"
      against which other patterns and collections can be tested for
      dominance efficiently.

      "variable_to_pattern_id" encodes the relevant information about the
      current clique. For every variable v, variable_to_pattern_id[v] is
      the id of the pattern containing v in the current clique,
      or -1 if the variable is not contained in the current
      clique. (Note that patterns in a pattern clique must be
      disjoint, which is verified by an assertion in debug mode.)

      To test if a given pattern v_1, ..., v_k is dominated by the
      current clique, we check that all entries variable_to_pattern_id[v_i]
      are equal and different from -1.

      "dominated_patterns" is a vector<bool> that can be used to
      quickly test whether a given pattern is dominated by the current
      clique. This is precomputed for every pattern whenever the
      current clique is set.
    */

    const PPDBCollection& pdbs;
    const std::vector<PatternSubCollection>& pattern_cliques;
    const int num_variables;

    std::vector<int> variable_to_pattern_id;
    std::vector<bool> dominated_patterns;

    void set_current_clique(int clique_id)
    {
        /*
          Set the current pattern collection to be used for
          is_pattern_dominated() or is_collection_dominated(). Compute
          dominated_patterns based on the current pattern collection.
        */
        variable_to_pattern_id.assign(num_variables, -1);
        assert(variable_to_pattern_id == std::vector<int>(num_variables, -1));
        for (PatternID pattern_id : pattern_cliques[clique_id]) {
            for (int variable : pdbs[pattern_id]->get_pattern()) {
                assert(variable_to_pattern_id[variable] == -1);
                variable_to_pattern_id[variable] = pattern_id;
            }
        }

        dominated_patterns.clear();
        dominated_patterns.reserve(pdbs.size());
        for (size_t i = 0; i < pdbs.size(); ++i) {
            dominated_patterns.push_back(is_pattern_dominated(i));
        }
    }

    bool is_pattern_dominated(int pattern_id) const
    {
        /*
          Check if the pattern with the given pattern_id is dominated
          by the current pattern clique.
        */
        const Pattern& pattern = pdbs[pattern_id]->get_pattern();
        assert(!pattern.empty());
        PatternID clique_pattern_id = variable_to_pattern_id[pattern[0]];
        if (clique_pattern_id == -1) {
            return false;
        }
        int pattern_size = pattern.size();
        for (int i = 1; i < pattern_size; ++i) {
            if (variable_to_pattern_id[pattern[i]] != clique_pattern_id) {
                return false;
            }
        }
        return true;
    }

    bool is_clique_dominated(int clique_id) const
    {
        /*
          Check if the collection with the given collection_id is
          dominated by the current pattern collection.
        */
        for (PatternID pattern_id : pattern_cliques[clique_id]) {
            if (!dominated_patterns[pattern_id]) {
                return false;
            }
        }
        return true;
    }

public:
    Pruner(
        const PPDBCollection& pdbs,
        const std::vector<PatternSubCollection>& pattern_cliques,
        int num_variables)
        : pdbs(pdbs)
        , pattern_cliques(pattern_cliques)
        , num_variables(num_variables)
    {
    }

    std::vector<bool>
    get_pruned_cliques(const utils::CountdownTimer& timer, utils::LogProxy log)
    {
        int num_cliques = pattern_cliques.size();
        std::vector<bool> pruned(num_cliques, false);
        /*
          Already pruned cliques are not used to prune other
          cliques. This makes things faster and helps handle
          duplicate cliques in the correct way: the first copy
          will survive and prune all duplicates.
        */
        for (int c1 = 0; c1 < num_cliques; ++c1) {
            if (!pruned[c1]) {
                set_current_clique(c1);
                for (int c2 = 0; c2 < num_cliques; ++c2) {
                    if (c1 != c2 && !pruned[c2] && is_clique_dominated(c2))
                        pruned[c2] = true;
                }
            }
            if (timer.is_expired()) {
                /*
                  Since after each iteration, we determined if a given
                  clique is pruned or not, we can just break the
                  computation here if reaching the time limit and use all
                  information we collected so far.
                */
                if (log.is_at_least_normal()) {
                    log << "Time limit reached. Abort dominance pruning."
                        << std::endl;
                }
                break;
            }
        }

        return pruned;
    }
};

/*
  Clique superset dominates clique subset iff for every pattern
  p_subset in subset there is a pattern p_superset in superset where
  p_superset is a superset of p_subset.
*/
extern void prune_dominated_cliques(
    PPDBCollection& pdbs,
    std::vector<PatternSubCollection>& pattern_cliques,
    int num_variables,
    double max_time,
    utils::LogProxy log)
{
    utils::CountdownTimer timer(max_time);

    int num_pdbs = pdbs.size();
    int num_cliques = pattern_cliques.size();

    std::vector<bool> pruned = Pruner(pdbs, pattern_cliques, num_variables)
                                   .get_pruned_cliques(timer, log);

    std::vector<PatternSubCollection> remaining_pattern_cliques;
    std::vector<bool> is_remaining_pattern(num_pdbs, false);
    int num_remaining_pdbs = 0;
    for (size_t i = 0; i < pattern_cliques.size(); ++i) {
        if (!pruned[i]) {
            PatternSubCollection& clique = pattern_cliques[i];
            for (PatternID pattern_id : clique) {
                if (!is_remaining_pattern[pattern_id]) {
                    is_remaining_pattern[pattern_id] = true;
                    ++num_remaining_pdbs;
                }
            }
            remaining_pattern_cliques.push_back(std::move(clique));
        }
    }

    PPDBCollection remaining_pdbs;
    remaining_pdbs.reserve(num_remaining_pdbs);
    std::vector<PatternID> old_to_new_pattern_id(num_pdbs, -1);
    for (PatternID old_pattern_id = 0; old_pattern_id < num_pdbs;
         ++old_pattern_id) {
        if (is_remaining_pattern[old_pattern_id]) {
            PatternID new_pattern_id = remaining_pdbs.size();
            old_to_new_pattern_id[old_pattern_id] = new_pattern_id;
            remaining_pdbs.push_back(std::move(pdbs[old_pattern_id]));
        }
    }
    for (PatternSubCollection& clique : remaining_pattern_cliques) {
        for (size_t i = 0; i < clique.size(); ++i) {
            PatternID old_pattern_id = clique[i];
            PatternID new_pattern_id = old_to_new_pattern_id[old_pattern_id];
            assert(new_pattern_id != -1);
            clique[i] = new_pattern_id;
        }
    }

    int num_pruned_collections = num_cliques - remaining_pattern_cliques.size();
    int num_pruned_patterns = num_pdbs - num_remaining_pdbs;

    pdbs.swap(remaining_pdbs);
    pattern_cliques.swap(remaining_pattern_cliques);

    if (log.is_at_least_normal()) {
        log << "Pruned " << num_pruned_collections << " of " << num_cliques
            << " pattern cliques" << std::endl;
        log << "Pruned " << num_pruned_patterns << " of " << num_pdbs << " PDBs"
            << std::endl;
        log << "Dominance pruning took " << timer.get_elapsed_time()
            << std::endl;
    }
}
} // namespace pdbs
} // namespace heuristics
} // namespace probfd