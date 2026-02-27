/// @file mem_set.h
/// @brief 内存哈希集合（C++20 重写版）
/// @note 改进: 直接用 using 声明暴露基类方法，消除几百行的转发代码
#pragma once

#include <functional>
#include "inner/base_mem_set.h"
#include "inner/is_trivial_decorator.h"

namespace ua
{

template <typename T, size_t MAX_SIZE = 0, typename HASH = std::hash<T>, typename IS_EQUAL = IsEqual<T>>
class MemSet : private inner::IsTrivialDecorator<BaseMemSet, T, MAX_SIZE, HASH, IS_EQUAL>
{
public:
    using BaseType = BaseMemSet<T, MAX_SIZE, HASH, IS_EQUAL>;
    using IntType = typename BaseType::IntType;
    using ValueType = typename BaseType::ValueType;
    using Iterator = typename BaseType::Iterator;

    // 暴露共享内存 init 接口
    using BaseType::init;
    using BaseType::need_mem_size;

    // 暴露容器操作
    using BaseType::clear;
    using BaseType::empty;
    using BaseType::full;
    using BaseType::size;
    using BaseType::capacity;
    using BaseType::insert;
    using BaseType::find;
    using BaseType::exist;
    using BaseType::erase;
    using BaseType::begin;
    using BaseType::end;
};

}  // namespace ua
