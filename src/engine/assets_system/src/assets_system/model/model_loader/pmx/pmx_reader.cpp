#include "pmx_reader.h"
#include <bit>
#include <cmath>
#include <format>
#include <limits>
#include <type_traits>

namespace enishi::assets_system {
    namespace {
        class PMXReader {
          private:
            std::span<const std::uint8_t> bytes;
            std::size_t position{};
            std::string error;
            std::string section{"header"};

          public:
            explicit PMXReader(std::span<const std::uint8_t> bytes)
                : bytes(bytes) {
            }

            bool require(bool condition, const char* message) {
                if (!condition && this->error.empty()) {
                    this->error = std::format(
                        "PMX {} at byte {}: {}", this->section, this->position, message);
                }
                return this->error.empty();
            }

            template <typename T> void read(T& value) {
                if (!this->require(!(sizeof(T) > this->bytes.size() - this->position),
                        "unexpected end of file")) {
                    return;
                }
                if constexpr (std::is_integral_v<T>) {
                    std::make_unsigned_t<T> bits{};
                    for (std::size_t i = 0; i < sizeof(T); ++i) {
                        bits |= static_cast<std::make_unsigned_t<T>>(this->bytes[this->position++])
                                << (8 * i);
                    }
                    value = std::bit_cast<T>(bits);
                } else {
                    static_assert(std::is_same_v<T, float>);
                    std::uint32_t bits{};
                    this->read(bits);
                    value = std::bit_cast<float>(bits);
                    this->require(std::isfinite(value), "non-finite float");
                }
            }

            template <typename T, std::size_t N> void read(std::array<T, N>& values) {
                for (auto& value : values) {
                    this->read(value);
                }
            }

            template <typename... T> void fields(T&... values) {
                (this->read(values), ...);
            }

            std::int32_t index(std::uint8_t size, bool vertex = false) {
                if (size == 1) {
                    std::uint8_t value{};
                    this->read(value);
                    return vertex ? value : std::bit_cast<std::int8_t>(value);
                }
                if (size == 2) {
                    std::uint16_t value{};
                    this->read(value);
                    return vertex ? value : std::bit_cast<std::int16_t>(value);
                }
                std::int32_t value{};
                this->read(value);
                return value;
            }

            std::int32_t count(std::size_t minimum_size) {
                std::int32_t value{};
                this->read(value);
                if (!this->require(
                        value >= 0 && static_cast<std::size_t>(value) <=
                                          (this->bytes.size() - this->position) / minimum_size,
                        "invalid count or truncated section")) {
                    return 0;
                }
                return value;
            }

            template <typename T, typename F>
            void records(std::vector<T>& values, std::size_t minimum_size, F read_record) {
                const auto size = this->count(minimum_size);
                for (std::int32_t i = 0; i < size && this->error.empty(); ++i) {
                    T value{};
                    read_record(value);
                    if (this->error.empty()) {
                        values.emplace_back(std::move(value));
                    }
                }
            }

