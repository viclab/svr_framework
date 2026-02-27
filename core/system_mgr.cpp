/// @file system_mgr.cpp
/// @brief 模块管理器实现（C++20 重写版）
#include "system_mgr.h"

namespace ua
{

bool SystemMgr::SystemInit()
{
    // 按优先级从高到低遍历
    for (auto r_beg = sys_index_vec_.rbegin(); r_beg != sys_index_vec_.rend(); ++r_beg)
    {
        for (auto index : *r_beg)
        {
            if (systems_[index] && !systems_[index]->OnInit())
                return false;
        }
    }
    return true;
}

void SystemMgr::SystemTick(uint64_t now_ms, uint64_t tick_count)
{
    for (auto r_beg = sys_index_vec_.rbegin(); r_beg != sys_index_vec_.rend(); ++r_beg)
    {
        for (auto index : *r_beg)
        {
            if (systems_[index])
                systems_[index]->OnTick(now_ms, tick_count);
        }
    }
}

size_t SystemMgr::SystemProc(uint64_t now_ms, uint64_t remain_ms, bool stop)
{
    // 为每个 system 平均分配时间
    const size_t active_count = [&]() {
        size_t count = 0;
        for (const auto& vec : sys_index_vec_)
            count += vec.size();
        return count > 0 ? count : 1;
    }();
    const uint64_t sys_per_ms = remain_ms > 0 ? std::max<uint64_t>(remain_ms / active_count, 1) : 1;

    size_t proc_count = 0;
    for (auto r_beg = sys_index_vec_.rbegin(); r_beg != sys_index_vec_.rend(); ++r_beg)
    {
        for (auto index : *r_beg)
        {
            if (systems_[index])
                proc_count += systems_[index]->OnProc(now_ms, sys_per_ms, stop);
        }
    }
    return proc_count;
}

void SystemMgr::SystemFinish()
{
    for (auto r_beg = sys_index_vec_.rbegin(); r_beg != sys_index_vec_.rend(); ++r_beg)
    {
        for (auto index : *r_beg)
        {
            if (systems_[index])
                systems_[index]->OnFinish();
        }
    }
}

}  // namespace ua
