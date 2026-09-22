#include "pmx_to_model_data.h"
#include "pmx_model_loader.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <engine_types/renderer/texture/model_texture.h>
#include <engine_types/renderer/uniform_buffer/material.h>
#include <format>
#include <glm/gtc/matrix_transform.hpp>
#include <limits>
#include <ranges>
#include <string>

namespace enishi::assets_system {
    namespace {
        constexpr std::int32_t NO_PMX_INDEX{-1};
        constexpr std::uint8_t PMX_DEFORM_BDEF4{2};
        constexpr std::uint8_t PMX_DEFORM_QDEF{4};
        constexpr std::uint16_t BONE_FLAG_INHERIT_ROTATION{0x0100};
        constexpr std::uint16_t BONE_FLAG_INHERIT_TRANSLATION{0x0200};
        constexpr std::uint16_t BONE_FLAG_LOCAL_TRANSFORM{0x0080};
        constexpr std::uint16_t BONE_FLAG_FIXED_AXIS{0x0400};
        constexpr std::uint16_t BONE_FLAG_LOCAL_COORDINATE{0x0800};
        constexpr std::uint16_t BONE_FLAG_AFTER_PHYSICS{0x1000};
        constexpr std::uint16_t BONE_FLAG_IK{0x0020};
        constexpr std::uint8_t MORPH_TYPE_GROUP{};
        constexpr std::uint8_t MORPH_TYPE_VERTEX{1};
        constexpr std::uint8_t MORPH_TYPE_BONE{2};
        constexpr std::uint8_t MORPH_TYPE_MATERIAL{8};
        constexpr std::uint8_t MORPH_TYPE_FLIP{9};
        constexpr std::uint8_t MORPH_TYPE_IMPULSE{10};
        constexpr std::uint8_t MATERIAL_FLAG_DRAW_BOTH_SIDES{0x01};
        constexpr std::uint8_t MATERIAL_FLAG_GROUND_SHADOW{0x02};
        constexpr std::uint8_t MATERIAL_FLAG_CAST_SHADOW{0x04};
        constexpr std::uint8_t MATERIAL_FLAG_RECEIVE_SHADOW{0x08};
        constexpr std::uint8_t MATERIAL_FLAG_DRAW_EDGE{0x10};
        constexpr std::uint8_t MATERIAL_FLAG_VERTEX_COLOR{0x20};
        constexpr std::uint8_t MATERIAL_FLAG_POINT_TOPOLOGY{0x40};
        constexpr std::uint8_t MATERIAL_FLAG_LINE_TOPOLOGY{0x80};
        constexpr std::uint8_t SPHERE_MODE_DISABLED{};
        constexpr std::uint8_t SPHERE_MODE_MULTIPLY{1};
        constexpr std::uint8_t SPHERE_MODE_ADD{2};
        constexpr std::uint8_t SPHERE_MODE_SUB_TEXTURE{3};
        constexpr std::uint8_t RIGID_BODY_MODE_KINEMATIC{};
        constexpr std::uint8_t RIGID_BODY_MODE_DYNAMIC{1};
        constexpr std::uint8_t RIGID_BODY_SHAPE_SPHERE{};
        constexpr std::uint8_t RIGID_BODY_SHAPE_BOX{1};
        constexpr std::uint8_t JOINT_TYPE_SPRING_SIX_DOF{};
        constexpr std::uint8_t JOINT_TYPE_WITH_CONSTRAINTS_END{2};
        constexpr std::uint8_t JOINT_TYPE_CONE_TWIST{3};
        constexpr std::uint8_t JOINT_TYPE_SLIDER{4};
        constexpr std::uint8_t JOINT_TYPE_HINGE{5};
        constexpr std::uint8_t SOFT_BODY_SHAPE_TRIANGLE_MESH{};
        constexpr std::uint8_t SOFT_BODY_FLAG_BENDING{1};
        constexpr std::uint8_t SOFT_BODY_FLAG_CLUSTER{2};
        constexpr std::uint8_t SOFT_BODY_FLAG_RANDOMIZE{4};

        foundation::Result<void, AssetError> validate_rigid_body_support(const PMXData& data) {
            for (const auto& body : data.rigid_bodies) {
                if (body.bone == NO_PMX_INDEX) {
                    return foundation::Error(AssetError::UnsupportedFeature,
                        "PMX runtime does not support rigid bodies without a related bone");
                }
            }
            return {};
        }

