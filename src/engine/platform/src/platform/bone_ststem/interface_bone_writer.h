#pragma once
#include <engine_types/assets/model/addons/bone.h>
#include <glm/glm.hpp>

namespace enishi::platform {
    class IBoneWriter {
      public:
        enum class MatrixOperator {
            Assign,       // to = from
            MulAssign,    // to = to * from
            LeftMulAssign // to = from * to
        };

      public:
        virtual ~IBoneWriter(void) noexcept = default;

        virtual void write_local(const types::BoneKind from,
            const types::BoneKind to,
            const MatrixOperator op) noexcept = 0;

        virtual void write_global(const types::BoneKind from,
            const types::BoneKind to,
            const MatrixOperator op) noexcept = 0;

        virtual void write_local_to_global(const types::BoneKind from,
            const types::BoneKind to,
            const MatrixOperator op) noexcept = 0;

        virtual void write_global_to_local(const types::BoneKind from,
            const types::BoneKind to,
            const MatrixOperator op) noexcept = 0;
    };
} // namespace enishi::platform