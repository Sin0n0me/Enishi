#include "../src/foundation/errors/errors.h"
#include "../src/foundation/result/result.h"

using namespace enishi::foundation;
Result<void, std::int16_t> test1() {
    return std::unexpected(1000);
}

Result<std::uint64_t, std::int16_t> test2() {
    const Result<std::uint64_t, std::int16_t> e = std::unexpected(-100);
    return e;
}

Result<void, Error<int>> test3() {
    using Err = Error<int>;
    return std::unexpected(Err(100));
}

Result<std::uint64_t, Error<int>> test4() {
    using Err = Error<int>;
    return std::unexpected(Err(12345));
}

Result<void, Error<int>> test5() {
    using Err = Error<int>;
    return Err(100);
}

Result<std::uint64_t, Error<float>> test6() {
    using Err = Error<int>;
    return Err(12345).propagation(0.0f);
}

Result<std::uint64_t, Error<float>> test7() {
    return test4().propagation(0.0f);
}

int main(void) {
    if (test1().is_ok()) {
        return 1;
    }
    if (test2().is_ok()) {
        return 1;
    }
    if (test3().is_ok()) {
        return 1;
    }
    if (test4().is_ok()) {
        return 1;
    }
    if (test5().is_ok()) {
        return 1;
    }
    if (test6().is_ok()) {
        return 1;
    }

    return 0;
}