/// @file id_generator.h
/// @brief 全局唯一 ID 生成器（C++20 重写版）
#pragma once

#include <atomic>
#include <cstdint>
#include "patterns/singleton.h"

namespace ua
{

class IDGenerator : public Singleton<IDGenerator>
{
public:
    bool Init() noexcept;
    /// 产生一个递增的唯一 ID
    [[nodiscard]] uint64_t GenerateSeqID() noexcept;

private:
    friend class Singleton<IDGenerator>;
    /// 改进: 使用 atomic 保证线程安全
    std::atomic<uint64_t> base_seq_id_{0};
};

}  // namespace ua
