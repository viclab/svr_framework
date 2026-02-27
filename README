
# svr_framework

> C++20 高性能服务器框架，基于模块化架构设计，提供共享内存容器、RPC/Protobuf 通信、协程调度、系统模块管理等核心能力。

## 特性

- **C++20 标准** — 使用 concepts、constexpr if、`[[nodiscard]]`、结构化绑定等现代特性
- **零外部依赖内核** — `core`、`common`、`containers`、`patterns` 模块无第三方依赖
- **共享内存容器** — 所有容器基于连续内存/定长数组实现，可直接放置于共享内存
- **可选 Protobuf** — `pb` 模块通过 CMake 选项 `UA_BUILD_PB` 按需编译
- **全面单元测试** — 基于 GoogleTest，覆盖 90+ 测试用例

## 目录结构

```
svr_framework/
├── CMakeLists.txt          # 构建配置
├── README.md               # 本文件
├── patterns/               # 设计模式
│   ├── singleton.h         #   单例模式（Meyers 单例 + main 前初始化 + thread_local）
│   └── obj_factory.h       #   对象工厂（数组工厂 TArrayFactory / 映射工厂 TMapFactory）
├── common/                 # 通用工具
│   ├── clock.h             #   高精度时钟（秒/毫秒/微秒）
│   ├── id_generator.h/cpp  #   分布式 ID 生成器（进程ID + 时间戳 + 自增序列号）
│   ├── timeout_queue.h/cpp #   超时队列（基于时间轮的定时任务管理）
│   └── utils.h/cpp         #   通用工具函数
├── containers/             # 共享内存容器
│   ├── inner/              #   内部实现（数据结构、traits、特化）
│   │   ├── traits_utils.h  #     素数判断、2 的幂判断、类型大小推导
│   │   ├── ring_buf_data.h #     环形缓冲区底层数据结构
│   │   ├── mem_set_data.h  #     开放寻址哈希表底层数据
│   │   ├── mem_lru_set_data.h #  LRU 哈希表底层数据
│   │   ├── base_mem_set.h  #     哈希集合基类实现
│   │   ├── base_mem_list.h #     双向链表基类实现
│   │   ├── base_mem_lru_set.h #  LRU 集合基类实现
│   │   ├── base_struct.h   #     基础结构体定义
│   │   ├── base_specialization.h # 模板特化辅助
│   │   └── is_trivial_decorator.h # trivial 类型装饰器
│   ├── fixed_vector.h      #   定长向量（编译期固定容量）
│   ├── fixed_ring_buf.h    #   定长环形缓冲区（定长元素）
│   ├── unfixed_ring_buf.h  #   变长环形缓冲区（变长数据块）
│   ├── mem_set.h           #   共享内存哈希集合（开放寻址）
│   ├── mem_map.h           #   共享内存哈希映射
│   ├── mem_list.h          #   共享内存双向链表
│   ├── mem_lru_set.h       #   共享内存 LRU 集合
│   ├── mem_lru_map.h       #   共享内存 LRU 映射
│   ├── fixed_mem_pool.h    #   定长内存池（下标分配）
│   ├── hash_mem_pool.h     #   哈希内存池（key-value 分配）
│   ├── protected_mem_pool.h #  带保护的内存池
│   └── queue_lock_free.h   #   无锁队列（多生产者-多消费者）
├── core/                   # 服务核心
│   ├── interface/          #   抽象接口层
│   │   ├── channel_interface.h    # 通信通道接口
│   │   ├── codec_interface.h      # 编解码器接口
│   │   ├── coroutine_interface.h  # 协程接口
│   │   ├── routing_interface.h    # 路由接口
│   │   ├── scheduler_interface.h  # 调度器接口
│   │   └── service_mesh.h         # 服务网格接口
│   ├── server_core.h/cpp   #   服务核心生命周期（Init/Tick/Proc/Finish）
│   ├── system_interface.h  #   系统模块抽象基类（ISystem）
│   ├── system_mgr.h/cpp    #   系统模块管理器（注册/获取/移除/生命周期分发）
│   ├── context.h           #   上下文体系（Context / ServerContext / ClientContext / AsyncTask）
│   ├── context_controller.h/cpp # 上下文控制器
│   ├── context_mgr.h/cpp   #   上下文管理器（thread_local 当前上下文）
│   ├── coro_mgr.h          #   协程管理器
│   ├── generate_type_id.h  #   编译期类型 ID 自动生成
│   ├── rpc_error.h         #   RPC 错误码枚举
│   ├── server_statistics.h #   服务统计（QPS、耗时分桶、日志计数）
│   ├── logger.h            #   日志系统（thread_local buffer + 可插拔输出）
│   ├── wait_group.h        #   WaitGroup（类似 Go sync.WaitGroup）
│   ├── timeout_decorator.h/cpp # 超时装饰器
│   └── transport.h/cpp     #   传输层管理
├── pb/                     # Protobuf/RPC 模块（可选）
│   ├── common_context.h    #   通用上下文定义
│   ├── pb_context.h/cpp    #   PB 上下文
│   ├── pb_service.h/cpp    #   PB RPC 服务
│   ├── intercepter.h       #   拦截器
│   ├── pkg_flag_type.h     #   包标志类型
│   └── rpc_methods_info.h  #   RPC 方法信息
└── tests/                  # 单元测试（GoogleTest）
    ├── patterns_test.cpp   #   singleton + obj_factory 测试
    ├── common_test.cpp     #   clock + id_generator + timeout_queue 测试
    ├── containers_test.cpp #   所有容器测试（43 个用例）
    ├── core_test.cpp       #   generate_type_id + rpc_error + server_statistics + system_mgr + wait_group 测试
    └── logger_test.cpp     #   日志系统测试
```

