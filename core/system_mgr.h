/// @file system_mgr.h
/// @brief 模块管理器（C++20 重写版）
/// @note 改进: RemoveSystem 同步清理优先级索引
///       改进: 使用 concepts 约束模板参数
#pragma once

#include <algorithm>
#include <array>
#include <concepts>
#include <memory>
#include <utility>
#include <vector>
#include "generate_type_id.h"
#include "system_interface.h"

namespace ua
{

/// 系统模块优先级
enum class SystemPriority : uint8_t
{
    Low = 0,
    Mid = 1,
    High = 2,
    Max_,
};

// C++20 concept：约束只能注册 ISystem 的子类
template <typename T>
concept SystemType = std::derived_from<T, ISystem>;

class SystemMgr
{
public:
    /// 添加一个系统模块
    template <SystemType T>
    bool AddSystem(std::unique_ptr<ISystem>&& sys, SystemPriority priority = SystemPriority::Mid)
    {
        if (!sys)
            return false;

        const size_t sys_id = AutoGenTypeID<SystemMgr>::GetID<T>();
        if (sys_id >= kMaxSystemsNum)
            return false;

        if (systems_[sys_id] != nullptr)
            return false;

        systems_[sys_id] = std::move(sys);
        sys_index_vec_[static_cast<size_t>(priority)].push_back(sys_id);
        return true;
    }

    /// 删除一个系统模块
    /// 改进: 同步清理优先级索引中的条目
    template <SystemType T>
    bool RemoveSystem()
    {
        const size_t sys_id = AutoGenTypeID<SystemMgr>::GetID<T>();
        if (sys_id >= kMaxSystemsNum)
            return false;

        if (!systems_[sys_id])
            return false;

        systems_[sys_id].reset();

        // 同步清理 sys_index_vec_
        for (auto& vec : sys_index_vec_)
        {
            std::erase(vec, sys_id);
        }
        return true;
    }

    /// 获取一个系统模块指针
    template <SystemType T>
    [[nodiscard]] const T* GetSystem() const
    {
        const size_t id = AutoGenTypeID<SystemMgr>::GetID<T>();
        if (id >= kMaxSystemsNum || !systems_[id])
            return nullptr;
        return static_cast<const T*>(systems_[id].get());
    }

    template <SystemType T>
    [[nodiscard]] T* GetSystem()
    {
        return const_cast<T*>(std::as_const(*this).GetSystem<T>());
    }

    /// 获取一个系统模块引用（断言非空）
    template <SystemType T>
    [[nodiscard]] const T& GetSystemRef() const
    {
        const T* sys = GetSystem<T>();
        assert(sys != nullptr);
        return *sys;
    }

    template <SystemType T>
    [[nodiscard]] T& GetSystemRef()
    {
        return const_cast<T&>(std::as_const(*this).GetSystemRef<T>());
    }

    virtual ~SystemMgr() = default;

protected:
    bool SystemInit();
    void SystemTick(uint64_t now_ms, uint64_t tick_count);
    size_t SystemProc(uint64_t now_ms, uint64_t remain_ms, bool stop);
    void SystemFinish();

private:
    static constexpr size_t kMaxSystemsNum = 50;
    std::array<std::unique_ptr<ISystem>, kMaxSystemsNum> systems_{};
    std::array<std::vector<size_t>, static_cast<size_t>(SystemPriority::Max_)> sys_index_vec_{};
};

}  // namespace ua
