#include "pmd_to_model_data.h"
#include <foundation/log/logger.h>
#include <foundation/str/str.h>
#include <foundation/str/to_utf8.h>
#include <glm/gtc/matrix_transform.hpp>

namespace enishi::assets_system {
    foundation::Result<types::ModelData, AssetError> PMDToModelData::to_model_data(
        const PMDData& data) {
        const std::string sjis_name(data.model_name.data(), data.model_name.size());
        const auto utf8_name = foundation::sjis_to_utf8(sjis_name);
        if (utf8_name.is_err()) {
            foundation::Logger::warning("utf8に変換できない文字が含まれています");
        }
        const std::string sjis_comment(data.comment.data(), data.comment.size());
        const auto utf8_comment = foundation::sjis_to_utf8(sjis_comment);
        foundation::Logger::info(utf8_comment.value_or(sjis_comment));

        auto bones = PMDToModelData::make_bone(data.bones);
        if (bones.is_err()) {
            return bones.error();
        }
        const auto& bone_resolver = bones.value();

        auto vertices = PMDToModelData::make_vertices(data.vertices);
        auto indices = PMDToModelData::make_indices(data.indices);
        auto iks = PMDToModelData::make_iks(data.iks, &bone_resolver);

        return types::ModelData{
            .name = utf8_name.value_or(sjis_name),
            .vertices = types::OwnedRenderData<types::SkinningVertex>(std::move(vertices)),
            .indices = types::OwnedRenderData<std::uint16_t>(std::move(indices)),
            .addons = {std::move(iks)},
        };
    }

    foundation::Result<BoneResolver, AssetError> PMDToModelData::make_bone(
        const std::vector<PMDBone>& bones) {
        constexpr std::uint32_t MMD_NONE_PARENT = 0xFFFF;
        auto bone_data = std::make_shared<BoneData>();

        const auto bone_size = bones.size();
        bone_data->bones.resize(bone_size);
        for (size_t i = 0; i < bone_size; ++i) {
            const auto& src_bone = bones[i];
            auto& dst_bone = bone_data->bones[i];

            // 名前の変換
            // 変換できない場合は仕方ないのでそのまま保持
            const std::string sjis_name(src_bone.name, sizeof(src_bone.name));
            const auto utf8_name = foundation::sjis_to_utf8(sjis_name);
            if (utf8_name.is_err()) {
                foundation::Logger::warning("utf8に変換できない文字が含まれています");
            }
            bone_data->bones[i].name = utf8_name.value_or(sjis_name);

            // ローカル行列作成
            const glm::vec3 position = {
                src_bone.position[0],
                src_bone.position[1],
                src_bone.position[2],
            };
            const glm::mat4 translate = glm::translate(glm::mat4(1.0f), position);
            const auto parent_index = src_bone.parent_index;
            if (parent_index == MMD_NONE_PARENT) {
                dst_bone.bind_bone.local = translate;
            } else {
                const auto& parent = bone_data->bones[parent_index];
                dst_bone.bind_bone.local = translate - parent.bind_bone.local;
            }
        }

        // 親を参照するので一度ローカル行列を全て作成してからグローバル作成
        for (size_t bone_index = 0; bone_index < bone_size; ++bone_index) {
            const auto& src_bone = bones[bone_index];
            auto& dst_bone = bone_data->bones[bone_index];

            // ボーンノードとグローバル行列の作成
            const auto parent_index = src_bone.parent_index;
            if (parent_index == MMD_NONE_PARENT) {
                dst_bone.bind_bone.global = dst_bone.bind_bone.local;
                dst_bone.bone_node.parent = types::INVALID_BONE_INDEX;
            } else {
                auto& parent = bone_data->bones[parent_index];
                dst_bone.bind_bone.global = parent.bind_bone.global * dst_bone.bind_bone.local;
                dst_bone.bone_node.parent = parent_index;
                parent.bone_node.children.push_back(bone_index);
            }

            // 逆変換
            dst_bone.bind_bone.global_inverse = glm::inverse(dst_bone.bind_bone.global);
        }

        // 作成
        bone_data->build_name_index();

        return BoneResolver(bone_data);
    }

    std::vector<types::SkinningVertex> PMDToModelData::make_vertices(
        const std::vector<PMDVertex>& vertices) {
        const auto vertex_size = vertices.size();
        std::vector<types::SkinningVertex> skinning_vertices(vertex_size);
        for (size_t i = 0; i < vertex_size; ++i) {
            const auto& vertex = vertices[i];

            // 0.0~1.0に正規化
            const float weight = static_cast<float>(vertex.bone_weight) / 100.0f;
            const glm::vec2 bone_weight{weight, 1.0f - weight};

            skinning_vertices.emplace_back(types::SkinningVertex{
                .vertex =
                    types::Vertex{
                        .position =
                            glm::vec3{
                                vertex.position[0],
                                vertex.position[1],
                                vertex.position[2],
                            },
                        .normal =
                            glm::vec3{
                                vertex.normal[0],
                                vertex.normal[1],
                                vertex.normal[2],
                            },
                        .uv =
                            glm::vec2{
                                vertex.uv[0],
                                vertex.uv[1],
                            },
                    },
                .skinning =
                    types::Skinning{
                        .bone_index =
                            glm::u16vec2{
                                vertex.bone_index[0],
                                vertex.bone_index[1],
                            },
                        .bone_weight = bone_weight,
                    },
            });
        }

        return skinning_vertices;
    }

    std::vector<std::uint16_t> PMDToModelData::make_indices(
        const std::vector<PMDVertexIndex>& indices) {
        const auto index_size = indices.size();
        std::vector<std::uint16_t> vertex_indices(index_size);
        for (size_t i = 0; i < index_size; ++i) {
            vertex_indices[i] = indices[i].index;
        }

        return vertex_indices;
    }

    std::vector<types::IK> PMDToModelData::make_iks(
        const std::vector<PMDIK>& iks, const IBoneResolver* bone_resolver) {
        const auto ik_size = iks.size();
        std::vector<types::IK> ik_vec(ik_size);

        for (const auto& ik : iks) {
            const auto ccdik = types::CCDIK{
                .iterations = ik.iterations,
                .target = ik.target_bone,
                .ik_bone = ik.ik_bone,
                .chain = ik.chain,
                .limit = ik.limit,
            };
            types::IK convert_ik{};

            const auto bone_name = bone_resolver->resolve_name(ik.ik_bone);
            const auto condition = [](const foundation::UTF8& name) -> std::optional<bool> {
                if (name.contains("膝") || name.contains("ひざ")) {
                    return true; // has_value()がtrueになるなら何を返してもいい
                }
                return {};
            };
            const bool is_limited_bone = bone_name.and_then(condition).has_value();

            if (is_limited_bone) {
                convert_ik.method = types::LimitedCCDIK{
                    .limit =
                        types::IKLimit{
                            .axis = MMD_KNEE_AXIS,
                        },
                    .ccdik = ccdik,
                };
            } else {
                convert_ik.method = ccdik;
            }

            ik_vec.emplace_back(convert_ik);
        }

        return ik_vec;
    }
} // namespace enishi::assets_system