        glm::vec3 vector(const PMXVec3& v) {
            return {v[0], v[1], v[2]};
        }
        glm::vec4 vector(const PMXVec4& v) {
            return {v[0], v[1], v[2], v[3]};
        }
        types::BoneIndex bone_index(std::int32_t index) {
            return index == NO_PMX_INDEX ? types::INVALID_BONE_INDEX
                                         : static_cast<types::BoneIndex>(index);
        }

        void make_vertices(types::ModelData& model, const PMXData& data) {
            const bool wide_skinning =
                std::ranges::any_of(data.vertices, [](const PMXVertex& vertex) {
                    return vertex.deform_type == PMX_DEFORM_BDEF4 ||
                           vertex.deform_type == PMX_DEFORM_QDEF ||
                           std::ranges::any_of(vertex.bones, [](std::int32_t index) {
                               return index > std::numeric_limits<std::uint16_t>::max() - 1;
                           });
                });
            model.vertices.reserve(data.vertices.size());
            model.skinning_methods.reserve(data.vertices.size());
            model.additional_uv_channels.resize(data.additional_uv_count);
            for (const auto& v : data.vertices) {
                types::VertexVariants vertex{
                    types::Vertex{vector(v.position), vector(v.normal), {v.uv[0], v.uv[1]}}};
                if (wide_skinning) {
                    vertex.emplace_back(
                        types::Skinning4{{static_cast<std::uint32_t>(v.bones[0]),
                                             static_cast<std::uint32_t>(v.bones[1]),
                                             static_cast<std::uint32_t>(v.bones[2]),
                                             static_cast<std::uint32_t>(v.bones[3])},
                            vector(v.weights)});
                } else {
                    vertex.emplace_back(types::Skinning{{static_cast<std::uint16_t>(v.bones[0]),
                                                            static_cast<std::uint16_t>(v.bones[1])},
                        {v.weights[0], v.weights[1]}});
                }
                vertex.emplace_back(types::EdgeFlag{v.edge_scale});
                model.vertices.push_back(std::move(vertex));
                // Spherical deformation falls back to its two linear blend weights.
                // Its format-specific correction vectors remain in PMXData.
                model.skinning_methods.push_back(v.deform_type == PMX_DEFORM_QDEF
                                                     ? types::SkinningMethod::DualQuaternion
                                                     : types::SkinningMethod::LinearBlend);
                for (std::size_t channel = 0; channel < v.additional_uvs.size(); ++channel) {
                    model.additional_uv_channels[channel].push_back(
                        vector(v.additional_uvs[channel]));
                }
            }
            model.indices = data.indices;
        }

        void make_bones(types::ModelData& model, const PMXData& data) {
            types::AddonBones bones(data.bones.size());
            types::AddonIKs iks;
            types::AddonBoneConstraints constraints;
            for (std::size_t i = 0; i < data.bones.size(); ++i) {
                const auto& src = data.bones[i];
                auto& dst = bones[i];
                dst.name = src.name;
                dst.bone_node.parent = bone_index(src.parent);
                const auto position = vector(src.position);
                const auto parent_position =
                    src.parent < 0 ? glm::vec3(0) : vector(data.bones[src.parent].position);
                dst.bind_bone.local = glm::translate(glm::mat4(1), position - parent_position);
                dst.bind_bone.global = glm::translate(glm::mat4(1), position);
                dst.bind_bone.global_inverse = glm::translate(glm::mat4(1), -position);
                if (src.parent > NO_PMX_INDEX) {
                    bones[src.parent].bone_node.children.push_back(i);
                }
                types::BoneConstraint constraint;
                constraint.bone = i;
                constraint.source = bone_index(src.inherit_parent);
                constraint.rotation_weight =
                    (src.flags & BONE_FLAG_INHERIT_ROTATION) != 0 ? src.inherit_weight : 0;
                constraint.translation_weight =
                    (src.flags & BONE_FLAG_INHERIT_TRANSLATION) != 0 ? src.inherit_weight : 0;
                constraint.local_space = (src.flags & BONE_FLAG_LOCAL_TRANSFORM) != 0;
                constraint.after_physics = (src.flags & BONE_FLAG_AFTER_PHYSICS) != 0;
                constraint.evaluation_order = src.layer;
                if ((src.flags & BONE_FLAG_FIXED_AXIS) != 0) {
                    constraint.rotation_axis = vector(src.fixed_axis);
                }
                if ((src.flags & BONE_FLAG_LOCAL_COORDINATE) != 0) {
                    const auto x = vector(src.local_x);
                    const auto z = vector(src.local_z);
                    constraint.local_axes = glm::mat3(x, glm::cross(z, x), z);
                }
                constraints.constraints.push_back(constraint);
                if ((src.flags & BONE_FLAG_IK) != 0) {
                    types::CCDIK ik{};
                    ik.iterations = static_cast<std::uint32_t>(src.ik_iterations);
                    ik.target = bone_index(src.ik_target);
                    ik.ik_bone = i;
                    ik.limit = types::IKLimitAngle{src.ik_angle};
                    for (const auto& link : src.ik_links) {
                        ik.chain.push_back(bone_index(link.bone));
                        ik.link_limits.push_back(
                            {link.limited != 0, vector(link.lower), vector(link.upper)});
                    }
                    iks.push_back(types::IK{std::move(ik)});
                }
            }
            model.addons.emplace_back(std::move(bones));
            model.addons.emplace_back(std::move(iks));
            model.addons.emplace_back(std::move(constraints));
        }

