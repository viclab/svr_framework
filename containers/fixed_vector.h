/// @file fixed_vector.h
/// @brief 定长 Vector（C++20 重写版）
/// @note 改进: 移除废弃的 std::iterator，改用 C++20 风格
///       改进: constexpr 和 [[nodiscard]]
///       改进: requires 约束 trivially_copyable
#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <type_traits>

namespace ua
{

/// C++20 随机访问迭代器（替代废弃的 std::iterator）
template <typename T>
class FixedVectorIterator
{
public:
    using difference_type = std::ptrdiff_t;
    using value_type = std::remove_const_t<T>;
    using pointer = T*;
    using reference = T&;
    using iterator_category = std::random_access_iterator_tag;

    FixedVectorIterator() = default;
    explicit FixedVectorIterator(T* it) : it_(it) {}

    // 支持从非 const 到 const 的隐式转换
    template <typename U>
        requires std::is_same_v<std::remove_const_t<T>, std::remove_const_t<U>>
    FixedVectorIterator(const FixedVectorIterator<U>& other) noexcept : it_(other.base()) {}

    reference operator*() const { return *it_; }
    pointer operator->() const { return it_; }
    reference operator[](difference_type d) const { return *(it_ + d); }

    FixedVectorIterator& operator++() { ++it_; return *this; }
    FixedVectorIterator operator++(int) { return FixedVectorIterator(it_++); }
    FixedVectorIterator& operator--() { --it_; return *this; }
    FixedVectorIterator operator--(int) { return FixedVectorIterator(it_--); }

    FixedVectorIterator& operator+=(difference_type d) { it_ += d; return *this; }
    FixedVectorIterator& operator-=(difference_type d) { it_ -= d; return *this; }

    friend FixedVectorIterator operator+(FixedVectorIterator it, difference_type d) { it += d; return it; }
    friend FixedVectorIterator operator+(difference_type d, FixedVectorIterator it) { it += d; return it; }
    friend FixedVectorIterator operator-(FixedVectorIterator it, difference_type d) { it -= d; return it; }
    friend difference_type operator-(const FixedVectorIterator& a, const FixedVectorIterator& b) { return a.it_ - b.it_; }

    auto operator<=>(const FixedVectorIterator& r) const = default;  // C++20 spaceship

    [[nodiscard]] pointer base() const { return it_; }

private:
    T* it_ = nullptr;
};

template <typename T, size_t MAX_SIZE, size_t INIT_SIZE = 0>
    requires(MAX_SIZE > 0 && INIT_SIZE <= MAX_SIZE && std::is_trivially_copyable_v<T>)
class FixedVector
{
public:
    using iterator = FixedVectorIterator<T>;
    using const_iterator = FixedVectorIterator<const T>;
    using value_type = T;
    using reference = T&;
    using const_reference = const T&;
    using pointer = T*;
    using const_pointer = const T*;
    using size_type = size_t;
    using difference_type = std::ptrdiff_t;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    reference operator[](size_t index)
    {
        assert(index < curr_size_);
        return data_[index];
    }
    const_reference operator[](size_t index) const
    {
        assert(index < curr_size_);
        return data_[index];
    }

    [[nodiscard]] pointer elements() { return data_; }
    [[nodiscard]] const_pointer elements() const { return data_; }

    [[nodiscard]] iterator begin() { return iterator(data_); }
    [[nodiscard]] const_iterator begin() const { return const_iterator(data_); }
    [[nodiscard]] const_iterator cbegin() const { return const_iterator(data_); }
    [[nodiscard]] iterator end() { return iterator(data_ + curr_size_); }
    [[nodiscard]] const_iterator end() const { return const_iterator(data_ + curr_size_); }
    [[nodiscard]] const_iterator cend() const { return const_iterator(data_ + curr_size_); }
    [[nodiscard]] reverse_iterator rbegin() { return reverse_iterator(end()); }
    [[nodiscard]] const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
    [[nodiscard]] reverse_iterator rend() { return reverse_iterator(begin()); }
    [[nodiscard]] const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

    /// 兼容老的 pbp 接口
    pointer Add()
    {
        if (curr_size_ + 1 <= MAX_SIZE)
        {
            memset(data_ + curr_size_, 0, sizeof(value_type));
            return &data_[curr_size_++];
        }
        return nullptr;
    }

    bool push_back(const value_type& value)
    {
        if (curr_size_ + 1 <= MAX_SIZE) { data_[curr_size_++] = value; return true; }
        return false;
    }

    bool push_back(value_type&& value)
    {
        if (curr_size_ + 1 <= MAX_SIZE) { data_[curr_size_++] = std::move(value); return true; }
        return false;
    }

    void resize(size_t s) { assert(s <= MAX_SIZE); curr_size_ = static_cast<uint32_t>(s); }
    [[nodiscard]] size_t size() const { return curr_size_; }
    [[nodiscard]] size_t max_size() const { return MAX_SIZE; }
    [[nodiscard]] size_t capacity() const { return MAX_SIZE; }

    iterator erase(const_iterator position) { return inner_erase(begin() + (position - cbegin())); }
    iterator erase(const_iterator first, const_iterator last)
    {
        return inner_erase(begin() + (first - cbegin()), begin() + (last - cbegin()));
    }

    void clear() { curr_size_ = 0; }
    [[nodiscard]] bool full() const { return curr_size_ == MAX_SIZE; }
    [[nodiscard]] bool empty() const { return curr_size_ == 0; }

private:
    iterator inner_erase(iterator first, iterator last)
    {
        if (first != last)
        {
            if (last != end())
                std::copy(last.base(), end().base(), first.base());
            erase_at_end(first.base() + (end().base() - last.base()));
        }
        return first;
    }

    iterator inner_erase(iterator position)
    {
        if (position + 1 != end())
            std::copy(position.base() + 1, end().base(), position.base());
        --curr_size_;
        return position;
    }

    void erase_at_end(pointer pos)
    {
        memset(pos, 0, sizeof(value_type) * (curr_size_ - static_cast<uint32_t>(pos - data_)));
        curr_size_ = static_cast<uint32_t>(pos - data_);
    }

private:
    uint32_t curr_size_ = INIT_SIZE;
    T data_[MAX_SIZE];
};

}  // namespace ua
