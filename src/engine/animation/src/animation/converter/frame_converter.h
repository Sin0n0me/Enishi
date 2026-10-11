#pragma once
#include "frame_converter.h"
#include <assets_system/animation/vmd/vmd_data.h>

namespace enishi::animation {
    //
    class FrameConverter {
      public:
        static Keyframes<int> to_keyframes(const assets_system::VMDData& data);
    };
} // namespace enishi::animation
