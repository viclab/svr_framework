/// @file common_test.cpp
/// @brief common 模块单元测试（Clock + IDGenerator + TimeoutQueue）
#include <gtest/gtest.h>
#include <vector>
#include "common/clock.h"
#include "common/id_generator.h"
#include "common/timeout_queue.h"

namespace ua::test
{

// ==================== Clock 测试 ====================

TEST(ClockTest, InitialValueIsZero)
{
    auto& clock = ua::Clock::GetInst();
    // 初始可能不是0（如果其他测试修改过），先更新
    clock.Update(0);
    EXPECT_EQ(clock.CurrentSec(), 0u);
    EXPECT_EQ(clock.CurrentMilliSec(), 0u);
    EXPECT_EQ(clock.CurrentMicroSec(), 0u);
}

TEST(ClockTest, UpdateAndQuery)
{
    auto& clock = ua::Clock::GetInst();
    // 设置为 1.5 秒 = 1,500,000 微秒
    clock.Update(1'500'000);
    EXPECT_EQ(clock.CurrentSec(), 1u);
    EXPECT_EQ(clock.CurrentMilliSec(), 1500u);
    EXPECT_EQ(clock.CurrentMicroSec(), 1'500'000u);
}

TEST(ClockTest, LargeTimestamp)
{
    auto& clock = ua::Clock::GetInst();
    // 模拟 2024 年的 epoch 时间戳（秒）
    uint64_t ts_sec = 1700000000ULL;
    clock.Update(ts_sec * 1'000'000);
    EXPECT_EQ(clock.CurrentSec(), ts_sec);
    EXPECT_EQ(clock.CurrentMilliSec(), ts_sec * 1000);
}

// ==================== IDGenerator 测试 ====================

TEST(IDGeneratorTest, GenerateSequentialIDs)
{
    auto& gen = ua::IDGenerator::GetInst();
    gen.Init();

    uint64_t id1 = gen.GenerateSeqID();
    uint64_t id2 = gen.GenerateSeqID();
    uint64_t id3 = gen.GenerateSeqID();

    // ID 应该严格递增
    EXPECT_LT(id1, id2);
    EXPECT_LT(id2, id3);
}

TEST(IDGeneratorTest, IDsAreUnique)
{
    auto& gen = ua::IDGenerator::GetInst();
    gen.Init();

    std::vector<uint64_t> ids;
    ids.reserve(1000);
    for (int i = 0; i < 1000; ++i)
        ids.push_back(gen.GenerateSeqID());

    // 排序去重后数量不变
    std::sort(ids.begin(), ids.end());
    auto last = std::unique(ids.begin(), ids.end());
    EXPECT_EQ(std::distance(ids.begin(), last), 1000);
}

// ==================== TimeoutQueue 测试 ====================

TEST(TimeoutQueueTest, AddAndExist)
{
    ua::TimeoutQueue queue;
    auto id = queue.Add([](uint64_t, uint32_t) {}, 100);
    EXPECT_NE(id, 0u);
    EXPECT_TRUE(queue.Exist(id));
    EXPECT_FALSE(queue.Exist(999));
}

TEST(TimeoutQueueTest, CancelTimer)
{
    ua::TimeoutQueue queue;
    auto id = queue.Add([](uint64_t, uint32_t) {}, 100);
    EXPECT_TRUE(queue.Exist(id));
    EXPECT_TRUE(queue.Cancel(id));
    EXPECT_FALSE(queue.Exist(id));
}

TEST(TimeoutQueueTest, CancelNonExistentReturnsFalse)
{
    ua::TimeoutQueue queue;
    EXPECT_FALSE(queue.Cancel(999));
}

TEST(TimeoutQueueTest, TimeOutFiresCallbackOnExpire)
{
    ua::TimeoutQueue queue;

    int fire_count = 0;
    queue.Add([&fire_count](uint64_t, uint32_t) { ++fire_count; }, 100);
    queue.Add([&fire_count](uint64_t, uint32_t) { ++fire_count; }, 200);

    // 时间还没到，不应触发
    EXPECT_EQ(queue.TimeOut(50), 0u);
    EXPECT_EQ(fire_count, 0);

    // 时间到了第一个
    EXPECT_EQ(queue.TimeOut(100), 1u);
    EXPECT_EQ(fire_count, 1);

    // 时间到了第二个
    EXPECT_EQ(queue.TimeOut(200), 1u);
    EXPECT_EQ(fire_count, 2);
}

TEST(TimeoutQueueTest, IntervalTimerRepeats)
{
    ua::TimeoutQueue queue;

    int fire_count = 0;
    auto id = queue.Add([&fire_count](uint64_t, uint32_t) { ++fire_count; },
                        100,   // expire_time
                        100);  // interval_time: 每 100 重复

    // 第一次触发
    EXPECT_EQ(queue.TimeOut(100), 1u);
    EXPECT_EQ(fire_count, 1);
    EXPECT_TRUE(queue.Exist(id));  // 仍然存在（循环定时器）

    // 第二次触发
    EXPECT_EQ(queue.TimeOut(200), 1u);
    EXPECT_EQ(fire_count, 2);
    EXPECT_TRUE(queue.Exist(id));
}

TEST(TimeoutQueueTest, ClearRemovesAll)
{
    ua::TimeoutQueue queue;
    auto id1 = queue.Add([](uint64_t, uint32_t) {}, 100);
    auto id2 = queue.Add([](uint64_t, uint32_t) {}, 200);
    queue.Clear();
    EXPECT_FALSE(queue.Exist(id1));
    EXPECT_FALSE(queue.Exist(id2));
}

TEST(TimeoutQueueTest, MultipleTimersSameExpireTime)
{
    ua::TimeoutQueue queue;

    int fire_count = 0;
    queue.Add([&fire_count](uint64_t, uint32_t) { ++fire_count; }, 100);
    queue.Add([&fire_count](uint64_t, uint32_t) { ++fire_count; }, 100);
    queue.Add([&fire_count](uint64_t, uint32_t) { ++fire_count; }, 100);

    EXPECT_EQ(queue.TimeOut(100), 3u);
    EXPECT_EQ(fire_count, 3);
}

}  // namespace ua::test
