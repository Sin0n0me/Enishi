#include "state_pool.h"

namespace enishi::renderer::directx {
    void StatePool::remove_native_state(const types::HandleId handle) noexcept {
        const auto opt_mapped_handle = this->handle_mapper.get(handle);
        if (opt_mapped_handle.is_none()) {
            return;
        }
        const auto& mapped_handle = opt_mapped_handle.unwrap();

        switch (mapped_handle.state_kind) {
            case types::StateKind::Rasterizer: {
                auto opt_native_resource =
                    this->native_rasterizer_states.get(mapped_handle.resource_index);
                if (opt_native_resource.is_none()) {
                    return;
                }
                opt_native_resource.unwrap_mut().Reset();
            } break;
            case types::StateKind::Blend: {
                auto opt_native_resource =
                    this->native_blend_states.get(mapped_handle.resource_index);
                if (opt_native_resource.is_none()) {
                    return;
                }
                opt_native_resource.unwrap_mut().Reset();
            } break;
            case types::StateKind::DepthStencil: {
                auto opt_native_resource =
                    this->native_depth_stencil_states.get(mapped_handle.resource_index);
                if (opt_native_resource.is_none()) {
                    return;
                }
                opt_native_resource.unwrap_mut().Reset();
            } break;
            default:
                return;
        }

        this->handle_mapper.remove(handle);
    }

    std::tuple<types::HandleId, StatePool::NativeRasterizerState&>
    StatePool::make_native_rasterizer_state(void) noexcept {
        return this->handle_mapper.make_from(
            this->native_rasterizer_states.make(), [](const std::size_t index) {
                return decltype(handle_mapper)::ValueType{
                    .state_kind = types::StateKind::Rasterizer,
                    .resource_index = index,
                };
            });
    }

    foundation::Option<StatePool::NativeRasterizerState&> StatePool::get_native_rasterizer_state(
        const types::HandleId handle) noexcept {
        return this->handle_mapper.get(handle).and_then(
            [this](const decltype(handle_mapper)::ValueType& mapped_handle)
                -> foundation::Option<NativeRasterizerState&> {
                if (mapped_handle.state_kind != types::StateKind::Rasterizer) {
                    return {};
                }
                return this->native_rasterizer_states.get(mapped_handle.resource_index);
            });
    }

    foundation::Option<const StatePool::NativeRasterizerState&>
    StatePool::get_native_rasterizer_state(const types::HandleId handle) const noexcept {
        return this->handle_mapper.get(handle).and_then(
            [this](const decltype(handle_mapper)::ValueType& mapped_handle)
                -> foundation::Option<const NativeRasterizerState&> {
                if (mapped_handle.state_kind != types::StateKind::Rasterizer) {
                    return {};
                }
                return this->native_rasterizer_states.get(mapped_handle.resource_index);
            });
    }

    std::tuple<types::HandleId, StatePool::NativeBlendState&> StatePool::make_native_blend_state(
        void) noexcept {
        return this->handle_mapper.make_from(
            this->native_blend_states.make(), [](const std::size_t index) {
                return decltype(handle_mapper)::ValueType{
                    .state_kind = types::StateKind::Blend,
                    .resource_index = index,
                };
            });
    }

    foundation::Option<StatePool::NativeBlendState&> StatePool::get_native_blend_state(
        const types::HandleId handle) noexcept {
        return this->handle_mapper.get(handle).and_then(
            [this](const decltype(handle_mapper)::ValueType& mapped_handle)
                -> foundation::Option<NativeBlendState&> {
                if (mapped_handle.state_kind != types::StateKind::Blend) {
                    return {};
                }
                return this->native_blend_states.get(mapped_handle.resource_index);
            });
    }

    foundation::Option<const StatePool::NativeBlendState&> StatePool::get_native_blend_state(
        const types::HandleId handle) const noexcept {
        return this->handle_mapper.get(handle).and_then(
            [this](const decltype(handle_mapper)::ValueType& mapped_handle)
                -> foundation::Option<const NativeBlendState&> {
                if (mapped_handle.state_kind != types::StateKind::Blend) {
                    return {};
                }
                return this->native_blend_states.get(mapped_handle.resource_index);
            });
    }

    std::tuple<types::HandleId, StatePool::NativeDepthStencilState&>
    StatePool::make_native_depth_stencil_state(void) noexcept {
        return this->handle_mapper.make_from(
            this->native_depth_stencil_states.make(), [](const std::size_t index) {
                return decltype(handle_mapper)::ValueType{
                    .state_kind = types::StateKind::DepthStencil,
                    .resource_index = index,
                };
            });
    }

    foundation::Option<StatePool::NativeDepthStencilState&>
    StatePool::get_native_depth_stencil_state(const types::HandleId handle) noexcept {
        return this->handle_mapper.get(handle).and_then(
            [this](const decltype(handle_mapper)::ValueType& mapped_handle)
                -> foundation::Option<NativeDepthStencilState&> {
                if (mapped_handle.state_kind != types::StateKind::DepthStencil) {
                    return {};
                }
                return this->native_depth_stencil_states.get(mapped_handle.resource_index);
            });
    }

    foundation::Option<const StatePool::NativeDepthStencilState&>
    StatePool::get_native_depth_stencil_state(const types::HandleId handle) const noexcept {
        return this->handle_mapper.get(handle).and_then(
            [this](const decltype(handle_mapper)::ValueType& mapped_handle)
                -> foundation::Option<const NativeDepthStencilState&> {
                if (mapped_handle.state_kind != types::StateKind::DepthStencil) {
                    return {};
                }
                return this->native_depth_stencil_states.get(mapped_handle.resource_index);
            });
    }

    std::tuple<types::HandleId, StatePool::NativeSamplerState&>
    StatePool::make_native_sampler_state(void) noexcept {
        return this->handle_mapper.make_from(
            this->native_sampler_states.make(), [](const std::size_t index) {
                return decltype(handle_mapper)::ValueType{
                    .state_kind = types::StateKind::Sampler,
                    .resource_index = index,
                };
            });
    }

    foundation::Option<StatePool::NativeSamplerState&> StatePool::get_native_sampler_state(
        const types::HandleId handle) noexcept {
        return this->handle_mapper.get(handle).and_then(
            [this](const decltype(handle_mapper)::ValueType& mapped_handle)
                -> foundation::Option<NativeSamplerState&> {
                if (mapped_handle.state_kind != types::StateKind::Sampler) {
                    return {};
                }
                return this->native_sampler_states.get(mapped_handle.resource_index);
            });
    }

    foundation::Option<const StatePool::NativeSamplerState&> StatePool::get_native_sampler_state(
        const types::HandleId handle) const noexcept {
        return this->handle_mapper.get(handle).and_then(
            [this](const decltype(handle_mapper)::ValueType& mapped_handle)
                -> foundation::Option<const NativeSamplerState&> {
                if (mapped_handle.state_kind != types::StateKind::Sampler) {
                    return {};
                }
                return this->native_sampler_states.get(mapped_handle.resource_index);
            });
    }

    foundation::Option<types::StateKind> StatePool::get_state_kind(
        const types::HandleId& handle) const noexcept {
        return this->handle_mapper.get(handle).map(
            [](const decltype(handle_mapper)::ValueType& mapped_handle) {
                return mapped_handle.state_kind;
            });
    }
} // namespace enishi::renderer::directx