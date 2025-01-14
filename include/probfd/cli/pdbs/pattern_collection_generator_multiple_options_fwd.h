#ifndef CLI_PDBS_PATTERN_COLLECTION_GENERATOR_MULTIPLE_OPTIONS_FWD_H
#define CLI_PDBS_PATTERN_COLLECTION_GENERATOR_MULTIPLE_OPTIONS_FWD_H

#include "probfd/cli/pdbs/pattern_collection_generator_options_fwd.h"

#include "probfd/type_traits.h"

#include <memory>
#include <utility>

namespace utils {
enum class Verbosity;
class RandomNumberGenerator;
} // namespace utils

namespace probfd::cli::pdbs {

using PatternCollectionGeneratorMultipleAdditionalArgs =
    std::tuple<int, int, double, double, double, double, bool, bool>;

using PatternCollectionGeneratorMultipleArgs = tuple_cat_t<
    PatternCollectionGeneratorMultipleAdditionalArgs,
    std::tuple<std::shared_ptr<utils::RandomNumberGenerator>>,
    PatternCollectionGeneratorArgs>;

} // namespace probfd::cli::pdbs

#endif // CLI_PDBS_PATTERN_COLLECTION_GENERATOR_MULTIPLE_OPTIONS_FWD_H
