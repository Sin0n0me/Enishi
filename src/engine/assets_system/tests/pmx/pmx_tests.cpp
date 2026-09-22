#include <algorithm>
#include <assets_system/model/model_loader/pmx/pmx_model_loader.h>
#include <assets_system/model/model_loader/pmx/pmx_to_model_data.h>
#include <bit>
#include <cstdlib>
#include <fstream>
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
        void floats(int count, float value = 0.0f) {
            for (int i = 0; i < count; ++i) {
                this->put(value);
            }
        }
    };

    std::int32_t morph_offset_index(std::uint8_t type) {
        if (type == 0) {
            return 1;
        }
        if (type == 9) {
            return 9;
        }
        if (type == 8) {
            return -1;
        }
        return 0;
    }

    int deform_bone_count(int type) {
        if (type == 0) {
            return 1;
        }
        if (type == 1 || type == 3) {
            return 2;
        }
        return 4;
    }

    Bytes full_fixture(std::uint8_t width) {
        Bytes out;
        for (const auto c : std::string_view("PMX ")) {
            out.put(static_cast<std::uint8_t>(c));
        }
        out.put(2.1f);
        out.put(std::uint8_t{8});
        out.put(std::uint8_t{1});
        out.put(std::uint8_t{4});
        for (int i = 0; i < 6; ++i) {
            out.put(width);
        }
        for (int i = 0; i < 4; ++i) {
            out.text("");
        }
        // Index 255 must remain unsigned with one-byte vertex indices.
        out.put(std::int32_t{256});
        for (int i = 0; i < 256; ++i) {
            out.floats(8);
            out.floats(16, 0.5f);
            out.put(std::uint8_t{0});
            out.index(0, width);
            out.put(1.0f);
        }
        out.put(std::int32_t{3});
        for (const auto index : {0, 1, 255}) {
            out.index(index, width);
        }
        out.put(std::int32_t{1});
        out.text("texture.png");
        out.put(std::int32_t{2});
        for (int i = 0; i < 2; ++i) {
            out.text("material");
            out.text("");
            out.floats(11, 1.0f);
            out.put(std::uint8_t{0x1F});
            out.floats(5, 0.5f);
            out.index(0, width);
            out.index(-1, width);
            out.put(std::uint8_t{0});
            out.put(static_cast<std::uint8_t>(i));
            if (i == 0) {
                out.index(0, width);
            } else {
                out.put(std::uint8_t{9});
            }
            out.text("memo");
            out.put(std::int32_t{i == 0 ? 3 : 0});
        }
        out.put(std::int32_t{2});
        for (int i = 0; i < 2; ++i) {
            out.text("bone");
            out.text("");
            out.floats(3);
            out.index(i == 0 ? 1 : -1, width);
            out.put(std::int32_t{0});
            out.put(static_cast<std::uint16_t>(i == 0 ? 0x3FA1 : 0));
            if (i == 0) {
                out.index(1, width);
                out.index(1, width);
                out.put(0.5f);
                out.floats(9);
                out.put(std::int32_t{123});
                out.index(1, width);
                out.put(std::int32_t{8});
                out.put(0.25f);
                out.put(std::int32_t{1});
                out.index(1, width);
                out.put(std::uint8_t{1});
                out.floats(3, -1.0f);
                out.floats(3, 1.0f);
            } else {
                out.floats(3);
            }
        }
        out.put(std::int32_t{11});
        for (std::uint8_t type = 0; type < 11; ++type) {
            out.text("morph");
            out.text("");
            out.put(std::uint8_t{4});
            out.put(type);
            out.put(std::int32_t{1});
            out.index(morph_offset_index(type), width);
            switch (type) {
                case 0:
                case 9:
                    out.put(0.5f);
                    break;
                case 1:
                    out.floats(3, 1.0f);
                    break;
                case 2:
                    out.floats(6);
                    out.put(1.0f);
                    break;
                case 8:
                    out.put(std::uint8_t{1});
                    out.floats(28, 1.0f);
                    break;
                case 10:
                    out.put(std::uint8_t{1});
                    out.floats(6, 0.5f);
                    break;
                default:
                    out.floats(4, 0.5f);
                    break;
            }
        }
        out.put(std::int32_t{1});
        out.text("display");
        out.text("");
        out.put(std::uint8_t{1});
        out.put(std::int32_t{2});
        out.put(std::uint8_t{0});
        out.index(0, width);
        out.put(std::uint8_t{1});
        out.index(1, width);
        out.put(std::int32_t{1});
        out.text("body");
        out.text("");
        out.index(0, width);
        out.put(std::uint8_t{15});
        out.put(std::uint16_t{3});
        out.put(std::uint8_t{2});
        out.floats(14, 0.5f);
        out.put(std::uint8_t{2});
        out.put(std::int32_t{6});
        for (std::uint8_t type = 0; type < 6; ++type) {
            out.text("joint");
            out.text("");
            out.put(type);
            out.index(0, width);
            out.index(-1, width);
            out.floats(24);
        }
        out.put(std::int32_t{1});
        out.text("soft");
        out.text("");
        out.put(std::uint8_t{0});
        out.index(0, width);
        out.put(std::uint8_t{0});
        out.put(std::uint16_t{0});
        out.put(std::uint8_t{7});
        out.put(std::int32_t{2});
        out.put(std::int32_t{3});
        out.floats(2, 1.0f);
        out.put(std::int32_t{4});
        out.floats(18, 0.5f);
        for (int i = 0; i < 4; ++i) {
            out.put(std::int32_t{2});
        }
        out.floats(3, 0.5f);
        out.put(std::int32_t{1});
        out.index(0, width);
        out.index(255, width);
        out.put(std::uint8_t{1});
        out.put(std::int32_t{1});
        out.index(255, width);
        return out;
    }

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
            const int bones = deform_bone_count(type);
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

    void empty_model_tests() {
        constexpr float version = 2.0f;
        constexpr std::uint8_t settings_size = 8;
        constexpr std::uint8_t utf8_encoding = 1;
        constexpr std::uint8_t index_width = 1;
        constexpr std::size_t index_kind_count = 6;
        constexpr std::size_t model_text_count = 4;
        constexpr std::size_t section_count = 9;
        Bytes bytes;
        for (const char character : std::string_view("PMX ")) {
            bytes.put(static_cast<std::uint8_t>(character));
        }
        bytes.put(version);
        bytes.put(settings_size);
        bytes.put(utf8_encoding);
        bytes.put(std::uint8_t{});
        for (std::size_t index = 0; index < index_kind_count; ++index) {
            bytes.put(index_width);
        }
        for (std::size_t index = 0; index < model_text_count; ++index) {
            bytes.text("");
        }
        for (std::size_t index = 0; index < section_count; ++index) {
            bytes.put(std::int32_t{});
        }
        const auto parsed = PMXModelLoader::parse(bytes.data);
        check(parsed.is_err() && parsed.unwrap_err().get_error() == AssetError::InvalidAssetData &&
                  parsed.unwrap_err().get_message().contains("no vertices"),
            "empty PMX must fail before reaching mesh generation");

        PMXData data;
        data.version = version;
        const auto converted = PMXToModelData::to_model_data("empty.pmx", data, nullptr);
        check(converted.is_err() &&
                  converted.unwrap_err().get_error() == AssetError::InvalidAssetData &&
                  converted.unwrap_err().get_message().contains("no vertices"),
            "empty direct conversion must not produce drawable model data");
    }

    void parser_tests() {
        for (const auto width : std::array<std::uint8_t, 3>{1, 2, 4}) {
            const auto full = full_fixture(width);
            auto parsed = PMXModelLoader::parse(full.data);
            if (parsed.is_err()) {
                std::cerr << parsed.unwrap_err().get_message() << '\n';
            }
            check(parsed.is_ok(), "full PMX fixture rejected");
            const auto& data = parsed.unwrap();
            check(data.indices[2] == 255, "unsigned vertex reference");
            check(data.vertices[0].additional_uvs[3][3] == 0.5f, "four additional UV channels");
            check(data.materials[1].toon_texture == 9, "shared toon index");
            check(data.bones[0].external_parent == 123 && data.bones[0].ik_links[0].upper[2] == 1,
                "conditional bone fields");
            check(data.morphs.size() == 11 && data.morphs[8].offsets[0].toon[3] == 1,
                "all binary morph payloads");
            check(data.display_frames[0].elements[1].index == 1, "display frame payload");
            check(data.rigid_bodies[0].friction == 0.5f && data.joints.size() == 6,
                "binary physics payloads");
            check(data.soft_bodies[0].anchors[0].vertex == 255 &&
                      data.soft_bodies[0].pinned_vertices[0] == 255,
                "soft body vertex indices");
            // Exercise every truncation point in the variable-length sections after vertices.
            const auto tail = 37 + 256 * (101 + width);
            for (std::size_t size = tail; size < full.data.size(); ++size) {
                check(PMXModelLoader::parse(std::span(full.data).first(size)).is_err(),
                    "truncated section accepted");
            }
        }
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
        for (const auto offset : {17u, 38u}) {
            bytes = fixture(2.0f, 1);
            std::fill_n(bytes.data.begin() + offset, 4, std::uint8_t{0xFF});
            check(PMXModelLoader::parse(bytes.data).is_err(),
                "negative text length or vertex count accepted");
            bytes.data[offset + 3] = 0x7F;
            check(PMXModelLoader::parse(bytes.data).is_err(),
                "oversized text length or vertex count accepted");
        }
        bytes = fixture(2.0f, 1);
        bytes.data[44] = 0x80;
        bytes.data[45] = 0x7F;
        check(PMXModelLoader::parse(bytes.data).is_err(), "infinite coordinate accepted");

        PMXData data;
        data.version = 2.0f;
        data.vertices.emplace_back();
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
        const std::filesystem::path path = "generated-pmx-loader-test.pmx";
        check(!std::filesystem::exists(path), "test fixture path already exists");
        bytes = full_fixture(4);
        {
            std::ofstream file(path, std::ios::binary);
            file.write(reinterpret_cast<const char*>(bytes.data.data()),
                static_cast<std::streamsize>(bytes.data.size()));
            check(file.good(), "could not write test fixture");
        }
        auto loaded = loader.load(path);
        check(std::filesystem::remove(path), "could not remove generated fixture");
        check(loaded.is_ok() &&
                  std::get<std::unique_ptr<PMXData>>(loaded.unwrap())->bones.size() == 2,
            "file load and model variant");
    }
} // namespace

void pmx_conversion_tests();

int main() {
    empty_model_tests();
    parser_tests();
    pmx_conversion_tests();
    std::cout << "PMX tests passed\n";
}
