#include "skinning_system.h"
#include "bone_inheritance.h"
#include "bone_view_factory.h"
#include "skinning_drag.h"
#include "skinning_physics.h"
#include <component/animation_component.h>
#include <component/ik_component.h>
#include <component/morph_component.h>
#include <component/physics_component.h>
#include <component/skinning_component.h>
#include <core/system/animation/cursor_ray.h>
#include <debug/log/logger.h>
#include <glm/gtc/matrix_transform.hpp>
#include <ik_system/ik_solver.h>
#include <numeric>
#include <utility>

namespace enishi::core {
    SkinningSystem::SkinningSystem(std::shared_ptr<ecs::Registry> registry,
        std::shared_ptr<sub_system::IPhysicsEngine> physics_engine) noexcept
        : registry(std::move(registry))
        , physics_engine(std::move(physics_engine)) {
    }

    void SkinningSystem::set_debug_playback(std::shared_ptr<debug::PlaybackControl> playback) {
        this->debug_playback = std::move(playback);
    }

    void SkinningSystem::pre_update(void) {
    }

    void SkinningSystem::set_cursor_window(const std::shared_ptr<platform::IWindow>& window) {
        this->cursor_window = window;
    }

    void SkinningSystem::set_camera(std::shared_ptr<sub_system::ICameraController> camera) {
        this->camera = std::move(camera);
    }

    void SkinningSystem::set_cursor_config(const animation::CursorPoseConfig& config) {
        this->cursor_config = config;
        for (auto& [entity, bones] : this->model_bones) {
            bones->cursor_pose.reset();
        }
    }

    void SkinningSystem::set_bone_names(const animation::BoneNameDictionary& dictionary) {
        this->bone_names = dictionary;
        for (auto& [entity, bones] : this->model_bones) {
            bones->cursor_pose.reset();
        }
    }

    void SkinningSystem::update(const types::DeltaTime& delta_time) {
        const bool debug_paused = this->debug_playback != nullptr && this->debug_playback->paused;
        if (debug_paused && this->debug_playback->stepped_models.empty()) {
            return;
        }
        const auto window = this->cursor_window.lock();
        const auto cursor = window != nullptr && this->camera != nullptr
                                ? read_cursor_ray(*window, this->camera->get_camera())
                                : std::nullopt;
        const auto drag_cursor = window != nullptr && this->camera != nullptr
                                     ? read_cursor_ray(*window, this->camera->get_camera(), true)
                                     : std::nullopt;
        for (auto [entity, animation, model, skinning] :
            this->registry->view<component::AnimationComponent,
                component::ModelComponent,
                component::SkinningComponent>()) {
            if (debug_paused && !this->debug_playback->stepped_models.contains(entity)) {
                continue;
            }
            auto opt_ik = this->registry->get<component::IKComponent>(entity);
            auto opt_physics = this->registry->get<component::PhysicsComponent>(entity);
            auto opt_physics_bodies =
                this->registry->get<component::PhysicsBodiesComponent>(entity);
            if (!model.visible ||
                (opt_physics_bodies.is_some() && !opt_physics_bodies.unwrap().enabled)) {
                const auto found = this->model_bones.find(entity);
                if (found != this->model_bones.end()) {
                    found->second->cursor_pose.reset();
                    found->second->pending_after_physics = false;
                    found->second->spring_state.chains.clear();
                }
                animation.pose_transition_active = false;
                continue;
            }

            auto& bones = this->get_or_build(
                entity, model, animation, opt_ik, opt_physics, opt_physics_bodies);
            bones.pending_after_physics = false;
            bones.spring_delta = debug_paused ? 0.0f : delta_time.to_float_second();
            for (std::size_t index = 0; index < bones.external_transforms.size(); ++index) {
                bones.external_transforms[index] = glm::mat4(1);
                const auto slot = bones.constraints[index].external_transform_slot;
                if (slot.has_value()) {
                    const auto found = model.external_transforms.find(*slot);
                    if (found != model.external_transforms.end()) {
                        bones.external_transforms[index] = found->second;
                    }
                }
            }
            bones.morph_delta.clear();
            const auto morph = this->registry->get<component::MorphComponent>(entity);
            if (morph.is_some()) {
                auto deltas = animation::evaluate_bone_morphs(
                    model.bone_node.size(), model.morph_targets.targets, morph.unwrap().weights);
                if (deltas.is_err()) {
                    debug::Logger::warning(deltas.unwrap_err().get_message());
                    continue;
                }
                bones.morph_delta = std::move(deltas).unwrap();
            }

            // モデルごとに順序を変えられるようにする。指定が無ければ既定順序を使う
            const std::span<const types::SkinningCommand> order =
                skinning.order.empty()
                    ? std::span<const types::SkinningCommand>(SkinningSystem::DEFAULT_ORDER)
                    : std::span<const types::SkinningCommand>(skinning.order);

            if (debug_paused) {
                // Stepping samples the file pose directly, without a transition or cursor motion.
                bones.cursor_pose.reset();
                animation.pose_transition_active = false;
            }
            if (!debug_paused && bones.cursor_pose == nullptr) {
                bones.cursor_pose = std::make_unique<animation::CursorPose>(
                    model, this->cursor_config, this->bone_names);
            }
            if (bones.cursor_pose != nullptr) {
                bones.cursor_pose->apply_transitioned(model,
                    animation,
                    bones.cursor_pose->is_hanging() ? drag_cursor : cursor,
                    delta_time.to_float_second(),
                    window != nullptr && !animation.motion_playing);
            }
            for (const auto command : order) {
                if (debug_paused && command == types::SkinningCommand::WriteBackPhysicsSimulate) {
                    continue;
                }
                this->execute_command(command, bones, opt_ik);
            }
            // IK and physics retain the corrected pose; source animation stays unmodified.
            if (bones.cursor_pose != nullptr) {
                bones.cursor_pose->restore(animation);
            }

            // Skinning行列の確定は順序に関わらず、全ステップの後に必ず行う
            this->write_skinning_matrices(animation, model, skinning);
            bones.pending_after_physics = true;
        }
        if (this->physics_engine == nullptr) {
            this->update_after_physics();
        }
    }

