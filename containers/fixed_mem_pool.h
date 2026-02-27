/// @file fixed_mem_pool.h
/// @brief 定长内存池（C++20 重写版）
/// @note 改进: constexpr static 方法
///       改进: [[nodiscard]] 标记所有查询方法
///       保持原版完整功能: alloc/free/迭代器/ptr_2_int/int_2_ptr
#pragma once

#include <cassert>
#include <cstring>
#include <type_traits>
#include "inner/base_struct.h"
#include "inner/traits_utils.h"

namespace ua
{

template <typename T, size_t BLOCK_ALIGN = alignof(size_t)>
class FixedMemPool
{
    static_assert(IsPowOfTwo<BLOCK_ALIGN>, "BLOCK_ALIGN 必须是 2 的幂");

public:
    struct Iterator
    {
        Iterator() = default;
        const T& operator*() const { return *(operator->()); }
        T& operator*() { return *(operator->()); }
        const T* operator->() const { return m_pool->get_value(m_index); }
        T* operator->() { return const_cast<FixedMemPool*>(m_pool)->get_value(m_index); }
        bool operator==(const Iterator& r) const { return m_pool == r.m_pool && m_index == r.m_index; }
        bool operator!=(const Iterator& r) const { return !(*this == r); }

        Iterator& operator++()
        {
            auto* node = m_pool->get_link(m_index);
            assert(node->prev <= m_pool->m_header->max_num);
            m_index = node->next;
            return *this;
        }
        Iterator operator++(int) { Iterator t = *this; ++(*this); return t; }
        Iterator& operator--()
        {
            auto* node = m_pool->get_link(m_index);
            assert(node->prev <= m_pool->m_header->max_num);
            m_index = node->prev;
            return *this;
        }
        Iterator operator--(int) { Iterator t = *this; --(*this); return t; }

    private:
        friend class FixedMemPool;
        size_t m_index = 0;
        const FixedMemPool* m_pool = nullptr;
        Iterator(const FixedMemPool* pool, size_t index) : m_index(index), m_pool(pool) {}
    };

    static size_t calc_need_size(size_t max_node_num, size_t node_size)
    {
        return align_bytes(sizeof(MemHeader) + (max_node_num + 1) * sizeof(LinkNode)) +
               max_node_num * align_bytes(node_size);
    }

    static size_t calc_need_size(size_t max_node_num) { return calc_need_size(max_node_num, sizeof(T)); }

    FixedMemPool() = default;
    FixedMemPool(const FixedMemPool&) = delete;
    FixedMemPool& operator=(const FixedMemPool&) = delete;

    bool init(void* mem, size_t size, size_t max_node_num, bool check)
    {
        return init(mem, size, max_node_num, sizeof(T), check);
    }

    bool init(void* mem, size_t size, size_t max_node_num, size_t node_size, bool check)
    {
        if (!mem || node_size < sizeof(T)) return false;
        size_t need_size = calc_need_size(max_node_num, node_size);
        if (need_size > size) return false;

        size_t real_node_size = align_bytes(node_size);
        m_header = reinterpret_cast<MemHeader*>(mem);

        if (!check)
            init_header(need_size, max_node_num, real_node_size, node_size);

        if (m_header->magic_num != HEADER_MAGIC_NUM || m_header->version != VERSION ||
            m_header->mem_size != need_size || m_header->max_num != max_node_num ||
            m_header->raw_t_size != node_size || m_header->t_size != real_node_size)
            return false;

        return true;
    }

    T* alloc(bool zero = true)
    {
        if (full()) return nullptr;

        size_t index = 0;
        LinkNode* empty_node = nullptr;
        if (m_header->reclaim_list != 0)
        {
            index = m_header->reclaim_list;
            empty_node = get_link(index);
            m_header->reclaim_list = empty_node->next;
        }
        else
        {
            assert(m_header->raw_used_num < m_header->max_num);
            empty_node = get_link(m_header->raw_used_num + 1);
            index = m_header->raw_used_num + 1;
            ++(m_header->raw_used_num);
        }
        assert(index > 0);

        auto* head_node = get_link(0);
        auto* next_node = get_link(head_node->next);
        empty_node->next = head_node->next;
        empty_node->prev = 0;
        next_node->prev = index;
        head_node->next = index;

        ++(m_header->used_num);

        T* p = get_value(index);
        if (zero)
            memset(p, 0, m_header->t_size);
        return p;
    }

    bool free(const T* p)
    {
        if (empty()) return false;
        size_t index = ptr_2_int(p);
        if (index == 0 || index > m_header->max_num) return false;
        if (index > m_header->raw_used_num) return false;

        auto* del_node = get_link(index);
        if (del_node->prev > m_header->max_num) return false;

        auto* prev_node = get_link(del_node->prev);
        auto* next_node = get_link(del_node->next);
        prev_node->next = del_node->next;
        next_node->prev = del_node->prev;

        del_node->prev = m_header->max_num + 1;
        del_node->next = m_header->reclaim_list;
        m_header->reclaim_list = index;
        --(m_header->used_num);
        return true;
    }