            void text(std::string& value, std::uint8_t encoding) {
                const auto size = this->count(1);
                if (!this->error.empty()) {
                    return;
                }
                const auto end = this->position + size;
                if (encoding == 1) {
                    value.assign(
                        reinterpret_cast<const char*>(this->bytes.data() + this->position), size);
                    while (this->position < end && this->error.empty()) {
                        const auto first = this->bytes[this->position++];
                        if (first < 0x80) {
                            continue;
                        }
                        int extra{};
                        if (first > 0xC1 && first < 0xE0) {
                            extra = 1;
                        } else if (first > 0xDF && first < 0xF0) {
                            extra = 2;
                        } else if (first > 0xEF && first < 0xF5) {
                            extra = 3;
                        }
                        if (!this->require(
                                extra != 0 && end - this->position > extra - 1, "invalid UTF-8")) {
                            return;
                        }
                        std::uint32_t code = first & ((1u << (6 - extra)) - 1);
                        for (int i = 0; i < extra; ++i) {
                            const auto next = this->bytes[this->position++];
                            this->require((next & 0xC0) == 0x80, "invalid UTF-8 continuation");
                            code = (code << 6) | (next & 0x3F);
                        }
                        std::uint32_t minimum_code{};
                        if (extra == 1) {
                            minimum_code = 0x80;
                        } else if (extra == 2) {
                            minimum_code = 0x800;
                        } else {
                            minimum_code = 0x10000;
                        }
                        this->require(code > minimum_code - 1 && code < 0x110000 &&
                                          !(code > 0xD7FF && code < 0xE000),
                            "invalid UTF-8 code point");
                    }
                    return;
                }
                if (!this->require(size % 2 == 0, "odd UTF-16 byte count")) {
                    return;
                }
                while (this->position < end && this->error.empty()) {
                    std::uint16_t unit{};
                    this->read(unit);
                    std::uint32_t code = unit;
                    if (unit > 0xD7FF && unit < 0xDC00) {
                        if (!this->require(end - this->position > 1, "truncated surrogate pair")) {
                            return;
                        }
                        std::uint16_t low{};
                        this->read(low);
                        if (!this->require(
                                low > 0xDBFF && low < 0xE000, "invalid surrogate pair")) {
                            return;
                        }
                        code = 0x10000 + ((unit - 0xD800) << 10) + low - 0xDC00;
                    } else if (!this->require(
                                   unit < 0xDC00 || unit > 0xDFFF, "unpaired low surrogate")) {
                        return;
                    }
                    if (code < 0x80) {
                        value.push_back(static_cast<char>(code));
                    } else {
                        int extra{3};
                        std::uint32_t prefix{0xF0};
                        if (code < 0x800) {
                            extra = 1;
                            prefix = 0xC0;
                        } else if (code < 0x10000) {
                            extra = 2;
                            prefix = 0xE0;
                        }
                        value.push_back(static_cast<char>(prefix | (code >> (extra * 6))));
                        for (int i = extra - 1; i != -1; --i) {
                            value.push_back(static_cast<char>(0x80 | ((code >> (i * 6)) & 0x3F)));
                        }
                    }
                }
            }

