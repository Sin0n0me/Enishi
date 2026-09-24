#include "pmx_runtime_support.h"

namespace enishi::assets_system {
    namespace {
        constexpr std::uint8_t DEFORM_BDEF4 = 2;
        constexpr std::uint8_t DEFORM_QDEF = 4;
        constexpr std::uint8_t MORPH_VERTEX = 1;
        constexpr std::uint16_t BONE_INHERIT_ROTATION = 0x0100;
        constexpr std::uint16_t BONE_INHERIT_TRANSLATION = 0x0200;
        constexpr std::uint16_t BONE_FIXED_AXIS = 0x0400;
        constexpr std::uint16_t BONE_LOCAL_AXES = 0x0800;
        constexpr std::uint16_t BONE_AFTER_PHYSICS = 0x1000;
        constexpr std::uint16_t BONE_EXTERNAL_PARENT = 0x2000;
        constexpr std::uint16_t UNSUPPORTED_BONE_FLAGS =
            BONE_INHERIT_ROTATION | BONE_INHERIT_TRANSLATION | BONE_FIXED_AXIS | BONE_LOCAL_AXES |
            BONE_AFTER_PHYSICS | BONE_EXTERNAL_PARENT;
        constexpr std::uint8_t MATERIAL_VERTEX_COLOR = 0x20;
        constexpr std::uint8_t MATERIAL_POINTS = 0x40;
        constexpr std::uint8_t MATERIAL_LINES = 0x80;
        constexpr std::uint8_t UNSUPPORTED_MATERIAL_FLAGS =
            MATERIAL_VERTEX_COLOR | MATERIAL_POINTS | MATERIAL_LINES;
        constexpr std::uint8_t SPHERE_SUB_TEXTURE = 3;
        constexpr std::uint8_t JOINT_SPRING_SIX_DOF = 0;
        constexpr std::int32_t NO_BODY = -1;
        constexpr std::int32_t NO_BONE = -1;
        constexpr std::size_t LEGACY_INFLUENCE_COUNT = 2;
        constexpr std::size_t FOUR_INFLUENCE_COUNT = 4;
        // Matches the bone matrix arrays in vs_model_gl.glsl and bones.hlsl.
        constexpr std::size_t RUNTIME_BONE_LIMIT = 512;

        foundation::Error<AssetError> unsupported(const char* feature) {
            return foundation::Error(AssetError::UnsupportedFeature,
                std::string("PMX runtime does not support ") + feature);
        }

        foundation::Result<void, AssetError> validate_rendering(const PMXData& data) {
            // TODO: Extend matrix buffer allocation/transfer and both shaders for large models,
            // then replace this limit. Four-influence vertices already use 32-bit references.
            if (data.bones.size() > RUNTIME_BONE_LIMIT) {
                return unsupported("models with more than 512 bones");
            }
            for (const auto& vertex : data.vertices) {
                const auto influence_count =
                    vertex.deform_type == DEFORM_BDEF4 || vertex.deform_type == DEFORM_QDEF
                        ? FOUR_INFLUENCE_COUNT
                        : LEGACY_INFLUENCE_COUNT;
                for (std::size_t influence = 0; influence < influence_count; ++influence) {
                    if (vertex.bones[influence] == NO_BONE && vertex.weights[influence] != 0.0f) {
                        return unsupported("weighted influences without bones");
                    }
                }
            }
            if (data.bones.empty()) {
                return unsupported("models without bones");
            }
            if (data.additional_uv_count != 0) {
                return unsupported("additional UV channels");
            }
            for (const auto& material : data.materials) {
                if ((material.flags & UNSUPPORTED_MATERIAL_FLAGS) != 0) {
                    return unsupported("vertex colors or point/line materials");
                }
                if (material.sphere_mode == SPHERE_SUB_TEXTURE) {
                    return unsupported("sphere sub-textures");
                }
            }
            return {};
        }

        foundation::Result<void, AssetError> validate_animation(const PMXData& data) {
            for (const auto& bone : data.bones) {
                if ((bone.flags & UNSUPPORTED_BONE_FLAGS) != 0) {
                    return unsupported("bone inheritance, axes, or external/physics transforms");
                }
                if (bone.layer != 0) {
                    return unsupported("bone evaluation layers");
                }
                for (const auto& link : bone.ik_links) {
                    if (link.limited != 0) {
                        return unsupported("per-link IK limits");
                    }
                }
            }
            for (const auto& morph : data.morphs) {
                if (morph.type != MORPH_VERTEX) {
                    return unsupported("non-vertex morphs");
                }
            }
            return {};
        }

        foundation::Result<void, AssetError> validate_physics(const PMXData& data) {
            for (const auto& joint : data.joints) {
                if (joint.type != JOINT_SPRING_SIX_DOF) {
                    return unsupported("non-spring-six-DOF joints");
                }
                if (joint.body_a == NO_BODY || joint.body_b == NO_BODY) {
                    return unsupported("joints without two rigid bodies");
                }
            }
            if (!data.soft_bodies.empty()) {
                return unsupported("soft bodies");
            }
            return {};
        }
    } // namespace

    foundation::Result<void, AssetError> validate_pmx_runtime_support(const PMXData& data) {
        auto result = validate_rendering(data);
        if (result.is_err()) {
            return result;
        }
        result = validate_animation(data);
        if (result.is_err()) {
            return result;
        }
        return validate_physics(data);
    }
} // namespace enishi::assets_system
