#include "rasterizer_pool.h"

namespace enishi::renderer::directx {
    std::tuple<types::HandleId, RasterizerPool::NativeRasterizer&>
    RasterizerPool::make_native_rasterizer(void) noexcept {
        return this->handle_mapper.make_from(
            this->native_rasterizers.make(), [](const std::size_t index) {
                return decltype(handle_mapper)::ValueType{
                    .resource_index = index,
                };
            });
    }

    void RasterizerPool::remove_native_rasterizer(const types::HandleId handle) noexcept {
        const auto opt_mapped_handle = this->handle_mapper.get(handle);
        if (opt_mapped_handle.is_none()) {
            return;
        }
        const auto& mapped_handle = opt_mapped_handle.unwrap();

        auto opt_native_resource = this->native_rasterizers.get(mapped_handle.resource_index);
        if (opt_native_resource.is_none()) {
            return;
        }
        opt_native_resource.unwrap_mut().Reset();

        this->handle_mapper.remove(handle);
    }

    foundation::Option<RasterizerPool::NativeRasterizer&> RasterizerPool::get_native_rasterizer(
        const types::HandleId handle) noexcept {
        return this->handle_mapper.get(handle).and_then(
            [this](const decltype(handle_mapper)::ValueType& mapped_handle) {
                return this->native_rasterizers.get(mapped_handle.resource_index);
            });
    }

    foundation::Option<const RasterizerPool::NativeRasterizer&>
    RasterizerPool::get_native_rasterizer(const types::HandleId handle) const noexcept {
        return this->handle_mapper.get(handle).and_then(
            [this](const decltype(handle_mapper)::ValueType& mapped_handle) {
                return this->native_rasterizers.get(mapped_handle.resource_index);
            });
    }
} // namespace enishi::renderer::directx