#pragma once
#include <memory>
#include <span>
#include <vector>

namespace enishi::types {
    enum class AccessFlag : std::uint8_t {
        None,
        ReadOnly,
        WriteOnly,
        ReadAndWrite,
    };

    enum class ResourceAccessFlag : std::uint8_t {
        CPURead,
        CPUWrite,
        GPURead,
        GPUWrite,
    };

    // 参照専用
    struct RenderData {
        std::span<const std::byte> bytes; // GPUへ渡すデータ
        std::uint32_t stride;             // 1つのデータ間隔

        constexpr std::size_t byte_width(void) const noexcept {
            return this->bytes.size_bytes();
        }

        const std::byte* raw_data(void) const noexcept {
            return this->bytes.data();
        }
    };

    template <typename T>
        requires std::is_trivially_copyable_v<T>
    class OwnedRenderData {
      private:
        std::vector<T> buffer;

      public:
        explicit OwnedRenderData(std::vector<T>&& buffer)
            : buffer(std::move(buffer)) {
        }

        T& operator[](const std::size_t index) {
            return this->buffer[index];
        }

        const T& operator[](const std::size_t index) const {
            return this->buffer[index];
        }

        [[nodiscard]] RenderData get_render_data(void) const {
            return RenderData{
                .bytes = std::as_bytes(std::span{this->buffer}),
                .stride = static_cast<std::uint32_t>(sizeof(T)),
            };
        }
    };
} // namespace enishi::types