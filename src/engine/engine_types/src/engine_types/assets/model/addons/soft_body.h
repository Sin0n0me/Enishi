#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace enishi::types {
    enum class SoftBodyShape {
        TriangleMesh,
        Rope,
    };
    enum class AerodynamicModel {
        Point,
        VertexTwoSided,
        VertexOneSided,
        FaceTwoSided,
        FaceOneSided,
    };

    struct SoftBodyAnchor {
        std::uint32_t rigid_body;
        std::uint32_t vertex;
    };

    struct SoftBodySettings {
        float velocity_correction{};
        float damping{};
        float drag{};
        float lift{};
        float pressure{};
        float volume_conservation{};
        float dynamic_friction{};
        float pose_matching{};
        float rigid_contact_hardness{};
        float kinetic_contact_hardness{};
        float soft_contact_hardness{};
        float anchor_hardness{};
        float rigid_cluster_hardness{};
        float kinetic_cluster_hardness{};
        float soft_cluster_hardness{};
        float rigid_cluster_impulse_split{};
        float kinetic_cluster_impulse_split{};
        float soft_cluster_impulse_split{};
        std::int32_t velocity_iterations{};
        std::int32_t position_iterations{};
        std::int32_t drift_iterations{};
        std::int32_t cluster_iterations{};
        float linear_stiffness{};
        float angular_stiffness{};
        float volume_stiffness{};
    };

    struct SoftBody {
        std::string name;
        SoftBodyShape shape{};
        std::uint32_t material{};
        std::uint8_t group_index{};
        std::uint16_t collision_mask{};
        bool generate_bending_constraints{};
        bool generate_clusters{};
        bool randomize_constraints{};
        std::int32_t bending_distance{};
        std::int32_t cluster_count{};
        float mass{};
        float collision_margin{};
        AerodynamicModel aerodynamic_model{};
        SoftBodySettings settings;
        std::vector<SoftBodyAnchor> anchors;
        std::vector<std::uint32_t> pinned_vertices;
    };

    struct AddonSoftBodies {
        std::vector<SoftBody> bodies;
    };
} // namespace enishi::types
