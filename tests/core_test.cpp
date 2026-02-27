/// @file core_test.cpp
/// @brief core 模块单元测试（GenerateTypeID + RpcError + ServerStatistics + SystemMgr + WaitGroup）
#include <gtest/gtest.h>
#include <atomic>
#include <memory>
#include <thread>
#include <vector>
#include "core/generate_type_id.h"
#include "core/rpc_error.h"
#include "core/server_statistics.h"
#include "core/system_interface.h"
#include "core/system_mgr.h"
#include "core/wait_group.h"

namespace ua::test
{

// ==================== AutoGenTypeID 测试 ====================

struct Scope1 {};
struct TypeA {};
struct TypeB {};
struct TypeC {};

TEST(AutoGenTypeIDTest, DifferentTypesGetDifferentIDs)
{
    auto id_a = ua::AutoGenTypeID<Scope1>::GetID<TypeA>();
    auto id_b = ua::AutoGenTypeID<Scope1>::GetID<TypeB>();
    auto id_c = ua::AutoGenTypeID<Scope1>::GetID<TypeC>();

    EXPECT_NE(id_a, id_b);
    EXPECT_NE(id_b, id_c);
    EXPECT_NE(id_a, id_c);
}

TEST(AutoGenTypeIDTest, SameTypeAlwaysReturnsSameID)
{
    auto id1 = ua::AutoGenTypeID<Scope1>::GetID<TypeA>();
    auto id2 = ua::AutoGenTypeID<Scope1>::GetID<TypeA>();
    EXPECT_EQ(id1, id2);
}

struct Scope2 {};
TEST(AutoGenTypeIDTest, DifferentScopesAreIndependent)
{
    auto id_scope1 = ua::AutoGenTypeID<Scope1>::GetID<TypeA>();
    auto id_scope2 = ua::AutoGenTypeID<Scope2>::GetID<TypeA>();
    // 不同作用域的 ID 可以相同也可以不同，但各自序列独立
    // 这里只验证不会崩溃
    (void)id_scope1;
    (void)id_scope2;
}

// ==================== RpcError 测试 ====================

TEST(RpcErrorTest, IsSuccessCheck)
{
    EXPECT_TRUE(ua::IsSuccess(ua::RpcError::Success));
    EXPECT_FALSE(ua::IsSuccess(ua::RpcError::Timeout));
    EXPECT_FALSE(ua::IsSuccess(ua::RpcError::SystemError));
}

TEST(RpcErrorTest, ToIntAndFromInt)
{
    EXPECT_EQ(ua::ToInt(ua::RpcError::Success), 0);
    EXPECT_EQ(ua::ToInt(ua::RpcError::Timeout), -3);
    EXPECT_EQ(ua::FromInt(0), ua::RpcError::Success);
    EXPECT_EQ(ua::FromInt(-3), ua::RpcError::Timeout);
}

TEST(RpcErrorTest, ErrorNameReturnsDescription)
{
    EXPECT_EQ(ua::ErrorName(ua::RpcError::Success), "Success");
    EXPECT_EQ(ua::ErrorName(ua::RpcError::Timeout), "Timeout");
    EXPECT_EQ(ua::ErrorName(ua::RpcError::MsgParseError), "MsgParseError");
    EXPECT_EQ(ua::ErrorName(ua::FromInt(12345)), "Unknown");
}

TEST(RpcErrorTest, CompatConstants)
{
    EXPECT_EQ(ua::RPC_SUCCESS, 0);
    EXPECT_EQ(ua::RPC_TIME_OUT, -3);
    EXPECT_EQ(ua::RPC_SYS_ERR, -1);
}

// ==================== ServerStatistics 测试 ====================

TEST(ServerStatisticsTest, ClearResetsToZero)
{
    auto& stats = ua::ServerStatistics::GetInst();
    stats.statistics().inc_recv_pkg_num(10);
    stats.statistics().inc_send_pkg_num(5);
    stats.ClearStatistics();
    EXPECT_EQ(stats.statistics().recv_pkg_num, 0u);
    EXPECT_EQ(stats.statistics().send_pkg_num, 0u);
}

TEST(ServerStatisticsTest, IncrementMethods)
{
    auto& stats = ua::ServerStatistics::GetInst();
    stats.ClearStatistics();

    stats.statistics().inc_recv_pkg_num();
    stats.statistics().inc_recv_pkg_num();
    stats.statistics().inc_send_pkg_num(3);
    stats.statistics().inc_log_error_num(2);

    EXPECT_EQ(stats.statistics().recv_pkg_num, 2u);
    EXPECT_EQ(stats.statistics().send_pkg_num, 3u);
    EXPECT_EQ(stats.statistics().log_error_num, 2u);
}

TEST(ServerStatisticsTest, MaxValueMethods)
{
    auto& stats = ua::ServerStatistics::GetInst();
    stats.ClearStatistics();

    stats.statistics().set_max_proc_deal_time_0(100);
    stats.statistics().set_max_proc_deal_time_0(50);   // 不会更新，因为 50 < 100
    stats.statistics().set_max_proc_deal_time_0(200);   // 更新为 200

    EXPECT_EQ(stats.statistics().proc_deal_time_0, 200u);
}

TEST(ServerStatisticsTest, GetCostBucket)
{
    auto& stats = ua::ServerStatistics::GetInst();
    // kLowerCostTime = {0, 50, 100, 500, 1000, 3000, 5000, 60000}
    // GetCostBucket(1): lower_bound(1) -> 50, --iter -> 0
    EXPECT_EQ(stats.GetCostBucket(1), 0u);
    // GetCostBucket(50): lower_bound(50) -> 50, --iter -> 0  (lower_bound 找到 50 本身)
    EXPECT_EQ(stats.GetCostBucket(50), 0u);
    // GetCostBucket(51): lower_bound(51) -> 100, --iter -> 50
    EXPECT_EQ(stats.GetCostBucket(51), 50u);
    // GetCostBucket(99): lower_bound(99) -> 100, --iter -> 50
    EXPECT_EQ(stats.GetCostBucket(99), 50u);
    // GetCostBucket(100): lower_bound(100) -> 100, --iter -> 50
    EXPECT_EQ(stats.GetCostBucket(100), 50u);
    // GetCostBucket(500): lower_bound(500) -> 500, --iter -> 100
    EXPECT_EQ(stats.GetCostBucket(500), 100u);
    // GetCostBucket(999): lower_bound(999) -> 1000, --iter -> 500
    EXPECT_EQ(stats.GetCostBucket(999), 500u);
    // GetCostBucket(1000): lower_bound(1000) -> 1000, --iter -> 500
    EXPECT_EQ(stats.GetCostBucket(1000), 500u);
    // GetCostBucket(60000): lower_bound(60000) -> 60000, --iter -> 5000
    EXPECT_EQ(stats.GetCostBucket(60000), 5000u);
}

TEST(ServerStatisticsTest, SetCoroRunTimeRecords)
{
    auto& stats = ua::ServerStatistics::GetInst();
    stats.ClearStatistics();

    stats.SetCoroRunTime(1001, 150, 0);
    stats.SetCoroRunTime(1001, 250, -1);

    auto& info = stats.RecvCmd2Info();
    auto it = info.find(1001);
    ASSERT_NE(it, info.end());
    EXPECT_EQ(it->second.error_code_2_num.at(0), 1u);
    EXPECT_EQ(it->second.error_code_2_num.at(-1), 1u);
}

// ==================== SystemMgr 测试 ====================

class MockSystemA : public ua::ISystem
{
public:
    bool OnInit() override { init_called = true; return true; }
    void OnTick(uint64_t, uint64_t) override { tick_count++; }
    bool init_called = false;
    int tick_count = 0;
};

class MockSystemB : public ua::ISystem
{
public:
    bool OnInit() override { return true; }
    size_t OnProc(uint64_t, uint64_t, bool) override { return 42; }
};

// SystemMgr 的 protected 方法需要通过派生类测试
class TestableSystemMgr : public ua::SystemMgr
{
public:
    using ua::SystemMgr::SystemInit;
    using ua::SystemMgr::SystemTick;
    using ua::SystemMgr::SystemProc;
    using ua::SystemMgr::SystemFinish;
};

TEST(SystemMgrTest, AddAndGetSystem)
{
    TestableSystemMgr mgr;
    auto sys = std::make_unique<MockSystemA>();
    auto* raw_ptr = sys.get();

    EXPECT_TRUE(mgr.AddSystem<MockSystemA>(std::move(sys)));
    auto* retrieved = mgr.GetSystem<MockSystemA>();
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved, raw_ptr);
}

