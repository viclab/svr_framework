/// @file generate_type_id.h
/// @brief 编译期类型 ID 生成器（C++20 重写版）
/// @note 利用函数模板特化的特性，为每个类型分配全局唯一的递增 ID
#pragma once

#include <atomic>
#include <cstddef>

namespace ua
{

/// 编译期自动生成类型 ID 的工具
/// @tparam Scope 作用域标识类型，不同作用域的 ID 独立递增
/// @tparam IDType ID 的数值类型
template <typename Scope, typename IDType = size_t>
struct AutoGenTypeID
{
    /// 获取类型 T 在 Scope 作用域中的唯一 ID
    /// C++20: 使用 std::atomic 保证线程安全（改进原版的非线程安全 inline static）
    template <typename T>
    static IDType GetID() noexcept
    {
        // 每个 T 的特化只初始化一次，利用 C++ 函数局部静态变量初始化保证
        static const IDType identity = base_id_.fetch_add(1, std::memory_order_relaxed);
        return identity;
    }

private:
    inline static std::atomic<IDType> base_id_{0};
};

}  // namespace ua
