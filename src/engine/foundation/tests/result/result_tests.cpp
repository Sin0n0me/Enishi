#include "result_tests.h"
#include <test_support/test_runner.h>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

namespace enishi::tests {
    namespace {
        enum class SourceError { Failed };
        enum class TargetError { Rejected };
        constexpr int VALUE = 42;
        constexpr int FALLBACK = 7;
        using IntResult = foundation::Result<int, SourceError>;
        using ReferenceResult = foundation::Result<int&, SourceError>;
        using OwnerResult = foundation::Result<std::unique_ptr<int>, SourceError>;
        static_assert(!std::is_constructible_v<ReferenceResult, int&&>);
        static_assert(
            std::is_same_v<decltype(std::declval<const ReferenceResult&>().unwrap()), const int&>);
        static_assert(
            std::is_same_v<decltype(std::declval<ReferenceResult&&>().unwrap_mut()), int&>);
    } // namespace

    void result_value_tests() {
        IntResult value = VALUE;
        check(value.is_ok() && !value.is_err() && value.unwrap() == VALUE, "value construction");
        value.unwrap_mut() = FALLBACK;
        check(value.unwrap() == FALLBACK, "mutable access");
        auto pointer = std::make_unique<int>(VALUE);
        const auto* address = pointer.get();
        OwnerResult owner = std::move(pointer);
        OwnerResult moved = std::move(owner);
        auto taken = std::move(moved).unwrap_mut();
        check(pointer == nullptr && taken.get() == address,
            "move-only ownership survives construction and extraction");
        OwnerResult fallback = foundation::Error(SourceError::Failed, "missing");
        check(*std::move(fallback).unwrap_or(std::make_unique<int>(FALLBACK)) == FALLBACK,
            "rvalue fallback accepts move-only values");
        const IntResult failure = foundation::Error(SourceError::Failed, "missing");
        check(failure.unwrap_or(FALLBACK) == FALLBACK && failure.unwrap_or_default() == 0,
            "error fallbacks");
        foundation::Result<long, SourceError> converted = IntResult{VALUE};
        check(converted.unwrap() == VALUE, "value conversion");
        foundation::VoidResult<SourceError> done;
        check(done.is_ok(), "void default is success");
    }

    void result_reference_tests() {
        int value = VALUE;
        int fallback = FALLBACK;
        ReferenceResult reference = value;
        check(&reference.unwrap() == &value, "const access preserves referenced identity");
        reference.unwrap_mut() = FALLBACK;
        check(value == FALLBACK, "reference mutation reaches original object");
        check(&std::move(reference).unwrap_mut() == &value,
            "rvalue reference result does not move the referent");
        const ReferenceResult failure = foundation::Error(SourceError::Failed, "missing");
        check(&failure.unwrap_or(std::ref(fallback)) == &fallback,
            "reference fallback preserves identity");
        foundation::Result<const int&, SourceError> constant = value;
        check(&constant.unwrap() == &value, "const reference construction");
    }

    void result_error_tests() {
        IntResult failure = foundation::Error(SourceError::Failed, "first");
        check(failure.is_err() && !failure.is_ok(), "error construction");
        check(&failure.add_message("second") == &failure, "lvalue message append returns itself");
        foundation::Result<void, TargetError> propagated =
            failure.propagation(TargetError::Rejected);
        check(propagated.is_err() && propagated.unwrap_err().get_error() == TargetError::Rejected &&
                  propagated.unwrap_err().get_message() == "first\nsecond",
            "propagation preserves context and replaces code");
        check(failure.unwrap_err().get_message() == "first\nsecond",
            "const propagation preserves source");
        auto annotated = std::move(failure).add_message("third");
        const auto error = std::move(annotated).take_err();
        check(error.get_message() == "first\nsecond\nthird",
            "rvalue annotation and error extraction");
        foundation::Result<long, SourceError> converted =
            IntResult{foundation::Error(SourceError::Failed, "convert")};
        check(converted.is_err() && converted.unwrap_err().get_message() == "convert",
            "error survives value type conversion");
        IntResult success = VALUE;
        check(
            success.add_message("ignored").unwrap() == VALUE, "annotating success preserves value");
    }

    void result_map_tests() {
        const IntResult value = VALUE;
        const auto mapped = value.map([](const int& number) { return std::to_string(number); });
        static_assert(std::is_same_v<std::remove_cvref_t<decltype(mapped)>,
            foundation::Result<std::string, SourceError>>);
        check(mapped.unwrap() == std::to_string(VALUE),
            "const map returns a public Result of the new type");
        const IntResult failure = foundation::Error(SourceError::Failed, "map failure");
        bool called = false;
        const auto failed = failure.map([&](int) {
            called = true;
            return std::string{};
        });
        check(!called && failed.is_err() && failed.unwrap_err().get_message() == "map failure",
            "failed map preserves error without invoking callback");
        OwnerResult owner = std::make_unique<int>(VALUE);
        const auto consumed =
            std::move(owner).map([](std::unique_ptr<int> pointer) { return *pointer; });
        check(consumed.unwrap() == VALUE, "rvalue map consumes a move-only value");
        int original = VALUE;
        ReferenceResult reference = original;
        auto mapped_reference =
            std::move(reference).map([](int& number) -> int& { return number; });
        check(
            &mapped_reference.unwrap_mut() == &original, "map preserves reference result identity");
        const auto done = value.map([&](int) { called = true; });
        check(called && done.is_ok(), "map supports void output");
        called = false;
        const auto failed_void = failure.map([&](int) { called = true; });
        check(!called && failed_void.is_err(), "failed map supports void output");
    }
} // namespace enishi::tests

int main() {
    enishi::tests::run_test("result_value_tests", enishi::tests::result_value_tests);
    enishi::tests::run_test("result_reference_tests", enishi::tests::result_reference_tests);
    enishi::tests::run_test("result_error_tests", enishi::tests::result_error_tests);
    enishi::tests::run_test("result_map_tests", enishi::tests::result_map_tests);
    return EXIT_SUCCESS;
}
