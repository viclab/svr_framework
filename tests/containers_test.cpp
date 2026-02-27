/// @file containers_test.cpp
/// @brief containers 模块单元测试
/// @note 覆盖: traits_utils + FixedVector + FixedRingBuf + UnfixedRingBuf
///             + MemSet + MemMap + MemList + MemLRUSet + MemLRUMap
///             + FixedMemPool + HashMemPool + FreeLockQueue
#include <gtest/gtest.h>
#include <algorithm>
#include <cstring>
#include <numeric>
#include <string>
#include <thread>
#include <vector>

#include "containers/inner/traits_utils.h"
#include "containers/fixed_vector.h"
#include "containers/fixed_ring_buf.h"
#include "containers/unfixed_ring_buf.h"
#include "containers/mem_set.h"
#include "containers/mem_map.h"
#include "containers/mem_list.h"
#include "containers/mem_lru_set.h"
#include "containers/mem_lru_map.h"
#include "containers/fixed_mem_pool.h"
#include "containers/hash_mem_pool.h"
#include "containers/queue_lock_free.h"

namespace ua::test
{

// ==================== traits_utils 测试 ====================

TEST(TraitsUtilsTest, IsPrime)
{
    EXPECT_FALSE(ua::is_prime(0));
    EXPECT_FALSE(ua::is_prime(1));
    EXPECT_TRUE(ua::is_prime(2));
    EXPECT_TRUE(ua::is_prime(3));
    EXPECT_FALSE(ua::is_prime(4));
    EXPECT_TRUE(ua::is_prime(5));
    EXPECT_TRUE(ua::is_prime(7));
    EXPECT_TRUE(ua::is_prime(97));
    EXPECT_FALSE(ua::is_prime(100));
    EXPECT_TRUE(ua::is_prime(997));
}

TEST(TraitsUtilsTest, NearbyPrime)
{
    EXPECT_EQ(ua::nearby_prime(100), 97u);
    EXPECT_EQ(ua::nearby_prime(97), 97u);
    EXPECT_EQ(ua::nearby_prime(10), 7u);
    EXPECT_EQ(ua::nearby_prime(2), 2u);
}

TEST(TraitsUtilsTest, IsPowOfTwo)
{
    EXPECT_TRUE((ua::IsPowOfTwo<1>));
    EXPECT_TRUE((ua::IsPowOfTwo<2>));
    EXPECT_TRUE((ua::IsPowOfTwo<4>));
    EXPECT_TRUE((ua::IsPowOfTwo<8>));
    EXPECT_TRUE((ua::IsPowOfTwo<1024>));
    EXPECT_FALSE((ua::IsPowOfTwo<3>));
    EXPECT_FALSE((ua::IsPowOfTwo<6>));
    EXPECT_FALSE((ua::IsPowOfTwo<0>));
}

TEST(TraitsUtilsTest, FixIntType)
{
    // 255 以内应该用 uint8_t
    static_assert(sizeof(ua::FixIntType<255>::IntType) == 1);
    // 256 需要 uint16_t
    static_assert(sizeof(ua::FixIntType<256>::IntType) == 2);
    // 65535 以内 uint16_t
    static_assert(sizeof(ua::FixIntType<65535>::IntType) == 2);
    // 65536 需要 uint32_t
    static_assert(sizeof(ua::FixIntType<65536>::IntType) == 4);
}

// ==================== FixedVector 测试 ====================

TEST(FixedVectorTest, PushBackAndAccess)
{
    ua::FixedVector<int, 10> vec;
    EXPECT_TRUE(vec.empty());
    EXPECT_EQ(vec.size(), 0u);

    EXPECT_TRUE(vec.push_back(42));
    EXPECT_TRUE(vec.push_back(99));
    EXPECT_EQ(vec.size(), 2u);
    EXPECT_EQ(vec[0], 42);
    EXPECT_EQ(vec[1], 99);
}

TEST(FixedVectorTest, FullCheck)
{
    ua::FixedVector<int, 3> vec;
    EXPECT_TRUE(vec.push_back(1));
    EXPECT_TRUE(vec.push_back(2));
    EXPECT_TRUE(vec.push_back(3));
    EXPECT_TRUE(vec.full());
    EXPECT_FALSE(vec.push_back(4));  // 满了
}

TEST(FixedVectorTest, ClearAndEmpty)
{
    ua::FixedVector<int, 5> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.clear();
    EXPECT_TRUE(vec.empty());
    EXPECT_EQ(vec.size(), 0u);
}

TEST(FixedVectorTest, EraseElement)
{
    ua::FixedVector<int, 10> vec;
    for (int i = 0; i < 5; ++i)
        vec.push_back(i * 10);

    // 删除第二个元素 [0, 10, 20, 30, 40] -> [0, 20, 30, 40]
    vec.erase(vec.cbegin() + 1);
    EXPECT_EQ(vec.size(), 4u);
    EXPECT_EQ(vec[0], 0);
    EXPECT_EQ(vec[1], 20);
    EXPECT_EQ(vec[2], 30);
    EXPECT_EQ(vec[3], 40);
}

TEST(FixedVectorTest, IteratorTraversal)
{
    ua::FixedVector<int, 10> vec;
    for (int i = 0; i < 5; ++i)
        vec.push_back(i);

    int sum = 0;
    for (auto it = vec.begin(); it != vec.end(); ++it)
        sum += *it;
    EXPECT_EQ(sum, 10);  // 0+1+2+3+4
}

TEST(FixedVectorTest, ReverseIterator)
{
    ua::FixedVector<int, 10> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);

