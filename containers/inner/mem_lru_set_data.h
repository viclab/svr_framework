/// @file mem_lru_set_data.h
/// @brief LRU 集合数据层（C++20 重写版）
#pragma once

#include <cstring>
#include "base_mem_set.h"
#include "base_struct.h"

namespace ua::inner
{

/// 编译期大小版本
template <typename T, size_t MAX_SIZE, typename HASH, typename IS_EQUAL>
struct LRUSetData
{
protected:
    using BaseType = BaseMemSet<T, MAX_SIZE, HASH, IS_EQUAL>;
    using IntType = typename FixIntType<MAX_SIZE + 1>::IntType;
    using LinkNode = Link<IntType>;

    LinkNode m_active_link[MAX_SIZE + 1] = {};
    BaseType m_base;

    void clear()
    {
        m_base.clear();
        m_active_link[0] = {};
    }

public:
    static constexpr size_t need_mem_size(size_t, size_t) { return 0; }
    bool init(void*, size_t, size_t, size_t, bool) { return false; }
};

/// 运行时大小版本（共享内存）
template <typename T, typename HASH, typename IS_EQUAL>
struct LRUSetData<T, 0, HASH, IS_EQUAL>
{
    static_assert(std::is_trivially_copyable_v<T>, "运行时大小版本要求 trivially_copyable 类型");

protected:
    using BaseType = BaseMemSet<T, 0, HASH, IS_EQUAL>;
    using IntType = size_t;
    using LinkNode = Link<IntType>;

    struct Head
    {
        size_t m_mem_size = 0;
        size_t m_max_num = 0;
    };
    Head* m_head = nullptr;
    LinkNode* m_active_link = nullptr;
    BaseType m_base;

    void clear()
    {
        m_base.clear();
        if (m_active_link)
            m_active_link[0] = {};
    }

public:
    static constexpr size_t need_mem_size(size_t max_num, size_t buckets_num)
    {
        return sizeof(Head) + sizeof(LinkNode) * (max_num + 1) + BaseType::need_mem_size(max_num, buckets_num);
    }

    bool init(void* mem, size_t mem_size, size_t max_num, size_t buckets_num, bool check = false)
    {
        if (!mem || mem_size < sizeof(Head))
            return false;

        auto* tmp_head = reinterpret_cast<Head*>(mem);
        if (check)
        {
            if (tmp_head->m_max_num != max_num || tmp_head->m_mem_size != mem_size)
                return false;
        }
        else
        {
            if (mem_size != need_mem_size(max_num, buckets_num))
                return false;
        }

        auto* base_mem = reinterpret_cast<uint8_t*>(mem) + sizeof(Head) + sizeof(LinkNode) * (max_num + 1);
        size_t base_mem_size = BaseType::need_mem_size(max_num, buckets_num);
        if (!m_base.init(base_mem, base_mem_size, max_num, buckets_num, check))
            return false;

        m_head = tmp_head;
        m_active_link = reinterpret_cast<LinkNode*>(reinterpret_cast<uint8_t*>(mem) + sizeof(Head));
        if (!check)
        {
            m_head->m_max_num = max_num;
            m_head->m_mem_size = mem_size;
            memset(m_active_link, 0, sizeof(LinkNode) * (max_num + 1));
        }
        return true;
    }
};

}  // namespace ua::inner
