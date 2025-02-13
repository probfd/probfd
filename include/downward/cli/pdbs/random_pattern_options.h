#ifndef DOWNWARD_PLUGINS_PDBS_RANDOM_PATTERN_H
#define DOWNWARD_PLUGINS_PDBS_RANDOM_PATTERN_H

#include <tuple>

namespace downward::cli::plugins {
class Feature;
class Options;
} // namespace downward::cli::plugins

namespace downward::cli::pdbs {

extern void
add_random_pattern_implementation_notes_to_feature(plugins::Feature& feature);

extern void
add_random_pattern_bidirectional_option_to_feature(plugins::Feature& feature);

extern std::tuple<bool> get_random_pattern_bidirectional_arguments_from_options(
    const plugins::Options& opts);

} // namespace downward::cli::pdbs

#endif
