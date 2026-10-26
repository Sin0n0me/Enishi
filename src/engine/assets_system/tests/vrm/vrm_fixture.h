#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace enishi::tests {
    class VRMFixture {
      public:
        std::string extension;
        std::string required_extensions;
        std::string buffer_uri;

      private:
        std::filesystem::path directory;
        std::filesystem::path path;

      public:
        explicit VRMFixture(bool legacy);
        ~VRMFixture();
        VRMFixture(const VRMFixture&) = delete;
        VRMFixture& operator=(const VRMFixture&) = delete;
        const std::filesystem::path& write();

      private:
        static std::vector<std::uint8_t> binary();
        std::string document() const;
    };
} // namespace enishi::tests
