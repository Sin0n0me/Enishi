#include "render_handle_mapper.h"

namespace enishi::renderer {
    RenderHandleMapper::RenderHandleMapper(void)
        : handle_allocator(std::make_unique<types::HandleAllocator>()) {
    }

    types::RenderHandle RenderHandleMapper::make(
        const types::RenderHandleType& handle_type, types::ResourceHandles&& mapped) noexcept {
        const auto handle = types::RenderHandle{
            this->handle_allocator->create(),
            handle_type,
        };
        this->handle_mapper.emplace(handle, std::move(mapped));

        return handle;
    }

    foundation::Option<types::ResourceHandles&> RenderHandleMapper::get(
        const types::RenderHandle& handle) noexcept {
        const auto iter = this->handle_mapper.find(handle);
        if (iter == this->handle_mapper.end()) {
            return {};
        }
        return iter->second;
    }

    types::ResourceHandles& RenderHandleMapper::operator[](const types::RenderHandle& handle) {
        return this->handle_mapper[handle];
    }

    foundation::Option<const types::ResourceHandles&> RenderHandleMapper::get(
        const types::RenderHandle& handle) const noexcept {
        const auto iter = this->handle_mapper.find(handle);
        if (iter == this->handle_mapper.end()) {
            return {};
        }
        return iter->second;
    }
} // namespace enishi::renderer