        void make_morphs(types::ModelData& model, const PMXData& data) {
            types::AddonMorphTargets targets;
            // Preserve the existing vertex-morph representation and source morph indices.
            types::AddonMorphs vertices;
            for (std::size_t i = 0; i < data.vertices.size(); ++i) {
                vertices.base_vertices.push_back({i, vector(data.vertices[i].position)});
            }
            vertices.vertices.resize(data.morphs.size());
            for (std::size_t i = 0; i < data.morphs.size(); ++i) {
                const auto& src = data.morphs[i];
                types::MorphTarget target;
                target.name = src.name;
                target.weight_mode = src.type == MORPH_TYPE_FLIP
                                         ? types::MorphWeightMode::DiscreteSelection
                                         : types::MorphWeightMode::Continuous;
                for (const auto& o : src.offsets) {
                    const auto index = static_cast<std::uint32_t>(o.index);
                    switch (src.type) {
                        case MORPH_TYPE_GROUP:
                        case MORPH_TYPE_FLIP:
                            target.offsets.emplace_back(types::MorphWeightOffset{index, o.weight});
                            break;
                        case MORPH_TYPE_VERTEX:
                            target.offsets.emplace_back(
                                types::VertexMorphOffset{index, vector(o.translation)});
                            vertices.vertices[i].push_back({index, vector(o.translation)});
                            break;
                        case MORPH_TYPE_BONE:
                            target.offsets.emplace_back(types::BoneMorphOffset{index,
                                vector(o.translation),
                                glm::quat(
                                    o.rotation[3], o.rotation[0], o.rotation[1], o.rotation[2])});
                            break;
                        case MORPH_TYPE_MATERIAL: {
                            types::MaterialMorphOffset material{index,
                                o.operation == 0 ? types::MorphOperation::Multiply
                                                 : types::MorphOperation::Add,
                                {}};
                            material.properties = {
                                {"diffuse", vector(o.diffuse)},
                                {"specular", glm::vec4(vector(o.specular), 0)},
                                {"shininess", glm::vec4(o.shininess, 0, 0, 0)},
                                {"ambient", glm::vec4(vector(o.ambient), 0)},
                                {"outline_color", vector(o.edge_color)},
                                {"outline_width", glm::vec4(o.edge_size, 0, 0, 0)},
                                {"base_color_texture_tint", vector(o.texture)},
                                {"environment_texture_tint", vector(o.sphere)},
                                {"shading_ramp_texture_tint", vector(o.toon)},
                            };
                            target.offsets.emplace_back(std::move(material));
                            break;
                        }
                        case MORPH_TYPE_IMPULSE:
                            target.offsets.emplace_back(types::ImpulseMorphOffset{index,
                                o.operation != 0,
                                vector(o.translation),
                                vector(o.torque),
                                o.translation == PMXVec3{} && o.torque == PMXVec3{}});
                            break;
                        default:
                            target.offsets.emplace_back(types::UVMorphOffset{
                                index, static_cast<std::uint32_t>(src.type - 3), vector(o.uv)});
                            break;
                    }
                }
                targets.targets.push_back(std::move(target));
            }
            model.addons.emplace_back(std::move(vertices));
            model.addons.emplace_back(std::move(targets));
        }

