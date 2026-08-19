#pragma once
#include <foundation/option/option.h>
#include <tuple>
#include <vector>

namespace enishi::renderer {
    template <typename T> class ResourcePool {
      private:
        std::vector<T> resources;

      public:
        [[nodiscard]] std::tuple<std::size_t, T&> emplace(T&& resource) {
            auto& vec = this->resources;
            const auto index = vec.size();
            auto& binding = vec.emplace_back(std::move(resource));
            return {index, binding};
        }

        template <typename... Args> [[nodiscard]] std::tuple<std::size_t, T&> make(Args&&... args) {
            auto& vec = this->resources;
            const auto index = vec.size();
            auto& binding = vec.emplace_back(T{std::forward<Args>(args)...});
            return {index, binding};
        }

        [[nodiscard]] foundation::Option<T&> get(const std::size_t index) noexcept {
            auto& vec = this->resources;
            if (vec.size() < index + 1) {
                return {};
            }
            return vec.at(index);
        }

        [[nodiscard]] foundation::Option<const T&> get(const std::size_t index) const noexcept {
            auto& vec = this->resources;
            if (vec.size() < index + 1) {
                return {};
            }
            return vec.at(index);
        }
    };
} // namespace enishi::renderer