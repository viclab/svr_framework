/// @file mem_list.h
/// @brief 内存链表（C++20 重写版）
/// @note 改进: using 声明暴露基类方法
#pragma once

#include "inner/base_mem_list.h"

namespace ua
{

template <typename T, size_t MAX_SIZE>
class MemList : private BaseMemList<T, MAX_SIZE>
{
public:
    using BaseType = BaseMemList<T, MAX_SIZE>;
    using IntType = typename BaseType::IntType;
    using ValueType = typename BaseType::ValueType;
    using Iterator = typename BaseType::Iterator;

    using BaseType::clear;
    using BaseType::empty;
    using BaseType::full;
    using BaseType::size;
    using BaseType::capacity;
    using BaseType::push_front;
    using BaseType::push_back;
    using BaseType::pop_front;
    using BaseType::pop_back;
    using BaseType::erase;
    using BaseType::find;
    using BaseType::find_if;
    using BaseType::get;
    using BaseType::begin;
    using BaseType::end;
};

}  // namespace ua
