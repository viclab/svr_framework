/// @file base_struct.h
/// @brief 基础数据结构（C++20 重写版）
#pragma once

#include <cstddef>
#include <cstdint>

namespace ua
{

/// 双向链表节点
template <typename T>
struct Link
{
    T prev{};
    T next{};
};

/// 键值对（保证 first 在 second 前面，用于共享内存场景替代 std::pair）
template <typename T1, typename T2>
struct Pair
{
    T1 first{};
    T2 second{};
};

}  // namespace ua
