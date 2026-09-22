#include "pmx_reader.h"
#include "pmx_model_loader.h"
#include <format>

namespace enishi::assets_system {
    namespace {
        constexpr float PMX_VERSION_2_0{2.0f};
        constexpr float PMX_VERSION_2_1{2.1f};
        constexpr std::uint8_t UTF8_ENCODING{1};
        constexpr std::uint8_t GLOBAL_SETTINGS_SIZE{8};
        constexpr std::uint8_t ENCODING_COUNT{2};
        constexpr std::uint8_t ADDITIONAL_UV_COUNT_LIMIT{5};
        constexpr std::uint8_t ONE_BYTE_INDEX_SIZE{1};
        constexpr std::uint8_t TWO_BYTE_INDEX_SIZE{2};
        constexpr std::uint8_t FOUR_BYTE_INDEX_SIZE{4};
        constexpr std::size_t VERTEX_MINIMUM_SIZE{38};
        constexpr std::size_t TEXTURE_MINIMUM_SIZE{4};
        constexpr std::size_t MATERIAL_MINIMUM_SIZE{80};
        constexpr std::size_t BONE_MINIMUM_SIZE{28};
        constexpr std::size_t MORPH_MINIMUM_SIZE{14};
        constexpr std::size_t DISPLAY_FRAME_MINIMUM_SIZE{13};
        constexpr std::size_t RIGID_BODY_MINIMUM_SIZE{69};
        constexpr std::size_t JOINT_MINIMUM_SIZE{107};
        constexpr std::size_t SOFT_BODY_MINIMUM_SIZE{141};
        constexpr std::uint16_t BONE_FLAG_TAIL_IS_BONE{0x0001};
        constexpr std::uint16_t BONE_FLAG_INHERIT{0x0300};
        constexpr std::uint16_t BONE_FLAG_FIXED_AXIS{0x0400};
        constexpr std::uint16_t BONE_FLAG_LOCAL_COORDINATE{0x0800};
        constexpr std::uint16_t BONE_FLAG_EXTERNAL_PARENT{0x2000};
        constexpr std::uint16_t BONE_FLAG_IK{0x0020};
        constexpr std::uint8_t PMX_DEFORM_BDEF1{};
        constexpr std::uint8_t PMX_DEFORM_BDEF2{1};
        constexpr std::uint8_t PMX_DEFORM_SDEF{3};
        constexpr std::uint8_t PMX_DEFORM_QDEF{4};
        constexpr std::uint8_t PMX_MORPH_GROUP{};
        constexpr std::uint8_t PMX_MORPH_VERTEX{1};
        constexpr std::uint8_t PMX_MORPH_BONE{2};
        constexpr std::uint8_t PMX_MORPH_MATERIAL{8};
        constexpr std::uint8_t PMX_MORPH_FLIP{9};
        constexpr std::uint8_t PMX_MORPH_IMPULSE{10};
        constexpr std::uint8_t PMX_MAX_MORPH_PANEL{4};
        constexpr std::uint8_t PMX_SHARED_TOON{1};
        constexpr std::uint8_t PMX_MAX_SHARED_TOON_INDEX{9};
        constexpr std::uint8_t PMX_MAX_SPHERE_MODE{3};
        constexpr std::uint8_t PMX_MAX_RIGID_BODY_GROUP{15};
        constexpr std::uint8_t PMX_MAX_RIGID_BODY_SHAPE{2};
        constexpr std::uint8_t PMX_MAX_RIGID_BODY_MODE{2};
        constexpr std::uint8_t PMX_MAX_SOFT_BODY_SHAPE{1};
        constexpr std::int32_t PMX_MINIMUM_VALID_VALUE{};
        constexpr std::uint32_t UTF8_SINGLE_BYTE_LIMIT{0x80};
        constexpr std::uint32_t UTF8_TWO_BYTE_LIMIT{0x800};
        constexpr std::uint32_t UTF8_THREE_BYTE_LIMIT{0x10000};
        constexpr std::uint32_t UTF8_CODE_POINT_LIMIT{0x110000};
        constexpr std::uint32_t UTF16_HIGH_SURROGATE_BEGIN{0xD800};
        constexpr std::uint32_t UTF16_HIGH_SURROGATE_END{0xDBFF};
        constexpr std::uint32_t UTF16_LOW_SURROGATE_BEGIN{0xDC00};
        constexpr std::uint32_t UTF16_LOW_SURROGATE_END{0xDFFF};
        constexpr std::uint32_t UTF16_SURROGATE_OFFSET{0x10000};
    } // namespace

