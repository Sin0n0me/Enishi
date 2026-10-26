#include "template_tests.h"
#include <concepts>
#include <cstdlib>
#include <ecs/component/component_pool.h>
#include <foundation/errors/errors.h>
#include <foundation/result/result.h>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace enishi::ecs::tests {
    namespace {
        enum class SourceError { ReadFailed };
        enum class TargetError { LoadFailed };

        static_assert(
            std::constructible_from<foundation::Result<std::unique_ptr<int>&, SourceError>,
                std::unique_ptr<int>&>);
        static_assert(
            !std::constructible_from<foundation::Result<std::unique_ptr<int>&, SourceError>,
                std::unique_ptr<int>&&>);

        void require(bool condition, const char* message) {
            if (!condition) {
                throw std::runtime_error(message);
            }
        }
    } // namespace

    void component_pool_cases() {
        ComponentPool<std::unique_ptr<int>> pool;
        const EntityID first{7};
        const EntityID second{42};
        constexpr int initial = 10;
        constexpr int replacement = 20;
        constexpr int ignored = 30;
        constexpr int other = 40;
        auto& inserted = pool.insert_or_replace(first, std::make_unique<int>(initial));
        require(*inserted == initial && pool.has(first), "Insert missing component");
        auto& replaced = pool.insert_or_replace(first, std::make_unique<int>(replacement));
        require(&inserted == &replaced && *replaced == replacement, "Replace in place");
        auto candidate = std::make_unique<int>(ignored);
        auto& retained = pool.insert_or_ignore(first, std::move(candidate));
        require(&retained == &replaced && *retained == replacement && candidate != nullptr,
            "Ignore preserves existing component and does not consume candidate");
        require(*pool.insert_or_ignore(second, std::make_unique<int>(other)) == other,
            "Ignore inserts a missing component");
        pool.remove(first);
        require(!pool.has(first) && pool.has(second), "Remove updates sparse indices");
        require(*pool.insert_or_replace(second, std::make_unique<int>(replacement)) == replacement,
            "Replace after dense relocation");
        require(*pool.insert_or_ignore(first, std::make_unique<int>(initial)) == initial,
            "Reinsert removed entity");
        const auto& const_pool = pool;
        require(*const_pool.get(first).unwrap() == initial &&
                    *const_pool.get(second).unwrap() == replacement,
            "Const lookup uses entity indices");
        require(pool.emplace(first, std::make_unique<int>(other)).is_err(),
            "Duplicate emplace returns an error");
    }

    void error_conversion_cases() {
        foundation::Error<SourceError> source(SourceError::ReadFailed, "first");
        source.add_message("second");
        foundation::Error<TargetError> converted(TargetError::LoadFailed, std::move(source));
        require(converted.get_error() == TargetError::LoadFailed &&
                    converted.get_message() == "first\nsecond",
            "Error conversion preserves messages and changes error code");
        converted.add_message("third");
        require(
            converted.get_message() == "first\nsecond\nthird", "Converted error remains usable");
    }
} // namespace enishi::ecs::tests

int main() {
    try {
        enishi::ecs::tests::component_pool_cases();
        enishi::ecs::tests::error_conversion_cases();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
