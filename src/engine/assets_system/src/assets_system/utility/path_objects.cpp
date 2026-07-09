#include "path_objects.h"

namespace enishi::assets_system {
    void PathObjects::add(std::filesystem::path& path) {
        const auto normalized_path = path.lexically_normal();
        this->path_entries.emplace_back(PathObjects::PathEntry{
            .path = normalized_path,
            .text = normalized_path.string<char>(),
        });
    }

    std::vector<std::filesystem::path> PathObjects::find(const std::regex& pattern) const {
        auto filtered = this->path_entries | std::views::filter([&](const PathEntry& entry) {
            return std::regex_match(entry.text, pattern);
        }) | std::views::transform([](const PathEntry& entry) { return entry.path; });

        std::vector<std::filesystem::path> result;
        std::ranges::copy(filtered, std::back_inserter(result));

        return result;
    }
} // namespace enishi::assets_system