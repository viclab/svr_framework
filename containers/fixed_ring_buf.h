/// @file fixed_ring_buf.h
/// @brief 定长环形缓冲区（C++20 重写版）
/// @note 改进: [[nodiscard]] 标记查询方法
#pragma once

#include <cassert>
#include "inner/ring_buf_data.h"

namespace ua
{

template <typename T, size_t MAX_SIZE = 0>
class FixedRingBuf : public FixedRingBufData<T, MAX_SIZE>
{
    using Data = FixedRingBufData<T, MAX_SIZE>;
    using IntType = typename Data::IntType;

public:
    void clear()
    {
        Data::set_start(0);
        Data::set_end(0);
        Data::set_used_num(0);
    }

    [[nodiscard]] bool empty() const { return Data::get_used_num() == 0 && Data::get_max_num() > 0; }
    [[nodiscard]] bool full() const { return Data::get_used_num() == Data::get_max_num(); }
    [[nodiscard]] size_t size() const { return Data::get_used_num(); }
    [[nodiscard]] size_t capacity() const { return Data::get_max_num(); }

    bool push(const T& value, bool over_write = false)
    {
        if (full())
        {
            if (over_write) pop();
            else return false;
        }
        Data::m_buf[Data::get_end()] = value;
        assert(Data::get_max_num() > 0);
        Data::set_end((Data::get_end() + 1) % Data::get_max_num());
        Data::set_used_num(Data::get_used_num() + 1);
        return true;
    }

    void pop()
    {
        if (!empty())
        {
            assert(Data::get_max_num() > 0);
            Data::set_start((Data::get_start() + 1) % Data::get_max_num());
            Data::set_used_num(Data::get_used_num() - 1);
        }
    }

    [[nodiscard]] T& front(size_t index = 0)
    {
        assert(index < Data::get_used_num() && Data::get_max_num() > 0);
        return Data::m_buf[(Data::get_start() + index) % Data::get_max_num()];
    }

    [[nodiscard]] const T& front(size_t index = 0) const
    {
        assert(index < Data::get_used_num() && Data::get_max_num() > 0);
        return Data::m_buf[(Data::get_start() + index) % Data::get_max_num()];
    }

    [[nodiscard]] T& back(size_t index = 0)
    {
        assert(index < Data::get_used_num() && Data::get_max_num() > 0);
        return Data::m_buf[(Data::get_end() + Data::get_max_num() - 1 - index) % Data::get_max_num()];
    }

    [[nodiscard]] const T& back(size_t index = 0) const
    {
        assert(index < Data::get_used_num() && Data::get_max_num() > 0);
        return Data::m_buf[(Data::get_end() + Data::get_max_num() - 1 - index) % Data::get_max_num()];
    }
};

}  // namespace ua