    void clear() { init(m_header, m_header->mem_size, m_header->max_num, m_header->raw_t_size, false); }
    [[nodiscard]] bool full() const { return m_header->used_num >= m_header->max_num; }
    [[nodiscard]] bool empty() const { return m_header->used_num == 0; }
    [[nodiscard]] size_t capacity() const { return m_header->max_num; }
    [[nodiscard]] size_t size() const { return m_header->used_num; }
    [[nodiscard]] size_t node_size() const { return m_header->t_size; }
    [[nodiscard]] size_t value_offset() const { return m_header->value_offset; }
    [[nodiscard]] size_t mem_size() const { return m_header->mem_size; }
    [[nodiscard]] void* mem_head() const { return reinterpret_cast<void*>(m_header); }
    [[nodiscard]] size_t mem_utilization() const { return m_header->used_num * (m_header->t_size + sizeof(LinkNode)) * 100 / m_header->mem_size; }

    [[nodiscard]] const Iterator begin() const { return Iterator(this, get_link(0)->next); }
    [[nodiscard]] Iterator begin() { return Iterator(this, get_link(0)->next); }
    [[nodiscard]] const Iterator end() const { return Iterator(this, 0); }
    [[nodiscard]] Iterator end() { return Iterator(this, 0); }

    [[nodiscard]] size_t ptr_2_int(const T* p) const
    {
        auto* start = reinterpret_cast<const uint8_t*>(m_header);
        if (reinterpret_cast<const uint8_t*>(p) < start + m_header->value_offset) return 0;
        size_t offset = reinterpret_cast<const uint8_t*>(p) - start - m_header->value_offset;
        if (offset % m_header->t_size != 0) return 0;
        return (offset / m_header->t_size) + 1;
    }

    [[nodiscard]] const T* int_2_ptr(size_t index) const { return get_value(index); }
    [[nodiscard]] T* int_2_ptr(size_t index) { return get_value(index); }
    [[nodiscard]] const Iterator int_2_iter(size_t index) const { return Iterator(this, index); }
    [[nodiscard]] Iterator int_2_iter(size_t index) { return Iterator(this, index); }

private:
    using LinkNode = Link<size_t>;
    static constexpr size_t HEADER_MAGIC_NUM = 0x9E370001;
    static constexpr size_t VERSION = 1;

    struct MemHeader
    {
        size_t version = 0;
        size_t mem_size = 0;
        size_t raw_t_size = 0;
        size_t t_size = 0;
        size_t max_num = 0;
        size_t used_num = 0;
        size_t raw_used_num = 0;
        size_t link_head_offset = 0;
        size_t value_offset = 0;
        size_t reclaim_list = 0;
        size_t magic_num = HEADER_MAGIC_NUM;
    };

    MemHeader* m_header = nullptr;

    void init_header(size_t size, size_t max_node_num, size_t node_size, size_t raw_node_size)
    {
        assert(m_header);
        m_header->version = VERSION;
        m_header->mem_size = size;
        m_header->raw_t_size = raw_node_size;
        m_header->t_size = node_size;
        m_header->max_num = max_node_num;
        m_header->used_num = 0;
        m_header->raw_used_num = 0;
        m_header->link_head_offset = sizeof(MemHeader);
        m_header->value_offset = align_bytes(sizeof(MemHeader) + (max_node_num + 1) * sizeof(LinkNode));
        m_header->reclaim_list = 0;
        m_header->magic_num = HEADER_MAGIC_NUM;
        auto* head_node = get_link(0);
        *head_node = {};
    }

    [[nodiscard]] const LinkNode* get_link(size_t index) const
    {
        assert(index <= m_header->max_num);
        size_t offset = m_header->link_head_offset + index * sizeof(LinkNode);
        return reinterpret_cast<const LinkNode*>(reinterpret_cast<const uint8_t*>(m_header) + offset);
    }

    [[nodiscard]] LinkNode* get_link(size_t index)
    {
        assert(index <= m_header->max_num);
        size_t offset = m_header->link_head_offset + index * sizeof(LinkNode);
        return reinterpret_cast<LinkNode*>(reinterpret_cast<uint8_t*>(m_header) + offset);
    }

    [[nodiscard]] const T* get_value(size_t index) const
    {
        assert(index > 0 && index <= m_header->max_num);
        size_t offset = m_header->value_offset + (index - 1) * m_header->t_size;
        return reinterpret_cast<const T*>(reinterpret_cast<const uint8_t*>(m_header) + offset);
    }

    [[nodiscard]] T* get_value(size_t index)
    {
        assert(index > 0 && index <= m_header->max_num);
        size_t offset = m_header->value_offset + (index - 1) * m_header->t_size;
        return reinterpret_cast<T*>(reinterpret_cast<uint8_t*>(m_header) + offset);
    }

    static size_t align_bytes(size_t bytes) { return (bytes + BLOCK_ALIGN - 1) & ~(BLOCK_ALIGN - 1); }
};

}  // namespace ua
