#pragma once
#include <chrono>
#include <cstdint>

namespace enishi::types {
    class DeltaTime {
      public:
        using Resolution = std::chrono::microseconds;

      public:
        static constexpr std::int64_t UNITS_PER_SECOND = Resolution::period::den;

      public:
        const Resolution delta_time;

      public:
        explicit constexpr DeltaTime(const std::chrono::nanoseconds& nano)
            : delta_time(std::chrono::duration_cast<DeltaTime::Resolution>(nano)) {
        }
        explicit constexpr DeltaTime(const std::chrono::microseconds& micro)
            : delta_time(std::chrono::duration_cast<DeltaTime::Resolution>(micro)) {
        }
        explicit constexpr DeltaTime(const std::chrono::milliseconds& mill)
            : delta_time(std::chrono::duration_cast<DeltaTime::Resolution>(mill)) {
        }
        explicit constexpr DeltaTime(const std::chrono::seconds& second)
            : delta_time(std::chrono::duration_cast<DeltaTime::Resolution>(second)) {
        }
        explicit constexpr DeltaTime(const float dt)
            : delta_time(std::chrono::duration_cast<DeltaTime::Resolution>(
                  std::chrono::duration<float>(dt))) {
        }
        explicit constexpr DeltaTime(const double dt)
            : delta_time(std::chrono::duration_cast<DeltaTime::Resolution>(
                  std::chrono::duration<double>(dt))) {
        }

        constexpr float to_float(void) const {
            return static_cast<float>(this->delta_time.count());
        }
        constexpr double to_double(void) const {
            return static_cast<double>(this->delta_time.count());
        }
        constexpr std::int64_t to_int64(void) const {
            return this->delta_time.count();
        }
        constexpr float to_float_second(void) const {
            return this->to_float() / static_cast<float>(DeltaTime::UNITS_PER_SECOND);
        }
        constexpr double to_double_second(void) const {
            return this->to_double() / static_cast<double>(DeltaTime::UNITS_PER_SECOND);
        }

        constexpr std::int64_t to_nano_second(void) const {
            return std::chrono::duration_cast<std::chrono::nanoseconds>(this->delta_time).count();
        }
        constexpr std::int64_t to_micro_second(void) const {
            return std::chrono::duration_cast<std::chrono::microseconds>(this->delta_time).count();
        }
        constexpr std::int64_t to_milli_second(void) const {
            return std::chrono::duration_cast<std::chrono::milliseconds>(this->delta_time).count();
        }
        constexpr std::int64_t to_second(void) const {
            return std::chrono::duration_cast<std::chrono::seconds>(this->delta_time).count();
        }

        constexpr bool operator<(const DeltaTime& dt) const {
            return this->delta_time < dt.delta_time;
        }
        constexpr bool operator>(const DeltaTime& dt) const {
            return this->delta_time > dt.delta_time;
        }
    };
} // namespace enishi::types