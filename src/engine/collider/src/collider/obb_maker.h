#pragma once
#include <engine_types/collider/obb.h>
#include <foundation/option/option.h>
#include <vector>

namespace enishi::collider {
    class OBBMaker {
      private:
        static foundation::Option<types::OBB> fit(const std::vector<glm::vec3>& positions,
            const glm::dvec3& mean,
            const glm::dmat3& axes);

      public:
        // Empty/non-finite input, or an unrepresentable result, returns an empty Option.
        [[nodiscard]] static foundation::Option<types::OBB> make_by_covariance_matrix(
            const std::vector<glm::vec3>& positions);

        // Refit transformed corners, including non-uniform scale and shear.
        // The matrix maps the original OBB space into world space.
        [[nodiscard]] static foundation::Option<types::OBB> transform(
            const types::OBB& obb, const glm::mat4& matrix);
    };
} // namespace enishi::collider
