#pragma once
#include "pmd_file_struct.h"
#include <cstdint>

namespace enishi::assets_system {
    // 生のデータに近い
    struct PMDData {
        std::array<std::int8_t, 20> model_name;
        std::array<std::int8_t, 256> comment;
        std::vector<PMDVertex> vertices;
        std::vector<PMDVertexIndex> indices;
        std::vector<PMDMaterial> materials;
        std::vector<PMDBone> bones;
        std::vector<PMDIK> iks;
        std::vector<PMDMorph> morphs;
        std::vector<PMDDisplayMorph> display_morphs;
        std::vector<PMDBoneFrameName> bone_frame_name;
        std::vector<PMDDisplayBone> display_bones;
        std::vector<PMDRigidBody> rigid_bodies;
        std::vector<PMDPhysicsJoint> physics_joints;
        PMDEnglishDictionary english_dictionary;
        PMDToonTexture toon_textures;
    };
} // namespace enishi::assets_system