#pragma once
#include <engine_types/renderer/vertex/vertex_layout.h>
#include <foundation/result/result.h>

namespace enishi::platform {
    class IPipelineLayout {
      public:
        virtual ~IPipelineLayout(void) noexcept = default;

        [[nodiscard]]
        virtual const types::VertexLayout& vertex_layout(void) const noexcept = 0;
    };
} // namespace enishi::platform