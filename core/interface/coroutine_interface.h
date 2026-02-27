/// @file coroutine_interface.h
/// @brief 非对称协程接口（C++20 重写版）
#pragma once

#include <cstdint>
#include <functional>

namespace ua
{

/// 协程对象接口，只提供 Resume / Yield 两个操作
class Coro
{
public:
    /// 唤醒当前协程
    virtual void Resume() = 0;
    /// 切回主协程
    virtual void Yield() = 0;

    virtual ~Coro() = default;
};

/// 协程创建和管理接口
class ICoroutine
{
public:
    /// 设置允许的协程最大数量
    virtual void SetMaxCoroNum(size_t max_num) = 0;
    /// 获取允许的协程最大数量
    [[nodiscard]] virtual size_t GetMaxCoroNum() const = 0;
    /// 获取当前在跑的协程数量
    [[nodiscard]] virtual size_t GetRunningCoro() const = 0;
    /// 获取所有的协程数量
    [[nodiscard]] virtual size_t GetTotalCoro() const = 0;

    /// 启动一个协程（C 风格函数指针版）
    using CoroFunc = void(void*);
    virtual bool Spawn(CoroFunc f, void* args) = 0;
    /// 启动一个协程（std::function 版）
    using CoroTask = std::function<void()>;
    virtual bool Spawn(CoroTask task) = 0;

    /// 获取当前协程的 Coro 指针，不在协程中返回 nullptr
    [[nodiscard]] virtual Coro* ThisCoro() const = 0;

    virtual ~ICoroutine() = default;
};

}  // namespace ua
