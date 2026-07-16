#pragma once
#include <engine_types/renderer/render_handle.h>
#include <foundation/result/result.h>

namespace enishi::platform {
    class IRenderPass {
      public:
        virtual ~IRenderPass(void) noexcept = default;

        //[[nodiscard]]

        virtual void add_mesh(const types::RenderHandle handle) noexcept = 0;


    };
} // namespace enishi::platform