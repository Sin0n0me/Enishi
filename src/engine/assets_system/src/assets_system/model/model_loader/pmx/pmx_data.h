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
        PMXVec3 position{}, normal{};
        PMXVec2 uv{};
        std::vector<PMXVec4> additional_uvs;
        std::uint8_t deform_type{};
        std::array<std::int32_t, 4> bones{-1, -1, -1, -1};
        PMXVec4 weights{};
        PMXVec3 sdef_center{}, sdef_radius0{}, sdef_radius1{};
        float edge_scale{};
    };

    struct PMXMaterial {
        std::string name, english_name, memo;
        PMXVec4 diffuse{}, edge_color{};
        PMXVec3 specular{}, ambient{};
        float shininess{}, edge_size{};
        std::uint8_t flags{}, sphere_mode{}, shared_toon{};
        std::int32_t texture{-1}, sphere_texture{-1}, toon_texture{-1};
        std::int32_t index_count{};
    };

    struct PMXIKLink {
        std::int32_t bone{-1};
        std::uint8_t limited{};
        PMXVec3 lower{}, upper{};
    };

    struct PMXBone {
        std::string name, english_name;
        PMXVec3 position{}, tail_offset{}, fixed_axis{}, local_x{}, local_z{};
        std::int32_t parent{-1}, layer{}, tail{-1}, inherit_parent{-1}, external_parent{};
        std::uint16_t flags{};
        float inherit_weight{};
        std::int32_t ik_target{-1}, ik_iterations{};
        float ik_angle{};
        std::vector<PMXIKLink> ik_links;
    };

    // The interpretation of each offset is determined by PMXMorph::type.
    struct PMXMorphOffset {
        std::int32_t index{-1};
        std::uint8_t operation{};
        float weight{}, shininess{}, edge_size{};
        PMXVec3 translation{}, specular{}, ambient{}, torque{};
        PMXVec4 rotation{}, uv{}, diffuse{}, edge_color{}, texture{}, sphere{}, toon{};
    };

    struct PMXMorph {
        std::string name, english_name;
        std::uint8_t panel{}, type{};
        std::vector<PMXMorphOffset> offsets;
    };

    struct PMXDisplayElement {
        std::uint8_t type{};
        std::int32_t index{-1};
    };

    struct PMXDisplayFrame {
        std::string name, english_name;
        std::uint8_t special{};
        std::vector<PMXDisplayElement> elements;
    };

    struct PMXRigidBody {
        std::string name, english_name;
        std::int32_t bone{-1};
        std::uint8_t group{}, shape{}, mode{};
        std::uint16_t non_collision_mask{};
        PMXVec3 size{}, position{}, rotation{};
        float mass{}, linear_damping{}, angular_damping{}, restitution{}, friction{};
    };

    struct PMXJoint {
        std::string name, english_name;
        std::uint8_t type{};
        std::int32_t body_a{-1}, body_b{-1};
        PMXVec3 position{}, rotation{}, translation_min{}, translation_max{};
        PMXVec3 rotation_min{}, rotation_max{}, translation_spring{}, rotation_spring{};
    };

    struct PMXSoftBodyAnchor {
        std::int32_t body{-1}, vertex{};
        std::uint8_t near_mode{};
    };

    struct PMXSoftBody {
        std::string name, english_name;
        std::uint8_t shape{}, group{}, flags{};
        std::int32_t material{-1}, link_distance{}, cluster_count{}, aero_model{};
        std::uint16_t non_collision_mask{};
        float mass{}, margin{};
        std::array<float, 12> config{};
        std::array<float, 6> cluster{};
        std::array<std::int32_t, 4> iterations{};
        std::array<float, 3> stiffness{};
        std::vector<PMXSoftBodyAnchor> anchors;
        std::vector<std::int32_t> pinned_vertices;
    };

    struct PMXData {
        float version{};
        std::uint8_t encoding{}, additional_uv_count{};
        std::array<std::uint8_t, 6> index_sizes{};
        std::string name, english_name, comment, english_comment;
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
