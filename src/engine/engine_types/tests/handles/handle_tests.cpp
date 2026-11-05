#include "handle_tests.h"
#include "../../../core/tests/test_runner.h"
#include <engine_types/handle/handle_allocator.h>
#include <engine_types/handle/handle_mapper.h>
#include <memory>
#include <unordered_set>
#include <vector>

namespace enishi::tests {
    void allocator_tests() {
        constexpr std::size_t HANDLE_COUNT = 1024;
        types::HandleAllocator allocator;
        const types::HandleId invalid;
        check(!invalid.is_valid() && !allocator.is_alive(invalid), "default handle is invalid");
        allocator.destroy(invalid);
        std::unordered_set<types::HandleId> active;
        std::vector<types::HandleId> handles;
        for (std::size_t index = 0; index < HANDLE_COUNT; ++index) {
            const auto handle = allocator.create();
            check(handle.is_valid() && allocator.is_alive(handle) && active.insert(handle).second,
                "every live allocation has a unique valid ID");
            handles.push_back(handle);
        }
        for (const auto handle : handles) {
            allocator.destroy(handle);
            allocator.destroy(handle);
            check(!allocator.is_alive(handle),
                "destroy clears liveness and repeated destroy is harmless");
        }
        active.clear();
        for (std::size_t index = 0; index < HANDLE_COUNT; ++index) {
            const auto handle = allocator.create();
            check(active.insert(handle).second, "double destroy must not recycle a live ID twice");
        }
        check(active.insert(allocator.create()).second, "allocation after recycling uses a new ID");
    }

    void mapper_tests() {
        constexpr int FIRST = 17;
        constexpr int SECOND = 29;
        types::HandleMapper<std::unique_ptr<int>> mapper;
        auto owner = std::make_unique<int>(FIRST);
        const auto* address = owner.get();
        const auto first = mapper.emplace(std::move(owner));
        check(owner == nullptr && mapper.get(first).unwrap().get() == address,
            "mapper owns the resource without copying it");
        const auto& readonly = mapper;
        check(readonly.get(first).is_some() && *readonly[first] == FIRST,
            "const access can instantiate and observes the resource");
        mapper.remove(first);
        mapper.remove(first);
        check(!mapper.contains(first) && mapper.get(first).is_none(), "removed resource is absent");
        const auto replacement = mapper.emplace(std::make_unique<int>(FIRST));
        const auto next = mapper.emplace(std::make_unique<int>(SECOND));
        check(replacement != next && *mapper.get(next).unwrap() == SECOND,
            "repeated removal cannot overwrite a replacement resource");
        mapper.remove(types::HandleId{});
        check(mapper.get(replacement).is_some() && mapper.get(next).is_some(),
            "invalid removal preserves live resources");
        types::HandleMapper<std::size_t> sizes;
        auto payload = std::make_unique<int>(SECOND);
        const auto* payload_address = payload.get();
        const auto result = sizes.make_from<std::unique_ptr<int>>(
            std::tuple{std::size_t{FIRST}, std::move(payload)},
            [](std::size_t size) { return size; });
        check(std::get<1>(result).get() == payload_address &&
                  sizes.get(std::get<0>(result)).unwrap() == FIRST,
            "make_from transfers a move-only payload and constructs the mapped resource");
    }
} // namespace enishi::tests

int main() {
    enishi::tests::run_test("allocator_tests", enishi::tests::allocator_tests);
    enishi::tests::run_test("mapper_tests", enishi::tests::mapper_tests);
    return EXIT_SUCCESS;
}
