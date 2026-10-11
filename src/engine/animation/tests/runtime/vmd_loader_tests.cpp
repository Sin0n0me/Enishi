#include <assets_system/animation/vmd/vmd_loader.h>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>

using namespace enishi;

namespace {
    void check(bool condition, const char* message) {
        if (!condition) {
            std::cerr << message << '\n';
            std::exit(EXIT_FAILURE);
        }
    }
    template <typename T> void append(std::vector<char>& data, const T& value) {
        const auto* bytes = reinterpret_cast<const char*>(&value);
        data.insert(data.end(), bytes, bytes + sizeof(value));
    }
    auto load(const std::vector<char>& data) {
        const auto path = std::filesystem::current_path() / "generated-motion.vmd";
        {
            std::ofstream file(path, std::ios::binary);
            file.write(data.data(), static_cast<std::streamsize>(data.size()));
        }
        auto result = assets_system::VMDLoader::load(path);
        std::filesystem::remove(path);
        return result;
    }
}

void vmd_loader_tests() {
    assets_system::VMDHeader header{};
    std::memcpy(header.header, "Vocaloid Motion Data 0002", sizeof("Vocaloid Motion Data 0002"));
    std::memcpy(header.model_name, "model", sizeof("model"));
    std::vector<char> data;
    append(data, header);
    constexpr std::uint32_t EMPTY = 0;
    constexpr std::uint32_t ONE = 1;
    append(data, EMPTY);
    check(load(data).is_ok(), "bone-only VMD permits omitted trailing sections");
    append(data, EMPTY); // morphs
    append(data, EMPTY); // camera
    append(data, EMPTY); // light
    append(data, EMPTY); // shadow
    append(data, ONE);
    constexpr std::uint32_t FRAME = 30;
    append(data, FRAME);
    append(data, std::uint8_t{1});
    append(data, ONE);
    assets_system::VMDIKInfo info{};
    std::memcpy(info.name, "leg", sizeof("leg"));
    append(data, info);
    const auto result = load(data);
    check(result.is_ok(), "complete VMD parses");
    check(std::string(result.unwrap()->name.data()) == "model", "model name retained");
    check(result.unwrap()->iks.size() == 1 && result.unwrap()->iks[0].frame == FRAME &&
              result.unwrap()->iks[0].ik_infos.size() == 1, "variable IK record parses field by field");
    data.pop_back();
    check(load(data).is_err(), "truncated record rejected");
    data.clear();
    append(data, header);
    append(data, std::numeric_limits<std::uint32_t>::max());
    check(load(data).is_err(), "oversized count rejected before allocation");
    data[0] = '?';
    check(load(data).is_err(), "invalid header rejected");
}
