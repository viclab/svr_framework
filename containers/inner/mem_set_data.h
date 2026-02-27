/// @file mem_set_data.h
/// @brief 哈希集合数据层（C++20 重写版）
/// @note 改进: constexpr 素数桶计算替代递归 TMP
///       改进: 用 if constexpr 区分 trivially_copyable
#pragma once

#include <cassert>
#include <cstring>
#include "traits_utils.h"

namespace ua::inner
{

/// 编译期固定大小版本
template <typename T, size_t MAX_SIZE, typename HASH>
struct SetData
{
protected:
    using IntType = typename FixIntType<MAX_SIZE>::IntType;
    using ValueType = T;
    using RealValueType = std::conditional_t<std::is_trivially_copyable_v<T>, T, char>;

    static constexpr size_t fix_bucket_size()
    {
        if constexpr ((sizeof(T) > 4 && MAX_SIZE <= 40) || (sizeof(T) <= 4 && MAX_SIZE <= 50))
            return 1;
        else
            return nearby_prime(MAX_SIZE);
    }

    template <typename K, size_t BUCKETS>
    static IntType get_bucket_index_impl(const K& key)
    {
        if constexpr (BUCKETS == 1)
            return 0;
        else
            return HASH{}(key) % BUCKETS;
    }

    IntType m_used = 0;
    IntType m_raw_used = 0;
    IntType m_free_index = 0;
    static constexpr size_t BUCKETS_SIZE = fix_bucket_size();
    IntType m_buckets[BUCKETS_SIZE] = {};
    IntType m_next[MAX_SIZE] = {};
    RealValueType m_value[MAX_SIZE * sizeof(T) / sizeof(RealValueType)];

    template <typename K>
    [[nodiscard]] IntType get_bucket_index(const K& key) const
    {
        return get_bucket_index_impl<K, BUCKETS_SIZE>(key);
    }

    [[nodiscard]] constexpr IntType get_used() const { return m_used; }
    [[nodiscard]] constexpr IntType get_raw_used() const { return m_raw_used; }
    [[nodiscard]] constexpr IntType get_free_index() const { return m_free_index; }
    [[nodiscard]] constexpr IntType get_max_num() const { return MAX_SIZE; }
    [[nodiscard]] constexpr IntType get_buckets_num() const { return BUCKETS_SIZE; }

    void set_used(IntType v) { m_used = v; }
    void incr_used() { ++m_used; }
    void decr_used() { --m_used; }
    void set_raw_used(IntType v) { m_raw_used = v; }
    void incr_raw_used() { ++m_raw_used; }
    void decr_raw_used() { --m_raw_used; }
    void set_free_index(IntType v) { m_free_index = v; }

public:
    static size_t need_mem_size(size_t, size_t) { return 0; }  // 编译期版本不需要
    bool init(void*, size_t, size_t, size_t, bool) { return false; }
    void* mem_head() const { return nullptr; }
    size_t value_offset() const { return 0; }
    size_t mem_size() const { return 0; }
};

/// 运行时大小版本（共享内存）
template <typename T, typename HASH>
struct SetData<T, 0, HASH>
{
    static_assert(std::is_trivially_copyable_v<T>, "运行时大小版本要求 trivially_copyable 类型");

protected:
    using IntType = size_t;
    using ValueType = T;
    using RealValueType = std::conditional_t<std::is_trivially_copyable_v<T>, T, char>;

    struct Head
    {
        IntType m_used = 0;
        IntType m_raw_used = 0;
        IntType m_free_index = 0;
        IntType m_max_num = 0;
        IntType m_buckets_num = 0;
        IntType m_mem_size = 0;
        IntType m_value_offset = 0;
    };

    Head* m_head = nullptr;
    IntType* m_buckets = nullptr;
    IntType* m_next = nullptr;
    RealValueType* m_value = nullptr;

    template <typename K>
    [[nodiscard]] IntType get_bucket_index(const K& key) const
    {
        IntType buckets_num = get_buckets_num();
        assert(buckets_num > 0);
        return HASH{}(key) % buckets_num;
    }

    [[nodiscard]] constexpr IntType get_used() const { return m_head->m_used; }
    [[nodiscard]] constexpr IntType get_raw_used() const { return m_head->m_raw_used; }
    [[nodiscard]] constexpr IntType get_free_index() const { return m_head->m_free_index; }
    [[nodiscard]] constexpr IntType get_max_num() const { return m_head ? m_head->m_max_num : 0; }
    [[nodiscard]] constexpr IntType get_buckets_num() const { return m_head ? m_head->m_buckets_num : 0; }

    void set_used(IntType v) { if (m_head) m_head->m_used = v; }
    void incr_used() { ++(m_head->m_used); }
    void decr_used() { --(m_head->m_used); }
    void set_raw_used(IntType v) { if (m_head) m_head->m_raw_used = v; }
    void incr_raw_used() { ++(m_head->m_raw_used); }
    void decr_raw_used() { --(m_head->m_raw_used); }
    void set_free_index(IntType v) { if (m_head) m_head->m_free_index = v; }

public:
    static size_t need_mem_size(size_t max_num, size_t buckets_num)
    {
        return sizeof(Head) + sizeof(IntType) * buckets_num + sizeof(IntType) * max_num +
               sizeof(RealValueType) * (max_num * sizeof(T) / sizeof(RealValueType));
    }

    bool init(void* mem, size_t mem_size, size_t max_num, size_t buckets_num, bool check = false)
    {
        if (!mem || need_mem_size(max_num, buckets_num) != mem_size)
            return false;

        auto* tmp_head = reinterpret_cast<Head*>(mem);
        if (check)
        {
            if (tmp_head->m_mem_size != mem_size || tmp_head->m_max_num != max_num ||
                tmp_head->m_buckets_num != buckets_num)
                return false;
        }
        else
        {
            memset(mem, 0, mem_size);
            tmp_head->m_max_num = max_num;
            tmp_head->m_buckets_num = buckets_num;
            tmp_head->m_mem_size = mem_size;
            tmp_head->m_value_offset = sizeof(Head) + sizeof(IntType) * buckets_num + sizeof(IntType) * max_num;
        }
        m_head = tmp_head;
        m_buckets = reinterpret_cast<IntType*>(reinterpret_cast<uint8_t*>(mem) + sizeof(Head));
        m_next = reinterpret_cast<IntType*>(reinterpret_cast<uint8_t*>(mem) + sizeof(Head) +
                                            sizeof(IntType) * buckets_num);
        m_value = reinterpret_cast<RealValueType*>(reinterpret_cast<uint8_t*>(mem) + sizeof(Head) +
                                                   sizeof(IntType) * buckets_num + sizeof(IntType) * max_num);
        return true;
    }

    void* mem_head() const { return reinterpret_cast<void*>(m_head); }
    size_t value_offset() const { return m_head->m_value_offset; }
    size_t mem_size() const { return m_head->m_mem_size; }
};

}  // namespace ua::inner
