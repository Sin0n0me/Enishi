#pragma once
#include <cstdint>
#include <vector>

namespace enishi::assets_system {
#pragma pack(push, 1)
    // 参考
    // https://w.atwiki.jp/kumiho_k/pages/15.html

    struct VMDHeader {
        char header[30];     // ファイルシグネチャ30Byte
        char model_name[20]; // モデル名
    };

    struct VMDBoneKeyFrame {
        char bone_name[15];             //
        std::uint32_t frame;            //
        float translation[3];           //
        float rotation[4];              //
        std::uint8_t interpolation[64]; //
    };

    struct VMDMorphKeyFrame {
        char morph_name[15]; //
        std::uint32_t frame;
        float weight;
    };

    struct VMDCameraKeyframe {
        std::uint32_t frame_index;
        float distance;
        float position[3];
        float rotation[3];
        std::uint8_t interpolation[24];
        std::uint32_t view_angle;
        std::uint8_t perspective;
    };

    struct VMDLightKeyFrame {
        std::uint32_t frame;
        float color[3];
        float position[3];
    };

    struct VMDShadowKeyFrame {
        std::uint32_t frame;
        std::uint8_t shadow_type;
        float distance;
    };

    struct VMDIKInfo {
        char name[20];
        std::uint8_t flag; // IKのon/off, 0:OFF, 1:ON
    };

    // 可変要素を含む
    struct VMDIKKeyFrame {
        std::uint32_t frame;
        std::uint8_t show_flag; //
        std::uint32_t count;
        std::vector<VMDIKInfo> ik_infos;
    };

#pragma pack(pop)
} // namespace enishi::assets_system