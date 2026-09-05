#pragma once
#include <filesystem>
#include <ranges>
#include <regex>
#include <string>
#include <unordered_set>

namespace enishi::assets_system {
    class PathObjects {
      private:
        struct PathEntry {
            std::filesystem::path path;
            std::string text;
        };

      private:
        std::vector<PathEntry> path_entries;

      public:
        PathObjects(void);

        void add(const std::filesystem::path& path);

        std::vector<std::filesystem::path> find(const std::regex& pattern) const;
    };
} // namespace enishi::assets_system