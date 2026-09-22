#include "ik_solver.h"
#include <algorithm>
#include <cmath>
#include <sub_system/ik/interface_ik_bone_view.h>
#include <sub_system/skinning_system/interface_bone_updater.h>

namespace enishi::ik {
    constexpr float EPSILON = 1e-5f;

    void IKSolver::apply_ik(const types::IK& ik,
        sub_system::IIKBoneViewList* const ik_view_list,
        sub_system::IBoneUpdater* const updater,
        const types::BoneIndex index) {
        if (const auto& ccd_ik = std::get_if<types::CCDIK>(&ik.method)) {
            IKSolver::ccd_ik(ik_view_list, updater, *ccd_ik, index);
        }
    }

    void IKSolver::ccd_ik(sub_system::IIKBoneViewList* const ik_view_list,
        sub_system::IBoneUpdater* const updater,
        const types::CCDIK& ik,
        const types::BoneIndex index) {
        // chainがない場合後の計算は無駄なので何もしない
        if (ik.chain.empty()) {
            return;
        }

        const auto opt_ik_bone = ik_view_list->get(ik.ik_bone);
        const auto opt_target_bone = ik_view_list->get(ik.target);
        auto opt_bone = ik_view_list->get(index);
        if (opt_bone.is_none() || opt_target_bone.is_none() || opt_ik_bone.is_none()) {
            return;
        }
        const auto& ik_bone = opt_ik_bone.unwrap();
        const auto& target_bone = opt_target_bone.unwrap();
        auto& bone = opt_bone.unwrap_mut();

        const auto chain_size = ik.chain.size();

        using LimitTuple = std::tuple<float, foundation::Option<glm::vec3>>;
        using LimitResult = foundation::Option<LimitTuple>;
        const auto opt_limit = [&]() -> LimitResult {
            if (const auto limit = std::get_if<types::IKLimitAngle>(&ik.limit)) {
                return LimitTuple{limit->limit, std::nullopt};
            } else if (const auto limit = std::get_if<types::IKLimitAxis>(&ik.limit)) {
                return LimitTuple{
                    limit->limit,
                    glm::vec3{0, 0, 0},
                };
            }
            return {};
        }();
        const auto& [limit, axis] = opt_limit.unwrap();
        const auto has_limited_axis = axis.is_some();
        const auto limited_axis = axis.unwrap_or_default();

        // IK計算時の回転量を保管しておく
        std::vector<glm::quat> rotate_buffer = std::vector<glm::quat>(chain_size);
        float max_distance = std::numeric_limits<float>::max();
        for (std::uint32_t i = 0; i < ik.iterations; ++i) {
            for (const auto chain_index : ik.chain) {
                const auto& opt_chain_bone = ik_view_list->get(chain_index);
                if (opt_chain_bone.is_none()) {
                    continue;
                }
                const auto& chain_bone = opt_chain_bone.unwrap();

                if (has_limited_axis) {
                    bone->set_ik_rotation(
                        IKSolver::limited_solve_ik(chain_bone->get_ik_global_transform(),
                            ik_bone->get_ik_global_transform(),
                            target_bone->get_ik_global_transform(),
                            limited_axis,
                            limit));
                } else {
                    bone->set_ik_rotation(IKSolver::solve_ik(chain_bone->get_ik_global_transform(),
                        ik_bone->get_ik_global_transform(),
                        target_bone->get_ik_global_transform(),
                        limit));
                }

                updater->update_local(chain_index);
                updater->update_global(chain_index);
            }

            // 発散防止
            const glm::vec4 ik_pos = ik_bone->get_ik_global_transform()[3];
            const glm::vec4 target_pos = target_bone->get_ik_global_transform()[3];
            const float distance = glm::length(target_pos - ik_pos);
            if (distance < max_distance) {
                max_distance = distance;
                for (std::uint32_t j = 0; j < chain_size; j++) {
                    const auto& chain_index = ik.chain[j];
                    const auto& chain_buffer = ik_view_list->get(chain_index);
                    rotate_buffer[j] = bone->get_ik_rotation();
                }
            } else {
                for (std::uint32_t j = 0; j < chain_size; j++) {
                    const auto& chain_index = ik.chain[j];
                    bone->set_ik_rotation(rotate_buffer[j]);
                    updater->update_local(chain_index);
                    updater->update_global(chain_index);
                }
                break;
            }
        }
    }

