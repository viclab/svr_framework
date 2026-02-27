/// @file mem_lru_map.h
/// @brief LRU 键值对 Map（C++20 重写版）
#pragma once

#include "inner/base_mem_lru_set.h"
#include "inner/base_specialization.h"
#include "inner/base_struct.h"
#include "inner/is_trivial_decorator.h"

namespace ua
{

template <typename KEY, typename VALUE, size_t MAX_SIZE = 0>
class MemLRUMap : private inner::IsTrivialDecorator<BaseMemLRUSet, Pair<KEY, VALUE>, MAX_SIZE,
                                                    std::hash<Pair<KEY, VALUE>>, IsEqual<Pair<KEY, VALUE>>>
{
public:
    using NodeType = Pair<KEY, VALUE>;
    using BaseType = BaseMemLRUSet<NodeType, MAX_SIZE, std::hash<NodeType>, IsEqual<NodeType>>;
    using IntType = typename BaseType::IntType;
    using Iterator = typename BaseType::Iterator;
    using DisuseCallback = typename BaseType::DisuseCallback;

    using BaseType::init;
    using BaseType::need_mem_size;
    using BaseType::clear;
    using BaseType::empty;
    using BaseType::full;
    using BaseType::size;
    using BaseType::capacity;
    using BaseType::begin;
    using BaseType::end;
    using BaseType::disuse;

    std::pair<Iterator, bool> insert(const KEY& key, const VALUE& value, bool force = false,
                                     const DisuseCallback& cb = nullptr)
    {
        return BaseType::insert(NodeType{key, value}, force, cb);
    }

    [[nodiscard]] const Iterator find(const KEY& key) const { return BaseType::find(key); }
    [[nodiscard]] Iterator find(const KEY& key) { return BaseType::find(key); }
    [[nodiscard]] bool exist(const KEY& key) const { return BaseType::exist(key); }
    void erase(const Iterator& it) { BaseType::erase(it); }
    void erase(const KEY& key) { BaseType::erase(key); }
    Iterator active(const KEY& key) { return BaseType::active(key); }
};

}  // namespace ua
