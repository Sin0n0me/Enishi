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

        // 全要素のサイズ
        constexpr std::size_t byte_width(void) const noexcept {
            return this->bytes.size_bytes();
        }

        const std::byte* raw_data(void) const noexcept {
            return this->bytes.data();
        }
    };

    class OwnedRenderData {
      public:
        using RawDataType = std::vector<std::byte>;

      private:
        RawDataType buffer;
        std::size_t stride; // 1つのデータ間隔

      public:
        // 任意のVec<T>をバイト列へ
        template <typename T>
            requires std::is_trivially_copyable_v<T>
        explicit constexpr OwnedRenderData(const std::vector<T>& buffer)
            : buffer(OwnedRenderData::to_byte_vector(buffer))
            , stride(sizeof(T)) {
        }
        template <typename T>
            requires std::is_trivially_copyable_v<T>
        explicit constexpr OwnedRenderData(std::vector<T>&& buffer)
            : buffer(OwnedRenderData::to_byte_vector(buffer))
            , stride(sizeof(T)) {
        }

        explicit OwnedRenderData(RawDataType&& buffer, const std::uint32_t stride)
            : buffer(std::move(buffer))
            , stride(stride) {
        }
        OwnedRenderData(OwnedRenderData&&) noexcept = default;
        OwnedRenderData& operator=(OwnedRenderData&&) noexcept = default;

        explicit operator bool(void) const noexcept {
            return !this->buffer.empty();
        };

        std::byte& operator[](const std::size_t index) {
            return this->buffer[index];
        }
        const std::byte& operator[](const std::size_t index) const {
            return this->buffer[index];
        }

        template <typename T>
            requires std::is_trivially_copyable_v<T>
        void update(const T& new_value, const std::size_t index) {
            constexpr auto INPUT_STRIDE = sizeof(T);
            if (INPUT_STRIDE != this->stride) {
                return;
            }
            const auto offset = index * this->stride;
            if (this->buffer.size() < offset + 1) {
                return;
            }
            auto target_span = std::span(this->buffer).subspan(offset, INPUT_STRIDE);
            std::memcpy(target_span.data(), std::addressof(new_value), INPUT_STRIDE);
        }

        [[nodiscard]] constexpr RenderData get_render_data(void) const {
            return RenderData{
                .bytes = std::as_bytes(std::span{this->buffer}),
                .stride = static_cast<decltype(RenderData::stride)>(this->stride),
            };
        }

      private:
        template <typename T>
            requires std::is_trivially_copyable_v<T>
        static RawDataType to_byte_vector(const std::vector<T>& src) {
            const auto byte_size = src.size() * sizeof(T);
            RawDataType dest(byte_size);
            std::memcpy(dest.data(), src.data(), byte_size);
            return dest;
        }
    };
} // namespace enishi::types