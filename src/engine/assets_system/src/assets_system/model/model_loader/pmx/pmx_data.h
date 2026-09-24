#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace enishi::assets_system {
    using PMXVec2 = std::array<float, 2>;
    using PMXVec3 = std::array<float, 3>;
    using PMXVec4 = std::array<float, 4>;

    struct PMXVertex {
        PMXVec3 position{};
        PMXVec3 normal{};
        PMXVec2 uv{};
        std::vector<PMXVec4> additional_uvs;
        std::uint8_t deform_type{};
        std::array<std::int32_t, 4> bones{-1, -1, -1, -1};
        PMXVec4 weights{};
        PMXVec3 sdef_center{};
        PMXVec3 sdef_radius0{};
        PMXVec3 sdef_radius1{};
        float edge_scale{};
    };

    struct PMXMaterial {
        std::string name;
        std::string english_name;
        std::string memo;
        PMXVec4 diffuse{};
        PMXVec4 edge_color{};
        PMXVec3 specular{};
        PMXVec3 ambient{};
        float shininess{};
        float edge_size{};
        std::uint8_t flags{};
        std::uint8_t sphere_mode{};
        std::uint8_t shared_toon{};
        std::int32_t texture{-1};
        std::int32_t sphere_texture{-1};
        std::int32_t toon_texture{-1};
        std::int32_t index_count{};
    };

    struct PMXIKLink {
        std::int32_t bone{-1};
        std::uint8_t limited{};
        PMXVec3 lower{};
        PMXVec3 upper{};
    };

    struct PMXBone {
        std::string name;
        std::string english_name;
        PMXVec3 position{};
        PMXVec3 tail_offset{};
        PMXVec3 fixed_axis{};
        PMXVec3 local_x{};
        PMXVec3 local_z{};
        std::int32_t parent{-1};
        std::int32_t layer{};
        std::int32_t tail{-1};
        std::int32_t inherit_parent{-1};
        std::int32_t external_parent{};
        std::uint16_t flags{};
        float inherit_weight{};
        std::int32_t ik_target{-1};
        std::int32_t ik_iterations{};
        float ik_angle{};
        std::vector<PMXIKLink> ik_links;
    };

    // The interpretation of each offset is determined by PMXMorph::type.
    struct PMXMorphOffset {
        std::int32_t index{-1};
        std::uint8_t operation{};
        float weight{};
        float shininess{};
        float edge_size{};
        PMXVec3 translation{};
        PMXVec3 specular{};
        PMXVec3 ambient{};
        PMXVec3 torque{};
        PMXVec4 rotation{};
        PMXVec4 uv{};
        PMXVec4 diffuse{};
        PMXVec4 edge_color{};
        PMXVec4 texture{};
        PMXVec4 sphere{};
        PMXVec4 toon{};
    };

    struct PMXMorph {
        std::string name;
        std::string english_name;
        std::uint8_t panel{};
        std::uint8_t type{};
        std::vector<PMXMorphOffset> offsets;
    };

    struct PMXDisplayElement {
        std::uint8_t type{};
        std::int32_t index{-1};
    };

    struct PMXDisplayFrame {
        std::string name;
        std::string english_name;
        std::uint8_t special{};
        std::vector<PMXDisplayElement> elements;
    };

    struct PMXRigidBody {
        std::string name;
        std::string english_name;
        std::int32_t bone{-1};
        std::uint8_t group{};
        std::uint8_t shape{};
        std::uint8_t mode{};
        std::uint16_t non_collision_mask{};
        PMXVec3 size{};
        PMXVec3 position{};
        PMXVec3 rotation{};
        float mass{};
        float linear_damping{};
        float angular_damping{};
        float restitution{};
        float friction{};
    };

    struct PMXJoint {
        std::string name;
        std::string english_name;
        std::uint8_t type{};
        std::int32_t body_a{-1};
        std::int32_t body_b{-1};
        PMXVec3 position{};
        PMXVec3 rotation{};
        PMXVec3 translation_min{};
        PMXVec3 translation_max{};
        PMXVec3 rotation_min{};
        PMXVec3 rotation_max{};
        PMXVec3 translation_spring{};
        PMXVec3 rotation_spring{};
    };

    struct PMXSoftBodyAnchor {
        std::int32_t body{-1};
        std::int32_t vertex{};
        std::uint8_t near_mode{};
    };

    struct PMXSoftBody {
        std::string name;
        std::string english_name;
        std::uint8_t shape{};
        std::uint8_t group{};
        std::uint8_t flags{};
        std::int32_t material{-1};
        std::int32_t link_distance{};
        std::int32_t cluster_count{};
        std::int32_t aero_model{};
        std::uint16_t non_collision_mask{};
        float mass{};
        float margin{};
        std::array<float, 12> config{};
        std::array<float, 6> cluster{};
        std::array<std::int32_t, 4> iterations{};
        std::array<float, 3> stiffness{};
        std::vector<PMXSoftBodyAnchor> anchors;
        std::vector<std::int32_t> pinned_vertices;
    };

    struct PMXData {
        float version{};
        std::uint8_t encoding{};
        std::uint8_t additional_uv_count{};
        std::array<std::uint8_t, 6> index_sizes{};
        std::string name;
        std::string english_name;
        std::string comment;
        std::string english_comment;
        std::vector<PMXVertex> vertices;
        std::vector<std::uint32_t> indices;
        std::vector<std::string> textures;
        std::vector<PMXMaterial> materials;
        std::vector<PMXBone> bones;
        std::vector<PMXMorph> morphs;
        std::vector<PMXDisplayFrame> display_frames;
        std::vector<PMXRigidBody> rigid_bodies;
        std::vector<PMXJoint> joints;
        std::vector<PMXSoftBody> soft_bodies;
    };
} // namespace enishi::assets_system
