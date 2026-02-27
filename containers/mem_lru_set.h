/// @file mem_lru_set.h
/// @brief LRU 集合（C++20 重写版）
#pragma once

#include "inner/base_mem_lru_set.h"
#include "inner/is_trivial_decorator.h"

namespace ua
{

template <typename T, size_t MAX_SIZE = 0, typename HASH = std::hash<T>, typename IS_EQUAL = IsEqual<T>>
class MemLRUSet : private inner::IsTrivialDecorator<BaseMemLRUSet, T, MAX_SIZE, HASH, IS_EQUAL>
{
public:
    using BaseType = BaseMemLRUSet<T, MAX_SIZE, HASH, IS_EQUAL>;
    using IntType = typename BaseType::IntType;
    using ValueType = typename BaseType::ValueType;
    using Iterator = typename BaseType::Iterator;
    using DisuseCallback = typename BaseType::DisuseCallback;

    using BaseType::init;
    using BaseType::need_mem_size;
    using BaseType::clear;
    using BaseType::empty;
    using BaseType::full;
    using BaseType::size;
    using BaseType::capacity;
    using BaseType::insert;
    using BaseType::find;
    using BaseType::exist;
    using BaseType::erase;
    using BaseType::active;
    using BaseType::disuse;
    using BaseType::begin;
    using BaseType::end;
};

}  // namespace ua
