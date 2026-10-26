#pragma once
#include "vrm_json.h"
#include <cgltf.h>
#include <engine_types/assets/model/model_data.h>
#include <map>

namespace enishi::assets_system {
    constexpr float VRM_MODEL_UNITS_PER_METER = 10.0f;

    struct VRMContext {
        const cgltf_data& source;
        types::ModelData& model;
        VRMJson extension;
        bool legacy{};
        glm::mat4 basis{1};
        std::vector<bool> active_nodes;
        std::map<std::pair<std::size_t, std::size_t>, std::uint32_t> morphs;
        types::AddonMorphTargets targets;
        std::vector<std::size_t> material_sources;
        std::vector<glm::vec2> raw_uvs;
        std::string error;

        bool fail(std::string message);
    };

    VRMJson vrm_extension(
        const cgltf_extension* extensions, std::size_t count, std::string_view name);
    bool make_vrm_bones(VRMContext& context);
} // namespace enishi::assets_system
