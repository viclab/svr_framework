/// @file base_specialization.h
/// @brief std::hash 和 IsEqual 对 Pair/std::pair 的特化（C++20 重写版）
#pragma once

#include <functional>
#include "base_struct.h"
#include "traits_utils.h"

namespace std
{

template <typename T1, typename T2>
struct hash<std::pair<T1, T2>>
{
    size_t operator()(const std::pair<T1, T2>& t) const { return hash<T1>{}(t.first); }
};

template <typename T1, typename T2>
struct hash<ua::Pair<T1, T2>>
{
    size_t operator()(const ua::Pair<T1, T2>& t) const { return hash<T1>{}(t.first); }
    size_t operator()(const T1& t) const { return hash<T1>{}(t); }
};

}  // namespace std

namespace ua
{

template <typename T1, typename T2>
struct IsEqual<std::pair<T1, T2>>
{
    using T = std::pair<T1, T2>;
    bool operator()(const T& x, const T& y) const { return IsEqual<T1>()(x.first, y.first); }
};

template <typename T1, typename T2>
struct IsEqual<Pair<T1, T2>>
{
    using T = Pair<T1, T2>;
    bool operator()(const T& x, const T& y) const { return IsEqual<T1>()(x.first, y.first); }
    bool operator()(const T& x, const T1& y) const { return IsEqual<T1>()(x.first, y); }
    bool operator()(const T1& x, const T& y) const { return IsEqual<T1>()(x, y.first); }
};

}  // namespace ua
