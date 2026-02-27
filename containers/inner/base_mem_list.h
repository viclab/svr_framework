/// @file base_mem_list.h
/// @brief 内存链表基础实现（C++20 重写版）
/// @note 改进: if constexpr 替代 enable_if 的四个 SFINAE 重载
///       改进: 所有方法内联，消除类外定义
#pragma once

#include <iterator>
#include <type_traits>
#include "base_struct.h"
#include "traits_utils.h"

namespace ua
{

template <typename T, size_t MAX_SIZE>
struct BaseMemList
{
    using IntType = typename FixIntType<MAX_SIZE + 1>::IntType;
    using ValueType = T;

    class Iterator
    {
        friend struct BaseMemList;
        const BaseMemList* m_list = nullptr;
        IntType m_index = 0;
        Iterator(const BaseMemList* list, IntType index) : m_list(list), m_index(index) {}

    public:
        using difference_type = std::ptrdiff_t;
        using value_type = T;
        using pointer = T*;
        using reference = T&;
        using iterator_category = std::bidirectional_iterator_tag;

        Iterator() = default;
        const T& operator*() const { return m_list->index_2_value(m_index - 1); }
        T& operator*() { return const_cast<BaseMemList*>(m_list)->index_2_value(m_index - 1); }
        const T* operator->() const { return &(**this); }
        T* operator->() { return &(**this); }
        bool operator==(const Iterator& r) const { return m_list == r.m_list && m_index == r.m_index; }
        bool operator!=(const Iterator& r) const { return !(*this == r); }
        Iterator& operator++() { m_index = m_list->m_link[m_index].next; return *this; }
        Iterator operator++(int) { Iterator t = *this; ++(*this); return t; }
        Iterator& operator--() { m_index = m_list->m_link[m_index].prev; return *this; }
        Iterator operator--(int) { Iterator t = *this; --(*this); return t; }
        [[nodiscard]] IntType to_int() const { return m_index; }
    };

    void clear()
    {
        destruct_all();
        m_used = 0;
        m_link[0] = {};
        m_free_index = 0;
        m_raw_used = 0;
    }

    [[nodiscard]] bool empty() const { return m_used == 0; }
    [[nodiscard]] bool full() const { return m_used >= MAX_SIZE; }
    [[nodiscard]] size_t size() const { return m_used; }
    [[nodiscard]] size_t capacity() const { return MAX_SIZE; }

    Iterator push_front(const T& value)
    {
        if (full()) return end();
        IntType empty_index = alloc_index();
        copy_one(empty_index, value);
        auto& head = m_link[0];
        m_link[head.next].prev = empty_index;
        m_link[empty_index].prev = 0;
        m_link[empty_index].next = head.next;
        head.next = empty_index;
        ++m_used;
        return Iterator(this, empty_index);
    }

    Iterator push_back(const T& value)
    {
        if (full()) return end();
        IntType empty_index = alloc_index();
        copy_one(empty_index, value);
        auto& head = m_link[0];
        m_link[head.prev].next = empty_index;
        m_link[empty_index].prev = head.prev;
        m_link[empty_index].next = 0;
        head.prev = empty_index;
        ++m_used;
        return Iterator(this, empty_index);
    }

    void pop_front()
    {
        if (empty()) return;
        auto& head = m_link[0];
        IntType del_index = head.next;
        auto& del_link = m_link[del_index];
        head.next = del_link.next;
        m_link[del_link.next].prev = 0;
        del_link.next = m_free_index;
        m_free_index = del_index;
        destruct_one(del_index);
        assert(m_used > 0);
        --m_used;
    }

    void pop_back()
    {
        if (empty()) return;
        auto& head = m_link[0];
        IntType del_index = head.prev;
        auto& del_link = m_link[del_index];
        head.prev = del_link.prev;
        m_link[del_link.prev].next = 0;
        del_link.next = m_free_index;
        m_free_index = del_index;
        destruct_one(del_index);
        assert(m_used > 0);
        --m_used;
    }

    void erase(const Iterator& it)
    {
        assert(it.m_list == this && m_used > 0);
        destruct_one(it.m_index);
        auto& del_link = m_link[it.m_index];
        m_link[del_link.prev].next = del_link.next;
        m_link[del_link.next].prev = del_link.prev;
        del_link.next = m_free_index;
        m_free_index = it.m_index;
        --m_used;
    }

    void erase(const T& value) { auto it = find(value); if (it != end()) erase(it); }
    void erase(IntType pos) { erase(Iterator(this, pos)); }

    [[nodiscard]] const Iterator find(const T& value) const { return Iterator(this, find_pos(value)); }
    [[nodiscard]] Iterator find(const T& value) { return Iterator(this, find_pos(value)); }

    template <typename Predicate>
    [[nodiscard]] const Iterator find_if(const Predicate& p) const { return Iterator(this, find_pos_if(p)); }
    template <typename Predicate>
    [[nodiscard]] Iterator find_if(const Predicate& p) { return Iterator(this, find_pos_if(p)); }

    [[nodiscard]] IntType find_pos(const T& value) const
    {
        for (IntType i = m_link[0].next; i != 0; i = m_link[i].next)
            if (index_2_value(i - 1) == value) return i;
        return 0;
    }

    template <typename Predicate>
    [[nodiscard]] IntType find_pos_if(const Predicate& p) const
    {
        for (IntType i = m_link[0].next; i != 0; i = m_link[i].next)
            if (p(index_2_value(i - 1))) return i;
        return 0;
    }

    [[nodiscard]] const T& get(IntType pos) const { return index_2_value(pos - 1); }

    [[nodiscard]] const Iterator begin() const { return Iterator(this, m_link[0].next); }
    [[nodiscard]] const Iterator end() const { return Iterator(this, 0); }
    [[nodiscard]] Iterator begin() { return Iterator(this, m_link[0].next); }
    [[nodiscard]] Iterator end() { return Iterator(this, 0); }

private:
    using RealValueType = std::conditional_t<std::is_trivially_copyable_v<T>, T, char>;
    using LinkNode = Link<IntType>;

    IntType m_used = 0;
    IntType m_free_index = 0;
    IntType m_raw_used = 0;
    LinkNode m_link[MAX_SIZE + 1] = {};
    RealValueType m_value[MAX_SIZE * sizeof(T) / sizeof(RealValueType)];

    T& index_2_value(IntType index) { return reinterpret_cast<T&>(m_value[index * sizeof(T) / sizeof(RealValueType)]); }
    const T& index_2_value(IntType index) const { return reinterpret_cast<const T&>(m_value[index * sizeof(T) / sizeof(RealValueType)]); }

    IntType alloc_index()
    {
        IntType empty_index = 0;
        if (m_free_index == 0)
        {
            assert(m_raw_used < MAX_SIZE);
            ++m_raw_used;
            empty_index = m_raw_used;
        }
        else
        {
            empty_index = m_free_index;
            m_free_index = m_link[m_free_index].next;
        }
        assert(empty_index > 0);
        return empty_index;
    }

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

// MAX_SIZE==0 时不支持 BaseMemList
template <typename T>
struct BaseMemList<T, 0>;

}  // namespace ua
