#pragma once
#include "../../../errors/errors.h"
#include "../../../interface_asset_system.h"
#include "../../bone/bone_resolver.h"
#include "../../morph/morph_resolver.h"
#include "pmd_data.h"
#include <cstdint>
#include <engine_types/assets/model/model_data.h>
#include <filesystem>
#include <foundation/result/result.h>
#include <vector>

namespace enishi::assets_system {
    class PMDToModelData {
      public:
        static constexpr glm::vec3 MMD_KNEE_AXIS = glm::vec3(-1.0f, 0.0f, 0.0f); // X軸固定

      public:
        static foundation::Result<types::ModelData, AssetError> to_model_data(
            const std::filesystem::path& path, const PMDData& data);

      private:
        [[nodiscard]] static std::tuple<std::vector<types::Bone>, BoneResolver> make_bone(
            const std::vector<PMDBone>& bones);

        [[nodiscard]] static std::vector<types::VertexVariants> make_vertices(
            const std::vector<PMDVertex>& vertices);

        [[nodiscard]] static std::vector<std::uint16_t> make_indices(
            const std::vector<PMDVertexIndex>& indices);

        [[nodiscard]] static types::AddonIKs make_iks(
            const std::vector<PMDIK>& iks, const IBoneResolver* bone_resolver);

        [[nodiscard]] static std::tuple<types::AddonMorphs, MorphResolver> make_morphs(
            const std::vector<PMDMorph>& morphs);

        [[nodiscard]] static types::AddonPhysicsJoints make_joints(
            const std::vector<PMDPhysicsJoint>& joints);

        [[nodiscard]] static types::AddonRigidBodies make_rigid_bodies(
            const std::vector<PMDRigidBody>& rigid_bodies);

        [[nodiscard]] static std::vector<types::Material> make_materials(
            const std::filesystem::path& path,
            const std::vector<PMDMaterial>& materials,
            const PMDToonTexture& toon_textures);

        [[nodiscard]] static glm::mat4 make_offset_from_pmd(const PMDRigidBody& rigid_body);

        [[nodiscard]] static types::RigidBodyShape make_shape_from_pmd(
            const PMDRigidBody& rigid_body);

        [[nodiscard]] static types::RigidBodyType make_rigid_body_type_from_pmd(
            const PMDRigidBody& rigid_body);
    };
} // namespace enishi::assets_system