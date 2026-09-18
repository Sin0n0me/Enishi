#pragma once
#include "../errors/errors.h"
#include <concepts>
#include <cstring>
#include <expected>
#include <filesystem>
#include <format>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace enishi::assets_system {
    namespace detail {
        // 現在の読み込み位置を保存し, スコープを抜けたときに元の位置に戻す
        class StreamPosGuard {
          private:
            std::istream& stream;
            std::streampos pos;
            std::ios::iostate state;

          public:
            explicit StreamPosGuard(std::istream& stream);
            ~StreamPosGuard(void) noexcept;

            [[nodiscard]] const std::streampos& get_position(void) const;

            [[nodiscard]] bool is_invalid(void) const noexcept;
        };
    } // namespace detail

    class BinaryReader {
      private:
        std::ifstream file;

      public:
        BinaryReader(void) noexcept = default;
        BinaryReader(BinaryReader&&) noexcept = default;
        BinaryReader& operator=(BinaryReader&&) noexcept = default;
        BinaryReader(const BinaryReader&) = delete;
        BinaryReader& operator=(const BinaryReader&) = delete;

      public:
        [[nodiscard]] static IOResult<BinaryReader> make_reader(const std::filesystem::path& path);

        [[nodiscard]] IOResult<void> read(void* data, const std::streamsize size);

        template <typename T>
            requires std::is_trivially_copyable_v<T>
        [[nodiscard]] IOResult<void> read_to(T* data) {
            return this->read(data, sizeof(T));
        }

        template <typename T>
            requires std::is_trivially_copyable_v<T>
        [[nodiscard]] IOResult<void> read_to_vec(std::vector<T>& data, const std::uint64_t count) {
            std::vector<T> temp(count);
            auto&& result = this->read(temp.data(), sizeof(T) * count);
            if (result.is_err()) {
                return result;
            }
            data = std::move(temp);
            return {};
        }

        [[nodiscard]] IOResult<std::vector<std::uint8_t>> read_all(void);

        [[nodiscard]] IOResult<void> read_magic_number_from_str(const std::string_view& expect);

        template <typename T>
            requires std::is_trivially_copyable_v<T>
        [[nodiscard]] IOResult<void> read_magic_number(const T expect) {
            detail::StreamPosGuard stream_pos{this->file};
            if (stream_pos.is_invalid()) {
                return foundation::Error(
                    IOError::InvalidStreamPosition, "Streamが無効な位置を指しています");
            }

            // 先頭に戻す
            this->file.seekg(0, std::ios::beg);

            T found{};
            auto&& result = this->read_to(&found);
            if (result.is_err()) {
                return result;
            }

            // マジックナンバーの比較
            if (found != expect) {
                return foundation::Error(IOError::MismatchHeader,
                    std::format("mismatch header. expect: {} found: {}", expect, found));
            }

            return {};
        }
    };
} // namespace enishi::assets_system
