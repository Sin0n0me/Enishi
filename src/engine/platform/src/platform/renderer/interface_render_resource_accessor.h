#pragma once
#include "../errors/renderer_errors.h"
#include <engine_types/handle/renderer/render_handle.h>
#include <engine_types/renderer/description/pipeline_description.h>
#include <engine_types/renderer/render_graph.h>
#include <foundation/result/result.h>
#include <render>
#include <span>

namespace enishi::platform {
    class IRenderResourceAccessor {
      public:
        virtual ~IRenderResourceAccessor(void) noexcept = default;

        [[nodiscard]] virtual IGPUResourceAccessor* get_resource_accessor(void) noexcept = 0;
        [[nodiscard]] virtual const IGPUResourceAccessor* get_resource_accessor(
            void) const noexcept = 0;
    };
} // namespace enishi::platform