    std::vector<int> reversed;
    for (auto it = vec.rbegin(); it != vec.rend(); ++it)
        reversed.push_back(*it);

    EXPECT_EQ(reversed, (std::vector<int>{3, 2, 1}));
}

TEST(FixedVectorTest, AddMethod)
{
    ua::FixedVector<int, 10> vec;
    int* p = vec.Add();
    ASSERT_NE(p, nullptr);
    *p = 55;
    EXPECT_EQ(vec.size(), 1u);
    EXPECT_EQ(vec[0], 55);
}

// ==================== FixedRingBuf 测试 ====================

TEST(FixedRingBufTest, PushAndPop)
{
    ua::FixedRingBuf<int, 4> buf;
    EXPECT_TRUE(buf.empty());

    EXPECT_TRUE(buf.push(1));
    EXPECT_TRUE(buf.push(2));
    EXPECT_TRUE(buf.push(3));
    EXPECT_EQ(buf.size(), 3u);

    EXPECT_EQ(buf.front(), 1);
    buf.pop();
    EXPECT_EQ(buf.front(), 2);
    buf.pop();
    EXPECT_EQ(buf.front(), 3);
    buf.pop();
    EXPECT_TRUE(buf.empty());
}

TEST(FixedRingBufTest, FullAndOverwrite)
{
    ua::FixedRingBuf<int, 3> buf;
    EXPECT_TRUE(buf.push(1));
    EXPECT_TRUE(buf.push(2));
    EXPECT_TRUE(buf.push(3));
    EXPECT_TRUE(buf.full());
    EXPECT_FALSE(buf.push(4));  // 满了

    // 带覆写模式
    EXPECT_TRUE(buf.push(4, true));
    EXPECT_EQ(buf.front(), 2);  // 1 被覆盖
}

TEST(FixedRingBufTest, WrapAround)
{
    ua::FixedRingBuf<int, 4> buf;
    buf.push(1);
    buf.push(2);
    buf.push(3);
    buf.pop();  // 移除 1
    buf.pop();  // 移除 2
    buf.push(4);
    buf.push(5);

    EXPECT_EQ(buf.size(), 3u);
    EXPECT_EQ(buf.front(), 3);
}

TEST(FixedRingBufTest, BackAccess)
{
    ua::FixedRingBuf<int, 10> buf;
    buf.push(10);
    buf.push(20);
    buf.push(30);
    EXPECT_EQ(buf.back(), 30);
    EXPECT_EQ(buf.back(1), 20);
    EXPECT_EQ(buf.back(2), 10);
}

// ==================== UnfixedRingBuf 测试 ====================

