/// @file traits_utils.h
/// @brief 编译期工具（C++20 重写版）
/// @note 改进: constexpr 函数替代递归 TMP，可读性大幅提升
///       改进: concepts 替代 SFINAE
#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <concepts>
#include <type_traits>

namespace ua
{

// ==================== 编译期类型大小推导 ====================

/// 根据数值范围选择最小的整型
template <size_t Size>
struct FixIntType
{
    static constexpr size_t bits = []() constexpr {
        size_t n = Size, b = 0;
        while (n > 0) { n >>= 1; ++b; }
        return b;
    }();
    static constexpr size_t bytes = (bits + 7) / 8;

    using IntType = std::conditional_t<
        bytes <= 1, uint8_t,
        std::conditional_t<bytes <= 2, uint16_t,
                           std::conditional_t<bytes <= 4, uint32_t, size_t>>>;
};

/// 是否为 2 的幂
template <size_t Num>
inline constexpr bool IsPowOfTwo = Num && ((Num & (Num - 1)) == 0);

// ==================== 编译期素数计算 ====================

/// constexpr 素数判断（替代原版递归 TMP）
constexpr bool is_prime(size_t n)
{
    if (n < 2) return false;
    if (n < 4) return true;
    if (n % 2 == 0 || n % 3 == 0) return false;
    for (size_t i = 5; i * i <= n; i += 6)
    {
        if (n % i == 0 || n % (i + 2) == 0)
            return false;
    }
    return true;
}

/// 寻找 <= n 的最近素数
constexpr size_t nearby_prime(size_t n)
{
    if (n > 800001) return n;  // 太大直接返回
    while (n > 1 && !is_prime(n)) --n;
    return n;
}

// ==================== 工具模板 ====================

/// 相等判断仿函数
template <typename T>
struct IsEqual
{
    using KeyType = T;
    bool operator()(const T& x, const T& y) const { return x == y; }
};

/// 提取 Key
template <typename T>
struct ExtractKey
{
    using KeyType = T;
    const KeyType& operator()(const T& x) const { return x; }
};

/// 归拢多个继承类
template <typename... Args>
struct TEmptyBase : public Args...
{
};

/// 统一指针和非指针
template <typename T>
T* Ptr(T& obj) { return &obj; }

template <typename T>
T* Ptr(T* obj) { return obj; }

}  // namespace ua
