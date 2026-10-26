#include "vrm_model_loader.h"
#include "../../../utility/binary_reader.h"
#include "vrm_context.h"
#include "vrm_expression.h"
#include "vrm_mesh.h"
#include "vrm_schema.h"
#include "vrm_spring.h"
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

namespace enishi::assets_system {
    namespace {
        bool validate_scene(VRMContext& context) {
            const auto& data = context.source;
            if (data.scene == nullptr || data.nodes_count == 0 || data.buffers_count != 1 ||
                data.buffers[0].uri != nullptr || data.file_type != cgltf_file_type_glb) {
                return context.fail("VRM requires a GLB scene with one embedded buffer");
            }
            // Check ancestry before cgltf traverses parents to compute world transforms.
            for (std::size_t index = 0; index < data.nodes_count; ++index) {
                const auto& node = data.nodes[index];
                for (std::size_t child = 0; child < node.children_count; ++child) {
                    if (node.children[child]->parent != &node) {
                        return context.fail("VRM node is attached to multiple parents");
                    }
                }
                const auto* parent = data.nodes[index].parent;
                std::size_t depth{};
                while (parent != nullptr) {
                    if (!(++depth < data.nodes_count)) {
                        return context.fail("Cyclic VRM node hierarchy");
                    }
                    parent = parent->parent;
                }
            }
            context.active_nodes.resize(data.nodes_count);
            std::vector<const cgltf_node*> pending;
            for (std::size_t index = 0; index < data.scene->nodes_count; ++index) {
                pending.push_back(data.scene->nodes[index]);
            }
            while (!pending.empty()) {
                const auto* node = pending.back();
                pending.pop_back();
                const auto index = static_cast<std::size_t>(node - data.nodes);
                if (context.active_nodes[index]) {
                    return context.fail("Duplicate VRM scene node");
                }
                context.active_nodes[index] = true;
                for (std::size_t child = 0; child < node->children_count; ++child) {
                    pending.push_back(node->children[child]);
                }
            }
            return true;
        }

        bool supported_extensions(VRMContext& context) {
            const std::vector<std::string_view> supported{vrm_schema::extension::VRM_0,
                vrm_schema::extension::VRM_1,
                vrm_schema::extension::SPRING_BONE,
                vrm_schema::extension::MTOON,
                vrm_schema::extension::KHR_MATERIALS_UNLIT,
                vrm_schema::extension::KHR_TEXTURE_TRANSFORM,
                vrm_schema::extension::KHR_MESH_QUANTIZATION,
                vrm_schema::extension::KHR_MATERIALS_EMISSIVE_STRENGTH};
            for (std::size_t index = 0; index < context.source.extensions_required_count; ++index) {
                const auto name = std::string_view(context.source.extensions_required[index]);
                if (std::ranges::find(supported, name) == supported.end()) {
                    return context.fail(
                        "Unsupported required VRM/glTF extension: " + std::string(name));
                }
            }
            return true;
        }
    } // namespace

    foundation::Result<ModelVariant, AssetError> VRMModelLoader::load(
        const std::filesystem::path& path) noexcept {
        auto reader = BinaryReader::make_reader(path);
        if (reader.is_err()) {
            return reader.propagation(AssetError::IOError);
        }
        auto bytes = reader.unwrap_mut().read_all();
        if (bytes.is_err()) {
            return bytes.propagation(AssetError::IOError);
        }
        cgltf_options options{};
        cgltf_data* parsed = nullptr;
        const auto& buffer = bytes.unwrap();
        const auto status = cgltf_parse(&options, buffer.data(), buffer.size(), &parsed);
        std::unique_ptr<cgltf_data, decltype(&cgltf_free)> data(parsed, cgltf_free);
        if (status != cgltf_result_success) {
            return foundation::Error(AssetError::InvalidAssetData, "Invalid VRM GLB data");
        }
        auto model = std::make_shared<types::ModelData>();
        model->path = path;
        const auto name = path.stem().u8string();
        model->name.assign(name.begin(), name.end());
        VRMContext context{*data, *model};
        if (!supported_extensions(context)) {
            return foundation::Error(AssetError::UnsupportedFeature, context.error);
        }
        context.extension = vrm_extension(
            data->data_extensions, data->data_extensions_count, vrm_schema::extension::VRM_1);
        if (!context.extension.valid) {
            context.legacy = true;
            context.extension = vrm_extension(
                data->data_extensions, data->data_extensions_count, vrm_schema::extension::VRM_0);
        }
        if (!context.extension.valid ||
            (!context.legacy && context.extension[vrm_schema::key::SPEC_VERSION].value !=
                                    vrm_schema::value::SPEC_VERSION_1)) {
            return foundation::Error(AssetError::UnsupportedFeature, "Expected VRM 0.x or VRM 1.0");
        }
        // VRM 0 faces -Z; VRM 1 faces +Z. Match MMD's front (-Z), in model units.
        const float facing = context.legacy ? 1.0f : -1.0f;
        context.basis =
            glm::scale(glm::mat4(1), glm::vec3(facing, 1, facing) * VRM_MODEL_UNITS_PER_METER);
        if (!validate_scene(context)) {
            return foundation::Error(AssetError::InvalidAssetData, context.error);
        }
        if (cgltf_load_buffers(&options, data.get(), nullptr) != cgltf_result_success ||
            cgltf_validate(data.get()) != cgltf_result_success) {
            return foundation::Error(
                AssetError::InvalidAssetData, "Invalid VRM buffer or accessor");
        }
        if (!make_vrm_bones(context) || !make_vrm_meshes(context) ||
            !make_vrm_expressions(context) || !make_vrm_springs(context)) {
            return foundation::Error(AssetError::InvalidAssetData, context.error);
        }
        model->addons.emplace_back(std::move(context.targets));
        return ModelVariant{std::move(model)};
    }

    foundation::UTF8 VRMModelLoader::get_supported_extension() const noexcept {
        return ".vrm";
    }
} // namespace enishi::assets_system
