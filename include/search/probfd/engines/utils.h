#ifndef PROBFD_ENGINES_UTILS_H
#define PROBFD_ENGINES_UTILS_H

#include "probfd/interval.h"

namespace probfd {
namespace engines {

/// Convenience value type alias for engines selecting interval iteration
/// behaviour based on a template parameter.
template <bool UseInterval>
using EngineValueType = std::conditional_t<UseInterval, Interval, value_t>;

/// Returns the lower bound of the interval.
value_t as_lower_bound(Interval interval);

/// Returns the value unchanged.
value_t as_lower_bound(value_t single);

/**
 * @brief Computes the assignments `lhs.lower <- min(lhs.lower, rhs.lower)` and
 * `lower <- min(lhs.lower, rhs.lower)`.
 *
 * @returns True if and only if \p rhs.lower was less than \p lhs.lower .
 */
bool set_min(Interval& lhs, Interval rhs);

/**
 * @brief Computes the assignment lhs <- min(lhs, rhs).
 *
 * @returns True if and only if \p rhs was less than \p lhs .
 */
bool set_min(value_t& lhs, value_t rhs);

/**
 * @brief Intersects two intervals and assigns the result to the left
 * operand.
 *
 * @returns \b true if the lower bound of the left operand changed by more
 * than g_epsilon or if check_upper is true and the greater
 * bound of the left operand changed by more than g_epsilon.
 * \b false otherwise.
 *
 * @warning It is undefined behaviour to call this function with two
 * non-intersecting intervals.
 */
bool update(Interval& lhs, Interval rhs, bool check_upper = true);

// Value update
bool update(value_t& lhs, value_t rhs);

} // namespace engines
} // namespace probfd

#endif