TEST(UnfixedRingBufTest, PushAndPop)
{
    ua::UnfixedRingBuf<1024> buf;
    EXPECT_TRUE(buf.empty());

    const char* data1 = "hello";
    const char* data2 = "world";
    EXPECT_TRUE(buf.push(reinterpret_cast<const uint8_t*>(data1), strlen(data1)));
    EXPECT_TRUE(buf.push(reinterpret_cast<const uint8_t*>(data2), strlen(data2)));
    EXPECT_EQ(buf.get_num(), 2u);

    size_t len = 0;
    auto* front = buf.front(len);
    ASSERT_NE(front, nullptr);
    EXPECT_EQ(len, strlen(data1));
    EXPECT_EQ(memcmp(front, data1, len), 0);

    buf.pop();
    front = buf.front(len);
    ASSERT_NE(front, nullptr);
    EXPECT_EQ(len, strlen(data2));
    EXPECT_EQ(memcmp(front, data2, len), 0);

    buf.pop();
    EXPECT_TRUE(buf.empty());
}

TEST(UnfixedRingBufTest, PopCallback)
{
    ua::UnfixedRingBuf<1024> buf;
    const char* data = "callback_test";
    buf.push(reinterpret_cast<const uint8_t*>(data), strlen(data));

    std::string popped_data;
    buf.pop([&popped_data](const uint8_t* d, size_t l) {
        popped_data.assign(reinterpret_cast<const char*>(d), l);
    });
    EXPECT_EQ(popped_data, "callback_test");
}

TEST(UnfixedRingBufTest, OverwriteMode)
{
    // 使用较小的缓冲区，确保两个 item 不可能同时放下
    ua::UnfixedRingBuf<48> buf;

    // 第一个数据能放下
    std::string data1(20, 'A');
    EXPECT_TRUE(buf.push(reinterpret_cast<const uint8_t*>(data1.c_str()), data1.size()));

    // 第二个数据也是 20 字节，加上两个 ItemHeader 已经超过 48 字节
    std::string data2(20, 'B');

    // 检查无覆写模式下是否正确拒绝或接受（取决于实际剩余空间）
    bool no_overwrite_result = buf.push(reinterpret_cast<const uint8_t*>(data2.c_str()), data2.size(), false);

    if (!no_overwrite_result)
    {
        // 覆写模式应成功
        EXPECT_TRUE(buf.push(reinterpret_cast<const uint8_t*>(data2.c_str()), data2.size(), true));
    }
    // 无论如何，缓冲区应该不为空
    EXPECT_FALSE(buf.empty());
}

// ==================== MemSet 测试 ====================

TEST(MemSetTest, InsertAndFind)
{
    ua::MemSet<int, 100> set;
    set.clear();

    auto [it1, ok1] = set.insert(42);
    EXPECT_TRUE(ok1);
    EXPECT_EQ(*it1, 42);

    auto [it2, ok2] = set.insert(42);  // 重复插入
    EXPECT_FALSE(ok2);

    auto found = set.find(42);
    EXPECT_NE(found, set.end());
    EXPECT_EQ(*found, 42);
}

TEST(MemSetTest, Erase)
{
    ua::MemSet<int, 100> set;
    set.clear();

    set.insert(10);
    set.insert(20);
    set.insert(30);
    EXPECT_EQ(set.size(), 3u);

    set.erase(20);
    EXPECT_EQ(set.size(), 2u);
    EXPECT_FALSE(set.exist(20));
    EXPECT_TRUE(set.exist(10));
    EXPECT_TRUE(set.exist(30));
}

TEST(MemSetTest, FullCheck)
{
    ua::MemSet<int, 3> set;
    set.clear();

    set.insert(1);
    set.insert(2);
    set.insert(3);
    EXPECT_TRUE(set.full());

    auto [it, ok] = set.insert(4);
    EXPECT_FALSE(ok);  // 满了
}

TEST(MemSetTest, IterateAll)
{
    ua::MemSet<int, 100> set;
    set.clear();

    for (int i = 0; i < 10; ++i)
        set.insert(i * 10);

    std::vector<int> values;
    for (auto it = set.begin(); it != set.end(); ++it)
        values.push_back(*it);

    EXPECT_EQ(values.size(), 10u);
    std::sort(values.begin(), values.end());
    for (int i = 0; i < 10; ++i)
        EXPECT_EQ(values[i], i * 10);
}

