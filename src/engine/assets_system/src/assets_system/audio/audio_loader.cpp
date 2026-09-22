#include "audio_loader.h"

#define NOMINMAX
#include <miniaudio.h>

namespace enishi::assets_system {
    class DecoderGuard {
      private:
        ma_decoder decoder{};
        bool initialized = false;

      public:
        DecoderGuard(void) = default;
        ~DecoderGuard() {
            if (this->initialized) {
                ma_decoder_uninit(&this->decoder);
            }
        }

        DecoderGuard(const DecoderGuard&) = delete;
        DecoderGuard& operator=(const DecoderGuard&) = delete;

        ma_decoder* get() {
            return &this->decoder;
        }

        void mark_initialized() {
            this->initialized = true;
        }
    };

    [[nodiscard]]
    ma_result init_decoder(const std::filesystem::path& path,
        const ma_decoder_config& config,
        ma_decoder* const decoder) {
        return ma_decoder_init_file(path.string<char>().c_str(), &config, decoder);
    }

    foundation::Result<types::AssetData, AssetError> AudioLoader::load(
        const std::filesystem::path& path) noexcept {
        // キャッシュがあれば使用
        const auto iter = this->cache.find(path.lexically_normal());
        if (iter != this->cache.end()) {
            return types::AssetData{iter->second};
        }

        constexpr ma_format output_format = ma_format_f32;
        constexpr ma_uint32 output_channels = 0;
        constexpr ma_uint32 output_sample_rate = 0;

        const auto config =
            ma_decoder_config_init(output_format, output_channels, output_sample_rate);

        DecoderGuard decoder_guard;

        const ma_result init_result = init_decoder(path, config, decoder_guard.get());

        if (init_result != MA_SUCCESS) {
            return foundation::Error(
                AssetError::InitError, "Failed to initialize audio decoder: " + path.string());
        }

        decoder_guard.mark_initialized();

        ma_uint64 frame_count = 0;

        const ma_result length_result =
            ma_decoder_get_length_in_pcm_frames(decoder_guard.get(), &frame_count);

        if (length_result != MA_SUCCESS) {
            return foundation::Error(
                AssetError::InvalidAssetData, "Failed to get audio length: " + path.string());
        }

        const ma_uint32 channels = decoder_guard.get()->outputChannels;
        const ma_uint32 sample_rate = decoder_guard.get()->outputSampleRate;

        if (channels == 0 || sample_rate == 0) {
            return foundation::Error(
                AssetError::InvalidAssetData, "Invalid audio format: " + path.string());
        }

        constexpr auto max_size = static_cast<ma_uint64>(std::numeric_limits<std::size_t>::max());

        if (frame_count > max_size / channels) {
            return foundation::Error(
                AssetError::InvalidAssetData, "Audio file is too large: " + path.string());
        }

        const auto sample_count = static_cast<std::size_t>(frame_count * channels);

        auto audio_data = std::make_shared<types::AudioData>();
        audio_data->samples = std::vector<float>(sample_count);
        audio_data->channels = channels;
        audio_data->sample_rate = sample_rate;

        ma_uint64 frames_read = 0;

        const ma_result read_result = ma_decoder_read_pcm_frames(
            decoder_guard.get(), audio_data->samples.data(), frame_count, &frames_read);

        if (read_result != MA_SUCCESS && read_result != MA_AT_END) {
            return foundation::Error(
                AssetError::InvalidAssetData, "Failed to decode audio: " + path.string());
        }

        if (frames_read != frame_count) {
            const auto actual_sample_count = static_cast<std::size_t>(frames_read * channels);

            audio_data->samples.resize(actual_sample_count);
        }

        return types::AssetData{audio_data};
    }

    std::vector<foundation::UTF8> AudioLoader::get_supported_extension(void) const noexcept {
        return {
            ".mp3",
            ".wav",
        };
    }

    types::AssetKind AudioLoader::get_target_asset_type(void) const noexcept {
        return types::AssetKind::Audio;
    }
} // namespace enishi::assets_system