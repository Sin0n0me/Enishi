#pragma once
#include "../../renderer/mesh_data.h"
#include "../../renderer/render_data.h"
#include "../texture/texture_data.h"
#include "bone.h"
#include "ik.h"
#include "material.h"
#include "morph.h"
#include "physics_joint.h"
#include "rigid_body.h"
#include <cstdint>
#include <glm/glm.hpp>
#include <variant>
#include <vector>

namespace enishi::types {
    struct Vertex {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 uv;
    };

    struct Skinning {
        glm::u16vec2 bone_index; // ボーン番号(GPU上では1つのu32扱い)
        glm::vec2 bone_weight;   // 0-1
    };

    struct SkinningVertex {
        Vertex vertex;
        Skinning skinning;
    };

    using Indices = std::variant<OwnedRenderData<std::uint8_t>,
        OwnedRenderData<std::uint16_t>,
        OwnedRenderData<std::uint32_t>>;

    using ModelAddon = std::variant<std::monostate,
        std::vector<IK>,
        Morphs,
        std::vector<RigidBody>,
        std::vector<PhysicsJoint>>;

    // このアプリケーション向けに設定されたモデルデータ
    struct ModelData {
        std::string name;
        OwnedRenderData<SkinningVertex> vertices;
        Indices indices;
        std::vector<Bone> bones;
        std::vector<ModelAddon> addons;
        std::vector<Material> materials;

        [[nodiscard]] MeshData to_mesh_data(void) const;
    };
} // namespace enishi::types