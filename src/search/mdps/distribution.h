#ifndef MDPS_DISTRIBUTION_H
#define MDPS_DISTRIBUTION_H

#include "value_type.h"

#include "../utils/iterators.h"
#include "../utils/range_proxy.h"
#include "../utils/rng.h"

#include <algorithm>
#include <cassert>
#include <vector>

namespace probabilistic {

/**
 * @brief A convenience class that wraps a list of element-probability
 * pairs.
 *
 * @tparam T The element type.
 *
 */
template <typename T>
class Distribution {
private:
    using distribution_t =
        typename std::vector<std::pair<T, value_type::value_t>>;
    distribution_t distribution_;

public:
    /// Iterator type.
    using iterator = typename distribution_t::iterator;
    using const_iterator = typename distribution_t::const_iterator;

    using element_iterator = utils::key_iterator<iterator>;
    using const_element_iterator = utils::key_iterator<const_iterator>;

    using probability_iterator = utils::val_iterator<iterator>;
    using const_probability_iterator = utils::val_iterator<const_iterator>;

    using element_range = utils::RangeProxy<element_iterator>;
    using const_element_range = utils::RangeProxy<element_iterator>;

    using probability_range = utils::RangeProxy<probability_iterator>;
    using const_probability_range = utils::RangeProxy<probability_iterator>;

    /// Constructs the empty list of element-probability pairs.
    Distribution() = default;

    /**
     * Constructs the list from a vector of element-probability pairs.
     */
    explicit Distribution(distribution_t d)
        : distribution_(std::move(d))
    {
    }

    /**
     * Returns the current number of element-probability pairs.
     */
    size_t size() const { return distribution_.size(); }

    /// Clears the list.
    void clear() { distribution_.clear(); }

    void swap(Distribution<T>& other)
    {
        other.distribution_.swap(distribution_);
    }

    /**
     * Adds element-probability pair (\p t, \p prob) to the list,
     * even if another pair with an element that compares equal to \p t is
     * already present.
     *
     * @see make_unique
     */
    void add(T t, value_type::value_t prob)
    {
        assert(prob > 0.0);
        distribution_.emplace_back(std::move(t), prob);
    }

    std::pair<iterator, bool> add_unique(T t, value_type::value_t prob)
    {
        assert(prob > 0.0);

        auto it = this->find(t);

        if (it != end()) {
            it->second += prob;
            return {it, false};
        }

        return {distribution_.emplace(it, std::move(t), prob), true};
    }

    iterator find(const T& t)
    {
        return std::find(elem_begin(), elem_end(), t).base;
    }

    /**
     * Returns true if and only if the list is empty.
     */
    bool empty() const { return distribution_.empty(); }

    /**
     * Multiplies each element probability with \p scale.
     */
    void normalize(const value_type::value_t& scale)
    {
        for (auto it = distribution_.begin(); it != distribution_.end(); it++) {
            it->second *= scale;
        }
    }

    /**
     * Normalizes the probabilities of the elements to sum up to one.
     */
    void normalize()
    {
        if (empty()) {
            return;
        }
        value_type::value_t sum = 0;
        for (auto it = begin(); it != end(); it++) {
            sum += it->second;
        }
        normalize(value_type::value_t(1.0) / sum);
    }

    /**
     * Merges equal elements in the list by adding their probabilities.
     */
    void make_unique()
    {
        if (empty()) {
            return;
        }

        std::sort(distribution_.begin(), distribution_.end());
        unsigned i = 0;
        for (unsigned j = 1; j < distribution_.size(); ++j) {
            if (distribution_[i].first == distribution_[j].first) {
                distribution_[i].second += distribution_[j].second;
                distribution_[j].second = 0;
            } else {
                i = j;
            }
        }
        i = 1;
        for (unsigned j = 1; j < distribution_.size(); ++j) {
            if (distribution_[j].second != 0) {
                distribution_[i] = distribution_[j];
                ++i;
            }
        }
        distribution_.erase(distribution_.begin() + i, distribution_.end());
    }

    iterator sample(utils::RandomNumberGenerator& rng) {
        assert(!empty());

        const value_type::value_t r = rng();

        auto it = distribution_.begin();
        value_type::value_t sum = it->second;

        while (sum <= r) {
            sum += (++it)->second;
        }

        return it;
    }

    /**
     * @brief Removes the element-probability pair pointed to by \p it.
     *
     * \returns An iterator to the element-probability pair directly behind the
     * erased pair.
     */
    iterator erase(iterator it) { return distribution_.erase(it); }

    iterator begin() { return distribution_.begin(); }

    iterator end() { return distribution_.end(); }

    const_iterator begin() const { return distribution_.begin(); }

    const_iterator end() const { return distribution_.end(); }

    element_iterator elem_begin()
    {
        return element_iterator(
            distribution_.begin(),
            &std::pair<T, value_type::value_t>::first);
    }

    element_iterator elem_end()
    {
        return element_iterator(
            distribution_.end(),
            &std::pair<T, value_type::value_t>::first);
    }

    const_element_iterator elem_begin() const
    {
        return element_iterator(
            distribution_.begin(),
            &std::pair<T, value_type::value_t>::first);
    }

    const_element_iterator elem_end() const
    {
        return element_iterator(
            distribution_.end(),
            &std::pair<T, value_type::value_t>::first);
    }

    element_range elements() { return element_range(elem_begin(), elem_end()); }

    const_element_range elements() const
    {
        return element_range(elem_begin(), elem_end());
    }

    probability_iterator prob_begin()
    {
        return probability_iterator(
            distribution_.begin(),
            &std::pair<T, value_type::value_t>::second);
    }

    probability_iterator prob_end()
    {
        return probability_iterator(
            distribution_.end(),
            &std::pair<T, value_type::value_t>::second);
    }

    probability_range probablities()
    {
        return probability_range(elem_begin(), elem_end());
    }

    const_probability_iterator prob_begin() const
    {
        return probability_iterator(
            distribution_.begin(),
            &std::pair<T, value_type::value_t>::second);
    }

    const_probability_iterator prob_end() const
    {
        return probability_iterator(
            distribution_.end(),
            &std::pair<T, value_type::value_t>::second);
    }

    const_probability_range probablities() const
    {
        return probability_range(elem_begin(), elem_end());
    }

    /**
     * @brief Obtains a reference to the internal list.
     */
    distribution_t& data() { return distribution_; }

    /**
     * @brief Obtains a const reference to the internal list.
     */
    const distribution_t& data() const { return distribution_; }
};

} // namespace probabilistic

#endif // __DISTRIBUTION_H__