    PMXReader::PMXReader(std::span<const std::uint8_t> bytes)
        : bytes(bytes) {
    }

    foundation::Result<PMXData, AssetError> PMXReader::parse() {
        PMXData data;
        auto header = this->read_header(data);
        if (header.is_err()) {
            return std::move(header).take_err();
        }
        this->read_vertices(data);
        this->read_indices(data);
        this->read_textures(data);
        this->read_materials(data);
        this->read_bones(data);
        this->read_morphs(data);
        this->read_display_frames(data);
        this->read_rigid_bodies(data);
        this->read_joints(data);
        if (data.version == PMX_VERSION_2_1) {
            this->read_soft_bodies(data);
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

    foundation::Result<void, AssetError> PMXReader::read_header(PMXData& data) {
        std::array<std::uint8_t, 4> magic{};
        this->read(magic);
        this->require(
            magic == std::array<std::uint8_t, 4>{'P', 'M', 'X', ' '}, "invalid signature");
        this->read(data.version);
        if (this->error.empty() && data.version != PMX_VERSION_2_0 &&
            data.version != PMX_VERSION_2_1) {
            return foundation::Error(
                AssetError::UnsupportedVersion, "PMX version must be 2.0 or 2.1");
        }
        std::uint8_t header_size{};
        this->fields(header_size, data.encoding, data.additional_uv_count, data.index_sizes);
        this->require(header_size == GLOBAL_SETTINGS_SIZE && data.encoding < ENCODING_COUNT &&
                          data.additional_uv_count < ADDITIONAL_UV_COUNT_LIMIT,
            "invalid global settings");
        for (const auto size : data.index_sizes) {
            this->require(size == ONE_BYTE_INDEX_SIZE || size == TWO_BYTE_INDEX_SIZE ||
                              size == FOUR_BYTE_INDEX_SIZE,
                "invalid index width");
        }
        this->text(data.name, data.encoding);
        this->text(data.english_name, data.encoding);
        this->text(data.comment, data.encoding);
        this->text(data.english_comment, data.encoding);
        return {};
    }

    void PMXReader::text(std::string& value, std::uint8_t encoding) {
        const auto size = this->count(ONE_BYTE_INDEX_SIZE);
        if (!this->error.empty()) {
            return;
        }
        const auto end = this->position + size;
        if (encoding == UTF8_ENCODING) {
            this->read_utf8(value, end);
            return;
        }
        this->read_utf16(value, end);
    }

    void PMXReader::read_utf8(std::string& value, std::size_t end) {
        value.assign(reinterpret_cast<const char*>(this->bytes.data() + this->position),
            end - this->position);
        while (this->position < end && this->error.empty()) {
            const auto first = this->bytes[this->position++];
            if (first < UTF8_SINGLE_BYTE_LIMIT) {
                continue;
            }
            if (!this->read_utf8_code_point(end, first)) {
                return;
            }
        }
    }

    bool PMXReader::read_utf8_code_point(std::size_t end, std::uint8_t first) {
        int extra{};
        if (first > 0xC1 && first < 0xE0) {
            extra = 1;
        } else if (first > 0xDF && first < 0xF0) {
            extra = 2;
        } else if (first > 0xEF && first < 0xF5) {
            extra = 3;
        }
        if (!this->require(extra != 0 && end - this->position > extra - 1, "invalid UTF-8")) {
            return false;
        }
        std::uint32_t code = first & ((1u << (6 - extra)) - 1);
        for (int i = 0; i < extra; ++i) {
            const auto next = this->bytes[this->position++];
            if (!this->require((next & 0xC0) == 0x80, "invalid UTF-8 continuation")) {
                return false;
            }
            code = (code << 6) | (next & 0x3F);
        }
        std::uint32_t minimum_code{UTF8_SINGLE_BYTE_LIMIT};
        if (extra == 2) {
            minimum_code = UTF8_TWO_BYTE_LIMIT;
        } else if (extra == 3) {
            minimum_code = UTF8_THREE_BYTE_LIMIT;
        }
        return this->require(
            minimum_code - 1 < code && code < UTF8_CODE_POINT_LIMIT &&
                (code < UTF16_HIGH_SURROGATE_BEGIN || UTF16_LOW_SURROGATE_END < code),
            "invalid UTF-8 code point");
    }

    void PMXReader::read_utf16(std::string& value, std::size_t end) {
        if (!this->require(
                (end - this->position) % TWO_BYTE_INDEX_SIZE == 0, "odd UTF-16 byte count")) {
            return;
        }
        while (this->position < end && this->error.empty()) {
            this->append_utf8(value, this->read_utf16_code_point(end));
        }
    }

    std::uint32_t PMXReader::read_utf16_code_point(std::size_t end) {
        std::uint16_t unit{};
        this->read(unit);
        std::uint32_t code = unit;
        if (unit > UTF16_HIGH_SURROGATE_BEGIN - 1 && unit < UTF16_LOW_SURROGATE_BEGIN) {
            if (!this->require(
                    end - this->position > ONE_BYTE_INDEX_SIZE, "truncated surrogate pair")) {
                return PMX_MINIMUM_VALID_VALUE;
            }
            std::uint16_t low{};
            this->read(low);
            if (!this->require(low > UTF16_HIGH_SURROGATE_END && low < UTF16_LOW_SURROGATE_END + 1,
                    "invalid surrogate pair")) {
                return PMX_MINIMUM_VALID_VALUE;
            }
            code = UTF16_SURROGATE_OFFSET + ((unit - UTF16_HIGH_SURROGATE_BEGIN) << 10) + low -
                   UTF16_LOW_SURROGATE_BEGIN;
        } else {
            this->require(unit < UTF16_LOW_SURROGATE_BEGIN || unit > UTF16_LOW_SURROGATE_END,
                "unpaired low surrogate");
        }
        return code;
    }

    void PMXReader::append_utf8(std::string& value, std::uint32_t code) {
        if (code < UTF8_SINGLE_BYTE_LIMIT) {
            value.push_back(static_cast<char>(code));
            return;
        }
        int extra{3};
        std::uint32_t prefix{0xF0};
        if (code < UTF8_TWO_BYTE_LIMIT) {
            extra = 1;
            prefix = 0xC0;
        } else if (code < UTF8_THREE_BYTE_LIMIT) {
            extra = 2;
            prefix = 0xE0;
        }
        value.push_back(static_cast<char>(prefix | (code >> (extra * 6))));
        for (int i = extra - 1; i > -1; --i) {
            value.push_back(static_cast<char>(0x80 | ((code >> (i * 6)) & 0x3F)));
        }
    }

    void PMXReader::read_vertices(PMXData& data) {
        this->section = "vertices";
        this->records(data.vertices, VERTEX_MINIMUM_SIZE, [&](PMXVertex& vertex) {
            this->fields(vertex.position, vertex.normal, vertex.uv);
            vertex.additional_uvs.resize(data.additional_uv_count);
            for (auto& uv : vertex.additional_uvs) {
                this->read(uv);
            }
            this->read(vertex.deform_type);
            const auto max_deform =
                data.version == PMX_VERSION_2_1 ? PMX_DEFORM_QDEF : PMX_DEFORM_SDEF;
            if (!this->require(vertex.deform_type < max_deform + 1, "invalid deformation type")) {
                return;
            }
            int bone_count{4};
            if (vertex.deform_type == PMX_DEFORM_BDEF1) {
                bone_count = 1;
            } else if (vertex.deform_type == PMX_DEFORM_BDEF2 ||
                       vertex.deform_type == PMX_DEFORM_SDEF) {
                bone_count = 2;
            }
            for (int i = 0; i < bone_count; ++i) {
                vertex.bones[i] = this->index(data.index_sizes[3]);
            }
            if (bone_count == 1) {
                vertex.weights[0] = 1.0f;
            } else if (bone_count == 2) {
                this->read(vertex.weights[0]);
                vertex.weights[1] = 1.0f - vertex.weights[0];
            } else {
                this->read(vertex.weights);
            }
            if (vertex.deform_type == PMX_DEFORM_SDEF) {
                this->fields(vertex.sdef_center, vertex.sdef_radius0, vertex.sdef_radius1);
            }
            this->read(vertex.edge_scale);
        });
    }

    void PMXReader::read_indices(PMXData& data) {
        this->section = "indices";
        const auto index_size =
            data.index_sizes[0] == 0 ? ONE_BYTE_INDEX_SIZE : data.index_sizes[0];
        this->records(data.indices, index_size, [&](std::uint32_t& value) {
            value = static_cast<std::uint32_t>(this->index(data.index_sizes[0], true));
        });
    }

    void PMXReader::read_textures(PMXData& data) {
        this->section = "textures";
        this->records(data.textures, TEXTURE_MINIMUM_SIZE, [&](std::string& value) {
            this->text(value, data.encoding);
        });
    }

    void PMXReader::read_materials(PMXData& data) {
        this->section = "materials";
        this->records(data.materials, MATERIAL_MINIMUM_SIZE, [&](PMXMaterial& material) {
            this->text(material.name, data.encoding);
            this->text(material.english_name, data.encoding);
            this->fields(material.diffuse,
                material.specular,
                material.shininess,
                material.ambient,
                material.flags,
                material.edge_color,
                material.edge_size);
            material.texture = this->index(data.index_sizes[1]);
            material.sphere_texture = this->index(data.index_sizes[1]);
            this->fields(material.sphere_mode, material.shared_toon);
            this->require(material.sphere_mode < PMX_MAX_SPHERE_MODE + 1 &&
                              material.shared_toon < PMX_SHARED_TOON + 1,
                "invalid texture mode");
            if (material.shared_toon == PMX_SHARED_TOON) {
                std::uint8_t toon{};
                this->read(toon);
                material.toon_texture = toon;
                this->require(toon < PMX_MAX_SHARED_TOON_INDEX + 1, "invalid shared toon index");
            } else {
                material.toon_texture = this->index(data.index_sizes[1]);
            }
            this->text(material.memo, data.encoding);
            this->read(material.index_count);
        });
    }

    void PMXReader::read_bones(PMXData& data) {
        this->section = "bones";
        this->records(data.bones, BONE_MINIMUM_SIZE, [&](PMXBone& bone) {
            this->text(bone.name, data.encoding);
            this->text(bone.english_name, data.encoding);
            this->read(bone.position);
            bone.parent = this->index(data.index_sizes[3]);
            this->fields(bone.layer, bone.flags);
            if ((bone.flags & BONE_FLAG_TAIL_IS_BONE) != 0) {
                bone.tail = this->index(data.index_sizes[3]);
            } else {
                this->read(bone.tail_offset);
            }
            if ((bone.flags & BONE_FLAG_INHERIT) != 0) {
                bone.inherit_parent = this->index(data.index_sizes[3]);
                this->read(bone.inherit_weight);
            }
            if ((bone.flags & BONE_FLAG_FIXED_AXIS) != 0) {
                this->read(bone.fixed_axis);
            }
            if ((bone.flags & BONE_FLAG_LOCAL_COORDINATE) != 0) {
                this->fields(bone.local_x, bone.local_z);
            }
            if ((bone.flags & BONE_FLAG_EXTERNAL_PARENT) != 0) {
                this->read(bone.external_parent);
            }
            if ((bone.flags & BONE_FLAG_IK) != 0) {
                bone.ik_target = this->index(data.index_sizes[3]);
                this->fields(bone.ik_iterations, bone.ik_angle);
                this->require(bone.ik_iterations > -1 && bone.ik_angle > -1, "invalid IK settings");
                this->records(bone.ik_links, 2, [&](PMXIKLink& link) {
                    link.bone = this->index(data.index_sizes[3]);
                    this->read(link.limited);
                    this->require(link.limited < 2, "invalid IK limit flag");
                    if (link.limited != 0) {
                        this->fields(link.lower, link.upper);
                    }
                });
            }
        });
    }

    void PMXReader::read_morphs(PMXData& data) {
        this->section = "morphs";
        this->records(data.morphs, MORPH_MINIMUM_SIZE, [&](PMXMorph& morph) {
            this->text(morph.name, data.encoding);
            this->text(morph.english_name, data.encoding);
            this->fields(morph.panel, morph.type);
            const auto max_type =
                data.version == PMX_VERSION_2_1 ? PMX_MORPH_IMPULSE : PMX_MORPH_FLIP;
            if (!this->require(morph.panel < PMX_MAX_MORPH_PANEL + 1 && morph.type < max_type + 1,
                    "invalid morph type or panel")) {
                return;
            }
            this->records(morph.offsets, 5, [&](PMXMorphOffset& offset) {
                std::size_t index_kind{};
                if (morph.type == PMX_MORPH_GROUP || morph.type == PMX_MORPH_FLIP) {
                    index_kind = 4;
                } else if (morph.type == PMX_MORPH_BONE) {
                    index_kind = 3;
                } else if (morph.type == PMX_MORPH_MATERIAL) {
                    index_kind = 2;
                } else if (morph.type == PMX_MORPH_IMPULSE) {
                    index_kind = 5;
                }
                offset.index = this->index(data.index_sizes[index_kind]);
                switch (morph.type) {
                    case PMX_MORPH_GROUP:
                    case PMX_MORPH_FLIP:
                        this->read(offset.weight);
                        break;
                    case PMX_MORPH_VERTEX:
                        this->read(offset.translation);
                        break;
                    case PMX_MORPH_BONE:
                        this->fields(offset.translation, offset.rotation);
                        break;
                    case PMX_MORPH_MATERIAL:
                        this->fields(offset.operation,
                            offset.diffuse,
                            offset.specular,
                            offset.shininess,
                            offset.ambient,
                            offset.edge_color,
                            offset.edge_size,
                            offset.texture,
                            offset.sphere,
                            offset.toon);
                        this->require(offset.operation < 2, "invalid material morph operation");
                        break;
                    case PMX_MORPH_IMPULSE:
                        this->fields(offset.operation, offset.translation, offset.torque);
                        this->require(offset.operation < 2, "invalid impulse coordinate space");
                        break;
                    default:
                        this->read(offset.uv);
                        break;
                }
            });
        });
    }

    void PMXReader::read_display_frames(PMXData& data) {
        this->section = "display frames";
        this->records(data.display_frames, DISPLAY_FRAME_MINIMUM_SIZE, [&](PMXDisplayFrame& frame) {
            this->text(frame.name, data.encoding);
            this->text(frame.english_name, data.encoding);
            this->read(frame.special);
            this->require(frame.special < 2, "invalid display frame flag");
            this->records(frame.elements, 2, [&](PMXDisplayElement& element) {
                this->read(element.type);
                this->require(element.type < 2, "invalid display element type");
                const std::size_t index_kind = element.type == 0 ? 3 : 4;
                element.index = this->index(data.index_sizes[index_kind]);
            });
        });
    }

    void PMXReader::read_rigid_bodies(PMXData& data) {
        this->section = "rigid bodies";
        this->records(data.rigid_bodies, RIGID_BODY_MINIMUM_SIZE, [&](PMXRigidBody& body) {
            this->text(body.name, data.encoding);
            this->text(body.english_name, data.encoding);
            body.bone = this->index(data.index_sizes[3]);
            this->fields(body.group,
                body.non_collision_mask,
                body.shape,
                body.size,
                body.position,
                body.rotation,
                body.mass,
                body.linear_damping,
                body.angular_damping,
                body.restitution,
                body.friction,
                body.mode);
            this->require(body.group < PMX_MAX_RIGID_BODY_GROUP + 1 &&
                              body.shape < PMX_MAX_RIGID_BODY_SHAPE + 1 &&
                              body.mode < PMX_MAX_RIGID_BODY_MODE + 1,
                "invalid rigid body settings");
        });
    }

    void PMXReader::read_joints(PMXData& data) {
        this->section = "joints";
        this->records(data.joints, JOINT_MINIMUM_SIZE, [&](PMXJoint& joint) {
            this->text(joint.name, data.encoding);
            this->text(joint.english_name, data.encoding);
            this->read(joint.type);
            const std::uint8_t max_joint_type = data.version == PMX_VERSION_2_1 ? 5 : 0;
            this->require(joint.type < max_joint_type + 1, "invalid joint type");
            joint.body_a = this->index(data.index_sizes[5]);
            joint.body_b = this->index(data.index_sizes[5]);
            this->fields(joint.position,
                joint.rotation,
                joint.translation_min,
                joint.translation_max,
                joint.rotation_min,
                joint.rotation_max,
                joint.translation_spring,
                joint.rotation_spring);
        });
    }

    void PMXReader::read_soft_bodies(PMXData& data) {
        this->section = "soft bodies";
        this->records(data.soft_bodies, SOFT_BODY_MINIMUM_SIZE, [&](PMXSoftBody& body) {
            this->text(body.name, data.encoding);
            this->text(body.english_name, data.encoding);
            this->read(body.shape);
            body.material = this->index(data.index_sizes[2]);
            this->fields(body.group,
                body.non_collision_mask,
                body.flags,
                body.link_distance,
                body.cluster_count,
                body.mass,
                body.margin,
                body.aero_model,
                body.config,
                body.cluster,
                body.iterations,
                body.stiffness);
            this->require(body.shape < PMX_MAX_SOFT_BODY_SHAPE + 1 &&
                              body.group < PMX_MAX_RIGID_BODY_GROUP + 1 &&
                              body.link_distance > -1 && body.cluster_count > -1 &&
                              body.aero_model > -1 && body.aero_model < 5,
                "invalid soft body settings");
            this->records(body.anchors, 3, [&](PMXSoftBodyAnchor& anchor) {
                anchor.body = this->index(data.index_sizes[5]);
                anchor.vertex = this->index(data.index_sizes[0], true);
                this->read(anchor.near_mode);
                this->require(anchor.near_mode < 2, "invalid soft body anchor flag");
            });
            this->records(body.pinned_vertices, 1, [&](std::int32_t& vertex) {
                vertex = this->index(data.index_sizes[0], true);
            });
        });
    }

    bool PMXReader::require(bool condition, const char* message) {
        if (!condition && this->error.empty()) {
            this->error =
                std::format("PMX {} at byte {}: {}", this->section, this->position, message);
        }
        return this->error.empty();
    }

    std::int32_t PMXReader::index(std::uint8_t size, bool vertex) {
        if (size == ONE_BYTE_INDEX_SIZE) {
            std::uint8_t value{};
            this->read(value);
            return vertex ? value : std::bit_cast<std::int8_t>(value);
        }
        if (size == TWO_BYTE_INDEX_SIZE) {
            std::uint16_t value{};
            this->read(value);
            return vertex ? value : std::bit_cast<std::int16_t>(value);
        }
        std::int32_t value{};
        this->read(value);
        return value;
    }

    std::int32_t PMXReader::count(std::size_t minimum_size) {
        std::int32_t value{};
        this->read(value);
        if (!this->require(value > -1 && !(static_cast<std::size_t>(value) >
                                             (this->bytes.size() - this->position) / minimum_size),
                "invalid count or truncated section")) {
            return 0;
        }
        return value;
    }

    foundation::Result<PMXData, AssetError> parse_pmx(std::span<const std::uint8_t> bytes) {
        return PMXReader(bytes).parse();
    }
} // namespace enishi::assets_system
