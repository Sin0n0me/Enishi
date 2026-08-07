#include "pmd_to_model_data.h"
#include <foundation/log/logger.h>
#include <foundation/str/str.h>
#include <foundation/str/to_utf8.h>
#include <glm/gtc/matrix_transform.hpp>
#include <ranges>

namespace enishi::assets_system {
    foundation::Result<AssetModelData, AssetError> PMDToModelData::to_model_data(
        const std::filesystem::path& path, const PMDData& data) {
        const std::string sjis_name(
            reinterpret_cast<const char*>(data.model_name.data()), data.model_name.size());
        auto&& utf8_name = foundation::sjis_to_utf8(sjis_name);
        if (utf8_name.is_err()) {
            foundation::Logger::warning("utf8に変換できない文字が含まれています");
        }
        foundation::Logger::info(utf8_name.unwrap_or(sjis_name));

        const std::string sjis_comment(
            reinterpret_cast<const char*>(data.comment.data()), data.comment.size());
        auto&& utf8_comment = foundation::sjis_to_utf8(sjis_comment);
        if (utf8_comment.is_err()) {
            foundation::Logger::warning("utf8に変換できない文字が含まれています");
        }
        foundation::Logger::info(utf8_comment.unwrap_or(sjis_comment));

        const auto parent_path = path.parent_path();

        auto [bones, bone_resolver] = PMDToModelData::make_bone(data.bones);
        auto [morphs, morph_resolver] = PMDToModelData::make_morphs(data.morphs);

        return std::make_shared<types::ModelData>(types::ModelData{
            .name = utf8_name.unwrap_or(sjis_name),
            .path = path,
            .vertices = {PMDToModelData::make_vertices(data.vertices)},
            .indices = PMDToModelData::make_indices(data.indices),
            .addons =
                {
                    std::move(bones),
                    std::move(morphs),
                    PMDToModelData::make_iks(data.iks, &bone_resolver),
                    PMDToModelData::make_rigid_bodies(data.rigid_bodies),
                    PMDToModelData::make_joints(data.physics_joints),
                },
            .materials =
                PMDToModelData::make_materials(parent_path, data.materials, data.toon_textures),
        });
        ;
    }

    std::tuple<std::vector<types::Bone>, BoneResolver> PMDToModelData::make_bone(
        const std::vector<PMDBone>& bones) {
        constexpr std::uint32_t MMD_NONE_PARENT = 0xFFFF;
        const auto bone_size = bones.size();
        auto bone_data = std::vector<types::Bone>(bone_size);
        BoneNameMapConstructor constructor;

        for (size_t i = 0; i < bone_size; ++i) {
            const auto& src_bone = bones[i];
            auto& dst_bone = bone_data[i];

            // 名前の変換
            // 変換できない場合は仕方ないのでそのまま保持
            const std::string sjis_name(src_bone.name, sizeof(src_bone.name));
            const auto utf8_name = foundation::sjis_to_utf8(sjis_name);
            if (utf8_name.is_err()) {
                foundation::Logger::warning("utf8に変換できない文字が含まれています");
            }
            constructor.bone_names.push_back(utf8_name.unwrap_or(sjis_name));

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
                const auto& parent = bone_data[parent_index];
                dst_bone.bind_bone.local = translate - parent.bind_bone.local;
            }
        }

        // 親を参照するので一度ローカル行列を全て作成してからグローバル作成
        for (size_t bone_index = 0; bone_index < bone_size; ++bone_index) {
            const auto& src_bone = bones[bone_index];
            auto& dst_bone = bone_data[bone_index];

            // ボーンノードとグローバル行列の作成
            const auto parent_index = src_bone.parent_index;
            if (parent_index == MMD_NONE_PARENT) {
                dst_bone.bind_bone.global = dst_bone.bind_bone.local;
                dst_bone.bone_node.parent = types::INVALID_BONE_INDEX;
            } else {
                auto& parent = bone_data[parent_index];
                dst_bone.bind_bone.global = parent.bind_bone.global * dst_bone.bind_bone.local;
                dst_bone.bone_node.parent = parent_index;
                parent.bone_node.children.push_back(bone_index);
            }

            // 逆変換
            dst_bone.bind_bone.global_inverse = glm::inverse(dst_bone.bind_bone.global);
        }

