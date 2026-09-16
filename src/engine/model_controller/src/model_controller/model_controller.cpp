#pragma once
#include "model_controller.h"
#include <foundation/log/logger.h>
#include <foundation/path/path_utility.h>
#include <algorithm>
#include <regex>

namespace enishi::model_controller {
    ModelController::ModelController(std::shared_ptr<platform::IAssetSystem> asset_system,
        std::shared_ptr<ModelRenderDataBuilder> builder) noexcept
        : asset_system(asset_system)
        , builder(builder) {
    }

    void ModelController::find_model(const std::filesystem::path& search_path) noexcept {
        const auto model_paths =
            this->asset_system->find_assets(search_path, types::AssetKind::Model);
        const auto pattern_str = std::format(
            ".*{}", this->asset_system->get_extensions_pattern(types::AssetKind::Model));
        const std::regex pattern(pattern_str);

        const auto asset_paths = model_paths.find(pattern);
        if (asset_paths.empty()) {
            foundation::Logger::warning(
                std::format("モデルデータが見つかりません. path: {}", search_path.string<char>()));
            return;
        }

        for (const auto& asset_path : asset_paths) {
            this->model_list[asset_path.stem().string<char>()] = asset_path;
        }
    }

    std::vector<foundation::UTF8> ModelController::get_model_list(void) const noexcept {
        std::vector<foundation::UTF8> names;
        names.reserve(this->model_list.size());
        for (const auto& [name, path] : this->model_list) {
            names.emplace_back(name);
        }
        std::ranges::sort(names);
        return names;
    }

    foundation::Option<foundation::UTF8> ModelController::get_current_model_name(
        void) const noexcept {
        return this->current_model_name;
    }

    foundation::Option<types::RenderHandle> ModelController::get_current_model_render_handle(
        void) const noexcept {
        return this->current_model_render_handle;
    }

    foundation::Result<types::RenderHandle, ControlError> ModelController::change_model(
        const foundation::UTF8& name,
        const std::vector<types::RenderHandle>& shader_reflections) noexcept {
        const auto iter = this->model_list.find(name);
        if (iter == this->model_list.end()) {
            return foundation::Error(
                ControlError::NotFound, std::format("モデルが見つかりません: {}", name));
        }
        const auto& path = iter->second;

        auto load_result = this->asset_system->load_asset(path);
        if (load_result.is_err()) {
            return load_result.propagation(ControlError::LoadFailed);
        }
        const auto& model_handle = load_result.unwrap();

        // 描画データへの変換はModelRenderDataBuilderの責務
        auto build_result = this->builder->build(model_handle, shader_reflections);
        if (build_result.is_err()) {
            return foundation::Error(ControlError::BuildFailed, "描画データの作成に失敗しました");
        }

        this->current_model_name = name;
        this->current_model_render_handle = build_result.unwrap();
        return this->current_model_render_handle.unwrap();
    }

    void make_wall(void) {
        constexpr float WALL_SIZE = 30.0f;
        constexpr float HALF_WALL_SIZE = WALL_SIZE / 2.0f;
        constexpr float FRONT_DEPTH = 0.0f;
        constexpr float FLOOR_DEPTH = 1.5f; // 奥行(床面)
        constexpr float OFFSET_Y = 0.0f;

        auto model_data = std::make_shared<types::ModelData>();
        model_data->vertices = {
            types::VertexVariants{
                types::VertexPosition{
                    .position = {-HALF_WALL_SIZE, -HALF_WALL_SIZE, FRONT_DEPTH + FLOOR_DEPTH},
                },
            },
            types::VertexVariants{
                types::VertexPosition{
                    .position = {-HALF_WALL_SIZE, WALL_SIZE, FRONT_DEPTH + FLOOR_DEPTH},
                },
            },
            types::VertexVariants{
                types::VertexPosition{
                    .position = {HALF_WALL_SIZE, WALL_SIZE, FRONT_DEPTH + FLOOR_DEPTH},
                },
            },
            types::VertexVariants{
                types::VertexPosition{
                    .position = {HALF_WALL_SIZE, -HALF_WALL_SIZE, FRONT_DEPTH + FLOOR_DEPTH},
                },
            },
        };

        model_data->indices = {std::vector<std::uint16_t>{
            0,
            1,
            2,
            0,
            2,
            3,
        }};

        /*
        // メッシュ作成
        auto mesh_handle = renderer->create_mesh(*model_data, model_shader_reflections);
        if (mesh_handle.is_err()) {
            // return mesh_handle.propagation(core::SystemError::ConstructRenderPassError);
        }
        */
    }
} // namespace enishi::model_controller