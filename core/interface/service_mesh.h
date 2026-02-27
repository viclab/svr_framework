/// @file service_mesh.h
/// @brief 服务网格/服务发现抽象接口（C++20 重写版）
#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace ua
{

class IServiceMesh
{
public:
    // svr_type, inst_id, attr
    using OnlineCaller = std::function<void(uint32_t, uint32_t, const std::map<std::string, std::string>&)>;
    virtual void SetOnlineCaller(OnlineCaller&& caller) = 0;

    // svr_type, inst_id
    using OfflineCaller = std::function<void(uint32_t, uint32_t)>;
    virtual void SetOfflineCaller(OfflineCaller&& caller) = 0;

    // svr_type, inst_id, attr
    using AttrChangeCaller = std::function<void(uint32_t, uint32_t, const std::map<std::string, std::string>&)>;
    virtual void AddAttrChangeCaller(AttrChangeCaller&& caller) = 0;

    virtual bool OnlineInst(uint32_t my_inst_id) = 0;
    virtual bool OfflineInst() = 0;
    [[nodiscard]] virtual uint32_t Process() = 0;

    struct AttrItem
    {
        std::string key;
        std::string value;
        bool insure = false;
    };

    virtual void SetAttr(const std::string& key, const std::string& value, bool insure) = 0;
    virtual void SetAttr(const std::vector<AttrItem>& attr_list) = 0;
    [[nodiscard]] virtual bool GetAttr(uint32_t inst_id, std::map<std::string, std::string>& attr) const = 0;
    [[nodiscard]] virtual bool GetAttr(uint32_t inst_id, const std::string& key, std::string& value) const = 0;

    virtual ~IServiceMesh() = default;
};

}  // namespace ua