            foundation::Result<PMXData, AssetError> parse() {
                PMXData data;
                std::array<std::uint8_t, 4> magic{};
                this->read(magic);
                this->require(
                    magic == std::array<std::uint8_t, 4>{'P', 'M', 'X', ' '}, "invalid signature");
                this->read(data.version);
                if (this->error.empty() && data.version != 2.0f && data.version != 2.1f) {
                    return foundation::Error(
                        AssetError::UnsupportedVersion, "PMX version must be 2.0 or 2.1");
                }
                std::uint8_t header_size{};
                this->fields(
                    header_size, data.encoding, data.additional_uv_count, data.index_sizes);
                this->require(
                    header_size == 8 && data.encoding <= 1 && data.additional_uv_count <= 4,
                    "invalid global settings");
                for (const auto size : data.index_sizes) {
                    this->require(size == 1 || size == 2 || size == 4, "invalid index width");
                }
                const auto text = [&](std::string& value) { this->text(value, data.encoding); };
                const auto names = [&](auto& value) {
                    text(value.name);
                    text(value.english_name);
                };
                const auto index = [&](int kind) {
                    return this->index(data.index_sizes[kind], kind == 0);
                };
                names(data);
                text(data.comment);
                text(data.english_comment);

                this->section = "vertices";
                this->records(data.vertices, 38, [&](PMXVertex& v) {
                    this->fields(v.position, v.normal, v.uv);
                    v.additional_uvs.resize(data.additional_uv_count);
                    for (auto& uv : v.additional_uvs) {
                        this->read(uv);
                    }
                    this->read(v.deform_type);
                    if (!this->require(v.deform_type < (data.version == 2.1f ? 5 : 4),
                            "invalid deformation type")) {
                        return;
                    }
                    int bone_count{4};
                    if (v.deform_type == 0) {
                        bone_count = 1;
                    } else if (v.deform_type == 1 || v.deform_type == 3) {
                        bone_count = 2;
                    }
                    for (int i = 0; i < bone_count; ++i) {
                        v.bones[i] = index(3);
                    }
                    if (bone_count == 1) {
                        v.weights[0] = 1.0f;
                    } else if (bone_count == 2) {
                        this->read(v.weights[0]);
                        v.weights[1] = 1.0f - v.weights[0];
                    } else {
                        this->read(v.weights);
                    }
                    if (v.deform_type == 3) {
                        this->fields(v.sdef_center, v.sdef_radius0, v.sdef_radius1);
                    }
                    this->read(v.edge_scale);
                });
                this->section = "indices";
                this->records(data.indices,
                    data.index_sizes[0] == 0 ? 1 : data.index_sizes[0],
                    [&](std::uint32_t& v) { v = static_cast<std::uint32_t>(index(0)); });
                this->section = "textures";
                this->records(data.textures, 4, text);
                this->section = "materials";
                this->records(data.materials, 80, [&](PMXMaterial& v) {
                    names(v);
                    this->fields(v.diffuse,
                        v.specular,
                        v.shininess,
                        v.ambient,
                        v.flags,
                        v.edge_color,
                        v.edge_size);
                    v.texture = index(1);
                    v.sphere_texture = index(1);
                    this->fields(v.sphere_mode, v.shared_toon);
                    this->require(v.sphere_mode < 4 && v.shared_toon < 2, "invalid texture mode");
                    if (v.shared_toon == 1) {
                        std::uint8_t toon{};
                        this->read(toon);
                        v.toon_texture = toon;
                        this->require(toon < 10, "invalid shared toon index");
                    } else {
                        v.toon_texture = index(1);
                    }
                    text(v.memo);
                    this->read(v.index_count);
                });
                this->section = "bones";
                this->records(data.bones, 28, [&](PMXBone& v) {
                    names(v);
                    this->read(v.position);
                    v.parent = index(3);
                    this->fields(v.layer, v.flags);
                    if ((v.flags & 1) != 0) {
                        v.tail = index(3);
                    } else {
                        this->read(v.tail_offset);
                    }
                    if ((v.flags & 0x0300) != 0) {
                        v.inherit_parent = index(3);
                        this->read(v.inherit_weight);
                    }
                    if ((v.flags & 0x0400) != 0) {
                        this->read(v.fixed_axis);
                    }
                    if ((v.flags & 0x0800) != 0) {
                        this->fields(v.local_x, v.local_z);
                    }
                    if ((v.flags & 0x2000) != 0) {
                        this->read(v.external_parent);
                    }
                    if ((v.flags & 0x0020) != 0) {
                        v.ik_target = index(3);
                        this->fields(v.ik_iterations, v.ik_angle);
                        this->require(
                            v.ik_iterations > -1 && v.ik_angle > -1, "invalid IK settings");
                        this->records(v.ik_links, 2, [&](PMXIKLink& link) {
                            link.bone = index(3);
                            this->read(link.limited);
                            this->require(link.limited < 2, "invalid IK limit flag");
                            if (link.limited != 0) {
                                this->fields(link.lower, link.upper);
                            }
                        });
                    }
                });
                this->section = "morphs";
                this->records(data.morphs, 14, [&](PMXMorph& v) {
                    names(v);
                    this->fields(v.panel, v.type);
                    if (!this->require(v.panel < 5 && v.type < (data.version == 2.1f ? 11 : 9),
                            "invalid morph type or panel")) {
                        return;
                    }
                    this->records(v.offsets, 5, [&](PMXMorphOffset& o) {
                        int index_kind{};
                        if (v.type == 0 || v.type == 9) {
                            index_kind = 4;
                        } else if (v.type == 2) {
                            index_kind = 3;
                        } else if (v.type == 8) {
                            index_kind = 2;
                        } else if (v.type == 10) {
                            index_kind = 5;
                        }
                        o.index = index(index_kind);
                        switch (v.type) {
                            case 0:
                            case 9:
                                this->read(o.weight);
                                break;
                            case 1:
                                this->read(o.translation);
                                break;
                            case 2:
                                this->fields(o.translation, o.rotation);
                                break;
                            case 8:
                                this->fields(o.operation,
                                    o.diffuse,
                                    o.specular,
                                    o.shininess,
                                    o.ambient,
                                    o.edge_color,
                                    o.edge_size,
                                    o.texture,
                                    o.sphere,
                                    o.toon);
                                this->require(o.operation < 2, "invalid material morph operation");
                                break;
                            case 10:
                                this->fields(o.operation, o.translation, o.torque);
                                this->require(o.operation < 2, "invalid impulse coordinate space");
                                break;
                            default:
                                this->read(o.uv);
                                break;
                        }
                    });
                });
                this->section = "display frames";
                this->records(data.display_frames, 13, [&](PMXDisplayFrame& v) {
                    names(v);
                    this->read(v.special);
                    this->require(v.special < 2, "invalid display frame flag");
                    this->records(v.elements, 2, [&](PMXDisplayElement& e) {
                        this->read(e.type);
                        this->require(e.type < 2, "invalid display element type");
                        e.index = index(e.type == 0 ? 3 : 4);
                    });
                });
                this->section = "rigid bodies";
                this->records(data.rigid_bodies, 69, [&](PMXRigidBody& v) {
                    names(v);
                    v.bone = index(3);
                    this->fields(v.group,
                        v.non_collision_mask,
                        v.shape,
                        v.size,
                        v.position,
                        v.rotation,
                        v.mass,
                        v.linear_damping,
                        v.angular_damping,
                        v.restitution,
                        v.friction,
                        v.mode);
                    this->require(
                        v.group < 16 && v.shape < 3 && v.mode < 3, "invalid rigid body settings");
                });
                this->section = "joints";
                this->records(data.joints, 107, [&](PMXJoint& v) {
                    names(v);
                    this->read(v.type);
                    this->require(v.type < (data.version == 2.1f ? 6 : 1), "invalid joint type");
                    v.body_a = index(5);
                    v.body_b = index(5);
                    this->fields(v.position,
                        v.rotation,
                        v.translation_min,
                        v.translation_max,
                        v.rotation_min,
                        v.rotation_max,
                        v.translation_spring,
                        v.rotation_spring);
                });
                if (data.version == 2.1f) {
                    this->section = "soft bodies";
                    this->records(data.soft_bodies, 141, [&](PMXSoftBody& v) {
                        names(v);
                        this->read(v.shape);
                        v.material = index(2);
                        this->fields(v.group,
                            v.non_collision_mask,
                            v.flags,
                            v.link_distance,
                            v.cluster_count,
                            v.mass,
                            v.margin,
                            v.aero_model,
                            v.config,
                            v.cluster,
                            v.iterations,
                            v.stiffness);
                        this->require(v.shape < 2 && v.group < 16 && v.link_distance > -1 &&
                                          v.cluster_count > -1 && v.aero_model > -1 &&
                                          v.aero_model < 5,
                            "invalid soft body settings");
                        this->records(v.anchors, 3, [&](PMXSoftBodyAnchor& a) {
                            a.body = index(5);
                            a.vertex = index(0);
                            this->read(a.near_mode);
                            this->require(a.near_mode < 2, "invalid soft body anchor flag");
                        });
                        this->records(v.pinned_vertices, 1, [&](std::int32_t& p) { p = index(0); });
                    });
                }
                this->require(this->position == this->bytes.size(), "unexpected trailing data");
                if (!this->error.empty()) {
                    return foundation::Error(AssetError::InvalidAssetData, std::move(this->error));
                }
                auto valid = PMXModelLoader::validate(data);
                if (valid.is_err()) {
                    return std::move(valid).take_err();
                }
                return data;
            }
        };
    } // namespace

    foundation::Result<PMXData, AssetError> parse_pmx(std::span<const std::uint8_t> bytes) {
        return PMXReader(bytes).parse();
    }
} // namespace enishi::assets_system
