# ua_framework_v2 - C++20 重写版构建配置
cmake_minimum_required(VERSION 3.20)
project(ua_framework_v2 VERSION 2.0.0 LANGUAGES CXX)

# ==================== C++20 标准 ====================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ==================== 编译选项 ====================
add_compile_options(
    -Wall -Wextra
    -Wno-unused-parameter
    -Wno-missing-field-initializers
)

# ==================== 头文件搜索路径 ====================
# 将项目根目录加入包含路径，使得 #include "patterns/singleton.h" 等路径生效
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# ==================== 库目标 ====================
# ==================== Protobuf 依赖（pb 模块需要） ====================
option(UA_BUILD_PB "构建 PB/RPC 模块（需要 protobuf）" OFF)

find_package(Protobuf QUIET)
if(Protobuf_FOUND AND UA_BUILD_PB)
    set(HAS_PROTOBUF TRUE)
    message(STATUS "Protobuf found: ${Protobuf_VERSION}, PB 模块将被编译")
else()
    set(HAS_PROTOBUF FALSE)
    message(STATUS "Protobuf 未找到或 UA_BUILD_PB=OFF, 跳过 PB 模块编译")
endif()

# 收集所有 .cpp 源文件
file(GLOB_RECURSE LIB_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/common/*.cpp"
)

if(HAS_PROTOBUF)
    file(GLOB_RECURSE PB_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/pb/*.cpp")
    list(APPEND LIB_SOURCES ${PB_SOURCES})
endif()

# 构建静态库
add_library(ua_framework_v2 STATIC ${LIB_SOURCES})
target_include_directories(ua_framework_v2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(HAS_PROTOBUF)
    target_link_libraries(ua_framework_v2 PUBLIC protobuf::libprotobuf)
    target_compile_definitions(ua_framework_v2 PUBLIC UA_HAS_PROTOBUF=1)
endif()

# ==================== 单元测试 ====================
option(UA_BUILD_TESTS "构建单元测试" ON)

if(UA_BUILD_TESTS)
    enable_testing()

    # 查找或下载 GoogleTest
    find_package(GTest QUIET)
    if(NOT GTest_FOUND)
        include(FetchContent)
        FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG        v1.14.0
        )
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)
    endif()

    # 收集所有测试源文件
    file(GLOB TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/tests/*_test.cpp")

    foreach(TEST_SOURCE ${TEST_SOURCES})
        # 从文件名提取测试名称: tests/foo_test.cpp -> foo_test
        get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)

        add_executable(${TEST_NAME} ${TEST_SOURCE})
        target_link_libraries(${TEST_NAME} PRIVATE
            ua_framework_v2
            GTest::gtest
            GTest::gtest_main
            pthread
        )
        target_include_directories(${TEST_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

        # 注册到 CTest
        add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    endforeach()

    # 添加一个聚合 target，一键运行所有测试
    add_custom_target(run_all_tests
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
        DEPENDS ${TEST_NAMES}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "运行所有单元测试"
    )
endif()
