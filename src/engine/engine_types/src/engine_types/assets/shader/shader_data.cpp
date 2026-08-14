#include "shader_data.h"

namespace enishi::types {
    std::size_t ShaderData::hash(void) const {
        constexpr auto MAGIC_NUMBER = 0x9e3779b9;
        const auto type = static_cast<std::size_t>(ShaderData::binary_type);
        std::size_t hash = this->code.size() ^ (type << 8); // 初期シードの代わり
        for (auto byte : this->code) {
            // hash_combine アルゴリズム
            hash ^= static_cast<std::size_t>(byte) + MAGIC_NUMBER + (hash << 6) + (hash >> 2);
        }

        return hash;
    }

    bool ShaderData::operator==(const ShaderData& other) const {
        return this->hash() == other.hash();
    }
} // namespace enishi::types