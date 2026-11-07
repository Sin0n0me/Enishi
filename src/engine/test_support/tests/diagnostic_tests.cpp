#include "diagnostic_tests.h"
#include <foundation/result/result.h>
#include <test_support/result_checks.h>
#include <memory>
#include <stdexcept>
#include <string_view>

namespace {
    enum class TestError { TestErrorFailure = 7 };
    using Result = enishi::foundation::Result<void, TestError>;
}
int main(int argc, char** argv) {
    const std::string_view mode = argc > 1 ? argv[1] : "success";
    enishi::tests::run_test("diagnostic_probe", [&]() {
        if (mode == "result") {
            Result failure = enishi::foundation::Error(TestError::TestErrorFailure, "original cause");
            static_cast<void>(failure.add_message("outer context"));
            enishi::tests::check(failure, "initialize renderer");
        } else if (mode == "empty") {
            Result failure = enishi::foundation::Error(TestError::TestErrorFailure);
            enishi::tests::success(failure);
        } else if (mode == "boolean") {
            enishi::tests::check(false, "boolean failure");
        } else if (mode == "exception") {
            throw std::runtime_error("exception detail");
        } else if (mode == "terminate") {
            std::terminate();
        } else {
            enishi::tests::success(Result{});
            enishi::foundation::Result<std::unique_ptr<int>, TestError> owner =
                std::make_unique<int>(42);
            const auto value = enishi::tests::take(std::move(owner));
            enishi::tests::check(*value == 42, "move-only value preserved");
        }
    });
}
