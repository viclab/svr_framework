/// @file clock.h
/// @brief 时钟工具（C++20 重写版）
#pragma once

#include <cstdint>
#include "patterns/singleton.h"

namespace ua
{

class Clock : public Singleton<Clock>
{
public:
    [[nodiscard]] uint64_t CurrentSec() const noexcept { return micro_sec_ / 1000000; }
    [[nodiscard]] uint64_t CurrentMilliSec() const noexcept { return micro_sec_ / 1000; }
    [[nodiscard]] uint64_t CurrentMicroSec() const noexcept { return micro_sec_; }
    void Update(uint64_t micro_sec) noexcept { micro_sec_ = micro_sec; }

private:
    friend class Singleton<Clock>;
    uint64_t micro_sec_ = 0;
};

}  // namespace ua
