/// @file base_mem_lru_set.h
/// @brief LRU 集合基础实现（C++20 重写版）
/// @note 所有方法内联
#pragma once

#include <cassert>
#include <functional>
#include <iterator>
#include "mem_lru_set_data.h"

namespace ua
{

template <typename T, size_t MAX_SIZE, typename HASH, typename IS_EQUAL>
struct BaseMemLRUSet : public inner::LRUSetData<T, MAX_SIZE, HASH, IS_EQUAL>
{
    using Data = inner::LRUSetData<T, MAX_SIZE, HASH, IS_EQUAL>;
    using IntType = typename Data::IntType;
    using LinkNode = typename Data::LinkNode;
    using ValueType = T;
    using DisuseCallback = std::function<bool(ValueType&)>;

    class Iterator
    {
        friend struct BaseMemLRUSet;
        const BaseMemLRUSet* m_set = nullptr;
        IntType m_index = 0;
        Iterator(const BaseMemLRUSet* set, IntType index) : m_set(set), m_index(index) {}

    public:
        using difference_type = std::ptrdiff_t;
        using value_type = T;
        using pointer = T*;
        using reference = T&;
        using iterator_category = std::bidirectional_iterator_tag;

        Iterator() = default;
        const T& operator*() const { return m_set->deref(m_index); }
        T& operator*() { return const_cast<BaseMemLRUSet*>(m_set)->deref(m_index); }
        const T* operator->() const { return &(**this); }
        T* operator->() { return &(**this); }
        bool operator==(const Iterator& r) const { return m_set == r.m_set && m_index == r.m_index; }
        bool operator!=(const Iterator& r) const { return !(*this == r); }
        Iterator& operator++() { m_index = m_set->m_active_link[m_index].next; return *this; }
        Iterator operator++(int) { Iterator t = *this; ++(*this); return t; }
        Iterator& operator--() { m_index = m_set->m_active_link[m_index].prev; return *this; }
        Iterator operator--(int) { Iterator t = *this; --(*this); return t; }
    };

    void clear() { Data::clear(); }
    [[nodiscard]] bool empty() const { return Data::m_base.empty(); }
    [[nodiscard]] bool full() const { return Data::m_base.full(); }
    [[nodiscard]] size_t size() const { return Data::m_base.size(); }
    [[nodiscard]] size_t capacity() const { return Data::m_base.capacity(); }

    std::pair<Iterator, bool> insert(const T& value, bool force, const DisuseCallback& cb = nullptr)
    {
        if (Data::m_base.full())
        {
            auto iter = find(value);
            if (iter != end()) return {iter, false};
            if (!force || disuse(1, cb) == 0)
                return {end(), false};
        }
        auto result_pair = Data::m_base.insert2(value);
        if (result_pair.second)
        {
            IntType index = result_pair.first;
            auto& head = Data::m_active_link[0];
            Data::m_active_link[head.next].prev = index;
            Data::m_active_link[index].prev = 0;
            Data::m_active_link[index].next = head.next;
            head.next = index;
        }
        return {Iterator(this, result_pair.first), result_pair.second};
    }

    template <typename K>
    [[nodiscard]] const Iterator find(const K& key) const { return Iterator(this, Data::m_base.find_index(key)); }
    template <typename K>
    [[nodiscard]] Iterator find(const K& key) { return Iterator(this, Data::m_base.find_index(key)); }

    template <typename K>
    [[nodiscard]] bool exist(const K& key) const { return Data::m_base.exist(key); }

    void erase(const Iterator& it) { assert(it.m_set == this); erase(*it); }

    template <typename K>
    void erase(const K& key)
    {
        IntType index = Data::m_base.erase(key);
        if (index > 0)
        {
            IntType next = Data::m_active_link[index].next;
            IntType prev = Data::m_active_link[index].prev;
            Data::m_active_link[prev].next = next;
            Data::m_active_link[next].prev = prev;
            Data::m_active_link[index] = {};
        }
    }

    template <typename K>
    Iterator active(const K& key)
    {
        IntType index = Data::m_base.find_index(key);
        if (index != 0)
        {
            // 先摘除
            IntType next = Data::m_active_link[index].next;
            IntType prev = Data::m_active_link[index].prev;
            Data::m_active_link[prev].next = next;
            Data::m_active_link[next].prev = prev;
            // 插到头部
            auto& head = Data::m_active_link[0];
            Data::m_active_link[head.next].prev = index;
            Data::m_active_link[index].prev = 0;
            Data::m_active_link[index].next = head.next;
            head.next = index;
        }
        return Iterator(this, index);
    }

    size_t disuse(size_t num, const DisuseCallback& cb = nullptr)
    {
        for (size_t i = 0; i < num; ++i)
        {
            if (Data::m_base.empty()) return i;
            if (cb && !cb(deref(Data::m_active_link[0].prev))) return i;
            erase(deref(Data::m_active_link[0].prev));
        }
        return num;
    }

    [[nodiscard]] const Iterator begin() const { return Iterator(this, Data::m_active_link[0].next); }
    [[nodiscard]] Iterator begin() { return Iterator(this, Data::m_active_link[0].next); }
    [[nodiscard]] const Iterator end() const { return Iterator(this, 0); }
    [[nodiscard]] Iterator end() { return Iterator(this, 0); }

private:
    [[nodiscard]] const T& deref(IntType index) const { return Data::m_base.deref(index); }
    [[nodiscard]] T& deref(IntType index) { return Data::m_base.deref(index); }
};

}  // namespace ua