        void make_materials(types::ModelData& model, const PMXData& data) {
            const auto texture_path = [&](std::string text) {
                std::replace(text.begin(), text.end(), '\\', '/');
                const std::u8string utf8(text.begin(), text.end());
                return (model.path.parent_path() / std::filesystem::path(utf8)).lexically_normal();
            };
            std::uint32_t first{};
            for (const auto& src : data.materials) {
                types::Material dst{};
                dst.name = types::UniformMaterial::UNIFORM_NAME;
                dst.display_name = src.name;
                dst.first_offset = first;
                dst.count = static_cast<std::uint32_t>(src.index_count);
                dst.instance_count = 1;
                first += dst.count;
                dst.double_sided = (src.flags & MATERIAL_FLAG_DRAW_BOTH_SIDES) != 0;
                dst.cast_ground_shadow = (src.flags & MATERIAL_FLAG_GROUND_SHADOW) != 0;
                dst.cast_shadow = (src.flags & MATERIAL_FLAG_CAST_SHADOW) != 0;
                dst.receive_shadow = (src.flags & MATERIAL_FLAG_RECEIVE_SHADOW) != 0;
                dst.outline_color = vector(src.edge_color);
                dst.outline_width = (src.flags & MATERIAL_FLAG_DRAW_EDGE) != 0 ? src.edge_size : 0;
                dst.vertex_color = (src.flags & MATERIAL_FLAG_VERTEX_COLOR) != 0;
                if ((src.flags & MATERIAL_FLAG_POINT_TOPOLOGY) != 0) {
                    dst.topology = types::MaterialTopology::Points;
                } else if ((src.flags & MATERIAL_FLAG_LINE_TOPOLOGY) != 0) {
                    dst.topology = types::MaterialTopology::Lines;
                } else {
                    dst.topology = types::MaterialTopology::Triangles;
                }
                dst.variants = {types::Diffuse{vector(src.diffuse)},
                    types::Specular{vector(src.specular), src.shininess},
                    types::Ambient{vector(src.ambient)},
                    glm::vec1(src.sphere_mode == SPHERE_MODE_MULTIPLY ? 1.0f : 0.0f),
                    glm::vec1(src.sphere_mode == SPHERE_MODE_ADD ? 1.0f : 0.0f),
                    glm::vec1((src.flags & MATERIAL_FLAG_DRAW_EDGE) != 0 ? 1.0f : 0.0f)};
                if (src.texture > NO_PMX_INDEX) {
                    dst.textures.push_back({texture_path(data.textures[src.texture]),
                        types::ModelTexture::MODEL_TEXTURE_NAME,
                        types::ModelTexture::MODEL_SAMPLER_NAME});
                }
                if (src.sphere_texture > NO_PMX_INDEX && src.sphere_mode != SPHERE_MODE_DISABLED) {
                    types::MaterialTexture texture{texture_path(data.textures[src.sphere_texture]),
                        types::ModelTexture::SPHERE_TEXTURE_NAME,
                        types::ModelTexture::SPHERE_SAMPLER_NAME};
                    texture.blend = src.sphere_mode == SPHERE_MODE_ADD
                                        ? types::MaterialTexture::Blend::Add
                                        : types::MaterialTexture::Blend::Multiply;
                    texture.coordinates =
                        src.sphere_mode == SPHERE_MODE_SUB_TEXTURE
                            ? types::MaterialTexture::Coordinates::UV
                            : types::MaterialTexture::Coordinates::NormalProjection;
                    texture.uv_channel = src.sphere_mode == SPHERE_MODE_SUB_TEXTURE ? 1 : 0;
                    dst.textures.push_back(std::move(texture));
                }
                if (src.toon_texture > NO_PMX_INDEX) {
                    const auto name = src.shared_toon != 0
                                          ? std::format("toon{:02}.bmp", src.toon_texture + 1)
                                          : data.textures[src.toon_texture];
                    dst.textures.push_back({texture_path(name),
                        types::ModelTexture::TOON_TEXTURE_NAME,
                        types::ModelTexture::TOON_SAMPLER_NAME});
                }
                model.materials.push_back(std::move(dst));
            }
        }