    void SkinningSystem::post_update(void) {
    }

    void SkinningSystem::render(void) const {
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
            const auto bodies = this->registry->get<component::PhysicsBodiesComponent>(entity);
            if (bodies.is_some() && !bodies.unwrap().enabled) {
                continue;
            }
            if (this->physics_engine != nullptr &&
                this->physics_engine->get_world()->get_config_reader()->can_update()) {
                SkinningPhysics::import_pose(bones);
            }
            this->evaluate_bone_phase(
                bones, this->registry->get<component::IKComponent>(entity), true);
            update_spring_bones(model, animation, bones.spring_state, bones.spring_delta);
            this->write_skinning_matrices(animation, model, skinning);
        }
    }

    bool SkinningSystem::begin_model_drag(types::ClientMousePosition cursor) {
        if (this->debug_playback != nullptr && this->debug_playback->paused) {
            return false;
        }
        const auto window = this->cursor_window.lock();
        if (window == nullptr || this->camera == nullptr) {
            return false;
        }
        const auto size = window->get_size();
        if (size.is_none()) {
            return false;
        }
        const auto camera = this->camera->get_camera();
        const auto ray = types::Ray::make_ray_from_screen(size.unwrap(), cursor, camera);
        // Transform the world-space ray into model space: origin = inverse(world) * origin,
        // direction = normalize(mat3(inverse(world)) * direction).
        const auto inverse = glm::inverse(camera.world);
        const animation::CursorRay local{{glm::vec3(inverse * glm::vec4(ray.origin, 1)),
                                             glm::normalize(glm::mat3(inverse) * ray.direction)},
            glm::normalize(glm::mat3(inverse) * glm::vec3(glm::inverse(camera.view)[2]))};
        return SkinningDrag::begin(*this->registry, this->model_bones, local);
    }

    void SkinningSystem::end_model_drag() {
        SkinningDrag::end(this->model_bones);
    }

    ModelBones& SkinningSystem::get_or_build(const types::HandleId entity,
        const component::ModelComponent& model,
        component::AnimationComponent& animation,
        foundation::Option<component::IKComponent&> ik,
        foundation::Option<component::PhysicsComponent&> physics,
        foundation::Option<component::PhysicsBodiesComponent&> physics_bodies) noexcept {
        const auto iter = this->model_bones.find(entity);
        if (iter != this->model_bones.end()) {
            return *iter->second;
        }

        auto bones = std::make_unique<ModelBones>();

        // Animation/Bindは全モデル共通で必須
        bones->animation_cache = std::make_unique<skinning_system::AnimationBonesCache>(
            model.bone_node, BoneViewFactory::make_animation_view(animation));
        bones->animation_updater =
            std::make_unique<skinning_system::AnimationBonesUpdater>(*bones->animation_cache);

        bones->bind_cache = std::make_unique<skinning_system::BindBonesCache>(
            BoneViewFactory::make_bind_view(model));

        // IKComponentを持たないモデルもある
        if (ik.is_some()) {
            bones->ik_base_local.resize(model.bone_node.size());
            bones->external_transforms.resize(model.bone_node.size(), glm::mat4(1));
            bones->constraints.resize(model.bone_node.size());
            for (const auto& constraint : model.bone_constraints.constraints) {
                if (constraint.bone < bones->constraints.size()) {
                    bones->constraints[constraint.bone] = constraint;
                }
            }
            bones->evaluation_order = model.evaluation_order;
            if (bones->evaluation_order.empty()) {
                bones->evaluation_order.resize(model.bone_node.size());
                std::iota(bones->evaluation_order.begin(),
                    bones->evaluation_order.end(),
                    types::BoneIndex{});
            }
            bones->ik_cache = std::make_unique<skinning_system::IKBoneCache>(
                model.bone_node, BoneViewFactory::make_ik_view(ik.unwrap_mut()));
            bones->ik_updater = std::make_unique<skinning_system::IKBonesUpdater>(*bones->ik_cache,
                *bones->bind_cache,
                bones->ik_base_local,
                bones->external_transforms);
        }

        SkinningPhysics::build(*bones, this->physics_engine.get(), model, physics, physics_bodies);

        auto& ref = *bones;
        this->model_bones.emplace(entity, std::move(bones));
        return ref;
    }

    void SkinningSystem::solve_ik(
        ModelBones& bones, foundation::Option<component::IKComponent&> opt_ik) const noexcept {
        if (bones.ik_cache == nullptr || bones.ik_updater == nullptr || opt_ik.is_none()) {
            return;
        }

        // Start each frame from animation, not the previous frame's IK correction.
        for (types::BoneIndex index = 0; index < bones.ik_cache->size(); ++index) {
            bones.ik_base_local[index] =
                bones.animation_cache->at(index)->get_animation_local_transform();
            if (index < bones.morph_delta.size()) {
                const auto* pose = bones.animation_cache->at(index);
                const auto& delta = bones.morph_delta[index];
                bones.ik_base_local[index] =
                    glm::translate(
                        glm::mat4(1), pose->get_animation_translation() + delta.translation) *
                    glm::mat4_cast(pose->get_animation_rotation() * delta.rotation) *
                    glm::scale(glm::mat4(1), pose->get_animation_scale());
            }
            bones.ik_cache->at(index)->set_ik_rotation(glm::quat(1, 0, 0, 0));
        }
        bones.ik_updater->update_global_form_roots();
        this->evaluate_bone_phase(bones, opt_ik, false);
    }

    void SkinningSystem::evaluate_bone_phase(ModelBones& bones,
        foundation::Option<component::IKComponent&> opt_ik,
        bool after_physics) const noexcept {
        if (bones.ik_cache == nullptr || opt_ik.is_none()) {
            return;
        }
        const auto& ik = opt_ik.unwrap();
        for (const auto bone_index : bones.evaluation_order) {
            if (bones.constraints[bone_index].after_physics != after_physics) {
                continue;
            }
            apply_bone_inheritance(bones, bone_index);
            if (ik.disabled_bones.contains(bone_index)) {
                continue;
            }
            const auto found = ik.ik_map.find(bone_index);
            if (found == ik.ik_map.end() || !(found->second < ik.iks.size())) {
                continue;
            }
            if (bones.cursor_pose != nullptr &&
                bones.cursor_pose->suppress_hanging_ik(bone_index)) {
                continue;
            }
            ik::IKSolver::apply_ik(ik.iks[found->second],
                bones.ik_cache.get(),
                bones.ik_updater.get(),
                bone_index,
                bones.ik_base_local);
        }
        for (types::BoneIndex index = 0; index < bones.ik_cache->size(); ++index) {
            bones.animation_cache->at(index)->set_animation_global_transform(
                bones.ik_cache->at(index)->get_ik_global_transform());
        }
    }

    void SkinningSystem::execute_command(const types::SkinningCommand command,
        ModelBones& bones,
        foundation::Option<component::IKComponent&> ik) const noexcept {
        switch (command) {
            case types::SkinningCommand::Animation:
                bones.animation_updater->update_global_form_roots();
                break;

            case types::SkinningCommand::IK:
                this->solve_ik(bones, ik);
                break;

            case types::SkinningCommand::PhysicsSimulate:
                SkinningPhysics::import_pose(bones);
                break;

            case types::SkinningCommand::WriteBackPhysicsSimulate:
                SkinningPhysics::write_back_pose(bones);
                break;

            case types::SkinningCommand::ReadBoneMatrices:
            case types::SkinningCommand::WriteBoneMatrices:
            case types::SkinningCommand::UpdateLocal:
            case types::SkinningCommand::UpdateGlobal:
            case types::SkinningCommand::ResetLocalTransform:
            case types::SkinningCommand::ResetPosition:
            case types::SkinningCommand::ResetRotate:
            case types::SkinningCommand::ResetScale:
                break;
        }
    }

    void SkinningSystem::write_skinning_matrices(const component::AnimationComponent& animation,
        const component::ModelComponent& model,
        component::SkinningComponent& skinning) const noexcept {
        const auto bone_count = model.bone_node.size();
        if (skinning.skinning_matrices.size() != bone_count) {
            skinning.skinning_matrices.resize(bone_count);
        }

        for (std::size_t i = 0; i < bone_count; ++i) {
            skinning.skinning_matrices[i] = animation.global[i] * model.bind_bone[i].global_inverse;
        }
    }
} // namespace enishi::core
