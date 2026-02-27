/// @file system_interface.h
/// @brief 系统模块抽象接口（C++20 重写版）
#pragma once

#include <cstdint>

namespace ua
{

/// 系统模块基类接口
class ISystem
{
public:
    /// 初始化
    virtual bool OnInit() { return true; }
    /// tick 定时调用
    virtual void OnTick(uint64_t now_ms, uint64_t tick_count) {}
    /// 分帧调用
    virtual size_t OnProc(uint64_t now_ms, uint64_t remain_ms, bool stop) { return 0; }
    /// 进程退出时回调
    virtual bool OnFinish() { return true; }

    virtual ~ISystem() = default;
};

}  // namespace ua
