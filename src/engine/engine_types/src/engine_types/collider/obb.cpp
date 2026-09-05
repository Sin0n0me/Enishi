#include "OBB.h"

namespace enishi::types {
    std::tuple<float, std::uint32_t, std::uint32_t> OBB::find_jacobi_pivot(
        const glm::mat4& matrix) noexcept {
        std::uint32_t p = 0;
        std::uint32_t q = 1;
        float max_value = std::fabsf(matrix[0][1]);
        for (int j = 0; j < 3; ++j) {
            for (int k = j + 1; k < 3; ++k) {
                const float v = std::fabsf(matrix[j][k]);
                if (max_value < v) {
                    max_value = v;
                    p = j;
                    q = k;
                }
            }
        }

        return {max_value, p, q};
    }

    glm::mat4 OBB::jacobi_eigen_decomposition(const glm::mat4& in_matrix) noexcept {
        glm::mat4 eigen_vectors = glm::mat4(1.0f);
        glm::mat4 matrix = in_matrix;
        for (int i = 0; i < 32; ++i) {
            const auto [max_value, p, q] = find_jacobi_pivot(matrix);
            if (max_value < 1e-6f) {
                break;
            }

            const float app = matrix[p][p];
            const float aqq = matrix[q][q];
            const float apq = matrix[p][q];
            const float phi = 0.5f * std::atanf(2.0f * apq / (aqq - app));
            const float c = std::cosf(phi);
            const float s = std::sinf(phi);

            glm::mat4 r = glm::mat4(1.0f);
            r[p][p] = c;
            r[q][q] = c;
            r[p][q] = s;
            r[q][p] = -s;

            matrix = glm::transpose(r) * matrix * r;
            eigen_vectors *= r;
        }

        return eigen_vectors;
    }

    OBB OBB::make_by_covariance_matrix(const std::vector<glm::vec3>& positions) {
        const float positions_size = static_cast<float>(positions.size());

        // 重心を求める
        glm::vec3 mean = glm::vec3(0.0f);
        for (const auto& position : positions) {
            mean += position;
        }
        mean *= 1.0f / positions_size;

        // 共分散行列の作成
        glm::mat4 matrix = glm::mat4(0.0f);
        for (const auto& position : positions) {
            const glm::vec4 vec_diff = glm::vec4(position - mean, 0.0f);

            // vec_diff.x * vec_diff + (x*x, x*y, x*z, x*w)
            matrix[0] += glm::vec4(vec_diff.x) * vec_diff;
            // vec_diff.y * vec_diff + (y*x, y*y, y*z, y*w)
            matrix[1] += glm::vec4(vec_diff.y) * vec_diff;
            // vec_diff.z * vec_diff + (z*x, z*y, z*z, z*w)
            matrix[2] += glm::vec4(vec_diff.z) * vec_diff;
        }
        matrix *= 1.0f / positions_size;
        matrix[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f); // 元の行列は単位行列ではないので

        // ヤコビ法による固有ベクトル取得
        const glm::mat4 eigen_vectors = jacobi_eigen_decomposition(matrix);

        return OBB::make(positions, mean, eigen_vectors);
    }

    OBB OBB::make(const std::vector<glm::vec3>& positions,
        const glm::vec3& mean,
        const glm::mat4& eigen_vectors) {
        // OBBの作成
        OBB obb{};
        for (int i = 0; i < 3; ++i) {
            obb.axis[i] = glm::normalize(eigen_vectors[i]);
        }

        // 軸空間でAABB
        // 各軸の最小値と最大値を求める
        glm::vec3 min_vec = glm::vec3(FLT_MAX, FLT_MAX, FLT_MAX);
        glm::vec3 max_vec = glm::vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
        for (const auto& position : positions) {
            const glm::vec3 d = position - mean;
            const auto dot_x = glm::dot(d, obb.axis[0]);
            const auto dot_y = glm::dot(d, obb.axis[1]);
            const auto dot_z = glm::dot(d, obb.axis[2]);
            const glm::vec3 local = glm::vec3(dot_x, dot_y, dot_z);
            min_vec = glm::min(min_vec, local);
            max_vec = glm::max(max_vec, local);
        }

        const glm::vec3 half = (max_vec - min_vec) * 0.5f;
        const glm::vec3 vec_offset = (max_vec + min_vec) * 0.5f;
        obb.half_extent = half;

        // 各軸のオフセットを求める
        const auto axis_offset_x = obb.axis[0] * vec_offset.x;
        const auto axis_offset_y = obb.axis[1] * vec_offset.y;
        const auto axis_offset_z = obb.axis[2] * vec_offset.z;

        glm::vec3 offset_mean = mean;
        offset_mean = offset_mean + axis_offset_x;
        offset_mean = offset_mean + axis_offset_y;
        offset_mean = offset_mean + axis_offset_z;

        // 中心補正
        obb.center = offset_mean;

        return obb;
    }
} // namespace enishi::types