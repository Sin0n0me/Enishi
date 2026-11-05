#include "option_lookup_tests.h"
#include "../../../core/tests/test_runner.h"
#include <memory>

namespace {
    class Interface {
      public:
        virtual ~Interface();
    };

    class Implementation final : public Interface {};

    Interface::~Interface() = default;

    // Match IKBoneCache: return a derived pointer as an Option of an interface
    // pointer. Keep this translation unit free of using-directives.
    enishi::foundation::Option<Interface*> mutable_view(
        const std::unique_ptr<Implementation>& value) {
        if (value == nullptr) {
            return {};
        }
        return value.get();
    }

    enishi::foundation::Option<const Interface*> const_view(
        const std::unique_ptr<Implementation>& value) {
        if (value == nullptr) {
            return {};
        }
        return value.get();
    }
} // namespace

void enishi::tests::option_lookup_tests() {
    const auto implementation = std::make_unique<Implementation>();
    const auto mutable_result = mutable_view(implementation);
    const auto const_result = const_view(implementation);
    enishi::tests::check(mutable_result.is_some() && const_result.is_some(),
        "derived pointer implicitly constructs mutable and const interface Options");
    enishi::tests::check(mutable_result.unwrap() == implementation.get() &&
                             const_result.unwrap() == implementation.get(),
        "implicit pointer conversion preserves object identity");

    const std::unique_ptr<Implementation> empty;
    enishi::tests::check(mutable_view(empty).is_none() && const_view(empty).is_none(),
        "empty cache returns absent Options");
}