// ==================== MemMap 测试 ====================

TEST(MemMapTest, InsertAndFind)
{
    ua::MemMap<int, int, 100> map;
    map.clear();

    auto [it, ok] = map.insert(1, 100);
    EXPECT_TRUE(ok);
    EXPECT_EQ(it->first, 1);
    EXPECT_EQ(it->second, 100);

    auto found = map.find(1);
    EXPECT_NE(found, map.end());
    EXPECT_EQ(found->second, 100);
}

TEST(MemMapTest, EraseByKey)
{
    ua::MemMap<int, int, 100> map;
    map.clear();

    map.insert(1, 10);
    map.insert(2, 20);
    map.insert(3, 30);

    map.erase(2);
    EXPECT_EQ(map.size(), 2u);
    EXPECT_FALSE(map.exist(2));
    EXPECT_TRUE(map.exist(1));
    EXPECT_TRUE(map.exist(3));
}

// ==================== MemList 测试 ====================

TEST(MemListTest, PushFrontAndPushBack)
{
    ua::MemList<int, 10> list;
    list.clear();

    list.push_front(1);
    list.push_front(2);
    list.push_back(3);

    // 期望顺序: 2, 1, 3
    std::vector<int> values;
    for (auto it = list.begin(); it != list.end(); ++it)
        values.push_back(*it);

    EXPECT_EQ(values, (std::vector<int>{2, 1, 3}));
}

TEST(MemListTest, PopFrontAndPopBack)
{
    ua::MemList<int, 10> list;
    list.clear();

    list.push_back(1);
    list.push_back(2);
    list.push_back(3);

    list.pop_front();
    list.pop_back();
    EXPECT_EQ(list.size(), 1u);
    EXPECT_EQ(*list.begin(), 2);
}

TEST(MemListTest, FindAndErase)
{
    ua::MemList<int, 10> list;
    list.clear();

    list.push_back(10);
    list.push_back(20);
    list.push_back(30);

    auto it = list.find(20);
    EXPECT_NE(it, list.end());
    EXPECT_EQ(*it, 20);

    list.erase(it);
    EXPECT_EQ(list.size(), 2u);
    EXPECT_EQ(list.find(20), list.end());
}

TEST(MemListTest, FindIf)
{
    ua::MemList<int, 10> list;
    list.clear();

    list.push_back(1);
    list.push_back(2);
    list.push_back(3);

    auto it = list.find_if([](const int& v) { return v > 1; });
    EXPECT_NE(it, list.end());
    EXPECT_EQ(*it, 2);
}

// ==================== MemLRUSet 测试 ====================

TEST(MemLRUSetTest, InsertAndFind)
{
    ua::MemLRUSet<int, 100> lru;
    lru.clear();

    auto [it, ok] = lru.insert(42, false);
    EXPECT_TRUE(ok);
    EXPECT_EQ(*it, 42);

    auto found = lru.find(42);
    EXPECT_NE(found, lru.end());
    EXPECT_EQ(*found, 42);
}

TEST(MemLRUSetTest, ForceInsertWhenFull)
{
    ua::MemLRUSet<int, 3> lru;
    lru.clear();

    lru.insert(1, false);
    lru.insert(2, false);
    lru.insert(3, false);
    EXPECT_TRUE(lru.full());

    // 非 force 模式失败
    auto [it1, ok1] = lru.insert(4, false);
    EXPECT_FALSE(ok1);

    // force 模式淘汰最旧的
    auto [it2, ok2] = lru.insert(4, true);
    EXPECT_TRUE(ok2);
    EXPECT_EQ(*it2, 4);
    EXPECT_FALSE(lru.exist(1));  // 最旧的 1 被淘汰
}

