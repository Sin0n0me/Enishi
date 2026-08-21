#pragma once
#include <engine_types/handle/handle_allocator.h>
#include <engine_types/handle/handle_type.h>
#include <foundation/option/option.h>
#include <functional>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace enishi::renderer {
    template <typename T> class HandleMapper {
      private:
        types::HandleAllocator handle_allocator;
        std::unordered_map<types::HandleId, T> handle_mapper;

      public:
        using KeyType = decltype(handle_mapper)::value_type::second_type;
        using ValueType = T;

      public:
        [[nodiscard]] types::HandleId emplace(T&& resource) {
            const auto handle = this->handle_allocator.create();
            this->handle_mapper.emplace(handle, std::move(resource));
            return handle;
        }

        template <typename... Args> [[nodiscard]] types::HandleId make(Args&&... args) {
            const auto handle = this->handle_allocator.create();
            this->handle_mapper.emplace(handle, T{std::forward<Args>(args)...});
            return handle;
        }

        template <typename U>
        [[nodiscard]] std::tuple<types::HandleId, U> make_from(
            std::tuple<std::size_t, U>&& tuple, std::function<T(std::size_t)> func) {
            const auto handle = this->handle_allocator.create();
            this->handle_mapper.emplace(handle, func(std::get<0>(tuple)));
            return {
                handle,
                std::get<1>(tuple),
            };
        }

        [[nodiscard]] foundation::Option<T&> get(const types::HandleId& handle) noexcept {
            if (!this->handle_allocator.is_alive(handle)) {
                return {};
            }
            const auto& iter = this->handle_mapper.find(handle);
            if (iter == this->handle_mapper.end()) {
                return {};
            }
            return iter->second;
        }

        [[nodiscard]] foundation::Option<const T&> get(
            const types::HandleId& handle) const noexcept {
            if (!this->handle_allocator.is_alive(handle)) {
                return {};
            }
            const auto& iter = this->handle_mapper.find(handle);
            if (iter == this->handle_mapper.end()) {
                return {};
            }
            return iter->second;
        }

        void remove(const types::HandleId& handle) {
            this->handle_mapper.erase(handle);
            this->handle_allocator.destroy(handle);
        }
    };
} // namespace enishi::renderer