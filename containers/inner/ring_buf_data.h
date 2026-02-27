/// @file ring_buf_data.h
/// @brief 环形缓冲区数据结构（C++20 重写版）
/// @note 改进: 用 requires 替代 enable_if；用 if constexpr 简化 MAX_SIZE==0 的分支
#pragma once

#include <sys/uio.h>
#include <algorithm>
#include <concepts>
#include <type_traits>
#include "traits_utils.h"

namespace ua
{

// ==================== 不定长环形缓冲区数据 ====================

template <size_t MAX_SIZE = 0>
struct UnfixedRingBufData
{
protected:
    using IntType = typename FixIntType<MAX_SIZE>::IntType;
    IntType m_start = 0;
    IntType m_end = 0;
    IntType m_used_size = 0;
    IntType m_item_num = 0;
    uint8_t m_buf[MAX_SIZE] = {};

    [[nodiscard]] constexpr IntType get_start() const { return m_start; }
    [[nodiscard]] constexpr IntType get_end() const { return m_end; }
    [[nodiscard]] constexpr IntType get_used_size() const { return m_used_size; }
    [[nodiscard]] constexpr IntType get_item_num() const { return m_item_num; }
    [[nodiscard]] constexpr IntType get_size() const { return MAX_SIZE; }

    void set_start(IntType v) { m_start = v; }
    void set_end(IntType v) { m_end = v; }
    void set_used_size(IntType v) { m_used_size = v; }
    void set_item_num(IntType v) { m_item_num = v; }
};

template <>
struct UnfixedRingBufData<0>
{
protected:
    using IntType = size_t;
    struct BuffHead
    {
        IntType m_start = 0;
        IntType m_end = 0;
        IntType m_used_size = 0;
        IntType m_item_num = 0;
        IntType m_size = 0;
    };
    BuffHead* m_head = nullptr;
    uint8_t* m_buf = nullptr;

    [[nodiscard]] constexpr IntType get_start() const { return m_head->m_start; }
    [[nodiscard]] constexpr IntType get_end() const { return m_head->m_end; }
    [[nodiscard]] constexpr IntType get_used_size() const { return m_head->m_used_size; }
    [[nodiscard]] constexpr IntType get_item_num() const { return m_head->m_item_num; }
    [[nodiscard]] constexpr IntType get_size() const { return m_head->m_size; }

    void set_start(IntType v) { m_head->m_start = v; }
    void set_end(IntType v) { m_head->m_end = v; }
    void set_used_size(IntType v) { m_head->m_used_size = v; }
    void set_item_num(IntType v) { m_head->m_item_num = v; }

public:
    [[nodiscard]] bool is_init() const { return m_buf != nullptr; }

    static size_t need_total_mem_size(size_t mem_size) { return sizeof(BuffHead) + mem_size; }

    bool init(void* mem, size_t mem_size, bool check = false)
    {
        if (!mem || mem_size < sizeof(BuffHead))
            return false;

        auto* tmp_head = reinterpret_cast<BuffHead*>(mem);
        if (check)
        {
            if (tmp_head->m_size != (mem_size - sizeof(BuffHead)) || tmp_head->m_used_size > tmp_head->m_size)
                return false;
        }
        else
        {
            *tmp_head = {};
            tmp_head->m_size = mem_size - sizeof(BuffHead);
        }

        m_head = tmp_head;
        m_buf = reinterpret_cast<uint8_t*>(mem) + sizeof(BuffHead);
        return true;
    }
};

// ==================== 定长环形缓冲区数据 ====================

/// 编译期大小版本（要求 trivially_copyable）
template <typename T, size_t MAX_SIZE = 0>
    requires(MAX_SIZE > 0 && std::is_trivially_copyable_v<T>)
struct FixedRingBufData
{
protected:
    using IntType = typename FixIntType<MAX_SIZE>::IntType;

    IntType m_start = 0;
    IntType m_end = 0;
    IntType m_used_num = 0;
    T m_buf[MAX_SIZE];

    [[nodiscard]] constexpr IntType get_start() const { return m_start; }
    [[nodiscard]] constexpr IntType get_end() const { return m_end; }
    [[nodiscard]] constexpr IntType get_used_num() const { return m_used_num; }
    [[nodiscard]] constexpr IntType get_max_num() const { return MAX_SIZE; }

    void set_start(IntType v) { m_start = v; }
    void set_end(IntType v) { m_end = v; }
    void set_used_num(IntType v) { m_used_num = v; }
};

/// 运行时大小版本（共享内存）
template <typename T>
    requires std::is_trivially_copyable_v<T>
struct FixedRingBufData<T, 0>
{
protected:
    using IntType = size_t;
    struct BuffHead
    {
        IntType m_start = 0;
        IntType m_end = 0;
        IntType m_used_num = 0;
        IntType m_max_num = 0;
    };
    BuffHead* m_head = nullptr;
    T* m_buf = nullptr;

    [[nodiscard]] constexpr IntType get_start() const { return m_head->m_start; }
    [[nodiscard]] constexpr IntType get_end() const { return m_head->m_end; }
    [[nodiscard]] constexpr IntType get_used_num() const { return m_head->m_used_num; }
    [[nodiscard]] constexpr IntType get_max_num() const { return m_head->m_max_num; }

    void set_start(IntType v) { m_head->m_start = v; }
    void set_end(IntType v) { m_head->m_end = v; }
    void set_used_num(IntType v) { m_head->m_used_num = v; }

public:
    static constexpr size_t mem_size(size_t size) { return sizeof(BuffHead) + sizeof(T) * size; }

    bool init(void* mem, size_t mem_size, bool check = false)
    {
        if (!mem || mem_size < sizeof(BuffHead))
            return false;

        m_head = reinterpret_cast<BuffHead*>(mem);
        if (check)
        {
            if (m_head->m_max_num != (mem_size - sizeof(BuffHead)) / sizeof(T))
                return false;
        }
        else
        {
            *m_head = {};
            m_head->m_max_num = (mem_size - sizeof(BuffHead)) / sizeof(T);
        }
        m_buf = reinterpret_cast<T*>(reinterpret_cast<uint8_t*>(mem) + sizeof(BuffHead));
        return true;
    }
};

}  // namespace ua
