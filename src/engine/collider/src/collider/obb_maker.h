#pragma once
#include <engine_types/collider/obb.h>
#include <engine_types/collider/ray.h>
#include <memory>

namespace enishi::collider {
    class OBBMaker {
      public:
        static types::OBB make_by_covariance_matrix(const std::vector<glm::vec3>& positions);

        static types::OBB make(const std::vector<glm::vec3>& positions,
            const glm::vec3& mean,
            const glm::mat4& eigen_vectors);

        // 最大値の取得
        static std::tuple<float, std::uint32_t, std::uint32_t> find_jacobi_pivot(
            const glm::mat4& matrix) noexcept;

        // ヤコビ法による固有ベクトル取得
        static glm::mat4 jacobi_eigen_decomposition(const glm::mat4& matrix) noexcept;
    };
} // namespace enishi::collider