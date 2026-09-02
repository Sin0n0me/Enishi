#pragma once
#include <engine_types/handle/handle_allocator.h>
#include <engine_types/handle/handle_type.h>
#include <foundation/option/option.h>
#include <functional>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace enishi::types {
    template <typename V> class HandleMapper {
      private:
        HandleAllocator handle_allocator;
        std::unordered_map<types::HandleId, V> handle_mapper;

      public:
        using KeyType = types::HandleId;
        using ValueType = V;

      public:
        [[nodiscard]] types::HandleId emplace(V&& resource) {
            const auto handle = this->handle_allocator.create();
            this->handle_mapper.emplace(handle, std::move(resource));
            return handle;
        }

        template <typename... Args> [[nodiscard]] types::HandleId make(Args&&... args) {
            const auto handle = this->handle_allocator.create();
            this->handle_mapper.emplace(handle, V{std::forward<Args>(args)...});
            return handle;
        }

        template <typename U>
        [[nodiscard]] std::tuple<types::HandleId, U> make_from(
            std::tuple<std::size_t, U>&& tuple, std::function<V(std::size_t)> func) {
            const auto handle = this->handle_allocator.create();
            this->handle_mapper.emplace(handle, func(std::get<0>(tuple)));
            return {
                handle,
                std::get<1>(tuple),
            };
        }

        [[nodiscard]] foundation::Option<V&> get(const types::HandleId& handle) noexcept {
            if (!this->handle_allocator.is_alive(handle)) {
                return {};
            }
            const auto& iter = this->handle_mapper.find(handle);
            if (iter == this->handle_mapper.end()) {
                return {};
            }
            return iter->second;
        }

        [[nodiscard]] foundation::Option<const V&> get(
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

        V& operator[](const types::HandleId& key) {
            return this->handle_mapper[key];
        }
        const V& operator[](const types::HandleId& key) const {
            return this->handle_mapper[key];
        }
    };

    template <typename T> using ResourceMapper = types::HandleMapper<T>;
} // namespace enishi::types