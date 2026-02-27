/// @file patterns_test.cpp
/// @brief patterns 模块单元测试（singleton + obj_factory）
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <thread>
#include "patterns/singleton.h"
#include "patterns/obj_factory.h"

namespace ua::test
{

// ==================== Singleton 测试 ====================

class TestSingletonClass : public ua::Singleton<TestSingletonClass>
{
public:
    int value = 0;
};

TEST(SingletonTest, GetInstReturnsSameInstance)
{
    auto& inst1 = TestSingletonClass::GetInst();
    auto& inst2 = TestSingletonClass::GetInst();
    EXPECT_EQ(&inst1, &inst2);
}

TEST(SingletonTest, StateIsPersistent)
{
    TestSingletonClass::GetInst().value = 42;
    EXPECT_EQ(TestSingletonClass::GetInst().value, 42);
    TestSingletonClass::GetInst().value = 0;  // 清理
}

// ==================== MSingleton 测试 ====================

class TestMSingletonClass : public ua::MSingleton<TestMSingletonClass>
{
public:
    int counter = 0;
};

TEST(MSingletonTest, GetInstReturnsSameInstance)
{
    auto& inst1 = TestMSingletonClass::GetInst();
    auto& inst2 = TestMSingletonClass::GetInst();
    EXPECT_EQ(&inst1, &inst2);
}

// 线程级单例测试
class TestThreadLocalSingleton : public ua::MSingleton<TestThreadLocalSingleton, true>
{
public:
    int thread_value = 0;
};

TEST(MSingletonTest, ThreadLocalInstanceIsDifferent)
{
    auto& main_inst = TestThreadLocalSingleton::GetInst();
    main_inst.thread_value = 100;

    int other_thread_value = 0;
    std::thread t([&other_thread_value]() {
        auto& inst = TestThreadLocalSingleton::GetInst();
        other_thread_value = inst.thread_value;  // 应该是0，不是100
    });
    t.join();

    EXPECT_EQ(main_inst.thread_value, 100);
    EXPECT_EQ(other_thread_value, 0);  // 线程级单例，互相独立
}

// ==================== ObjFactory 测试 ====================

// 基类和派生类
struct IAnimal
{
    virtual ~IAnimal() = default;
    virtual std::string Name() = 0;
};

struct Cat : public IAnimal
{
    std::string Name() override { return "Cat"; }
};

struct Dog : public IAnimal
{
    std::string Name() override { return "Dog"; }
};

// 工厂 creator 类型
using AnimalCreator = std::function<std::unique_ptr<IAnimal>()>;

TEST(ObjFactoryTest, ArrayFactoryRegisterAndCreate)
{
    ua::TArrayFactory<std::unique_ptr<IAnimal>, AnimalCreator, 16> factory;

    factory.Register(0, []() -> std::unique_ptr<IAnimal> { return std::make_unique<Cat>(); });
    factory.Register(1, []() -> std::unique_ptr<IAnimal> { return std::make_unique<Dog>(); });

    auto cat = factory.Create(0);
    auto dog = factory.Create(1);

    ASSERT_NE(cat, nullptr);
    ASSERT_NE(dog, nullptr);
    EXPECT_EQ(cat->Name(), "Cat");
    EXPECT_EQ(dog->Name(), "Dog");
}

TEST(ObjFactoryTest, MapFactoryRegisterAndCreate)
{
    ua::TMapFactory<std::unique_ptr<IAnimal>, AnimalCreator, std::string> factory;

    factory.Register("cat", []() -> std::unique_ptr<IAnimal> { return std::make_unique<Cat>(); });
    factory.Register("dog", []() -> std::unique_ptr<IAnimal> { return std::make_unique<Dog>(); });

    auto cat = factory.Create("cat");
    auto dog = factory.Create("dog");

    ASSERT_NE(cat, nullptr);
    ASSERT_NE(dog, nullptr);
    EXPECT_EQ(cat->Name(), "Cat");
    EXPECT_EQ(dog->Name(), "Dog");
}

TEST(ObjFactoryTest, CreateUnregisteredReturnsNull)
{
    ua::TMapFactory<std::unique_ptr<IAnimal>, AnimalCreator, std::string> factory;
    // 未注册 key "fish"
    auto result = factory.Create("fish");
    EXPECT_EQ(result, nullptr);
}

TEST(ObjFactoryTest, MapFactoryDuplicateKeyFails)
{
    ua::TMapFactory<std::unique_ptr<IAnimal>, AnimalCreator, std::string> factory;
    bool ok1 = factory.Register("cat", []() -> std::unique_ptr<IAnimal> { return std::make_unique<Cat>(); });
    bool ok2 = factory.Register("cat", []() -> std::unique_ptr<IAnimal> { return std::make_unique<Dog>(); });
    EXPECT_TRUE(ok1);
    EXPECT_FALSE(ok2);  // 重复 key 注册失败
}

}  // namespace ua::test
