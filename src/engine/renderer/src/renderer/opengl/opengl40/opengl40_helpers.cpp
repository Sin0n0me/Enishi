#include "opengl40_helpers.h"

#include <cstring>
#include <glad/gl.h>

namespace enishi::renderer::opengl::helpers {
    namespace {
        thread_local platform::IOpenGLContext* current_context = nullptr;

        GLADapiproc get_proc_address(const char* const name) {
            if (current_context == nullptr) {
                return nullptr;
            }
            return reinterpret_cast<GLADapiproc>(current_context->get_proc_address(name));
        }
    } // namespace

    bool load_gl40_functions(platform::IOpenGLContext& context) {
        current_context = &context;
        const auto loaded = gladLoadGL(get_proc_address);
        current_context = nullptr;
        return loaded != GL_FALSE && GLAD_GL_VERSION_4_0 != GL_FALSE;
    }

    std::uint32_t to_gl_compare(const types::CompareOperator value) {
        switch (value) {
            case types::CompareOperator::Never:
                return GL_NEVER;
            case types::CompareOperator::Less:
                return GL_LESS;
            case types::CompareOperator::Equal:
                return GL_EQUAL;
            case types::CompareOperator::LessEqual:
                return GL_LEQUAL;
            case types::CompareOperator::Greater:
                return GL_GREATER;
            case types::CompareOperator::NotEqual:
                return GL_NOTEQUAL;
            case types::CompareOperator::GreaterEqual:
                return GL_GEQUAL;
            case types::CompareOperator::Always:
                return GL_ALWAYS;
        }
        return GL_ALWAYS;
    }

    std::uint32_t to_gl_stencil_operator(const types::StencilOpeartor value) {
        switch (value) {
            case types::StencilOpeartor::Keep:
                return GL_KEEP;
            case types::StencilOpeartor::Zero:
                return GL_ZERO;
            case types::StencilOpeartor::Replace:
                return GL_REPLACE;
            case types::StencilOpeartor::IncrementClamp:
                return GL_INCR;
            case types::StencilOpeartor::DecrementClamp:
                return GL_DECR;
            case types::StencilOpeartor::Invert:
                return GL_INVERT;
            case types::StencilOpeartor::IncrementWrap:
                return GL_INCR_WRAP;
            case types::StencilOpeartor::DecrementWrap:
                return GL_DECR_WRAP;
        }
        return GL_KEEP;
    }

    std::uint32_t to_gl_depth_attachment(const types::ImageFormat value) {
        if (value == types::ImageFormat::D24_UNORM_S8_UINT) {
            return GL_DEPTH_STENCIL_ATTACHMENT;
        }
        return GL_DEPTH_ATTACHMENT;
    }

    std::uint32_t to_gl_min_filter(
        const types::FilterMode min_filter, const types::FilterMode mip_filter) {
        if (min_filter == types::FilterMode::Nearest) {
            if (mip_filter == types::FilterMode::Nearest) {
                return GL_NEAREST_MIPMAP_NEAREST;
            }
            return GL_NEAREST_MIPMAP_LINEAR;
        }
        if (mip_filter == types::FilterMode::Nearest) {
            return GL_LINEAR_MIPMAP_NEAREST;
        }
        return GL_LINEAR_MIPMAP_LINEAR;
    }

