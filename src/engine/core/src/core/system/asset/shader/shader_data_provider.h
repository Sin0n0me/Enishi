#pragma once
#include <engine_types/assets/shader/shader_data.h>
#include <filesystem>
#include <foundation/option/option.h>
#include <foundation/result/result.h>
#include <foundation/str/str.h>
#include <platform/asset/interface_asset_system.h>
#include <platform/asset/interface_shader_data_provider.h>

namespace enishi::core {
    class ShaderDataProvider final : public platform::IShaderDataProvider {
      private:
        std::shared_ptr<platform::IAssetSystem> asset_system;
        std::vector<int> cache;

      public:
        explicit ShaderDataProvider(std::shared_ptr<platform::IAssetSystem> asset_system);

        foundation::Result<std::vector<platform::ShaderDataEntry>, platform::RenderError> get(
            std::span<const std::tuple<types::ShaderKind, std::filesystem::path>> paths)
            const override;
    };
} // namespace enishi::core