    glm::quat IKSolver::solve_ik(const glm::mat4& bone_global,
        const glm::mat4& ik_bone_global,
        const glm::mat4& target_bone_global,
        const float ik_limit) {
        const glm::mat4 inv_bone = glm ::inverse(bone_global); // inverse用
        const glm::vec4 bone_pos = bone_global[3];
        const glm::vec4 ik_pos = ik_bone_global[3] - bone_pos;
        const glm::vec4 target_pos = target_bone_global[3] - bone_pos;
        const glm::vec3 ik_vec =
            glm::normalize(inv_bone * glm::vec4(ik_pos.x, ik_pos.y, ik_pos.z, 0.0f));
        const glm::vec3 target_vec =
            glm::normalize(inv_bone * glm::vec4(target_pos.x, target_pos.y, target_pos.z, 0.0f));

        //
        const float dist = glm::length(target_vec - ik_vec);
        if (dist < EPSILON) {
            return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        }

        const float dot = std::clamp(glm::dot(target_vec, ik_vec), -1.0f, 1.0f);

        // acos(dot)=[0, π]
        const auto angle = std::min(std::acos(dot), ik_limit);
        if (angle < EPSILON) {
            return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        }

        // 回転軸決め
        const glm::vec3 cross = glm::normalize(glm::cross(target_vec, ik_vec));
        if (glm::length(cross) < EPSILON) {
            return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        }

        // 回転の作成
        const glm::quat rotation = angle * cross;

        return glm::normalize(rotation);
    }

    glm::quat IKSolver::limited_solve_ik(const glm::mat4& bone_global,
        const glm::mat4& ik_bone_global,
        const glm::mat4& target_bone_global,
        const glm::vec3& twist_axis,
        const float ik_limit) {
        const auto ik_quaternion =
            IKSolver::solve_ik(bone_global, ik_bone_global, target_bone_global, ik_limit);

        return IKSolver::decompose_swing_twist(ik_quaternion,
            twist_axis,
            0.0f,
            ik_limit,
            0.0f // スイングは禁止
        );
    }

    glm::quat IKSolver::decompose_swing_twist(const glm::quat& q,
        const glm::vec3& twist_axis,
        const float twist_min,
        const float twist_max,
        const float swing_max) {
        const glm::vec3 decomp_vec = glm::vec3(q.x, q.y, q.z);
        const auto decomp_w = q.w;

        // 射影によるTwistの抽出
        const glm::vec3 projection = twist_axis * glm::dot(decomp_vec, twist_axis);
        const glm::quat twist =
            glm::normalize(glm::quat(projection.x, projection.y, projection.z, decomp_w));

        // Twistの角度の取得
        const glm::vec3 twist_vec = glm::vec3(twist.x, twist.y, twist.z);
        auto twist_angle = 2.0f * std::atan2(glm::length(twist_vec), twist.w);

        // 符号補正(軸方向)
        if (glm::dot(twist_axis, twist_vec) < 0.0f) {
            twist_angle *= -1.0;
        }
        twist_angle = glm::clamp(twist_angle, twist_min, twist_max);

        const glm::quat limited_twist = glm::angleAxis(twist_angle, twist_axis);

        // Swing 成分
        const glm::quat twist_inverse = glm::inverse(twist);
        const glm::quat swing = glm::normalize(q * twist_inverse);

        // 再合成
        return IKSolver::clamp_swing_cone(swing, twist_axis, swing_max) * limited_twist;
    }

    glm::quat IKSolver::clamp_swing_cone(
        const glm::quat& swing, const glm::vec3& twist_axis, const float max) {
        // スイング後の軸方向
        const glm::vec3 d = swing * twist_axis;
        const auto cos_theta = glm::dot(twist_axis, d);
        const auto cos_theta_max = std::cos(max);

        // 制限内ならそのまま返す
        if (cos_theta_max <= cos_theta) {
            return swing;
        }

        // 制限外は円錐表面へ射影

        // 分解
        const glm::vec3 parallel = twist_axis * cos_theta;
        const glm::vec3 perpendicular = d - parallel;
        if (glm::length(perpendicular) < EPSILON) {
            return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        }

        const glm::vec3 v = glm::normalize(perpendicular);

        // 最大角で再構成
        const glm::vec3 clamped = glm::normalize(twist_axis * cos_theta_max + v * std::sin(max));

        // 新しい Swing クォータニオンを構成
        return IKSolver::quaternion_from_to(twist_axis, clamped);
    }

    glm::quat IKSolver::quaternion_from_to(const glm::quat& from, const glm::quat& to) {
        const float dot = glm::dot(from, to);

        // ほぼ同一方向
        if (dot > 0.999999f) {
            return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        }

        // ほぼ反対方向
        // fromの垂直なベクトルを返す
        if (dot < -0.999999f) {
            constexpr auto PI = glm::pi<float>();

            // Z成分とX成分の絶対値を比較, 適切な平面上の垂線を選択する
            const glm::vec3 orthogonal = std::abs(from.z) < std::abs(from.x)
                                             ? glm::vec3(-from.y, from.x, 0.0f)
                                             : glm::vec3(0.0f, -from.z, from.y);

            return glm::angleAxis(PI, glm::normalize(orthogonal));
        }

        const glm::quat axis = glm::normalize(glm::cross(from, to));
        return glm::angleAxis(std::acos(dot), glm::vec3(axis.x, axis.y, axis.z));
    }
} // namespace enishi::ik