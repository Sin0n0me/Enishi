#include "buffer.h"

namespace enishi::renderer::directx {
    Buffer::Buffer(const VertexParameter& parameter)
        : buffer({})
        , parameter(parameter) {
    }

    Buffer::Buffer(const IndexParameter& parameter)
        : buffer({})
        , parameter(parameter) {
    }

    Buffer::Buffer(const UniformParameter& parameter)
        : buffer({})
        , parameter(parameter) {
    }
} // namespace enishi::renderer::directx