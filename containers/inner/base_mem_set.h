/// @file base_mem_set.h
/// @brief 内存哈希集合基础实现（C++20 重写版）
/// @note 改进: if constexpr 替代 enable_if 的 destruct/copy 分支
///       改进: [[nodiscard]] 标记查询方法
#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <type_traits>
#include <utility>
#include "mem_set_data.h"

namespace ua
{

template <typename T, size_t MAX_SIZE, typename HASH, typename IS_EQUAL>
struct BaseMemSet : public inner::SetData<T, MAX_SIZE, HASH>
{
    using Data = inner::SetData<T, MAX_SIZE, HASH>;
    using IntType = typename Data::IntType;
    using ValueType = typename Data::ValueType;
    using RealValueType = typename Data::RealValueType;

    class Iterator
    {
        friend struct BaseMemSet;
        const BaseMemSet* m_set = nullptr;
        IntType m_index = 0;
        Iterator(const BaseMemSet* set, IntType index) : m_set(set), m_index(index) {}

    public:
        using difference_type = std::ptrdiff_t;
        using value_type = T;
        using pointer = T*;
        using reference = T&;
        using iterator_category = std::forward_iterator_tag;

        Iterator() = default;
        const T& operator*() const { return m_set->index_2_value(m_index - 1); }
        T& operator*() { return const_cast<BaseMemSet*>(m_set)->index_2_value(m_index - 1); }
        const T* operator->() const { return &(**this); }
        T* operator->() { return &(**this); }
        bool operator==(const Iterator& r) const { return m_set == r.m_set && m_index == r.m_index; }
        bool operator!=(const Iterator& r) const { return !(*this == r); }

        Iterator& operator++()
        {
            IntType next_index = m_set->m_next[m_index - 1];
            if (next_index == 0)
            {
                IntType next_bucket = m_set->get_bucket_index(m_set->index_2_value(m_index - 1)) + 1;
                IntType buckets_num = m_set->get_buckets_num();
                while (next_bucket < buckets_num)
                {
                    if (m_set->m_buckets[next_bucket] != 0)
                    {
                        next_index = m_set->m_buckets[next_bucket];
                        break;
                    }
                    ++next_bucket;
                }
            }
            m_index = next_index;
            return *this;
        }
        Iterator operator++(int) { Iterator t = *this; ++(*this); return t; }
    };

    void clear()
    {
        destruct_all();
        memset(Data::m_next, 0, sizeof(IntType) * Data::get_max_num());
        Data::set_used(0);
        Data::set_raw_used(0);
        Data::set_free_index(0);
        memset(Data::m_buckets, 0, sizeof(IntType) * Data::get_buckets_num());
    }

    [[nodiscard]] bool empty() const { return Data::get_used() == 0 && Data::get_max_num() > 0; }
    [[nodiscard]] bool full() const { return Data::get_used() == Data::get_max_num(); }
    [[nodiscard]] size_t size() const { return Data::get_used(); }
    [[nodiscard]] size_t capacity() const { return Data::get_max_num(); }

    std::pair<Iterator, bool> insert(const T& value)
    {
        IntType bucket_index = Data::get_bucket_index(value);
        IntType index = find_index_impl(bucket_index, value);
        if (index != 0)
            return {Iterator(this, index), false};
        if (full())
            return {end(), false};
        return {Iterator(this, insert_impl(bucket_index, value)), true};
    }

    std::pair<IntType, bool> insert2(const T& value)
    {
        IntType bucket_index = Data::get_bucket_index(value);
        IntType index = find_index_impl(bucket_index, value);
        if (index != 0)
            return {index, false};
        if (full())
            return {0, false};
        return {insert_impl(bucket_index, value), true};
    }

    template <typename K>
    [[nodiscard]] const Iterator find(const K& key) const
    {
        return Iterator(this, find_index_impl(Data::get_bucket_index(key), key));
    }
    template <typename K>
    [[nodiscard]] Iterator find(const K& key)
    {
        return Iterator(this, find_index_impl(Data::get_bucket_index(key), key));
    }
    template <typename K>
    [[nodiscard]] IntType find_index(const K& key) const
    {
        return find_index_impl(Data::get_bucket_index(key), key);
    }

    template <typename K>
    [[nodiscard]] bool exist(const K& key) const { return find(key) != end(); }