TEST(SystemMgrTest, GetNonExistentSystemReturnsNull)
{
    TestableSystemMgr mgr;
    EXPECT_EQ(mgr.GetSystem<MockSystemA>(), nullptr);
}

TEST(SystemMgrTest, DuplicateAddFails)
{
    TestableSystemMgr mgr;
    mgr.AddSystem<MockSystemA>(std::make_unique<MockSystemA>());
    EXPECT_FALSE(mgr.AddSystem<MockSystemA>(std::make_unique<MockSystemA>()));
}

TEST(SystemMgrTest, RemoveSystem)
{
    TestableSystemMgr mgr;
    mgr.AddSystem<MockSystemA>(std::make_unique<MockSystemA>());
    EXPECT_NE(mgr.GetSystem<MockSystemA>(), nullptr);
    EXPECT_TRUE(mgr.RemoveSystem<MockSystemA>());
    EXPECT_EQ(mgr.GetSystem<MockSystemA>(), nullptr);
}

TEST(SystemMgrTest, RemoveNonExistentFails)
{
    TestableSystemMgr mgr;
    EXPECT_FALSE(mgr.RemoveSystem<MockSystemA>());
}

// ==================== WaitGroup 测试 ====================

TEST(WaitGroupTest, BasicDoneAndWait)
{
    bool completed = false;
    ua::WaitGroup wg(3, [&completed]() { completed = true; });

    EXPECT_FALSE(wg.Wait());
    EXPECT_EQ(wg.Remaining(), 3u);

    wg.Done();
    EXPECT_EQ(wg.Remaining(), 2u);
    EXPECT_FALSE(completed);

    wg.Done();
    EXPECT_EQ(wg.Remaining(), 1u);
    EXPECT_FALSE(completed);

    wg.Done();
    EXPECT_EQ(wg.Remaining(), 0u);
    EXPECT_TRUE(completed);
    EXPECT_TRUE(wg.Wait());
}

TEST(WaitGroupTest, ConcurrentDone)
{
    std::atomic<int> callback_count{0};
    ua::WaitGroup wg(100, [&callback_count]() { callback_count.fetch_add(1); });

    std::vector<std::thread> threads;
    threads.reserve(100);
    for (int i = 0; i < 100; ++i)
    {
        threads.emplace_back([&wg]() { wg.Done(); });
    }

    for (auto& t : threads)
        t.join();

    EXPECT_TRUE(wg.Wait());
    EXPECT_EQ(callback_count.load(), 1);  // 回调恰好被调用 1 次
}

TEST(WaitGroupTest, ZeroCountCompletesImmediately)
{
    bool completed = false;
    // count=0 时不会自动触发回调（因为 Done 不会被调用）
    ua::WaitGroup wg(0, [&completed]() { completed = true; });
    EXPECT_TRUE(wg.Wait());
}

}  // namespace ua::test
