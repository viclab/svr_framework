/// @file hash_mem_pool.h
/// @brief 带哈希桶的定长内存池（C++20 重写版）
/// @note 改进: [[nodiscard]] + 内联实现
#pragma once

#include <cstring>
#include <functional>
#include <utility>
#include "fixed_mem_pool.h"

namespace ua
{

template <typename KEY, typename VALUE, size_t EXTEND_SIZE = sizeof(VALUE), typename HASH = std::hash<KEY>,
          template <typename T, size_t BLOCK_ALIGN = alignof(size_t)> typename POOL = FixedMemPool>
class HashMemPool
{
    static_assert(sizeof(VALUE) <= EXTEND_SIZE, "sizeof(VALUE) > EXTEND_SIZE");
    static_assert(std::is_trivial_v<KEY>, "KEY 必须是 trivial 类型");

    struct InnerNode
    {
        KEY first;
        uint8_t extend[EXTEND_SIZE];
    };

public:
    struct Node
    {
        KEY first;
        VALUE second;
    };
    struct HashNode : public InnerNode
    {
        size_t next;
    };

    struct Iterator
    {
        Iterator() = default;
        const Node& operator*() const { return reinterpret_cast<const Node&>(*m_iter); }
        Node& operator*() { return reinterpret_cast<Node&>(*m_iter); }
        const Node* operator->() const { return &(**this); }
        Node* operator->() { return &(**this); }
        bool operator==(const Iterator& r) const { return m_iter == r.m_iter; }
        bool operator!=(const Iterator& r) const { return m_iter != r.m_iter; }
        Iterator& operator++() { ++m_iter; return *this; }
        Iterator operator++(int) { Iterator t = *this; ++(*this); return t; }
        Iterator& operator--() { --m_iter; return *this; }
        Iterator operator--(int) { Iterator t = *this; --(*this); return t; }

    private:
        friend class HashMemPool;
        typename POOL<InnerNode>::Iterator m_iter;
        Iterator(const typename POOL<InnerNode>::Iterator& iter) : m_iter(iter) {}
    };

    static size_t calc_mem_size(uint32_t max_node, uint32_t bucket_num)
    {
        return sizeof(HashHeader) + sizeof(size_t) * bucket_num +
               InnerMemPool::calc_need_size(max_node, sizeof(HashNode));
    }

    [[nodiscard]] size_t size() const { return m_pool.size(); }
    [[nodiscard]] size_t capacity() const { return m_pool.capacity(); }
    [[nodiscard]] bool empty() const { return m_pool.empty(); }
    [[nodiscard]] bool full() const { return m_pool.full(); }
    [[nodiscard]] size_t value_offset() const { return m_header ? sizeof(HashHeader) + sizeof(size_t) * m_header->bucket_num + m_pool.value_offset() : 0; }
    [[nodiscard]] size_t mem_size() const { return m_header ? sizeof(HashHeader) + sizeof(size_t) * m_header->bucket_num + m_pool.mem_size() : 0; }
    [[nodiscard]] void* mem_head() const { return reinterpret_cast<void*>(m_header); }

    bool init(void* mem, uint32_t max_node, uint32_t bucket_num, uint32_t mem_size, bool check = false)
    {
        if (!mem || calc_mem_size(max_node, bucket_num) < mem_size)
            return false;

        auto* p = reinterpret_cast<char*>(mem);
        m_header = reinterpret_cast<HashHeader*>(p);
        p += sizeof(HashHeader);
        m_buckets = reinterpret_cast<size_t*>(p);
        p += sizeof(size_t) * bucket_num;

        if (check)
        {
            if (m_header->bucket_num != bucket_num || m_header->max_node != max_node)
                return false;
        }
        else
        {
            memset(m_buckets, 0, sizeof(size_t) * bucket_num);
            m_header->bucket_num = bucket_num;
            m_header->max_node = max_node;
        }

        size_t pool_size = InnerMemPool::calc_need_size(max_node, sizeof(HashNode));
        return m_pool.init(p, pool_size, max_node, sizeof(HashNode), check);
    }

    void clear()
    {
        if (m_header)
            init(m_header, m_header->max_node, m_header->bucket_num,
                 static_cast<uint32_t>(calc_mem_size(m_header->max_node, m_header->bucket_num)), false);
    }

    std::pair<Iterator, bool> insert(const KEY& key)
    {
        if (full()) return {end(), false};
        size_t node_ref = find_ref(key);
        if (node_ref != 0) return {Iterator(m_pool.int_2_iter(node_ref)), false};

        auto* node = static_cast<HashNode*>(m_pool.alloc());
        if (!node) return {end(), false};

        node->first = key;
        size_t index = bucket_index(key);
        node->next = m_buckets[index];
        m_buckets[index] = m_pool.ptr_2_int(node);
        return {Iterator(m_pool.int_2_iter(m_buckets[index])), true};
    }

    std::pair<Iterator, bool> insert(const KEY& key, const VALUE& value)
    {
        auto result = insert(key);
        if (result.second)
            memcpy(&(result.first->second), &value, sizeof(VALUE));
        return result;
    }

    [[nodiscard]] const Iterator find(const KEY& key) const { return Iterator(m_pool.int_2_iter(find_ref(key))); }
    [[nodiscard]] Iterator find(const KEY& key) { return Iterator(m_pool.int_2_iter(find_ref(key))); }

    Iterator get_or_insert(const KEY& key)
    {
        size_t ref = find_ref(key);
        if (ref != 0) return Iterator(m_pool.int_2_iter(ref));
        return insert(key).first;
    }

    bool erase(const KEY& key)
    {
        size_t index = bucket_index(key);
        size_t* pre = &(m_buckets[index]);
        for (size_t ref = m_buckets[index]; ref != 0;)
        {
            auto* node = static_cast<HashNode*>(m_pool.int_2_ptr(ref));
            if (node->first == key)
            {
                *pre = node->next;
                m_pool.free(node);
                return true;
            }
            pre = &(node->next);
            ref = node->next;
        }
        return false;
    }

    bool erase(const Iterator& it) { return erase(it->first); }

    [[nodiscard]] size_t ref(const Node* node) const { return m_pool.ptr_2_int(reinterpret_cast<const InnerNode*>(node)); }
    [[nodiscard]] const Node* deref(size_t pos) const { return reinterpret_cast<const Node*>(m_pool.int_2_ptr(pos)); }
    [[nodiscard]] Node* deref(size_t pos) { return reinterpret_cast<Node*>(m_pool.int_2_ptr(pos)); }

    [[nodiscard]] const Iterator begin() const { return Iterator(m_pool.begin()); }
    [[nodiscard]] Iterator begin() { return Iterator(m_pool.begin()); }
    [[nodiscard]] const Iterator end() const { return Iterator(m_pool.end()); }
    [[nodiscard]] Iterator end() { return Iterator(m_pool.end()); }

    [[nodiscard]] size_t bucket_index(const KEY& key) const { return HASH{}(key) % m_header->bucket_num; }
    [[nodiscard]] size_t find_ref(const KEY& key) const
    {
        size_t index = bucket_index(key);
        for (size_t ref = m_buckets[index]; ref != 0;)
        {
            auto* node = static_cast<const HashNode*>(m_pool.int_2_ptr(ref));
            if (node->first == key) return ref;
            ref = node->next;
        }
        return 0;
    }

private:
    struct HashHeader
    {
        size_t bucket_num;
        size_t max_node;
    };

    HashHeader* m_header = nullptr;
    size_t* m_buckets = nullptr;
    using InnerMemPool = POOL<InnerNode>;
    InnerMemPool m_pool;
};

}  // namespace ua