        void make_physics(types::ModelData& model, const PMXData& data) {
            types::AddonRigidBodies bodies;
            for (const auto& src : data.rigid_bodies) {
                types::PhysicsRigidBody dst{};
                dst.name = src.name;
                dst.relate_bone_index = static_cast<std::uint32_t>(src.bone);
                dst.group_index = src.group;
                dst.group_mask = static_cast<std::uint16_t>(~src.non_collision_mask);
                if (src.mode == RIGID_BODY_MODE_KINEMATIC) {
                    dst.kind = types::RigidBodyKind::Kinematic;
                } else if (src.mode == RIGID_BODY_MODE_DYNAMIC) {
                    dst.kind = types::RigidBodyKind::Dynamic;
                } else {
                    dst.kind = types::RigidBodyKind::DynamicAdjustBone;
                }
                if (src.shape == RIGID_BODY_SHAPE_SPHERE) {
                    dst.shape = types::RBShapeSphere{src.size[0]};
                } else if (src.shape == RIGID_BODY_SHAPE_BOX) {
                    dst.shape = types::RBShapeBox{src.size[0], src.size[1], src.size[2]};
                } else {
                    dst.shape = types::RBShapeCapsule{src.size[0], src.size[1]};
                }
                dst.position = vector(src.position);
                if (src.bone > NO_PMX_INDEX) {
                    dst.position -= vector(data.bones[src.bone].position);
                }
                dst.rotation = vector(src.rotation);
                dst.mass = src.mode == RIGID_BODY_MODE_KINEMATIC ? 0 : src.mass;
                dst.linear_damping = src.linear_damping;
                dst.angular_damping = src.angular_damping;
                dst.restitution = src.restitution;
                dst.friction = src.friction;
                bodies.push_back(dst);
            }
            types::AddonPhysicsJoints joints;
            constexpr std::array kinds{types::JointKind::SpringSixDof,
                types::JointKind::SixDof,
                types::JointKind::PointToPoint,
                types::JointKind::ConeTwist,
                types::JointKind::Slider,
                types::JointKind::Hinge};
            for (const auto& src : data.joints) {
                types::PhysicsJoint dst{};
                dst.name = src.name;
                dst.kind = kinds[src.type];
                dst.rigid_body_a = static_cast<std::uint32_t>(src.body_a);
                dst.rigid_body_b = static_cast<std::uint32_t>(src.body_b);
                dst.position = vector(src.position);
                dst.rotation = vector(src.rotation);
                if (src.type < JOINT_TYPE_WITH_CONSTRAINTS_END) {
                    dst.constrain_position_min = vector(src.translation_min);
                    dst.constrain_position_max = vector(src.translation_max);
                    dst.constrain_rotation_min = vector(src.rotation_min);
                    dst.constrain_rotation_max = vector(src.rotation_max);
                }
                if (src.type == JOINT_TYPE_SPRING_SIX_DOF) {
                    dst.spring_position = vector(src.translation_spring);
                    dst.spring_rotation = vector(src.rotation_spring);
                }
                if (src.type == JOINT_TYPE_CONE_TWIST || src.type == JOINT_TYPE_HINGE) {
                    dst.softness = src.translation_spring[0];
                    dst.bias = src.translation_spring[1];
                    dst.relaxation = src.translation_spring[2];
                }
                if (src.type == JOINT_TYPE_CONE_TWIST) {
                    dst.angular_span = vector(src.rotation_min);
                    dst.damping = src.translation_min[0];
                    dst.fix_threshold = src.translation_max[0];
                    dst.angular_motor = {src.translation_min[2] != 0, 0, src.translation_max[2]};
                    dst.motor_target_rotation = vector(src.rotation_spring);
                }
                if (src.type == JOINT_TYPE_SLIDER) {
                    dst.constrain_position_min.x = src.translation_min[0];
                    dst.constrain_position_max.x = src.translation_max[0];
                    dst.linear_motor = {src.translation_spring[0] != 0,
                        src.translation_spring[1],
                        src.translation_spring[2]};
                }
                if (src.type == JOINT_TYPE_SLIDER || src.type == JOINT_TYPE_HINGE) {
                    dst.constrain_rotation_min.x = src.rotation_min[0];
                    dst.constrain_rotation_max.x = src.rotation_max[0];
                    dst.angular_motor = {src.rotation_spring[0] != 0,
                        src.rotation_spring[1],
                        src.rotation_spring[2]};
                }
                joints.push_back(dst);
            }
            model.addons.emplace_back(std::move(bodies));
            model.addons.emplace_back(std::move(joints));
        }

