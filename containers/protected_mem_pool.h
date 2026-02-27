/// @file protected_mem_pool.h
/// @brief 越界保护内存池（C++20 重写版）
/// @note 在每个节点后面加一页 mprotect(PROT_NONE) 的内存
#pragma once

#include <sys/mman.h>
#include <cassert>
#include <cstring>
#include <vector>
#include "fixed_mem_pool.h"

namespace ua
{

static constexpr size_t FENCE_SIZE = 4096;

template <typename T>
class ProtectedMemPool : public FixedMemPool<T, FENCE_SIZE>
{
    using BasePool = FixedMemPool<T, FENCE_SIZE>;

public:
    bool init(void* mem, size_t size, size_t max_node_num, bool check)
    {
        return init(mem, size, max_node_num, sizeof(T), check);
    }

    bool init(void* mem, size_t size, size_t max_node_num, size_t node_size, bool check)
    {
        if (!BasePool::init(mem, size, max_node_num, node_size + FENCE_SIZE, check))
            return false;

        if (!check)
        {
            // 为每个节点设置内存屏障
            for (auto* t = BasePool::alloc(false); t; t = BasePool::alloc(false))
            {
                auto* protect_addr = reinterpret_cast<uint8_t*>(t) + this->node_size_real();
                if (mprotect(protect_addr, FENCE_SIZE, PROT_NONE) != 0)
                    return false;
            }
            BasePool::clear();
        }
        else
        {
            size_t saved_size = BasePool::size();

            std::vector<T*> temp_dels;
            for (auto* t = BasePool::alloc(false); t; t = BasePool::alloc(false))
                temp_dels.push_back(t);

            for (auto beg = BasePool::begin(); beg != BasePool::end(); ++beg)
            {
                auto* protect_addr = reinterpret_cast<uint8_t*>(&(*beg)) + this->node_size_real();
                if (mprotect(protect_addr, FENCE_SIZE, PROT_NONE) != 0)
                    return false;
            }

            for (auto* t : temp_dels)
                BasePool::free(t);

            assert(saved_size == BasePool::size());
        }
        return true;
    }

    T* alloc(bool zero = true)
    {
        auto* t = BasePool::alloc(false);
        if (t && zero)
            memset(t, 0, node_size_real());
        return t;
    }

    [[nodiscard]] size_t node_size_real() const { return BasePool::node_size() - FENCE_SIZE; }

    static size_t calc_need_size(size_t max_node_num, size_t node_size)
    {
        return BasePool::calc_need_size(max_node_num, node_size + FENCE_SIZE);
    }

    static size_t calc_need_size(size_t max_node_num)
    {
        return BasePool::calc_need_size(max_node_num, sizeof(T) + FENCE_SIZE);
    }
};

}  // namespace ua