    bool supports_anisotropy(void) {
        constexpr char EXTENSION_NAME[] = "GL_EXT_texture_filter_anisotropic";
        GLint extension_count = 0;
        glGetIntegerv(GL_NUM_EXTENSIONS, &extension_count);
        for (GLint index = 0; index < extension_count; ++index) {
            const auto* extension =
                reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, index));
            if (extension != nullptr && std::strcmp(extension, EXTENSION_NAME) == 0) {
                return true;
            }
        }
        return false;
    }

    std::uint32_t to_gl_blend_factor(const types::BlendFactor value) {
        switch (value) {
            case types::BlendFactor::Zero:
                return GL_ZERO;
            case types::BlendFactor::One:
                return GL_ONE;
            case types::BlendFactor::SrcColor:
                return GL_SRC_COLOR;
            case types::BlendFactor::OneMinusSrcColor:
                return GL_ONE_MINUS_SRC_COLOR;
            case types::BlendFactor::DstColor:
                return GL_DST_COLOR;
            case types::BlendFactor::OneMinusDstColor:
                return GL_ONE_MINUS_DST_COLOR;
            case types::BlendFactor::SrcAlpha:
                return GL_SRC_ALPHA;
            case types::BlendFactor::OneMinusSrcAlpha:
                return GL_ONE_MINUS_SRC_ALPHA;
            case types::BlendFactor::DstAlpha:
                return GL_DST_ALPHA;
            case types::BlendFactor::OneMinusDstAlpha:
                return GL_ONE_MINUS_DST_ALPHA;
            case types::BlendFactor::ConstantColor:
                return GL_CONSTANT_COLOR;
            case types::BlendFactor::OneMinusConstantColor:
                return GL_ONE_MINUS_CONSTANT_COLOR;
            case types::BlendFactor::ConstantAlpha:
                return GL_CONSTANT_ALPHA;
            case types::BlendFactor::OneMinusConstantAlpha:
                return GL_ONE_MINUS_CONSTANT_ALPHA;
            case types::BlendFactor::SrcAlphaSaturate:
                return GL_SRC_ALPHA_SATURATE;
            case types::BlendFactor::Src1Color:
                return GL_SRC1_COLOR;
            case types::BlendFactor::OneMinusSrc1Color:
                return GL_ONE_MINUS_SRC1_COLOR;
            case types::BlendFactor::Src1Alpha:
                return GL_SRC1_ALPHA;
            case types::BlendFactor::OneMinusSrc1Alpha:
                return GL_ONE_MINUS_SRC1_ALPHA;
        }
        return GL_ONE;
    }

    std::uint32_t to_gl_blend_operator(const types::BlendOperator value) {
        switch (value) {
            case types::BlendOperator::Add:
                return GL_FUNC_ADD;
            case types::BlendOperator::Subtract:
                return GL_FUNC_SUBTRACT;
            case types::BlendOperator::ReverseSubtract:
                return GL_FUNC_REVERSE_SUBTRACT;
            case types::BlendOperator::Min:
                return GL_MIN;
            case types::BlendOperator::Max:
                return GL_MAX;
        }
        return GL_FUNC_ADD;
    }

    std::uint32_t to_gl_address_mode(const types::AddressMode value) {
        switch (value) {
            case types::AddressMode::Repeat:
                return GL_REPEAT;
            case types::AddressMode::Clamp:
                return GL_CLAMP_TO_EDGE;
            case types::AddressMode::Mirror:
                return GL_MIRRORED_REPEAT;
        }
        return GL_REPEAT;
    }

    std::uint32_t to_gl_image_internal_format(const types::ImageFormat value) {
        switch (value) {
            case types::ImageFormat::D32_FLOAT:
                return GL_DEPTH_COMPONENT32F;
            case types::ImageFormat::D24_UNORM_S8_UINT:
                return GL_DEPTH24_STENCIL8;
            case types::ImageFormat::D16_UNORM:
                return GL_DEPTH_COMPONENT16;
            case types::ImageFormat::RGBA16_FLOAT:
                return GL_RGBA16F;
            default:
                return GL_RGBA8;
        }
    }

    std::uint32_t to_gl_image_format(const types::ImageFormat value) {
        switch (value) {
            case types::ImageFormat::D32_FLOAT:
            case types::ImageFormat::D16_UNORM:
                return GL_DEPTH_COMPONENT;
            case types::ImageFormat::D24_UNORM_S8_UINT:
                return GL_DEPTH_STENCIL;
            case types::ImageFormat::BGRA8_UNORM:
                return GL_BGRA;
            default:
                return GL_RGBA;
        }
    }

    std::uint32_t to_gl_image_type(const types::ImageFormat value) {
        switch (value) {
            case types::ImageFormat::D32_FLOAT:
                return GL_FLOAT;
            case types::ImageFormat::D16_UNORM:
                return GL_UNSIGNED_SHORT;
            case types::ImageFormat::D24_UNORM_S8_UINT:
                return GL_UNSIGNED_INT_24_8;
            default:
                return GL_UNSIGNED_BYTE;
        }
    }

    std::uint32_t to_gl_texture_internal_format(const types::TextureFormat value) {
        switch (value) {
            case types::TextureFormat::R8_UNORM:
                return GL_R8;
            case types::TextureFormat::RG8_UNORM:
                return GL_RG8;
            case types::TextureFormat::RGBA16_FLOAT:
                return GL_RGBA16F;
            case types::TextureFormat::RGBA8_SRGB:
                return GL_SRGB8_ALPHA8;
            default:
                return GL_RGBA8;
        }
    }

    std::uint32_t to_gl_texture_format(const types::TextureFormat value) {
        switch (value) {
            case types::TextureFormat::R8_UNORM:
                return GL_RED;
            case types::TextureFormat::RG8_UNORM:
                return GL_RG;
            default:
                return GL_RGBA;
        }
    }

    std::uint32_t to_gl_texture_type(const types::TextureFormat value) {
        if (value == types::TextureFormat::RGBA16_FLOAT) {
            return GL_HALF_FLOAT;
        }
        return GL_UNSIGNED_BYTE;
    }
} // namespace enishi::renderer::opengl::helpers