        void make_soft_bodies(types::ModelData& model, const PMXData& data) {
            types::AddonSoftBodies bodies;
            constexpr std::array aero_models{types::AerodynamicModel::Point,
                types::AerodynamicModel::VertexTwoSided,
                types::AerodynamicModel::VertexOneSided,
                types::AerodynamicModel::FaceTwoSided,
                types::AerodynamicModel::FaceOneSided};
            for (const auto& src : data.soft_bodies) {
                types::SoftBody dst;
                dst.name = src.name;
                dst.shape = src.shape == SOFT_BODY_SHAPE_TRIANGLE_MESH
                                ? types::SoftBodyShape::TriangleMesh
                                : types::SoftBodyShape::Rope;
                dst.material = static_cast<std::uint32_t>(src.material);
                dst.group_index = src.group;
                dst.collision_mask = static_cast<std::uint16_t>(~src.non_collision_mask);
                dst.generate_bending_constraints = (src.flags & SOFT_BODY_FLAG_BENDING) != 0;
                dst.generate_clusters = (src.flags & SOFT_BODY_FLAG_CLUSTER) != 0;
                dst.randomize_constraints = (src.flags & SOFT_BODY_FLAG_RANDOMIZE) != 0;
                dst.bending_distance = src.link_distance;
                dst.cluster_count = src.cluster_count;
                dst.mass = src.mass;
                dst.collision_margin = src.margin;
                dst.aerodynamic_model = aero_models[src.aero_model];
                dst.settings = {src.config[0],
                    src.config[1],
                    src.config[2],
                    src.config[3],
                    src.config[4],
                    src.config[5],
                    src.config[6],
                    src.config[7],
                    src.config[8],
                    src.config[9],
                    src.config[10],
                    src.config[11],
                    src.cluster[0],
                    src.cluster[1],
                    src.cluster[2],
                    src.cluster[3],
                    src.cluster[4],
                    src.cluster[5],
                    src.iterations[0],
                    src.iterations[1],
                    src.iterations[2],
                    src.iterations[3],
                    src.stiffness[0],
                    src.stiffness[1],
                    src.stiffness[2]};
                for (const auto& anchor : src.anchors) {
                    dst.anchors.push_back({static_cast<std::uint32_t>(anchor.body),
                        static_cast<std::uint32_t>(anchor.vertex)});
                }
                for (const auto vertex : src.pinned_vertices) {
                    dst.pinned_vertices.push_back(static_cast<std::uint32_t>(vertex));
                }
                bodies.bodies.push_back(std::move(dst));
            }
            model.addons.emplace_back(std::move(bodies));
        }
    } // namespace

    foundation::Result<types::AssetModelData, AssetError> PMXToModelData::to_model_data(
        const std::filesystem::path& path, const PMXData& data, IAssetLoader* texture_loader) {
        auto valid = PMXModelLoader::validate(data);
        if (valid.is_err()) {
            return std::move(valid).take_err();
        }
        auto supported = validate_rigid_body_support(data);
        if (supported.is_err()) {
            return std::move(supported).take_err();
        }
        auto model = std::make_shared<types::ModelData>();
        model->name = data.name;
        model->path = path;
        make_vertices(*model, data);
        make_bones(*model, data);
        make_morphs(*model, data);
        make_materials(*model, data);
        make_physics(*model, data);
        make_soft_bodies(*model, data);
        for (const auto& material : model->materials) {
            for (const auto& texture : material.textures) {
                if (model->textures.contains(texture.path)) {
                    continue;
                }
                if (texture_loader == nullptr) {
                    return foundation::Error(
                        AssetError::MissingDependency, "PMX requires a texture loader");
                }
                auto loaded = texture_loader->load(texture.path);
                if (loaded.is_err()) {
                    return std::move(loaded).take_err().add_message(
                        "PMX material texture could not be loaded");
                }
                const auto value = std::get_if<types::AssetTextureData>(&loaded.unwrap());
                if (value == nullptr || *value == nullptr) {
                    return foundation::Error(
                        AssetError::InvalidAssetData, "PMX texture loader returned invalid data");
                }
                model->textures.emplace(texture.path, *value);
            }
        }
        return model;
    }
} // namespace enishi::assets_system
