#pragma once
#include <foundation/option/option.h>
#include <span>
#include <tuple>
#include <vector>

namespace enishi::foundation {
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

        [[nodiscard]] Option<T&> get(const std::size_t index) noexcept {
            auto& vec = this->resources;
            if (vec.size() < index + 1) {
                return {};
            }
            return vec.at(index);
        }

        [[nodiscard]] Option<const T&> get(const std::size_t index) const noexcept {
            auto& vec = this->resources;
            if (vec.size() < index + 1) {
                return {};
            }
            return vec.at(index);
        }

        [[nodiscard]] std::span<const T&> get_all(void) const noexcept {
            return this->resources;
        }
    };
} // namespace enishi::foundation