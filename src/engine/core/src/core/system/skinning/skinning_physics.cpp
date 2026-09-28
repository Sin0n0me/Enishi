#include "bone_view_factory.h"
#include "skinning_system.h"
#include <algorithm>
#include <core/system/physics/physics_body_factory.h>
#include <foundation/log/logger.h>

namespace enishi::core {
    void SkinningSystem::build_physics(ModelBones& bones,
        const component::ModelComponent& model,
        foundation::Option<component::PhysicsComponent&> physics,
        foundation::Option<component::PhysicsBodiesComponent&> physics_bodies) const {
        // PhysicsComponentを持たないモデルもある
        if (physics.is_some()) {
            auto views = BoneViewFactory::to_shared_views(
                BoneViewFactory::make_physics_view(physics.unwrap_mut()));

            bones.physics_cache = std::make_shared<skinning_system::PhysicsBonesCache>(
                model.bone_node, std::move(views));
            bones.physics_updater =
                std::make_shared<skinning_system::PhysicsBonesUpdater>(*bones.physics_cache);

            // 剛体を生成する前に、物理用ボーンを現在のアニメーション姿勢で初期化する。
            // これにより Bullet 側の初期剛体座標がモデルのボーン座標と一致する。
            const auto bone_count = bones.physics_cache->size();
            for (types::BoneIndex i = 0; i < bone_count; ++i) {
                bones.physics_cache->at(i)->set_physics_global(
                    bones.animation_cache->at(i)->get_animation_global_transform());
            }
            for (types::BoneIndex i = 0; i < bone_count; ++i) {
                bones.physics_updater->update_local(i);
            }

            bones.physics_driven.resize(model.bone_node.size(), false);
            if (physics_bodies.is_some()) {
                for (const auto& body : physics_bodies.unwrap().rigid_bodies) {
                    if (body.relate_bone_index < bones.physics_driven.size() &&
                        body.kind != types::RigidBodyKind::Kinematic) {
                        bones.physics_driven[body.relate_bone_index] = true;
                    }
                }
            }
            if (physics_bodies.is_some() && this->physics_engine != nullptr) {
                auto built = PhysicsBodyFactory::build(*this->physics_engine->get_world(),
                    physics_bodies.unwrap_mut(),
                    bones.physics_cache,
                    bones.physics_updater);
                if (built.is_err()) {
                    foundation::Logger::warning(built.unwrap_err().get_message());
                    std::fill(bones.physics_driven.begin(), bones.physics_driven.end(), false);
                } else {
                    physics_bodies.unwrap_mut().handles = std::move(built).unwrap_mut();
                }
            }
        }
    }

    void SkinningSystem::import_physics_pose(ModelBones& bones) const noexcept {
        if (bones.physics_cache == nullptr || bones.ik_cache == nullptr) {
            return;
        }
        const auto nodes = bones.ik_updater->bone_nodes();
        const auto update =
            [&](auto&& self, types::BoneIndex index, const glm::mat4& parent) -> void {
            auto* view = bones.ik_cache->at(index);
            const auto global = bones.physics_driven[index]
                                    ? bones.physics_cache->at(index)->get_physics_global()
                                    : bones.external_transforms[index] * parent *
                                          bones.ik_base_local[index] *
                                          glm::mat4_cast(view->get_ik_rotation());
            view->set_ik_global_transform(global);
            for (const auto child : nodes[index].children) {
                self(self, child, global);
            }
        };
        for (types::BoneIndex index = 0; index < nodes.size(); ++index) {
            if (!nodes[index].has_parent()) {
                update(update, index, glm::mat4(1));
            }
        }
        // Rebase only simulated bones. Descendants keep their authored local pose,
        // and after-physics IK can operate on the resulting hierarchy normally.
        for (types::BoneIndex index = 0; index < nodes.size(); ++index) {
            if (!bones.physics_driven[index]) {
                continue;
            }
            const auto parent =
                nodes[index].has_parent()
                    ? bones.ik_cache->at(nodes[index].parent)->get_ik_global_transform()
                    : glm::mat4(1);
            auto* view = bones.ik_cache->at(index);
            bones.ik_base_local[index] = glm::inverse(bones.external_transforms[index] * parent) *
                                         view->get_ik_global_transform();
            view->set_ik_rotation(glm::quat(1, 0, 0, 0));
        }
    }

    void SkinningSystem::update_after_physics(void) {
        for (auto [entity, animation, model, skinning] :
            this->registry->view<component::AnimationComponent,
                component::ModelComponent,
                component::SkinningComponent>()) {
            const auto found = this->model_bones.find(entity);
            if (found == this->model_bones.end() || !found->second->pending_after_physics) {
                continue;
            }
            auto& bones = *found->second;
            bones.pending_after_physics = false;
            if (this->physics_engine != nullptr &&
                this->physics_engine->get_world()->get_config_reader()->can_update()) {
                this->import_physics_pose(bones);
            }
            this->evaluate_bone_phase(
                bones, this->registry->get<component::IKComponent>(entity), true);
            this->write_skinning_matrices(animation, model, skinning);
        }
    }
} // namespace enishi::core
