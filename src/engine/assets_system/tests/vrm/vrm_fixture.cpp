#include "vrm_fixture.h"
#include "../../../core/tests/test_runner.h"
#include <array>
#include <bit>
#include <chrono>
#include <fstream>

namespace enishi::tests {
    namespace {
        constexpr std::uint32_t GLB_MAGIC = 0x46546c67;
        constexpr std::uint32_t GLB_VERSION = 2;
        constexpr std::uint32_t JSON_CHUNK = 0x4e4f534a;
        constexpr std::uint32_t BIN_CHUNK = 0x004e4942;
        constexpr std::uint32_t HEADER_BYTES = 12;
        constexpr std::uint32_t CHUNK_HEADER_BYTES = 8;
        constexpr std::uint32_t CHUNK_COUNT = 2;
        constexpr std::size_t ALIGNMENT = 4;
        constexpr std::size_t BITS_PER_BYTE = 8;

        void append(std::vector<std::uint8_t>& bytes, std::uint32_t value) {
            for (std::size_t byte = 0; byte < sizeof(value); ++byte) {
                bytes.push_back(static_cast<std::uint8_t>(value >> (byte * BITS_PER_BYTE)));
            }
        }
    } // namespace

    VRMFixture::VRMFixture(bool legacy) {
        if (legacy) {
            this->extension =
                R"("VRM":{"humanoid":{"humanBones":[{"bone":"leftUpperArm","node":1},{"bone":"leftLowerArm","node":2}]},"blendShapeMaster":{"blendShapeGroups":[{"name":"smile","presetName":"joy","isBinary":true,"binds":[{"mesh":0,"index":0,"weight":50}]}]},"secondaryAnimation":{"boneGroups":[{"bones":[1],"stiffiness":1,"dragForce":0.4}]}})";
        } else {
            this->extension =
                R"("VRMC_vrm":{"specVersion":"1.0","humanoid":{"humanBones":{"leftUpperArm":{"node":1},"leftLowerArm":{"node":2}}},"expressions":{"preset":{"happy":{"isBinary":true,"morphTargetBinds":[{"node":0,"index":0,"weight":0.5}]}}}},"VRMC_springBone":{"specVersion":"1.0","springs":[{"joints":[{"node":1,"stiffness":1,"dragForce":0.4},{"node":2}]}]})";
        }
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        this->directory =
            std::filesystem::temp_directory_path() / ("enishi-vrm-test-" + std::to_string(stamp));
        check(std::filesystem::create_directory(this->directory), "create fixture directory");
        this->path = this->directory / "synthetic.vrm";
    }

    VRMFixture::~VRMFixture() {
        std::error_code error;
        std::filesystem::remove(this->path, error);
        std::filesystem::remove(this->directory, error);
    }

    std::vector<std::uint8_t> VRMFixture::binary() {
        // Two VEC3 arrays: triangle positions followed by one morph's displacements.
        constexpr std::array<float, 18> values{
            0, 0, 0, 1, 0, 0, 0, 1, 0, 0.1f, 0, 0, 0, 0, 0, 0, 0, 0};
        std::vector<std::uint8_t> bytes;
        for (const auto value : values) {
            append(bytes, std::bit_cast<std::uint32_t>(value));
        }
        return bytes;
    }

    std::string VRMFixture::document() const {
        // Fixed byte offsets describe the two independently authored VEC3 arrays above.
        return R"({"asset":{"version":"2.0"},"scene":0,"scenes":[{"nodes":[0]}],
"nodes":[{"mesh":0,"children":[1]},{"translation":[-0.1,1,0],"children":[2]},{"translation":[-0.2,0,0]}],
"meshes":[{"primitives":[{"attributes":{"POSITION":0},"targets":[{"POSITION":1}]}]}],
"buffers":[{"byteLength":72)" +
               this->buffer_uri + R"(}],
"bufferViews":[{"buffer":0,"byteOffset":0,"byteLength":36},{"buffer":0,"byteOffset":36,"byteLength":36}],
"accessors":[{"bufferView":0,"componentType":5126,"count":3,"type":"VEC3","min":[0,0,0],"max":[1,1,0]},
{"bufferView":1,"componentType":5126,"count":3,"type":"VEC3"}],"extensions":{)" +
               this->extension + "}" + this->required_extensions + "}";
    }

    const std::filesystem::path& VRMFixture::write() {
        auto json = this->document();
        while (json.size() % ALIGNMENT != 0) {
            json.push_back(' ');
        }
        const auto buffer = VRMFixture::binary();
        std::vector<std::uint8_t> bytes;
        append(bytes, GLB_MAGIC);
        append(bytes, GLB_VERSION);
        // GLB length = file header + two chunk headers + JSON bytes + binary bytes.
        append(bytes,
            HEADER_BYTES + CHUNK_HEADER_BYTES * CHUNK_COUNT +
                static_cast<std::uint32_t>(json.size() + buffer.size()));
        append(bytes, static_cast<std::uint32_t>(json.size()));
        append(bytes, JSON_CHUNK);
        bytes.insert(bytes.end(), json.begin(), json.end());
        append(bytes, static_cast<std::uint32_t>(buffer.size()));
        append(bytes, BIN_CHUNK);
        bytes.insert(bytes.end(), buffer.begin(), buffer.end());
        std::ofstream stream(this->path, std::ios::binary | std::ios::trunc);
        stream.write(reinterpret_cast<const char*>(bytes.data()),
            static_cast<std::streamsize>(bytes.size()));
        stream.close();
        check(stream.good(), "write synthetic VRM");
        return this->path;
    }
} // namespace enishi::tests
