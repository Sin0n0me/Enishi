#include <assets_system/model/model_loader/pmx/pmx_model_loader.h>
#include <bit>
#include <cstdlib>
#include <iostream>

using namespace enishi;
using namespace enishi::assets_system;

namespace {
    void check(bool condition, const char* message) {
        if (!condition) {
            std::cerr << message << '\n';
            std::exit(1);
        }
    }

    struct Bytes {
        std::vector<std::uint8_t> data;

        template <typename T> void put(T value) {
            if constexpr (std::is_same_v<T, float>) {
                this->put(std::bit_cast<std::uint32_t>(value));
            } else {
                for (std::size_t i = 0; i < sizeof(T); ++i) {
                    this->data.push_back(static_cast<std::uint8_t>(
                        static_cast<std::make_unsigned_t<T>>(value) >> (8 * i)));
                }
            }
        }
        void text(std::string_view value) {
            this->put(static_cast<std::int32_t>(value.size()));
            this->data.insert(this->data.end(), value.begin(), value.end());
        }
        void index(std::int32_t value, std::uint8_t width) {
            for (std::uint8_t i = 0; i < width; ++i) {
                this->put(static_cast<std::uint8_t>(static_cast<std::uint32_t>(value) >> (8 * i)));
            }
        }
    };

    Bytes fixture(float version,
        std::uint8_t width,
        std::uint8_t encoding = 1,
        std::string_view name = "model") {
        Bytes out;
        for (const auto c : std::string_view("PMX ")) {
            out.put(static_cast<std::uint8_t>(c));
        }
        out.put(version);
        out.put(std::uint8_t{8});
        out.put(encoding);
        out.put(std::uint8_t{0});
        for (int i = 0; i < 6; ++i) {
            out.put(width);
        }
        out.text(name);
        for (int i = 0; i < 3; ++i) {
            out.text("");
        }
        const int types = version == 2.1f ? 5 : 4;
        out.put(types);
        for (int type = 0; type < types; ++type) {
            for (int i = 0; i < 8; ++i) {
                out.put(0.0f);
            }
            out.put(static_cast<std::uint8_t>(type));
            const int bones = type == 0 ? 1 : type == 1 || type == 3 ? 2 : 4;
            for (int i = 0; i < bones; ++i) {
                out.index(-1, width);
            }
            if (bones == 2) {
                out.put(0.25f);
            }
            if (bones == 4) {
                for (int i = 0; i < 4; ++i) {
                    out.put(0.25f);
                }
            }
            if (type == 3) {
                for (int i = 0; i < 9; ++i) {
                    out.put(1.0f);
                }
            }
            out.put(1.0f);
        }
        for (int i = 0; i < (version == 2.1f ? 9 : 8); ++i) {
            out.put(std::int32_t{0});
        }
        return out;
    }

    void parser_tests() {
        for (const auto version : {2.0f, 2.1f}) {
            for (const auto width : std::array<std::uint8_t, 3>{1, 2, 4}) {
                const auto bytes = fixture(version, width);
                auto result = PMXModelLoader::parse(bytes.data);
                if (result.is_err()) {
                    std::cerr << result.unwrap_err().get_message() << '\n';
                }
                check(result.is_ok(), "valid PMX rejected");
                check(result.unwrap().vertices[1].weights[1] == 0.75f, "BDEF2 complement");
                check(result.unwrap().vertices[2].bones[3] == -1, "signed index sentinel");
                check(result.unwrap().vertices[3].sdef_radius1[2] == 1.0f, "SDEF payload");
                for (std::size_t size = 0; size < bytes.data.size(); ++size) {
                    check(PMXModelLoader::parse(std::span(bytes.data).first(size)).is_err(),
                        "truncation accepted");
                }
            }
        }
        auto bytes = fixture(2.0f, 1, 0, std::string("\x3D\xD8\x00\xDE", 4));
        auto unicode = PMXModelLoader::parse(bytes.data);
        check(unicode.is_ok() && unicode.unwrap().name == "\xF0\x9F\x98\x80",
            "UTF-16 surrogate pair");
        for (const auto name : {std::string("\x00\xDC", 2), std::string("x", 1)}) {
            check(PMXModelLoader::parse(fixture(2.0f, 1, 0, name).data).is_err(),
                "invalid UTF-16 accepted");
        }
        for (const auto name : {"\xC0\x80", "\xED\xA0\x80", "\xF4\x90\x80\x80", "\xE3"}) {
            check(PMXModelLoader::parse(fixture(2.0f, 1, 1, name).data).is_err(),
                "invalid UTF-8 accepted");
        }
        bytes = fixture(2.0f, 1);
        bytes.data[11] = 3;
        check(PMXModelLoader::parse(bytes.data).is_err(), "invalid index width accepted");
        bytes = fixture(2.0f, 1);
        bytes.data.push_back(0);
        check(PMXModelLoader::parse(bytes.data).is_err(), "trailing data accepted");
        bytes = fixture(2.0f, 1);
        bytes.data[0] = 'p';
        check(PMXModelLoader::parse(bytes.data).is_err(), "invalid signature accepted");
        check(PMXModelLoader::parse(fixture(3.0f, 1).data).is_err(), "unknown version accepted");

        PMXData data;
        data.version = 2.0f;
        data.bones.resize(2);
        data.bones[0].parent = 1;
        data.bones[1].parent = 0;
        check(PMXModelLoader::validate(data).is_err(), "cyclic bones accepted");
        data.bones[1].parent = -1;
        check(PMXModelLoader::validate(data).is_ok(), "forward parent rejected");
        data.bones[1].parent = 2;
        check(PMXModelLoader::validate(data).is_err(), "out of range parent accepted");
        PMXModelLoader loader;
        check(loader.get_supported_extension() == ".pmx", "extension registration");
        check(loader.load("missing-pmx-test-file.pmx").is_err(), "missing file accepted");
    }
} // namespace

int main() {
    parser_tests();
    std::cout << "PMX tests passed\n";
}