        return {bone_data, BoneResolver(constructor)};
    }

    std::vector<types::VertexVariants> PMDToModelData::make_vertices(
        const std::vector<PMDVertex>& vertices) {
        const auto vertex_size = vertices.size();

        std::vector<types::VertexVariants> skinning_vertices;
        skinning_vertices.reserve(vertex_size);
        for (size_t i = 0; i < vertex_size; ++i) {
            const auto& vertex = vertices[i];

            // 0.0~1.0に正規化
            const float weight = static_cast<float>(vertex.bone_weight) / 100.0f;
            const glm::vec2 bone_weight{weight, 1.0f - weight};

            skinning_vertices.emplace_back(types::VertexVariants{
                types ::Vertex{
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

    types::AddonIKs PMDToModelData::make_iks(
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
                            .axis = PMDToModelData::MMD_KNEE_AXIS,
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

    std::tuple<types::AddonMorphs, MorphResolver> PMDToModelData::make_morphs(
        const std::vector<PMDMorph>& morphs) {
        types::AddonMorphs model_morphs;
        MorphNameMapConstructor constructor;
        const auto size = morphs.size();
        const auto transform = [](const PMDMorphVertex& v) {
            return types::MorphVertex{
                .index = v.index,
                .offset =
                    glm::vec3{
                        v.position[0],
                        v.position[1],
                        v.position[2],
                    },
            };
        };

        // ベースの作成
        model_morphs.base_vertices =
            morphs[0].vertices | std::views::transform(transform) | std::ranges::to<std::vector>();

        // indexが0はベースの頂点群なので開始は1
        for (std::size_t i = 1; i < size; ++i) {
            const auto& morph = morphs[i];

            // 名前の変換
            // 変換できない場合は仕方ないのでそのまま保持
            const std::string sjis_name(morph.name, sizeof(morph.name));
            const auto utf8_name = foundation::sjis_to_utf8(sjis_name);
            if (utf8_name.is_err()) {
                foundation::Logger::warning("utf8に変換できない文字が含まれています");
            }
            constructor.morph_names.push_back(utf8_name.unwrap_or(sjis_name));

            // モーフで扱う頂点を共通の型に変換
            const auto vertices =
                morph.vertices | std::views::transform(transform) | std::ranges::to<std::vector>();

            model_morphs.vertices.emplace_back(vertices);
        }

        return {model_morphs, MorphResolver(constructor)};
    }

    types::AddonPhysicsJoints PMDToModelData::make_joints(
        const std::vector<PMDPhysicsJoint>& joints) {
        const auto size = joints.size();
        auto model_joints = std::vector<types::PhysicsJoint>(size);

        for (const auto& joint : joints) {
            joint;

            types::PhysicsJoint{};
        }

        return model_joints;
    }

    types::AddonRigidBodies PMDToModelData::make_rigid_bodies(
        const std::vector<PMDRigidBody>& rigid_bodies) {
        const auto size = rigid_bodies.size();
        auto model_rigid_bodies = types::AddonRigidBodies(size);

        for (const auto& rigid_body : rigid_bodies) {
            const auto offset = PMDToModelData::make_offset_from_pmd(rigid_body);
            const auto shape = PMDToModelData::make_shape_from_pmd(rigid_body);
            const auto rigid_body_type = PMDToModelData::make_rigid_body_type_from_pmd(rigid_body);
            const bool is_kinematic = rigid_body_type == types::RigidBodyType::Kinematic;
            const float mass = is_kinematic ? 0.0f : rigid_body.mass;

            const auto rb = types::RigidBody{
                .group = rigid_body.group_index,
                .group_mask = rigid_body.group_target,
                .rigid_body_type = rigid_body_type,
                .shape = shape,
                .offset = offset,
                .mass = mass,
                .position =
                    glm::vec3{
                        rigid_body.position[0],
                        rigid_body.position[1],
                        rigid_body.position[2],
                    },
                .rotation =
                    glm::vec3{
                        rigid_body.rotation[0],
                        rigid_body.rotation[1],
                        rigid_body.rotation[2],
                    },
                .linear_damping = rigid_body.linear_damping,
                .angular_damping = rigid_body.angular_damping,
                .restitution = rigid_body.restitution,
                .friction = rigid_body.friction,
            };
            model_rigid_bodies.emplace_back(rb);
        }

        return model_rigid_bodies;
    }

    std::vector<types::Material> PMDToModelData::make_materials(
        const std::filesystem::path& model_path,
        const std::vector<PMDMaterial>& pmd_materials,
        const PMDToonTexture& toon_textures) {
        std::vector<types::Material> materials;

        for (const auto& pmd_material : pmd_materials) {
            types::Material material{
                .count = pmd_material.index_count,
            };

            material.variants.emplace_back(types::Ambient{
                .color =
                    glm::vec3{
                        pmd_material.ambient[0],
                        pmd_material.ambient[1],
                        pmd_material.ambient[2],
                    },
            });
            material.variants.emplace_back(types::Diffuse{
                .color =
                    glm::vec4{
                        pmd_material.diffuse[0],
                        pmd_material.diffuse[1],
                        pmd_material.diffuse[2],
                        pmd_material.diffuse[3],
                    },
            });
            material.variants.emplace_back(types::Specular{
                .color =
                    glm::vec3{
                        pmd_material.specular[0],
                        pmd_material.specular[1],
                        pmd_material.specular[2],
                    },
                .shininess = pmd_material.shininess,
            });

            std::vector<std::filesystem::path> paths{};

            auto normalized_path = [](const std::string& path) {
                return std::filesystem::path{path}.lexically_normal();
            };

            // 使用するテクスチャパスのセット
            const auto toon_index = pmd_material.toon_index;
            if (toon_index < PMDToonTexture::MAX_FILE_COUNT) {
                paths.emplace_back(
                    model_path / normalized_path(toon_textures.file_names[toon_index]));
            }

            // スフィアがついている場合があるので分離
            auto texture_path = std::string{pmd_material.texture_file};
            const auto pos = texture_path.find('*');
            if (pos == std::string::npos) {
                // スフィアがない場合
                paths.emplace_back(model_path / normalized_path(texture_path));
            } else {
                // スフィア付きの場合
                auto texture = model_path / normalized_path(texture_path.substr(0, pos));
                auto sphere = model_path / normalized_path(texture_path.substr(pos + 1));

                paths.emplace_back(texture);
                paths.emplace_back(sphere);
            }

            material.texture_paths = std::move(paths);

            materials.emplace_back(material);
        }

        return materials;
    }

    // PMDはボーンとの相対座標なので剛体中心とのオフセットは以下で求める(列優先の場合)
    // Offset = T * R
    // PMXの場合はモデル座標での数値なので以下で求める(列優先の場合)
    // Offset = Inverse(global) * T * R
    glm::mat4 PMDToModelData::make_offset_from_pmd(const PMDRigidBody& rigid_body) {
        const glm::mat4 ident = glm::mat4(1.0f);
        const glm::vec3 rotate = glm::vec3{
            rigid_body.rotation[0],
            rigid_body.rotation[1],
            rigid_body.rotation[2],
        };
        const glm::mat4 rx = glm::rotate(ident, rotate.x, glm::vec3{1, 0, 0});
        const glm::mat4 ry = glm::rotate(ident, rotate.y, glm::vec3{0, 1, 0});
        const glm::mat4 rz = glm::rotate(ident, rotate.z, glm::vec3{0, 0, 1});
        const glm::mat4 rotate_matrix = ry * rx * rz;
        const glm::mat4 translate_matrix = glm::translate(ident,
            glm::vec3{
                rigid_body.position[0],
                rigid_body.position[1],
                rigid_body.position[2],
            });
        const glm::mat4 offset = translate_matrix * rotate_matrix;

        return offset;
    }

    types::RigidBodyShape PMDToModelData::make_shape_from_pmd(const PMDRigidBody& rigid_body) {
        switch (rigid_body.shape_type) {
            case PMDShapeType::Sphere: {
                return types::ShapeSphere{
                    .radius = rigid_body.shape_size[0],
                };
            }
            case PMDShapeType::Box: {
                return types::ShapeBox{
                    .width = rigid_body.shape_size[0],
                    .height = rigid_body.shape_size[1],
                    .depth = rigid_body.shape_size[2],
                };
            }
            case PMDShapeType::Capsule: {
                return types::ShapeCapsule{
                    .radius = rigid_body.shape_size[0],
                    .height = rigid_body.shape_size[1],
                };
            }
            default:
                break;
        }

        return types::RigidBodyShape();
    }

    types::RigidBodyType PMDToModelData::make_rigid_body_type_from_pmd(
        const PMDRigidBody& rigid_body) {
        switch (rigid_body.rigid_body_type) {
            case PMDRigidBodyType::FollowBone:
                return types::RigidBodyType::Kinematic;
            case PMDRigidBodyType::PhysicsSimulation:
                return types::RigidBodyType::Dynamic;
            case PMDRigidBodyType::PhysicsSimulationAndBoneAlignment:
                return types::RigidBodyType::DynamicAdjustBone;
            default:
                break;
        }

        return types::RigidBodyType::Dynamic;
    }
} // namespace enishi::assets_system