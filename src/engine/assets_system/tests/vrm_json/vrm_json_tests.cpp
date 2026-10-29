#include "vrm_json_tests.h"
#include "../../../core/tests/test_runner.h"
#include <assets_system/model/model_loader/vrm/vrm_json.h>
#include <cmath>
#include <limits>
#include <locale>

namespace enishi::tests {
    namespace {
        class CommaDecimal : public std::numpunct<char> {
          protected:
            char do_decimal_point() const override;
        };
        char CommaDecimal::do_decimal_point() const {
            return ',';
        }
        float number(const char* text) {
            assets_system::VRMJson value;
            value.valid = true;
            value.value = text;
            return value.number();
        }
    } // namespace
    void vrm_json_number_tests() {
        check(number("1.25") == 1.25f, "decimal fraction");
        check(number("-2.5e+2") == -250.0f, "signed exponent");
        check(number("1E-2") == 0.01f, "negative exponent");
        check(number("0e-999") == 0.0f, "zero with a tiny exponent");
        check(std::signbit(number("-0")), "negative zero preserved");
        check(
            number("3.4028234663852886e38") == std::numeric_limits<float>::max(), "maximum float");
        check(number("1.401298464324817e-45") == std::numeric_limits<float>::denorm_min(),
            "subnormal float");
        for (const char* text : {"",
                 " ",
                 " 1",
                 "1 ",
                 "+1",
                 "1x",
                 "1,25",
                 "1e",
                 "1e+",
                 "--1",
                 "0x1p2",
                 "nan",
                 "inf",
                 "-inf",
                 "1e999",
                 "-1e999",
                 "1e-999",
                 "-1e-999"}) {
            check(std::isnan(number(text)), text);
        }
        constexpr float FALLBACK = 42.0f;
        check(assets_system::VRMJson{}.number(FALLBACK) == FALLBACK, "missing value uses fallback");
        const auto json = assets_system::VRMJson::parse(R"({"value":1.25,"index":3})");
        check(json["value"].number() == 1.25f, "parsed JSON numeric value");
        check(json["index"].index() == 3, "integer parsing remains available");
    }
    void vrm_json_locale_tests() {
        const auto original = std::locale();
        std::locale::global(std::locale(original, new CommaDecimal));
        check(number("1.25") == 1.25f, "decimal dot under a comma locale");
        check(std::isnan(number("1,25")), "comma remains invalid under a comma locale");
        std::locale::global(original);
    }
} // namespace enishi::tests
int main() {
    enishi::tests::run_test("VRM JSON numbers", enishi::tests::vrm_json_number_tests);
    enishi::tests::run_test("VRM JSON locale independence", enishi::tests::vrm_json_locale_tests);
}
