/*
 * Copyright (c) 2017 Peter Hatina <phatina@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#ifndef __ORDERED_MAP_H
#define __ORDERED_MAP_H

#include <algorithm>
#include <map>
#include <vector>

template <typename Key, typename Index, typename Compare>
class compare_helper
{
public:
    compare_helper(std::map<Key, Index>* k)
        : keys_{k}
        , cmp_{}
    {
    }

    bool operator()(const Key& lhs, const Key& rhs) const
    {
        const decltype(auto) lhs_{keys_->find(lhs)};
        const decltype(auto) rhs_{keys_->find(rhs)};

        return cmp_(lhs_->second, rhs_->second);
    }

private:
    std::map<Key, Index>* keys_;
    Compare cmp_;
};

template <
    typename Key,
    typename T,
    typename Index = uint64_t,
    typename Compare = std::less<Index>,
    typename Allocator = std::allocator<std::pair<const Key, T>>>
class ordered_map
{
public:
    using CompareHelper = compare_helper<Key, Index, Compare>;
    using InnerMapType = std::map<Key, T, CompareHelper, Allocator>;
    using KeyContainer = std::map<Key, Index>;

    using key_type = Key;
    using mapped_type = T;
    using value_type = std::pair<const Key, T>;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using key_compare = Compare;
    using allocator_type = Allocator;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = typename std::allocator_traits<Allocator>::pointer;
    using const_pointer = typename std::allocator_traits<Allocator>::const_pointer;

    using iterator = typename InnerMapType::iterator;
    using const_iterator = typename InnerMapType::const_iterator;
    using reverse_iterator = typename InnerMapType::reverse_iterator;
    using const_reverse_iterator = typename InnerMapType::const_reverse_iterator;

public:
    ordered_map()
        : keys_{}
        , compare_{&keys_}
        , map_{compare_}
    {
    }

    template<class InputIterator>
    ordered_map(InputIterator first, InputIterator last)
        : keys_{}
        , compare_{&keys_}
        , map_{compare_}
    {
        for (auto it = first; it != last; ++it)
            insert(*it);
    }

    ordered_map(std::initializer_list<value_type>&& init)
        : ordered_map()
    {
        for (auto& x : init)
            insert(x);
    }

    T& at(const Key& key) { return map_.at(key); }
    const T& at(const Key& key) const { return map_.at(key); }

    T& operator[](const Key& key)
    {
        add_index(key);
        return map_[key];
    }

    T& operator[](Key&& key)
    {
        add_index(key);
        return map_[key];
    }

    std::vector<Key> keys() const
    {
        std::vector<Key> k(keys_.size());
        std::transform(
            std::begin(*this),
            std::end(*this),
            std::begin(k),
            [](const auto& p){ return p.first; });
        return k;
    }

    iterator begin() noexcept { return map_.begin(); }
    iterator end() noexcept { return map_.end(); }
    const_iterator begin() const noexcept { return map_.begin(); }
    const_iterator end() const noexcept { return map_.end(); }

    const_iterator cbegin() const noexcept { return map_.cbegin(); }
    const_iterator cend() const noexcept { return map_.cend(); }

    reverse_iterator rbegin() noexcept { return map_.rbegin(); }
    reverse_iterator rend() noexcept { return map_.rend(); }

    const_reverse_iterator rbegin() const noexcept { return map_.rbegin(); }
    const_reverse_iterator rend() const noexcept { return map_.rend(); }

    const_reverse_iterator crbegin() const noexcept { return map_.crbegin(); }
    const_reverse_iterator crend() const noexcept { return map_.crend(); }

    bool empty() const noexcept { return map_.empty(); }
    size_type size() const noexcept { return map_.size(); }
    size_type max_size() const noexcept { return map_.max_size(); }

    void clear() noexcept
    {
        map_.clear();
        keys_.clear();
    }

    std::pair<iterator, bool> insert(const value_type& value)
    {
        add_index(value.first);
        return map_.insert(value);
    }

    template<class P>
    std::pair<iterator, bool> insert( P&& value )
    {
        add_index(value.first);
        return map_.insert(std::forward<P>(value));
    }

    iterator insert(const_iterator hint, const value_type& value)
    {
        add_index(value.first);
        return map_.insert(hint, value);
    }

    iterator insert(const_iterator hint, value_type&& value)
    {
        add_index(value.first);
        return map_.insert(hint, value);
    }

    template<class InputIt>
    void insert(InputIt first, InputIt last)
    {
        for (auto it = first; it != last; ++it)
            add_index(it->first);
        map_.insert(first, last);
    }

    void insert(std::initializer_list<value_type> ilist)
    {
        for (auto value : ilist)
            add_index(value.first);
        map_.insert(ilist);
    }

    template<class... Args>
    std::pair<iterator, bool> emplace(Args&& ... args)
    {
        typename ordered_map::value_type value(std::forward<Args>(args)...);
        add_index(value.first);
        return map_.emplace(std::move(value));
    }

    template<class... Args>
    iterator emplace_hint(const_iterator hint, Args&& ... args)
    {
        typename ordered_map::value_type value(std::forward<Args>(args)...);
        add_index(value.first);
        return map_.emplace_hint(hint, std::move(value));
    }

    iterator erase(const_iterator pos)
    {
        remove_index(pos->first);
        return map_.erase(pos);
    }

    iterator erase(const_iterator first, const_iterator last)
    {
        for (auto it = first; it != last; ++it)
            remove_index(it->first);
        return map_.erase(first, last);
    }

    size_type erase(const key_type& key)
    {
        auto res = map_.erase(key);
        if (res > 0)
            remove_index(key);
        return res;
    }

    void swap(ordered_map& other)
    {
        std::swap(map_, other.map_);
        std::swap(compare_, other.compare_);
        std::swap(keys_, other.keys_);
    }

    size_type count(const Key& key) const { return map_.count(key); }
    iterator find(const Key& key) { return map_.find(key); }
    const_iterator find(const Key& key) const { return map_.find(key); }

    std::pair<iterator, iterator> equal_range(const Key& key)
    {
        return map_.equal_range(key);
    }

    std::pair<const_iterator, const_iterator> equal_range(const Key& key) const
    {
        return map_.equal_range(key);
    }

    iterator lower_bound(const Key& key) { return map_.lower_bound(key); }
    const_iterator lower_bound(const Key& key) const { return map_.lower_bound(key); }
    iterator upper_bound(const Key& key) { return map_.upper_bound(key); }
    const_iterator upper_bound(const Key& key) const { return map_.upper_bound(key); }

    key_compare key_comp() const { return compare_; }

    bool operator==(const ordered_map& rhs) const { return map_ == rhs.map_; }
    bool operator!=(const ordered_map& rhs) const { return map_ != rhs.map_; }
    bool operator< (const ordered_map& rhs) const { return map_ <  rhs.map_; }
    bool operator<=(const ordered_map& rhs) const { return map_ <= rhs.map_; }
    bool operator> (const ordered_map& rhs) const { return map_ >  rhs.map_; }
    bool operator>=(const ordered_map& rhs) const { return map_ >= rhs.map_; }

private:
    void add_index(const Key& key) { keys_.insert({key, index_++}); }
    void remove_index(const Key& key) { keys_.erase(key); }

    KeyContainer keys_;
    CompareHelper compare_;
    InnerMapType map_;
    Index index_ = 0;
};

#endif // __ORDERED_MAP_H
