#pragma once
#include "overlay_line.h"
#include <engine_types/collider/obb.h>
#include <vector>

namespace enishi::debug {
    void append_obb_wireframe(
        const types::OBB& obb, const glm::mat4& view_projection, std::vector<OverlayLine>& lines);
} // namespace enishi::debug
