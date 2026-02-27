/// @file utils.h
/// @brief 工具函数（C++20 重写版）
#pragma once

#include <cstdint>
#include <string>

namespace ua::utils
{

/// 获取当前实时的时间（毫秒）
uint64_t CurrentRealMilliSec();
/// 获取当前实时的时间（微秒）
uint64_t CurrentRealMicroSec();

/// 文件路径是否存在
bool IsPathExist(const std::string& path_name);
/// 封装的 mmap 相关
char* GetMmapMem(const std::string& mmap_file_name, uint32_t mem_size, bool& is_exist,
                 std::string* err_msg = nullptr);
/// 获取文件句柄
int GetFileFd(const std::string& file_path, bool allow_create, std::string* err_msg = nullptr);

}  // namespace ua::utils
