#pragma once
#include <cstdint>
#include <vector>

namespace enishi::types {
    struct AudioData {
        std::vector<float> samples;
        std::uint32_t channels = 0;
        std::uint32_t sample_rate = 0;

        [[nodiscard]]
        constexpr std::uint64_t frame_count(void) const noexcept {
            return this->channels == 0 ? 0 : this->samples.size() / this->channels;
        }
    };
} // namespace enishi::types