    IntType erase(const Iterator& it)
    {
        assert(it.m_set == this);
        if (it.m_index > 0)
            return erase(index_2_value(it.m_index - 1));
        return 0;
    }

    template <typename K>
    IntType erase(const K& key)
    {
        if (Data::get_used() == 0) return 0;

        IntType bucket_index = Data::get_bucket_index(key);
        assert(bucket_index < Data::get_buckets_num());
        if (Data::m_buckets[bucket_index] == 0) return 0;

        IS_EQUAL is_equal;
        IntType* pre = &(Data::m_buckets[bucket_index]);
        for (IntType index = Data::m_buckets[bucket_index]; index != 0;
             pre = &(Data::m_next[index - 1]), index = Data::m_next[index - 1])
        {
            if (is_equal(index_2_value(index - 1), key))
            {
                assert(Data::get_used() > 0);
                *pre = Data::m_next[index - 1];
                Data::m_next[index - 1] = Data::get_free_index();
                Data::set_free_index(index);
                Data::decr_used();
                destruct_one(index);
                return index;
            }
        }
        return 0;
    }

    [[nodiscard]] const Iterator begin() const { return Iterator(this, find_first_used_bucket()); }
    [[nodiscard]] Iterator begin() { return Iterator(this, find_first_used_bucket()); }
    [[nodiscard]] const Iterator end() const { return Iterator(this, 0); }
    [[nodiscard]] Iterator end() { return Iterator(this, 0); }

    [[nodiscard]] const T& deref(IntType index) const { return index_2_value(index - 1); }
    [[nodiscard]] T& deref(IntType index) { return index_2_value(index - 1); }

protected:
    [[nodiscard]] IntType find_first_used_bucket() const
    {
        IntType buckets_num = Data::get_buckets_num();
        for (IntType i = 0; i < buckets_num; ++i)
            if (Data::m_buckets[i] != 0) return Data::m_buckets[i];
        return 0;
    }

    template <typename K>
    [[nodiscard]] IntType find_index_impl(IntType bucket_index, const K& key) const
    {
        assert(bucket_index < Data::get_buckets_num());
        IS_EQUAL is_equal;
        for (IntType index = Data::m_buckets[bucket_index]; index != 0; index = Data::m_next[index - 1])
            if (is_equal(index_2_value(index - 1), key)) return index;
        return 0;
    }

    IntType insert_impl(IntType bucket_index, const T& value)
    {
        IntType empty_index = 0;
        if (Data::get_free_index() == 0)
        {
            assert(Data::get_raw_used() < Data::get_max_num());
            Data::incr_raw_used();
            empty_index = Data::get_raw_used();
        }
        else
        {
            empty_index = Data::get_free_index();
            Data::set_free_index(Data::m_next[empty_index - 1]);
        }
        assert(empty_index > 0);
        Data::m_next[empty_index - 1] = Data::m_buckets[bucket_index];
        Data::m_buckets[bucket_index] = empty_index;
        Data::incr_used();
        copy_one(empty_index, value);
        return empty_index;
    }

    T* get_value(IntType index)
    {
        return reinterpret_cast<T*>(&Data::m_value[index * sizeof(T) / sizeof(RealValueType)]);
    }

    T& index_2_value(IntType index)
    {
        return reinterpret_cast<T&>(Data::m_value[index * sizeof(T) / sizeof(RealValueType)]);
    }

    const T& index_2_value(IntType index) const
    {
        return reinterpret_cast<const T&>(Data::m_value[index * sizeof(T) / sizeof(RealValueType)]);
    }

    // 改进: if constexpr 替代 enable_if 的四个 SFINAE 重载
    void destruct_all()
    {
        if constexpr (!std::is_trivially_copyable_v<T>)
        {
            auto beg = begin();
            while (beg != end()) { beg->~T(); ++beg; }
        }
    }

    void destruct_one(IntType index)
    {
        if constexpr (!std::is_trivially_copyable_v<T>)
            index_2_value(index - 1).~T();
    }

    void copy_one(IntType index, const T& value)
    {
        if constexpr (std::is_trivially_copyable_v<T>)
            index_2_value(index - 1) = value;
        else
            new (&(index_2_value(index - 1))) T(value);
    }
};

}  // namespace ua
