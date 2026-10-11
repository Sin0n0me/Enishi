#pragma once
#include <cstdint>

namespace enishi::assets_system {
    // Values and underlying types match the PMX binary format.
    enum class PMXDeformType : std::uint8_t {
        BDEF1 = 0,
        BDEF2 = 1,
        BDEF4 = 2,
        SDEF = 3,
        QDEF = 4,
    };

    enum class PMXMorphType : std::uint8_t {
        Group = 0,
        Vertex = 1,
        Bone = 2,
        UV = 3,
        AdditionalUV1 = 4,
        AdditionalUV2 = 5,
        AdditionalUV3 = 6,
        AdditionalUV4 = 7,
        Material = 8,
        Flip = 9,
        Impulse = 10,
    };

    enum class PMXSphereMode : std::uint8_t {
        Disabled = 0,
        Multiply = 1,
        Add = 2,
        SubTexture = 3,
    };

    enum class PMXRigidBodyShape : std::uint8_t {
        Sphere = 0,
        Box = 1,
        Capsule = 2,
    };

    enum class PMXRigidBodyMode : std::uint8_t {
        Kinematic = 0,
        Dynamic = 1,
        DynamicAdjustBone = 2,
    };

    enum class PMXJointType : std::uint8_t {
        SpringSixDof = 0,
        SixDof = 1,
        PointToPoint = 2,
        ConeTwist = 3,
        Slider = 4,
        Hinge = 5,
    };

    enum class PMXSoftBodyShape : std::uint8_t {
        TriangleMesh = 0,
        Rope = 1,
    };

    enum class PMXAerodynamicModel : std::int32_t {
        Point = 0,
        VertexTwoSided = 1,
        VertexOneSided = 2,
        FaceTwoSided = 3,
        FaceOneSided = 4,
    };
} // namespace enishi::assets_system
