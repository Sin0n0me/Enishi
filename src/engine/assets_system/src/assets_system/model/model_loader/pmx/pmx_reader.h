#pragma once
#include "../../../errors/errors.h"
#include "pmx_data.h"
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <foundation/result/result.h>
#include <span>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace enishi::assets_system {
    class PMXReader {
      private:
        std::span<const std::uint8_t> bytes;
        std::size_t position{};
        std::string error;
        std::string section{"header"};

      public:
        explicit PMXReader(std::span<const std::uint8_t> bytes);

        [[nodiscard]] foundation::Result<PMXData, AssetError> parse();

      private:
        bool require(bool condition, const char* message);

        template <typename T> void read(T& value) {
            if (!this->require(
                    !(sizeof(T) > this->bytes.size() - this->position), "unexpected end of file")) {
                return;
            }
            if constexpr (std::is_integral_v<T>) {
                std::make_unsigned_t<T> bits{};
                for (std::size_t i = 0; i < sizeof(T); ++i) {
                    bits |= static_cast<std::make_unsigned_t<T>>(this->bytes[this->position++])
                            << (8 * i);
                }
                value = std::bit_cast<T>(bits);
            } else {
                static_assert(std::is_same_v<T, float>);
                std::uint32_t bits{};
                this->read(bits);
                value = std::bit_cast<float>(bits);
                this->require(std::isfinite(value), "non-finite float");
            }
        }

        template <typename T, std::size_t N> void read(std::array<T, N>& values) {
            for (auto& value : values) {
                this->read(value);
            }
        }

        template <typename... T> void fields(T&... values) {
            (this->read(values), ...);
        }

        std::int32_t index(std::uint8_t size, bool vertex = false);
        std::int32_t count(std::size_t minimum_size);

        template <typename T, typename F>
        void records(std::vector<T>& values, std::size_t minimum_size, F read_record) {
            const auto size = this->count(minimum_size);
            for (std::int32_t i = 0; i < size && this->error.empty(); ++i) {
                T value{};
                read_record(value);
                if (this->error.empty()) {
                    values.emplace_back(std::move(value));
                }
            }
        }

        void text(std::string& value, std::uint8_t encoding);
        void read_utf8(std::string& value, std::size_t end);
        void read_utf16(std::string& value, std::size_t end);
        bool read_utf8_code_point(std::size_t end, std::uint8_t first);
        std::uint32_t read_utf16_code_point(std::size_t end);
        static void append_utf8(std::string& value, std::uint32_t code);

        [[nodiscard]] foundation::Result<void, AssetError> read_header(PMXData& data);
        void read_vertices(PMXData& data);
        void read_indices(PMXData& data);
        void read_textures(PMXData& data);
        void read_materials(PMXData& data);
        void read_bones(PMXData& data);
        void read_morphs(PMXData& data);
        void read_display_frames(PMXData& data);
        void read_rigid_bodies(PMXData& data);
        void read_joints(PMXData& data);
        void read_soft_bodies(PMXData& data);
    };

    [[nodiscard]] foundation::Result<PMXData, AssetError> parse_pmx(
        std::span<const std::uint8_t> bytes);
} // namespace enishi::assets_system
