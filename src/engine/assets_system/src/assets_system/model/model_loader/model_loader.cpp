#include "model_loader.h"
#include "../bone/bone_data.h"
#include "../bone_resolver.h"
#include "pmd/pmd_model_loader.h"
#include "pmd/pmd_to_model_data.h"

namespace enishi::assets_system {
    ModelLoader::ModelLoader(void) {
        std::vector<std::unique_ptr<IModelLoader>> loaders;
        loaders.push_back(std::make_unique<PMDModelLoader>());

        for (auto& element : loaders) {
            if (bool(element)) {
                auto extension = element->get_supported_extension();
                this->loaders.emplace(std::move(extension), std::move(element));
            }
        }
    }

    foundation::Result<AssetData, AssetError> ModelLoader::load(
        const std::filesystem::path& path) noexcept {
        if (!path.has_extension()) {
            return foundation::Error(AssetError::NotFound, "不明なファイルです");
        }

        const auto extension = path.extension().string<char>();
        auto iter = this->loaders.find(extension);
        if (iter == this->loaders.end()) {
            return foundation::Error(AssetError::NotFound, "対応していないファイル形式です");
        }

        auto load_data = iter->second->load(path);
        if (load_data.is_err()) {
            return load_data.error();
        }
        auto& model_data = load_data.value();

        if (auto pmd_data = std::get_if<std::unique_ptr<PMDData>>(&model_data)) {
            auto convert_data = PMDToModelData::to_model_data(*pmd_data->get());
            if (convert_data.is_err()) {
                return convert_data.add_message("データの変換に失敗しました").error();
            }

            return convert_data.value();
        }

        // 仮
        return foundation::Error(AssetError::NotFound);
    }

    std::vector<foundation::UTF8> ModelLoader::get_supported_extension(void) const noexcept {
        std::vector<foundation::UTF8> extensions(this->loaders.size());

        for (const auto& [extension, _] : this->loaders) {
            extensions.push_back(extension);
        }

        return extensions;
    }

    AssetType ModelLoader::get_target_asset_type(void) const noexcept {
        return AssetType::Model;
    }
} // namespace enishi::assets_system