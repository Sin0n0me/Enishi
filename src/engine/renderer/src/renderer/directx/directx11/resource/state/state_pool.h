#pragma once
#include "interface_native_state_accessor.h"
#include <engine_types/handle/handle_mapper.h>
#include <engine_types/renderer/description/state_kind.h>
#include <foundation/pool/resource_pool.h>
#include <renderer/common/resource_accessor.h>

namespace enishi::renderer::directx {
    class StatePool : public INativeStateAccessor {
      private:
        struct StateHandle {
            types::StateKind state_kind;
            std::size_t resource_index;
        };

      private:
        ResourceAccessor& interfaces;
        types::ResourceMapper<StateHandle> handle_mapper;
        foundation::ResourcePool<NativeRasterizerState> native_rasterizer_states;
        foundation::ResourcePool<NativeBlendState> native_blend_states;
        foundation::ResourcePool<NativeDepthStencilState> native_depth_stencil_states;
        foundation::ResourcePool<NativeSamplerState> native_sampler_states;

      public:
        explicit StatePool(ResourceAccessor& interfaces);
        void remove_native_state(const types::HandleId handle) noexcept override;
        std::tuple<types::HandleId, NativeRasterizerState&> make_native_rasterizer_state(
            void) noexcept override;
        foundation::Option<NativeRasterizerState&> get_native_rasterizer_state(
            const types::HandleId handle) noexcept override;
        foundation::Option<const NativeRasterizerState&> get_native_rasterizer_state(
            const types::HandleId handle) const noexcept override;
        std::tuple<types::HandleId, NativeBlendState&> make_native_blend_state(
            void) noexcept override;
        foundation::Option<NativeBlendState&> get_native_blend_state(
            const types::HandleId handle) noexcept override;
        foundation::Option<const NativeBlendState&> get_native_blend_state(
            const types::HandleId handle) const noexcept override;
        std::tuple<types::HandleId, NativeDepthStencilState&> make_native_depth_stencil_state(
            void) noexcept override;
        foundation::Option<NativeDepthStencilState&> get_native_depth_stencil_state(
            const types::HandleId handle) noexcept override;
        foundation::Option<const NativeDepthStencilState&> get_native_depth_stencil_state(
            const types::HandleId handle) const noexcept override;
        std::tuple<types::HandleId, NativeSamplerState&> make_native_sampler_state(
            void) noexcept override;
        foundation::Option<NativeSamplerState&> get_native_sampler_state(
            const types::HandleId handle) noexcept override;
        foundation::Option<const NativeSamplerState&> get_native_sampler_state(
            const types::HandleId handle) const noexcept override;
    };
} // namespace enishi::renderer::directx
