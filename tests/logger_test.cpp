/// @file logger_test.cpp
/// @brief Logger 模块单元测试
#include <gtest/gtest.h>
#include <string>
#include <thread>
#include <vector>
#include "core/logger.h"

namespace ua::test
{

TEST(LoggerTest, FormatBasicString)
{
    auto& logger = ua::Logger::GetInst();
    uint32_t len = logger.Format("hello %s, num=%d", "world", 42);
    EXPECT_GT(len, 0u);
    EXPECT_STREQ(logger.GetBuff(), "hello world, num=42");
}

TEST(LoggerTest, FormatEmptyString)
{
    auto& logger = ua::Logger::GetInst();
    uint32_t len = logger.Format("");
    EXPECT_EQ(len, 0u);
}

TEST(LoggerTest, FormatLongStringTruncated)
{
    auto& logger = ua::Logger::GetInst();
    // 生成一个超长字符串
    std::string long_str(5000, 'A');
    uint32_t len = logger.Format("%s", long_str.c_str());
    // 应该被截断到 4K - 1
    EXPECT_LT(len, 4096u);
}

TEST(LoggerTest, CanOutputWithNoFunc)
{
    auto& logger = ua::Logger::GetInst();
    // 没有设置 can_output_func_ 时应返回 false
    logger.SetCanOutputFunc(nullptr);
    EXPECT_FALSE(logger.CanOutput(ua::Logger::PRIORITY_INFO));
}

TEST(LoggerTest, CanOutputWithFunc)
{
    auto& logger = ua::Logger::GetInst();
    logger.SetCanOutputFunc([](ua::Logger::PriorityType priority) {
        return priority <= ua::Logger::PRIORITY_INFO;
    });

    EXPECT_TRUE(logger.CanOutput(ua::Logger::PRIORITY_ERROR));
    EXPECT_TRUE(logger.CanOutput(ua::Logger::PRIORITY_WARN));
    EXPECT_TRUE(logger.CanOutput(ua::Logger::PRIORITY_INFO));
    EXPECT_FALSE(logger.CanOutput(ua::Logger::PRIORITY_DEBUG));
    EXPECT_FALSE(logger.CanOutput(ua::Logger::PRIORITY_TRACE));

    logger.SetCanOutputFunc(nullptr);  // 清理
}

TEST(LoggerTest, OutputCallback)
{
    auto& logger = ua::Logger::GetInst();

    std::vector<std::pair<ua::Logger::PriorityType, std::string>> logged;
    logger.SetOutputFunc([&logged](ua::Logger::PriorityType p, const char* msg, uint32_t len) {
        logged.emplace_back(p, std::string(msg, len));
    });

    logger.Output(ua::Logger::PRIORITY_ERROR, "test error", 10);
    logger.Output(ua::Logger::PRIORITY_INFO, "test info", 9);

    ASSERT_EQ(logged.size(), 2u);
    EXPECT_EQ(logged[0].first, ua::Logger::PRIORITY_ERROR);
    EXPECT_EQ(logged[0].second, "test error");
    EXPECT_EQ(logged[1].first, ua::Logger::PRIORITY_INFO);
    EXPECT_EQ(logged[1].second, "test info");

    logger.SetOutputFunc(nullptr);  // 清理
}

TEST(LoggerTest, ThreadLocalBufferIsolation)
{
    auto& logger = ua::Logger::GetInst();

    // 主线程格式化
    logger.Format("main_thread_%d", 1);
    std::string main_result = logger.GetBuff();

    // 子线程格式化不影响主线程
    std::thread t([&logger]() {
        logger.Format("sub_thread_%d", 2);
    });
    t.join();

    // 主线程 buffer 不变（thread_local 隔离）
    EXPECT_EQ(std::string(logger.GetBuff()), main_result);
}

}  // namespace ua::test
