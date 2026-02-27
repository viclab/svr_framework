/// @file mem_map.h
/// @brief 内存哈希键值对 Map（C++20 重写版）
/// @note 改进: using 声明 + 额外的 insert(key, value) 重载
#pragma once

#include "inner/base_mem_set.h"
#include "inner/base_specialization.h"
#include "inner/base_struct.h"
#include "inner/is_trivial_decorator.h"

namespace ua
{

template <typename KEY, typename VALUE, size_t MAX_SIZE = 0>
class MemMap : private inner::IsTrivialDecorator<BaseMemSet, Pair<KEY, VALUE>, MAX_SIZE,
                                                 std::hash<Pair<KEY, VALUE>>, IsEqual<Pair<KEY, VALUE>>>
{
public:
    using T = Pair<KEY, VALUE>;
    using BaseType = BaseMemSet<T, MAX_SIZE, std::hash<T>, IsEqual<T>>;
    using IntType = typename BaseType::IntType;
    using Iterator = typename BaseType::Iterator;

    using BaseType::init;
    using BaseType::need_mem_size;
    using BaseType::clear;
    using BaseType::empty;
    using BaseType::full;
    using BaseType::size;
    using BaseType::capacity;
    using BaseType::begin;
    using BaseType::end;

    std::pair<Iterator, bool> insert(const KEY& key, const VALUE& value)
    {
        return BaseType::insert(T{key, value});
    }

    [[nodiscard]] const Iterator find(const KEY& key) const { return BaseType::find(key); }
    [[nodiscard]] Iterator find(const KEY& key) { return BaseType::find(key); }
    [[nodiscard]] bool exist(const KEY& key) const { return BaseType::exist(key); }
    void erase(const Iterator& it) { BaseType::erase(it); }
    void erase(const KEY& key) { BaseType::erase(key); }
};

}  // namespace ua
