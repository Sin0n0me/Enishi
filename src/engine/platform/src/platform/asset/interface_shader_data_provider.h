#pragma once
#include <engine_types/assets/asset_data.h>
#include <engine_types/assets/shader/shader_data.h>
#include <engine_types/assets/shader/shader_kind.h>
#include <filesystem>
#include <foundation/result/result.h>
#include <foundation/str/str.h>
#include <platform/errors/renderer_errors.h>
#include <span>
#include <unordered_map>
#include <vector>

namespace enishi::platform {
    struct ShaderDataEntry {
        types::ShaderKind kind;
        std::filesystem::path path;
        types::AssetShaderData data;
    };

    class IShaderDataProvider {
      public:
        virtual ~IShaderDataProvider(void) noexcept = default;

        [[nodiscard]]
        virtual foundation::Result<std::vector<ShaderDataEntry>, RenderError> get(
            std::span<const std::tuple<types::ShaderKind, std::filesystem::path>> paths) const = 0;
    };
} // namespace enishi::platform