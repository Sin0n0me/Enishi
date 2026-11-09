#include "option_tests.h"
#include <test_support/test_runner.h>
#include "option_lookup_tests.h"
#include <foundation/option/option.h>
#include <memory>
#include <optional>
#include <utility>

namespace enishi::tests {
    void option_value_tests() {
        constexpr int VALUE = 42;
        const foundation::Option<int> empty;
        const foundation::Option<int> absent = std::nullopt;
        check(empty.is_none() && absent.is_none(), "default and nullopt constructors");
        foundation::Option<int> reset = int{VALUE};
        reset = std::nullopt;
        check(reset.is_none(), "nullopt assignment clears a value");
        const foundation::Option<int> value = int{VALUE};
        check(value.is_some() && value.unwrap() == VALUE, "value constructor");
        const auto copied = value;
        auto movable = value;
        const auto moved = std::move(movable);
        check(copied.unwrap() == VALUE && moved.unwrap() == VALUE, "copy and move constructors");
        const std::optional<int> standard{VALUE};
        const foundation::Option<int> from_copy = standard;
        const foundation::Option<int> from_move = std::optional<int>{VALUE};
        const foundation::Option<int> from_empty = std::optional<int>{};
        check(from_copy.unwrap() == VALUE && from_move.unwrap() == VALUE,
            "inherited optional storage constructors");
        check(from_empty.is_none(), "empty optional storage constructor");
        auto owner = std::make_unique<int>(VALUE);
        const auto* address = owner.get();
        foundation::Option<std::unique_ptr<int>> owned = std::move(owner);
        foundation::Option<std::unique_ptr<int>> moved_owner = std::move(owned);
        check(owner == nullptr && moved_owner.unwrap().get() == address,
            "move-only construction preserves ownership");
        const auto mapped = value.map([](int number) { return std::to_string(number); });
        const auto mapped_empty = empty.map([](int number) { return std::to_string(number); });
        check(mapped.unwrap() == std::to_string(VALUE) && mapped_empty.is_none(),
            "map constructs another Option specialization");
    }

    void option_pointer_tests() {
        constexpr int VALUE = 42;
        int value = VALUE;
        int* pointer = &value;
        const foundation::Option<int*> from_lvalue = pointer;
        const foundation::Option<int*> from_rvalue = &value;
        const foundation::Option<const int*> constant = &value;
        check(from_lvalue.unwrap() == pointer && from_rvalue.unwrap() == pointer &&
                  constant.unwrap() == pointer,
            "pointer and const pointer constructors preserve identity");
        const foundation::Option<int*> null_pointer = static_cast<int*>(nullptr);
        const foundation::Option<int*> absent = std::nullopt;
        check(null_pointer.is_some() && null_pointer.unwrap() == nullptr && absent.is_none(),
            "engaged null pointer differs from an absent Option");
    }

    void option_reference_tests() {
        constexpr int INITIAL = 42;
        constexpr int UPDATED = 43;
        int value = INITIAL;
        foundation::Option<int&> reference = value;
        const foundation::Option<const int&> constant = value;
        check(&reference.unwrap() == &value && &constant.unwrap() == &value,
            "reference constructors preserve identity");
        reference.unwrap_mut() = UPDATED;
        check(value == UPDATED && constant.unwrap() == UPDATED,
            "reference Options observe the original object");
        const foundation::Option<int&> wrapped = std::ref(value);
        const foundation::Option<int&> absent = std::nullopt;
        check(&wrapped.unwrap() == &value && absent.is_none(),
            "reference wrapper and nullopt constructors");
    }
} // namespace enishi::tests

int main() {
    enishi::tests::run_test("option_lookup_tests", enishi::tests::option_lookup_tests);
    enishi::tests::run_test("option_value_tests", enishi::tests::option_value_tests);
    enishi::tests::run_test("option_pointer_tests", enishi::tests::option_pointer_tests);
    enishi::tests::run_test("option_reference_tests", enishi::tests::option_reference_tests);
    return EXIT_SUCCESS;
}