## 环境要求

| 依赖项 | 最低版本 | 说明 |
|--------|---------|------|
| C++ 编译器 | GCC 10+ / Clang 13+ | 需支持 C++20 |
| CMake | 3.20+ | 构建系统 |
| GoogleTest | 1.14+ | 单元测试（自动通过 FetchContent 下载） |
| Protobuf | 3.x+ | **可选**，仅 `pb` 模块需要 |

## 构建

### 基础构建（不含 PB 模块）

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### 包含 PB 模块

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DUA_BUILD_PB=ON
make -j$(nproc)
```

### 运行单元测试

```bash
cd build
ctest --output-on-failure

# 或使用聚合 target
make run_all_tests
```

### CMake 选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `UA_BUILD_TESTS` | `ON` | 是否编译单元测试 |
| `UA_BUILD_PB` | `OFF` | 是否编译 Protobuf/RPC 模块 |

## 快速上手

### 单例模式

```cpp
#include "patterns/singleton.h"

// Meyers 单例（懒加载）
class MyService : public ua::MSingleton<MyService> {
public:
    void DoWork() { /* ... */ }
};

// 线程级单例（每个线程独享实例）
class ThreadCache : public ua::MSingleton<ThreadCache, true> {
public:
    int local_counter = 0;
};

// main 前初始化单例（避免多线程竞争）
class GlobalConfig : public ua::Singleton<GlobalConfig> {
public:
    std::string config_path;
};

// 使用
MyService::GetInst().DoWork();
```

### 对象工厂

```cpp
#include "patterns/obj_factory.h"

// 数组工厂（通过整数 ID 查找，O(1)）
ua::TArrayFactory<std::unique_ptr<IAnimal>, AnimalCreator, 16> arr_factory;
arr_factory.Register(0, []() { return std::make_unique<Cat>(); });
auto cat = arr_factory.Create(0);

// 映射工厂（通过任意 key 查找）
ua::TMapFactory<std::unique_ptr<IAnimal>, AnimalCreator, std::string> map_factory;
map_factory.Register("dog", []() { return std::make_unique<Dog>(); });
auto dog = map_factory.Create("dog");
```

### 共享内存容器

```cpp
#include "containers/fixed_vector.h"
#include "containers/mem_map.h"
#include "containers/mem_lru_set.h"

// 定长向量（可放置于共享内存）
ua::FixedVector<int, 1024> vec;
vec.push_back(42);

// 共享内存哈希映射
ua::MemMap<uint64_t, PlayerData, 10007> player_map;
player_map.insert(uid, data);
auto* player = player_map.find(uid);

// LRU 集合（自动淘汰最久未使用的元素）
ua::MemLRUSet<uint64_t, 1024> lru;
lru.insert(key, true);  // force=true 强制插入
lru.active(key);         // 访问后移到头部
auto evicted = lru.disuse();  // 淘汰尾部元素
```

### 无锁队列

```cpp
#include "containers/queue_lock_free.h"

