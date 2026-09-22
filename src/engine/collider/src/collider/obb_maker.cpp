#include "obb_maker.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace enishi::collider {
    namespace {
        bool finite(const glm::vec3& value) {
            return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
        }

        glm::dmat3 eigen_vectors(const glm::dmat3& input_matrix) {
            glm::dmat3 matrix = input_matrix;
            glm::dmat3 result(1.0);
            for (int iteration = 0; iteration < 32; ++iteration) {
                int p = 0;
                int q = 1;
                for (int i = 0; i < 3; ++i) {
                    for (int j = i + 1; j < 3; ++j) {
                        if (std::abs(matrix[i][j]) > std::abs(matrix[p][q])) {
                            p = i;
                            q = j;
                        }
                    }
                }
                const double scale = std::max(
                    {std::abs(matrix[0][0]), std::abs(matrix[1][1]), std::abs(matrix[2][2])});
                if (std::abs(matrix[p][q]) <= scale * 1e-12) {
                    break;
                }
                const double angle =
                    0.5 * std::atan2(2.0 * matrix[p][q], matrix[p][p] - matrix[q][q]);
                glm::dmat3 rotation(1.0);
                rotation[p][p] = rotation[q][q] = std::cos(angle);
                rotation[p][q] = std::sin(angle);
                rotation[q][p] = -std::sin(angle);
                matrix = glm::transpose(rotation) * matrix * rotation;
                result *= rotation;
            }
            return result;
        }
    } // namespace

    foundation::Option<types::OBB> OBBMaker::make_by_covariance_matrix(
        const std::vector<glm::vec3>& positions) {
        if (positions.empty()) {
            return {};
        }
        glm::dvec3 mean(0.0);
        for (const auto& position : positions) {
            if (!finite(position)) {
                return {};
            }
            mean += glm::dvec3(position);
        }
        mean /= static_cast<double>(positions.size());
        glm::dmat3 covariance(0.0);
        for (const auto& position : positions) {
            const glm::dvec3 delta = glm::dvec3(position) - mean;
            covariance += glm::outerProduct(delta, delta);
        }
        covariance /= static_cast<double>(positions.size());
        const glm::dmat3 axes = eigen_vectors(covariance);
        return OBBMaker::fit(positions, mean, axes);
    }

    foundation::Option<types::OBB> OBBMaker::fit(
        const std::vector<glm::vec3>& positions, const glm::dvec3& mean, const glm::dmat3& axes) {
        types::OBB obb{};
        obb.axis[0] = glm::normalize(glm::vec3(axes[0]));
        obb.axis[1] = glm::normalize(
            glm::vec3(axes[1]) - obb.axis[0] * glm::dot(obb.axis[0], glm::vec3(axes[1])));
        obb.axis[2] = glm::normalize(glm::cross(obb.axis[0], obb.axis[1]));

        glm::dvec3 lower(std::numeric_limits<double>::max());
        glm::dvec3 upper(std::numeric_limits<double>::lowest());
        for (const auto& position : positions) {
            const glm::dvec3 delta = glm::dvec3(position) - mean;
            for (int i = 0; i < 3; ++i) {
                const double projection = glm::dot(delta, glm::dvec3(obb.axis[i]));
                lower[i] = std::min(lower[i], projection);
                upper[i] = std::max(upper[i], projection);
            }
        }
        glm::dvec3 center = mean;
        for (int i = 0; i < 3; ++i) {
            center += glm::dvec3(obb.axis[i]) * ((lower[i] + upper[i]) * 0.5);
        }
        obb.center = glm::vec3(center);
        if (!finite(obb.center)) {
            return {};
        }

        // Reproject around the rounded center so rounding cannot exclude input vertices.
        glm::dvec3 extent(0.0);
        for (const auto& position : positions) {
            const glm::dvec3 delta = glm::dvec3(position) - glm::dvec3(obb.center);
            for (int i = 0; i < 3; ++i) {
                extent[i] = std::max(extent[i], std::abs(glm::dot(delta, glm::dvec3(obb.axis[i]))));
            }
        }
        for (int i = 0; i < 3; ++i) {
            obb.half_extent[i] = static_cast<float>(extent[i]);
            if (static_cast<double>(obb.half_extent[i]) < extent[i]) {
                obb.half_extent[i] =
                    std::nextafter(obb.half_extent[i], std::numeric_limits<float>::infinity());
            }
        }
        if (!finite(obb.half_extent)) {
            return {};
        }
        return obb;
    }

    foundation::Option<types::OBB> OBBMaker::transform(
        const types::OBB& obb, const glm::mat4& matrix) {
        if (!finite(obb.center) || !finite(obb.half_extent)) {
            return {};
        }
        for (int i = 0; i < 3; ++i) {
            if (obb.half_extent[i] < 0.0f || !finite(obb.axis[i]) ||
                std::abs(glm::dot(obb.axis[i], obb.axis[i]) - 1.0f) > 1e-5f) {
                return {};
            }
            for (int j = 0; j < i; ++j) {
                if (std::abs(glm::dot(obb.axis[i], obb.axis[j])) > 1e-5f) {
                    return {};
                }
            }
        }
        for (int col = 0; col < 4; ++col) {
            for (int row = 0; row < 4; ++row) {
                if (!std::isfinite(matrix[col][row])) {
                    return {};
                }
            }
        }
        if (matrix[0][3] != 0.0f || matrix[1][3] != 0.0f || matrix[2][3] != 0.0f ||
            matrix[3][3] != 1.0f) {
            return {};
        }
        std::vector<glm::vec3> corners;
        corners.reserve(8);
        for (int bits = 0; bits < 8; ++bits) {
            glm::dvec3 corner(obb.center);
            for (int axis = 0; axis < 3; ++axis) {
                corner += glm::dvec3(obb.axis[axis]) * static_cast<double>(obb.half_extent[axis]) *
                          ((bits & (1 << axis)) ? 1.0 : -1.0);
            }
            corners.emplace_back(glm::dmat4(matrix) * glm::dvec4(corner, 1.0));
            if (!finite(corners.back())) {
                return {};
            }
        }
        // Preserve the rotated frame even when repeated eigenvalues make PCA axes ambiguous.
        glm::dvec3 directions[3];
        int longest = 0;
        for (int i = 0; i < 3; ++i) {
            directions[i] = glm::dmat3(matrix) * glm::dvec3(obb.axis[i]);
            if (glm::dot(directions[i], directions[i]) >
                glm::dot(directions[longest], directions[longest])) {
                longest = i;
            }
        }
        if (glm::dot(directions[longest], directions[longest]) == 0.0) {
            return OBBMaker::make_by_covariance_matrix(corners);
        }
        glm::dmat3 axes(1.0);
        axes[0] = glm::normalize(directions[longest]);
        glm::dvec3 perpendicular(0.0);
        for (int i = 0; i < 3; ++i) {
            const glm::dvec3 candidate = directions[i] - axes[0] * glm::dot(directions[i], axes[0]);
            if (glm::dot(candidate, candidate) > glm::dot(perpendicular, perpendicular)) {
                perpendicular = candidate;
            }
        }
        if (glm::dot(perpendicular, perpendicular) <=
            glm::dot(directions[longest], directions[longest]) * 1e-24) {
            return OBBMaker::make_by_covariance_matrix(corners);
        }
        axes[1] = glm::normalize(perpendicular);
        axes[2] = glm::normalize(glm::cross(axes[0], axes[1]));
        const glm::dvec3 center(glm::dmat4(matrix) * glm::dvec4(obb.center, 1.0));
        return OBBMaker::fit(corners, center, axes);
    }
} // namespace enishi::collider