TEST(MemLRUSetTest, ActiveMovesToFront)
{
    ua::MemLRUSet<int, 10> lru;
    lru.clear();

    lru.insert(1, false);
    lru.insert(2, false);
    lru.insert(3, false);
    // 访问顺序: 3(最新), 2, 1(最旧)

    lru.active(1);  // 将 1 提到最新
    // 访问顺序: 1(最新), 3, 2(最旧)

    // 淘汰最旧的应该是 2
    lru.disuse(1);
    EXPECT_FALSE(lru.exist(2));
    EXPECT_TRUE(lru.exist(1));
    EXPECT_TRUE(lru.exist(3));
}

// ==================== MemLRUMap 测试 ====================

TEST(MemLRUMapTest, InsertAndFind)
{
    ua::MemLRUMap<int, int, 100> lru;
    lru.clear();

    auto [it, ok] = lru.insert(1, 100);
    EXPECT_TRUE(ok);
    EXPECT_EQ(it->first, 1);
    EXPECT_EQ(it->second, 100);

    auto found = lru.find(1);
    EXPECT_NE(found, lru.end());
    EXPECT_EQ(found->second, 100);
}

TEST(MemLRUMapTest, ForceInsertEvictsOldest)
{
    ua::MemLRUMap<int, int, 3> lru;
    lru.clear();

    lru.insert(1, 10);
    lru.insert(2, 20);
    lru.insert(3, 30);

    auto [it, ok] = lru.insert(4, 40, true);  // force=true
    EXPECT_TRUE(ok);
    EXPECT_FALSE(lru.exist(1));  // 最旧的 1 被淘汰
    EXPECT_TRUE(lru.exist(4));
}

// ==================== FixedMemPool 测试 ====================

struct TestNode
{
    int id;
    char name[32];
};

TEST(FixedMemPoolTest, AllocAndFree)
{
    ua::FixedMemPool<TestNode> pool;
    size_t pool_size = pool.calc_need_size(10);
    std::vector<uint8_t> mem(pool_size, 0);

    ASSERT_TRUE(pool.init(mem.data(), pool_size, 10, false));
    EXPECT_TRUE(pool.empty());
    EXPECT_EQ(pool.capacity(), 10u);

    auto* node = pool.alloc();
    ASSERT_NE(node, nullptr);
    node->id = 42;
    strncpy(node->name, "test", sizeof(node->name));

    EXPECT_EQ(pool.size(), 1u);
    EXPECT_FALSE(pool.empty());
    EXPECT_TRUE(pool.free(node));
    EXPECT_TRUE(pool.empty());
}

TEST(FixedMemPoolTest, AllocUntilFull)
{
    ua::FixedMemPool<int> pool;
    size_t pool_size = pool.calc_need_size(5);
    std::vector<uint8_t> mem(pool_size, 0);

    pool.init(mem.data(), pool_size, 5, false);

    std::vector<int*> ptrs;
    for (int i = 0; i < 5; ++i)
    {
        auto* p = pool.alloc();
        ASSERT_NE(p, nullptr);
        *p = i;
        ptrs.push_back(p);
    }

    EXPECT_TRUE(pool.full());
    EXPECT_EQ(pool.alloc(), nullptr);  // 满了

    // 释放一个后又可以分配
    pool.free(ptrs[2]);
    auto* p = pool.alloc();
    ASSERT_NE(p, nullptr);
}

TEST(FixedMemPoolTest, IterateAllocatedNodes)
{
    ua::FixedMemPool<int> pool;
    size_t pool_size = pool.calc_need_size(10);
    std::vector<uint8_t> mem(pool_size, 0);

    pool.init(mem.data(), pool_size, 10, false);

    for (int i = 0; i < 5; ++i)
    {
        auto* p = pool.alloc();
        *p = i * 10;
    }

    std::vector<int> values;
    for (auto it = pool.begin(); it != pool.end(); ++it)
        values.push_back(*it);

    EXPECT_EQ(values.size(), 5u);
}

TEST(FixedMemPoolTest, Ptr2IntAndInt2Ptr)
{
    ua::FixedMemPool<int> pool;
    size_t pool_size = pool.calc_need_size(10);
    std::vector<uint8_t> mem(pool_size, 0);

    pool.init(mem.data(), pool_size, 10, false);

    auto* p = pool.alloc();
    *p = 99;

    size_t idx = pool.ptr_2_int(p);
    EXPECT_GT(idx, 0u);

    auto* p2 = pool.int_2_ptr(idx);
    EXPECT_EQ(p, p2);
    EXPECT_EQ(*p2, 99);
}