ua::FreeLockQueue<Message, 4096> queue;

// 生产者线程
queue.Push(msg);

// 消费者线程
Message out;
if (queue.Pop(out)) {
    // 处理消息
}
```

### 系统模块管理

```cpp
#include "core/system_interface.h"
#include "core/system_mgr.h"

// 定义系统模块
class PhysicsSystem : public ua::ISystem {
public:
    bool OnInit() override { /* 初始化物理引擎 */ return true; }
    void OnTick(uint64_t now_ms, uint64_t tick_count) override { /* 每帧更新 */ }
    bool OnFinish() override { /* 清理资源 */ return true; }
};

// 注册和使用
SystemMgr mgr;
mgr.AddSystem<PhysicsSystem>();
auto* physics = mgr.GetSystem<PhysicsSystem>();
```

### 日志系统

```cpp
#include "core/logger.h"

// 设置日志输出（可插拔）
ua::Logger::GetInst().SetCanOutputFunc([](ua::Logger::PriorityType p) {
    return p <= ua::Logger::PRIORITY_INFO;
});
ua::Logger::GetInst().SetOutputFunc([](ua::Logger::PriorityType p, const char* msg, uint32_t len) {
    printf("%.*s\n", len, msg);
});

// 使用宏输出日志
UA_LOG_INFO(uid, "player login|name=%s|level=%d", name, level);
UA_LOG_ERROR(uid, "rpc failed|ret=%d", ret);
```

### WaitGroup

```cpp
#include "core/wait_group.h"

ua::WaitGroup wg(3);

for (int i = 0; i < 3; i++) {
    std::thread([&wg]() {
        // 执行异步任务...
        wg.Done();  // 完成一个
    }).detach();
}

wg.Wait();  // 阻塞直到 3 个任务全部完成
```

## 架构概览

```
┌─────────────────────────────────────────────────────┐
│                   业务层 (Business)                   │
├─────────────────────────────────────────────────────┤
│  pb/          Protobuf/RPC 通信层（可选）              │
│  ├── PBService     RPC 服务注册与分发                  │
│  ├── PBContext      PB 请求上下文                      │
│  └── Intercepter    拦截器链                          │
├─────────────────────────────────────────────────────┤
│  core/        服务核心层                               │
│  ├── ServerCore      服务生命周期 (Init/Tick/Proc/Finish) │
│  ├── SystemMgr       系统模块管理 (注册/获取/分发)       │
│  ├── Context*        上下文体系 (Server/Client/Async)   │
│  ├── Logger          日志系统 (thread_local + 可插拔)    │
│  ├── interface/      抽象接口层                         │
│  │   ├── IChannel        通信通道                      │
│  │   ├── ICodec          编解码器                      │
│  │   ├── ICoroutine      协程                          │
│  │   ├── IRouting        路由                          │
│  │   ├── IScheduler      调度器                        │
│  │   └── IServiceMesh    服务网格                      │
│  └── ...             统计、超时、传输等                  │
├─────────────────────────────────────────────────────┤
│  common/      通用工具层                               │
│  ├── Clock           高精度时钟                        │
│  ├── IDGenerator     分布式 ID 生成器                   │
│  └── TimeoutQueue    超时队列（时间轮）                  │
├─────────────────────────────────────────────────────┤
│  containers/  共享内存容器层                            │
│  ├── FixedVector / FixedRingBuf / UnfixedRingBuf      │
│  ├── MemSet / MemMap / MemList                        │
│  ├── MemLRUSet / MemLRUMap                            │
│  ├── FixedMemPool / HashMemPool / ProtectedMemPool    │
│  └── FreeLockQueue                                    │
├─────────────────────────────────────────────────────┤
│  patterns/    设计模式层                               │
│  ├── Singleton / MSingleton    单例模式                 │
│  └── TArrayFactory / TMapFactory  对象工厂             │
└─────────────────────────────────────────────────────┘
```

## 作为子模块集成

```bash
# 在你的项目中添加为子模块
git submodule add <repo_url> third_party/svr_framework

# 在你的 CMakeLists.txt 中
add_subdirectory(third_party/svr_framework)
target_link_libraries(your_target PRIVATE svr_framework)
```

## 许可证

内部项目，仅限内部使用。

