#pragma once
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace enishi::assets_system {
    // Internal VRM extension representation; no parser types cross the assets boundary.
    struct VRMJson {
        std::string value;
        std::map<std::string, VRMJson> members;
        std::vector<VRMJson> elements;
        bool valid{};

        static VRMJson parse(const char* text);
        const VRMJson& operator[](std::string_view key) const;
        float number(float fallback = 0) const;
        std::size_t index() const;
        bool boolean() const;
    };
} // namespace enishi::assets_system
