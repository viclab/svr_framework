/// @file scheduler_interface.h
/// @brief 请求调度器抽象接口（C++20 重写版）
#pragma once

#include <cstdint>
#include <functional>

namespace ua
{

class IScheduler
{
public:
    /// 处理函数: (gid, data, len, custom_data) -> bool
    using ProcFunc = std::function<bool(uint64_t, const char*, uint32_t, uint64_t)>;

    void SetProcFunc(ProcFunc func) noexcept { proc_func_ = std::move(func); }
    void SetStop(bool stop) noexcept { stop_ = stop; }
    [[nodiscard]] bool IsStop() const noexcept { return stop_; }

    /// 请求包入调度器，失败返回 false
    virtual bool OnRequest(uint64_t seq, uint64_t gid, const char* data, uint32_t len, uint64_t custom_data) = 0;
    /// 处理完一个请求包后回调
    virtual void OnResponse(uint64_t gid) = 0;
    /// 驱动调度器，proc_num=0 处理所有包，返回已处理数量
    [[nodiscard]] virtual uint32_t LoopOnce(uint32_t proc_num) = 0;
    /// 获取缓存的包数量
    [[nodiscard]] virtual size_t CacheNum(uint64_t gid) const = 0;

    virtual ~IScheduler() = default;

protected:
    bool ProcOnce(uint64_t gid, const char* data, uint32_t len, uint64_t custom_data)
    {
        return proc_func_(gid, data, len, custom_data);
    }

private:
    ProcFunc proc_func_;
    bool stop_ = false;
};

}  // namespace ua
