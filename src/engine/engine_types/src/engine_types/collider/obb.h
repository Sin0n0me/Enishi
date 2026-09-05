#pragma once
#include <glm/glm.hpp>
#include <tuple>
#include <unordered_map>
#include <vector>

constexpr float EXTENT_MARGIN = 1.1f;

namespace enishi::types {
    struct OBB {
        glm::vec3 center;      // ローカル空間
        glm::vec3 axis[3];     // 正規化された軸（ローカル）
        glm::vec3 half_extent; // 半径

        // 最大値の取得
        static std::tuple<float, std::uint32_t, std::uint32_t> find_jacobi_pivot(
            const glm::mat4& matrix) noexcept;

        // ヤコビ法による固有ベクトル取得
        static glm::mat4 jacobi_eigen_decomposition(const glm::mat4& matrix) noexcept;

        //
        static OBB make_by_covariance_matrix(const std::vector<glm::vec3>& positions);

        static OBB make(const std::vector<glm::vec3>& positions,
            const glm::vec3& mean,
            const glm::mat4& eigen_vectors);
    };
} // namespace enishi::types
