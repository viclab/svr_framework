/// @file singleton.h
/// @brief 单例模式实现（C++20 重写版）
/// @note 1. MSingleton: Meyers单例，支持 thread_local 线程级单例
///       2. Singleton: main 前初始化单例，避免多线程竞争
///       使用 C++20 concepts 代替 enable_if_t
#pragma once

#include <concepts>

namespace ua
{

/// Meyers 单例，支持线程级单例
/// @tparam T 实际类型
/// @tparam MultiThread true 时退化为 thread_local 线程级单例
template <typename T, bool MultiThread = false>
class MSingleton
{
public:
    static T& GetInst() noexcept
    {
        if constexpr (MultiThread)
        {
            static thread_local T obj;
            return obj;
        }
        else
        {
            static T obj;
            return obj;
        }
    }

protected:
    ~MSingleton() = default;
    MSingleton() = default;
    MSingleton(const MSingleton&) = delete;
    MSingleton& operator=(const MSingleton&) = delete;
};

/// main 前初始化单例
/// 通过静态变量链式触发保证在 main() 之前初始化
/// @tparam T 实际类型
template <typename T>
class Singleton
{
private:
    struct ObjectCreator
    {
        ObjectCreator() noexcept { Singleton<T>::GetInst(); }
        void do_nothing() const noexcept {}
    };
    static inline ObjectCreator create_object_{};

protected:
    ~Singleton() = default;
    Singleton() = default;
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;

public:
    static T& GetInst() noexcept
    {
        static T obj;
        create_object_.do_nothing();
        return obj;
    }
};

}  // namespace ua
