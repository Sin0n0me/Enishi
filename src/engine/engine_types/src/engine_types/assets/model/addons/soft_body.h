#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace enishi::types {
    enum class SoftBodyShape { TriangleMesh, Rope };
    enum class AerodynamicModel {
        Point,
        VertexTwoSided,
        VertexOneSided,
        FaceTwoSided,
        FaceOneSided
    };

    struct SoftBodyAnchor {
        std::uint32_t rigid_body;
        std::uint32_t vertex;
    };

    struct SoftBodySettings {
        float velocity_correction{}, damping{}, drag{}, lift{}, pressure{}, volume_conservation{};
        float dynamic_friction{}, pose_matching{};
        float rigid_contact_hardness{}, kinetic_contact_hardness{}, soft_contact_hardness{},
            anchor_hardness{};
        float rigid_cluster_hardness{}, kinetic_cluster_hardness{}, soft_cluster_hardness{};
        float rigid_cluster_impulse_split{}, kinetic_cluster_impulse_split{},
            soft_cluster_impulse_split{};
        std::int32_t velocity_iterations{}, position_iterations{}, drift_iterations{},
            cluster_iterations{};
        float linear_stiffness{}, angular_stiffness{}, volume_stiffness{};
    };

    struct SoftBody {
        std::string name;
        SoftBodyShape shape{};
        std::uint32_t material{};
        std::uint8_t group_index{};
        std::uint16_t collision_mask{};
        bool generate_bending_constraints{}, generate_clusters{}, randomize_constraints{};
        std::int32_t bending_distance{}, cluster_count{};
        float mass{}, collision_margin{};
        AerodynamicModel aerodynamic_model{};
        SoftBodySettings settings;
        std::vector<SoftBodyAnchor> anchors;
        std::vector<std::uint32_t> pinned_vertices;
    };

    struct AddonSoftBodies {
        std::vector<SoftBody> bodies;
    };
} // namespace enishi::types
