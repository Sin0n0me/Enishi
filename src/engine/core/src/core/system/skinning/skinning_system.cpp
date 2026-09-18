#include "skinning_system.h"
#include "bone_view_factory.h"
#include <component/animation_component.h>
#include <component/ik_component.h>
#include <component/physics_component.h>
#include <component/skinning_component.h>
#include <core/system/physics/physics_body_factory.h>
#include <ik_system/ik_solver.h>
#include <utility>

namespace enishi::core {
    SkinningSystem::SkinningSystem(ecs::Registry& registry,
        std::shared_ptr<sub_system::IPhysicsEngine> physics_engine) noexcept
        : registry(&registry), physics_engine(std::move(physics_engine)) {
    }

    bool SkinningSystem::should_close(void) {
        return false;
    }

    void SkinningSystem::pre_update(void) {
    }

    void SkinningSystem::update(const types::DeltaTime& delta_time) {
        for (auto [entity, animation, model, skinning] :
            this->registry->view<component::AnimationComponent,
                component::ModelComponent,
                component::SkinningComponent>()) {
            auto opt_ik = this->registry->get<component::IKComponent>(entity);
            auto opt_physics = this->registry->get<component::PhysicsComponent>(entity);
            auto opt_physics_bodies =
                this->registry->get<component::PhysicsBodiesComponent>(entity);

            auto& bones = this->get_or_build(
                entity, model, animation, opt_ik, opt_physics, opt_physics_bodies);

            // モデルごとに順序を変えられるようにする。指定が無ければ既定順序を使う
            const std::span<const types::SkinningCommand> order =
                skinning.order.empty()
                    ? std::span<const types::SkinningCommand>(SkinningSystem::DEFAULT_ORDER)
                    : std::span<const types::SkinningCommand>(skinning.order);

            for (const auto command : order) {
                this->execute_command(command, bones, opt_ik);
            }

            // Skinning行列の確定は順序に関わらず、全ステップの後に必ず行う
            this->write_skinning_matrices(animation, model, skinning);
        }
    }

    void SkinningSystem::post_update(void) {
    }

    void SkinningSystem::render(void) const {
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
            bones->ik_cache = std::make_unique<skinning_system::IKBoneCache>(
                model.bone_node, BoneViewFactory::make_ik_view(ik.unwrap_mut()));
            bones->ik_updater = std::make_unique<skinning_system::IKBonesUpdater>(
                *bones->ik_cache, *bones->bind_cache);
        }

        // PhysicsComponentを持たないモデルもある
        if (physics.is_some()) {
            auto views = BoneViewFactory::to_shared_views(
                BoneViewFactory::make_physics_view(physics.unwrap_mut()));

            bones->physics_cache = std::make_shared<skinning_system::PhysicsBonesCache>(
                model.bone_node, std::move(views));
            bones->physics_updater =
                std::make_shared<skinning_system::PhysicsBonesUpdater>(*bones->physics_cache);

            if (physics_bodies.is_some()) {
                PhysicsBodyFactory::build(*this->physics_engine->get_world(),
                    physics_bodies.unwrap_mut(),
                    bones->physics_cache,
                    bones->physics_updater);
            }
        }

        auto& ref = *bones;
        this->model_bones.emplace(entity, std::move(bones));
        return ref;
    }

    void SkinningSystem::solve_ik(
        ModelBones& bones, foundation::Option<component::IKComponent&> opt_ik) const noexcept {
        if (bones.ik_cache || bones.ik_updater || opt_ik.is_none()) {
            return;
        }
        const auto& ik = opt_ik.unwrap();

        for (const auto& [bone_index, ik_index] : ik.ik_map) {
            ik::IKSolver::apply_ik(
                ik.iks[ik_index], bones.ik_cache.get(), bones.ik_updater.get(), bone_index);
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
                if (bones.physics_cache && bones.animation_cache) {
                    const auto bone_count = bones.physics_cache->size();
                    for (types::BoneIndex i = 0; i < bone_count; ++i) {
                        auto* const physics_view = bones.physics_cache->at(i);
                        auto* const animation_view = bones.animation_cache->at(i);
                        animation_view->set_animation_global_transform(
                            physics_view->get_physics_global());
                    }
                }
                break;

            case types::SkinningCommand::WriteBackPhysicsSimulate:
                if (bones.physics_cache && bones.animation_cache) {
                    const auto bone_count = bones.physics_cache->size();
                    for (types::BoneIndex i = 0; i < bone_count; ++i) {
                        auto* const animation_view = bones.animation_cache->at(i);
                        auto* const physics_view = bones.physics_cache->at(i);
                        physics_view->set_physics_global(
                            animation_view->get_animation_global_transform());
                    }
                    bones.physics_updater->update_global_form_roots();
                }
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