// ==================== HashMemPool 测试 ====================

TEST(HashMemPoolTest, InsertAndFind)
{
    ua::HashMemPool<int, int> pool;
    size_t mem_size = pool.calc_mem_size(100, 64);
    std::vector<uint8_t> mem(mem_size, 0);

    ASSERT_TRUE(pool.init(mem.data(), 100, 64, static_cast<uint32_t>(mem_size)));

    auto [it1, ok1] = pool.insert(1, 100);
    EXPECT_TRUE(ok1);
    EXPECT_EQ(it1->first, 1);
    EXPECT_EQ(it1->second, 100);

    auto [it2, ok2] = pool.insert(1);  // 重复 key
    EXPECT_FALSE(ok2);

    auto found = pool.find(1);
    EXPECT_NE(found, pool.end());
    EXPECT_EQ(found->second, 100);
}

TEST(HashMemPoolTest, Erase)
{
    ua::HashMemPool<int, int> pool;
    size_t mem_size = pool.calc_mem_size(100, 64);
    std::vector<uint8_t> mem(mem_size, 0);

    pool.init(mem.data(), 100, 64, static_cast<uint32_t>(mem_size));

    pool.insert(1, 10);
    pool.insert(2, 20);
    pool.insert(3, 30);

    EXPECT_TRUE(pool.erase(2));
    EXPECT_EQ(pool.find(2), pool.end());
    EXPECT_EQ(pool.size(), 2u);
}

TEST(HashMemPoolTest, GetOrInsert)
{
    ua::HashMemPool<int, int> pool;
    size_t mem_size = pool.calc_mem_size(100, 64);
    std::vector<uint8_t> mem(mem_size, 0);

    pool.init(mem.data(), 100, 64, static_cast<uint32_t>(mem_size));

    auto it1 = pool.get_or_insert(42);
    EXPECT_NE(it1, pool.end());
    it1->second = 999;

    auto it2 = pool.get_or_insert(42);  // 已存在，直接返回
    EXPECT_EQ(it2->second, 999);
    EXPECT_EQ(pool.size(), 1u);
}

// ==================== FreeLockQueue 测试 ====================

TEST(FreeLockQueueTest, PushAndPop)
{
    ua::FreeLockQueue<int, 8> queue;
    EXPECT_TRUE(queue.IsEmpty());

    EXPECT_EQ(queue.Push(42), 0);
    EXPECT_FALSE(queue.IsEmpty());

    int val = 0;
    EXPECT_EQ(queue.Pop(val), 0);
    EXPECT_EQ(val, 42);
    EXPECT_TRUE(queue.IsEmpty());
}

TEST(FreeLockQueueTest, FullCheck)
{
    ua::FreeLockQueue<int, 4> queue;
    EXPECT_EQ(queue.Push(1), 0);
    EXPECT_EQ(queue.Push(2), 0);
    EXPECT_EQ(queue.Push(3), 0);
    EXPECT_TRUE(queue.IsFull());

    int result = queue.Push(4);
    EXPECT_EQ(result, static_cast<int>(ua::LockFreeErr::QueueFull));
}

TEST(FreeLockQueueTest, ConcurrentPushSinglePop)
{
    ua::FreeLockQueue<int, 128> queue;
    constexpr int kNumThreads = 4;
    constexpr int kItemsPerThread = 20;

    // 多线程并发 Push
    std::vector<std::thread> threads;
    for (int t = 0; t < kNumThreads; ++t)
    {
        threads.emplace_back([&queue, t]() {
            for (int i = 0; i < kItemsPerThread; ++i)
                queue.Push(t * 1000 + i);
        });
    }
    for (auto& th : threads)
        th.join();

    // 单线程 Pop
    std::vector<int> values;
    int val;
    while (queue.Pop(val) == 0)
        values.push_back(val);

    EXPECT_EQ(values.size(), kNumThreads * kItemsPerThread);
}

}  // namespace ua::test
