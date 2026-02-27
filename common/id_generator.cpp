/// @file id_generator.cpp
/// @brief ID 生成器实现（C++20 重写版）
#include "id_generator.h"
#include "clock.h"

namespace ua
{

bool IDGenerator::Init() noexcept
{
    // 高 32 位为当前秒级时间戳，低 32 位从 0 递增
    base_seq_id_.store(
        (static_cast<uint64_t>(Clock::GetInst().CurrentSec()) << 32) & 0xFFFFFFFF00000000ULL,
        std::memory_order_relaxed);
    return true;
}

uint64_t IDGenerator::GenerateSeqID() noexcept
{
    return base_seq_id_.fetch_add(1, std::memory_order_relaxed) + 1;
}

}  // namespace ua
