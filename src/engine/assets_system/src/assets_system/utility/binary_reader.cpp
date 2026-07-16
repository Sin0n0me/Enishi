#include "binary_reader.h"
#include <expected>
#include <filesystem>
#include <format>
#include <type_traits>

namespace enishi::assets_system {
    detail::StreamPosGuard::StreamPosGuard(std::istream& stream)
        : stream(stream)
        , pos(stream.tellg())
        , state(stream.rdstate()) {
    }

    detail::StreamPosGuard::~StreamPosGuard(void) noexcept {
        this->stream.clear(this->state);
        if (!this->is_invalid()) {
            this->stream.seekg(this->pos);
        }
    }

    const std::streampos& detail::StreamPosGuard::get_position(void) const {
        return this->pos;
    }

    bool detail::StreamPosGuard::is_invalid(void) const noexcept {
        return this->pos == std::streampos(-1);
    }

    IOResult<BinaryReader> BinaryReader::make_reader(const std::filesystem::path& path) {
        BinaryReader reader{};

        reader.file = std::ifstream(path, std::ios::binary | std::ios::in);
        if (!reader.file.is_open()) {
            return foundation::Error(IOError::FileNotFound,
                path.string() + " というパスもしくはファイルは存在しない, または権限がありません");
        }

        return std::move(reader);
    }

    IOResult<void> BinaryReader::read(void* data, const std::streamsize size) {
        if (!bool(this->file)) {
            return foundation::Error(IOError::ReadFailed);
        }

        this->file.read(reinterpret_cast<char*>(data), size);
        const auto read_size = this->file.gcount();

        if (read_size != size) {
            const std::string read_result_message =
                std::format("期待したサイズ: {} 実際に読み取ったサイズ: {}", size, read_size);
            if (this->file.eof()) {
                return foundation::Error(IOError::UnexpectedEof,
                    std::format("予期しない終端です {}", read_result_message));
            }

            if (this->file.bad()) {
                return foundation::Error(IOError::ReadFailed,
                    std::format("ファイルの読み取りに失敗しました {}", read_result_message));
            }

            return foundation::Error(IOError::InvalidData,
                std::format("データの読み取りに失敗しました {}", read_result_message));
        }

        return {};
    }

    IOResult<std::vector<std::uint8_t>> BinaryReader::read_all(void) {
        detail::StreamPosGuard stream_pos{this->file};
        if (stream_pos.is_invalid()) {
            return foundation::Error(
                IOError::InvalidStreamPosition, "Streamが無効な位置を指しています");
        }

        // 一旦終端まで移動しtellgで現在位置からサイズを求める
        this->file.seekg(0, std::ios::end);
        const std::streamsize size = this->file.tellg();
        if (size < 0) {
            return foundation::Error(IOError::InvalidStreamPosition);
        }
        this->file.seekg(0, std::ios::beg);

        std::vector<std::uint8_t> vec;
        const auto result = this->read_to_vec(vec, size);

        if (result.is_err()) {
            return result.unwrap_err();
        }

        return vec;
    }

    IOResult<void> BinaryReader::read_magic_number_from_str(const std::string_view& expect) {
        detail::StreamPosGuard stream_pos{this->file};
        if (stream_pos.is_invalid()) {
            return foundation::Error(
                IOError::InvalidStreamPosition, "Streamが無効な位置を指しています");
        }

        // 先頭に戻す
        this->file.seekg(0, std::ios::beg);

        // 比較用に一時バッファ作成
        const std::size_t size = expect.size();
        std::vector<char> buff(size);
        const auto result = this->read(buff.data(), sizeof(char) * size);
        if (result.is_err()) {
            return result.unwrap_err();
        }

        // マジックナンバーの比較
        if (std::memcmp(buff.data(), expect.data(), size) != 0) {
            const std::string mes =
                std::format("予期しないファイルヘッダです\n期待した値: {}\n実際の値: {}",
                    expect,
                    std::string(buff.begin(), buff.end()));
            return foundation::Error(IOError::MismatchHeader, mes);
        }

        return {};
    }
} // namespace enishi::assets_system