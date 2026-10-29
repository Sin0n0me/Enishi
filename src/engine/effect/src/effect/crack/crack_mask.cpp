#include "crack_mask.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <numbers>

namespace enishi::effect {
    namespace {
        constexpr std::uint32_t MAX_MASK_SIZE = 2048;
        constexpr std::uint32_t MIN_MASK_SIZE = 32;
        constexpr std::uint32_t MAX_CELLS = 32;
        constexpr std::size_t CHANNELS = 4;
        constexpr float BYTE_MAX = 255.0f;
        constexpr float MAX_WIDTH = 0.2f;
        constexpr float MAX_DEPTH = 0.1f;
        constexpr float MAX_VIEW_SLOPE = 2.0f;
        constexpr float JITTER_MARGIN = 0.2f;
        constexpr float JITTER_SPAN = 0.6f;
        constexpr int NEIGHBOR_RADIUS = 2;
        struct Point {
            float x;
            float y;
        };
        std::uint32_t hash(std::uint32_t value) {
            constexpr std::uint32_t MIX_A = 0x7feb352d;
            constexpr std::uint32_t MIX_B = 0x846ca68b;
            constexpr unsigned SHIFT_A = 16;
            constexpr unsigned SHIFT_B = 15;
            value = (value ^ (value >> SHIFT_A)) * MIX_A;
            value = (value ^ (value >> SHIFT_B)) * MIX_B;
            return value ^ (value >> SHIFT_A);
        }
        Point feature(int x, int y, std::uint32_t seed) {
            constexpr std::uint32_t X_SALT = 0x9e3779b9;
            constexpr std::uint32_t Y_SALT = 0x85ebca6b;
            constexpr std::uint32_t AXIS_SALT = 0xc2b2ae35;
            constexpr std::uint32_t FRACTION_MASK = 0xffff;
            constexpr float FRACTION_SCALE = 1.0f / static_cast<float>(FRACTION_MASK);
            const auto value = hash(static_cast<std::uint32_t>(x) * X_SALT ^
                                    static_cast<std::uint32_t>(y) * Y_SALT ^ seed);
            // One feature per cell: p = cell + margin + span * random_fraction.
            return {static_cast<float>(x) + JITTER_MARGIN +
                        JITTER_SPAN * static_cast<float>(value & FRACTION_MASK) * FRACTION_SCALE,
                static_cast<float>(y) + JITTER_MARGIN +
                    JITTER_SPAN * static_cast<float>(hash(value ^ AXIS_SALT) & FRACTION_MASK) *
                        FRACTION_SCALE};
        }
        float squared_distance(Point a, Point b) {
            const float x = a.x - b.x;
            const float y = a.y - b.y;
            // Squared Euclidean distance avoids square roots in the nearest-feature search.
            return x * x + y * y;
        }
        float boundary_distance(Point sample, std::uint32_t seed, int period = 0) {
            const int cx = static_cast<int>(std::floor(sample.x));
            const int cy = static_cast<int>(std::floor(sample.y));
            constexpr std::size_t NEIGHBOR_SIDE = 2 * NEIGHBOR_RADIUS + 1;
            constexpr std::size_t NEIGHBOR_COUNT = NEIGHBOR_SIDE * NEIGHBOR_SIDE;
            std::array<Point, NEIGHBOR_COUNT> candidates{};
            std::size_t count = 0;
            std::size_t nearest = 0;
            float nearest_squared = std::numeric_limits<float>::max();
            for (int y = -NEIGHBOR_RADIUS; y < NEIGHBOR_RADIUS + 1; ++y) {
                for (int x = -NEIGHBOR_RADIUS; x < NEIGHBOR_RADIUS + 1; ++x) {
                    const int column = cx + x;
                    // Hash wrapped angular cells but keep their unwrapped positions across the
                    // seam.
                    const int wrapped = period == 0 ? column : (column % period + period) % period;
                    auto point = feature(wrapped, cy + y, seed);
                    point.x += static_cast<float>(column - wrapped);
                    const float distance = squared_distance(sample, point);
                    if (distance < nearest_squared) {
                        nearest = count;
                        nearest_squared = distance;
                    }
                    candidates[count++] = point;
                }
            }
            float edge = std::numeric_limits<float>::max();
            for (std::size_t index = 0; index < count; ++index) {
                if (index == nearest) {
                    continue;
                }
                const float separation =
                    std::sqrt(squared_distance(candidates[index], candidates[nearest]));
                // Distance to the bisector: (|x-b|^2 - |x-a|^2) / (2 * |b-a|).
                edge = std::min(edge,
                    (squared_distance(sample, candidates[index]) - nearest_squared) /
                        (2.0f * separation));
            }
            return std::max(edge, 0.0f);
        }
        float radial_distance(Point position, const CrackSettings& settings) {
            constexpr float FULL_TURN = 2.0f * std::numbers::pi_v<float>;
            constexpr float IMPACT_CORE_FRACTION = 0.125f;
            const float radius = std::sqrt(position.x * position.x + position.y * position.y);
            if (radius < settings.impact_radius * IMPACT_CORE_FRACTION) {
                return 0.0f;
            }
            const float spokes = static_cast<float>(settings.radial_spokes);
            // Polar Voronoi coordinates: u = angle * spokes / 2pi,
            // v = log(1 + radius / core_radius) / log(growth).
            // Thus ring radii r(n) = core_radius * (growth^n - 1) spread out from the impact.
            const float angle = std::atan2(position.y, position.x) / FULL_TURN * spokes;
            const float ring =
                std::log1p(radius / settings.impact_radius) / std::log(settings.ring_growth);
            const int period = static_cast<int>(settings.radial_spokes);
            float distance = boundary_distance({angle, ring}, settings.seed, period);
            const int sector = static_cast<int>(std::floor(angle));
            // Seeded spoke angles extend continuously from the impact; Voronoi edges connect them.
            for (int offset = -1; offset < 2; ++offset) {
                const int column = sector + offset;
                const int wrapped = (column % period + period) % period;
                const float spoke =
                    feature(wrapped, 0, settings.seed).x + static_cast<float>(column - wrapped);
                distance = std::min(distance, std::abs(angle - spoke));
            }
            return distance;
        }
        float smooth(float value) {
            const float t = std::clamp(value, 0.0f, 1.0f);
            // Cubic Hermite interpolation s(t) = t^2 * (3 - 2t).
            return t * t * (3.0f - 2.0f * t);
        }
        std::uint8_t encode(float value) {
            return static_cast<std::uint8_t>(std::lround(std::clamp(value, 0.0f, 1.0f) * BYTE_MAX));
        }
    } // namespace
    foundation::Result<void, EffectError> validate_crack_settings(const CrackSettings& settings) {
        const std::array scalars{settings.impact_radius,
            settings.ring_growth,
            settings.width,
            settings.feather,
            settings.depth,
            settings.view_x,
            settings.view_y,
            settings.duration_seconds,
            settings.fade_seconds,
            settings.opacity};
        constexpr std::uint32_t MIN_SPOKES = 6;
        constexpr std::uint32_t MAX_SPOKES = 64;
        constexpr float MIN_IMPACT_RADIUS = 0.01f;
        constexpr float MAX_IMPACT_RADIUS = 0.2f;
        constexpr float MIN_RING_GROWTH = 1.2f;
        constexpr float MAX_RING_GROWTH = 3.0f;
        if ((settings.pattern != CrackPattern::Voronoi &&
                settings.pattern != CrackPattern::Radial) ||
            settings.radial_spokes < MIN_SPOKES || settings.radial_spokes > MAX_SPOKES ||
            settings.impact_radius < MIN_IMPACT_RADIUS ||
            settings.impact_radius > MAX_IMPACT_RADIUS || settings.ring_growth < MIN_RING_GROWTH ||
            settings.ring_growth > MAX_RING_GROWTH ||
            !std::all_of(
                scalars.begin(), scalars.end(), [](float value) { return std::isfinite(value); }) ||
            settings.mask_size < MIN_MASK_SIZE || settings.mask_size > MAX_MASK_SIZE ||
            settings.cells == 0 || settings.cells > MAX_CELLS || !(settings.width > 0) ||
            settings.width > MAX_WIDTH || !(settings.feather > 0) || settings.feather > MAX_WIDTH ||
            settings.depth < 0 || settings.depth > MAX_DEPTH ||
            std::abs(settings.view_x) > MAX_VIEW_SLOPE ||
            std::abs(settings.view_y) > MAX_VIEW_SLOPE || !(settings.duration_seconds > 0) ||
            settings.fade_seconds < 0 || settings.fade_seconds > settings.duration_seconds ||
            settings.opacity < 0 || settings.opacity > 1) {
            return foundation::Error{EffectError::InvalidArgument,
                "Invalid crack mask, depth, view, opacity or timing settings"};
        }
        return {};
    }
    foundation::Result<types::TextureData, EffectError> make_crack_mask(
        const CrackSettings& settings) {
        auto valid = validate_crack_settings(settings);
        if (valid.is_err()) {
            return std::move(valid).take_err();
        }
        types::TextureData texture;
        texture.format = types::TextureFormat::RGBA8_UNORM;
        texture.is_cubemap = false;
        texture.width = settings.mask_size;
        texture.height = settings.mask_size;
        texture.depth = 1;
        texture.array_size = 1;
        types::MipData mip;
        mip.width = texture.width;
        mip.height = texture.height;
        mip.row_pitch = static_cast<std::uint32_t>(CHANNELS) * texture.width;
        mip.slice_pitch = mip.row_pitch * texture.height;
        mip.pixels.resize(mip.slice_pitch);
        for (std::uint32_t y = 0; y < texture.height; ++y) {
            for (std::uint32_t x = 0; x < texture.width; ++x) {
                // Pixel centers in cell coordinates: p = (pixel + 0.5) * cells / resolution.
                const Point sample{(static_cast<float>(x) + 0.5f) *
                                       static_cast<float>(settings.cells) /
                                       static_cast<float>(texture.width),
                    (static_cast<float>(y) + 0.5f) * static_cast<float>(settings.cells) /
                        static_cast<float>(texture.height)};
                // The impact mask covers [-1, 1]^2, independently of its eventual screen origin.
                const Point radial_position{
                    2.0f * (static_cast<float>(x) + 0.5f) / texture.width - 1.0f,
                    2.0f * (static_cast<float>(y) + 0.5f) / texture.height - 1.0f};
                const float distance = settings.pattern == CrackPattern::Radial
                                           ? radial_distance(radial_position, settings)
                                           : boundary_distance(sample, settings.seed);
                const std::size_t offset = static_cast<std::size_t>(y) * mip.row_pitch +
                                           static_cast<std::size_t>(x) * CHANNELS;
                // Groove depth and edge coverage are independent, avoiding a solid polygon fill.
                mip.pixels[offset] = encode(1.0f - smooth(distance / settings.width));
                mip.pixels[offset + 1] =
                    encode(1.0f - smooth((distance - settings.width) / settings.feather));
            }
        }
        texture.mips.push_back(std::move(mip));
        return texture;
    }
} // namespace enishi::effect
