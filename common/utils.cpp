/// @file utils.cpp
/// @brief 工具函数实现（C++20 重写版）
#include "utils.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <chrono>
#include <cstring>

namespace ua::utils
{

uint64_t CurrentRealMilliSec()
{
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count());
}

uint64_t CurrentRealMicroSec()
{
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count());
}

bool IsPathExist(const std::string& path_name)
{
    return access(path_name.c_str(), F_OK) == 0;
}

/// RAII 文件句柄管理
class ScopedFd
{
public:
    explicit ScopedFd(int fd) noexcept : fd_(fd) {}
    ~ScopedFd() { if (fd_ >= 0) close(fd_); }
    int Release() noexcept { int fd = fd_; fd_ = -1; return fd; }
    [[nodiscard]] int Get() const noexcept { return fd_; }
    ScopedFd(const ScopedFd&) = delete;
    ScopedFd& operator=(const ScopedFd&) = delete;
private:
    int fd_;
};

static int EnsureDir(const std::string& file_name, bool allow_create, uint32_t& flag, std::string* err_msg)
{
    auto found = file_name.find_last_of('/');
    if (found != std::string::npos)
    {
        std::string dir = file_name.substr(0, found);
        if (!IsPathExist(dir))
        {
            if (!allow_create)
            {
                if (err_msg)
                    *err_msg += (dir + " not exist and not allow create");
                return -1;
            }
            int ret = mkdir(dir.c_str(), 0755);
            if (ret != 0)
            {
                if (err_msg)
                {
                    *err_msg += "mkdir failed, error: ";
                    *err_msg += strerror(errno);
                }
                return -1;
            }
        }
    }
    flag = O_RDWR;
    if (!IsPathExist(file_name))
    {
        if (!allow_create)
        {
            if (err_msg)
                *err_msg += file_name + " not exist and not allow create";
            return -1;
        }
        flag |= O_CREAT;
    }
    return 0;
}

char* GetMmapMem(const std::string& mmap_file_name, uint32_t mem_size, bool& is_exist, std::string* err_msg)
{
    is_exist = true;
    uint32_t flag = 0;
    if (EnsureDir(mmap_file_name, true, flag, err_msg) != 0)
        return nullptr;

    int fd = open(mmap_file_name.c_str(), static_cast<int>(flag), 0666);
    if (fd < 0)
    {
        if (err_msg)
        {
            *err_msg += "open file failed, error: ";
            *err_msg += strerror(errno);
        }
        return nullptr;
    }
    ScopedFd scoped_fd(fd);

    struct stat file_stat{};
    if (fstat(fd, &file_stat) == -1)
    {
        if (err_msg)
        {
            *err_msg += "get file stat failed, error: ";
            *err_msg += strerror(errno);
        }
        return nullptr;
    }

    if (file_stat.st_size == 0)
    {
        ftruncate(fd, mem_size);
        is_exist = false;
    }

    auto* file_buf = static_cast<char*>(mmap(nullptr, mem_size, PROT_WRITE, MAP_SHARED, fd, SEEK_SET));
    if (file_buf == MAP_FAILED)
    {
        if (err_msg)
        {
            *err_msg += "map file_write fail, error: ";
            *err_msg += strerror(errno);
        }
        return nullptr;
    }

    // 映射建立后文件可关闭
    close(scoped_fd.Release());

    if (msync(file_buf, mem_size, MS_SYNC) == -1)
    {
        if (err_msg)
        {
            *err_msg += "msync failed, error: ";
            *err_msg += strerror(errno);
        }
        munmap(file_buf, mem_size);
        return nullptr;
    }

    return file_buf;
}

int GetFileFd(const std::string& file_path, bool allow_create, std::string* err_msg)
{
    uint32_t flag = 0;
    if (EnsureDir(file_path, allow_create, flag, err_msg) != 0)
        return -1;

    int fd = open(file_path.c_str(), static_cast<int>(flag), 0666);
    if (fd < 0 && err_msg)
    {
        *err_msg += "open file failed, error: ";
        *err_msg += strerror(errno);
    }
    return fd;
}

}  // namespace ua::utils
