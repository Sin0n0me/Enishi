#include "animation_system.h"
#include "animation/animation_player.h"
#include <engine_types/skinning/skinning_command.h>

namespace enishi::core {
    AnimationSystem::AnimationSystem(const std::shared_ptr<ecs::Registry> registry)
        : registry(registry) {
    }

    bool enishi::core::AnimationSystem::should_close(void) {
        return false;
    }

    void enishi::core::AnimationSystem::pre_update(void) {
    }

    void enishi::core::AnimationSystem::post_update(void) {
    }

    void AnimationSystem::update(const types::DeltaTime& delta_time) {
        auto view = this->registry->view<int>();

        /*
        for (auto [entity, animation, model, ik] : view) {
            this->animation(animation, model, ik);
        }
        */
    }

    void enishi::core::AnimationSystem::render(void) const {
    }

    void AnimationSystem::animation(component::AnimationComponent& animation,
        const component::ModelComponent& model,
        const component::IKComponent& ik) {
        /*
        const auto size = animation.bone_buffer.size();
        for (const auto command : animation.commands) {
            switch (command) {
                case types::SkinningCommand::Animation: {
                    for (std::uint32_t i = 0; i < size; ++i) {
                        AnimationPlayer::apply_animation(animation, model, i);
                    }
                } break;
                case types::SkinningCommand::IK: {
                    for (std::uint32_t i = 0; i < size; ++i) {
                        AnimationPlayer::apply_ik(animation, ik, model, i);
                    }
                } break;
                case types::SkinningCommand::PhysicsSimulate: {
                    for (std::uint32_t i = 0; i < size; ++i) {
                        AnimationPlayer::apply_physics(animation, model, i);
                    }
                } break;
                case types::SkinningCommand::WriteBackPhysicsSimulate: {
                    for (std::uint32_t i = 0; i < size; ++i) {
                        // AnimationPlayer::apply_physics(animation, model, i);
                    }
                } break;

                case types::SkinningCommand::ResetLocalTransform: {
                    for (std::uint32_t i = 0; i < size; ++i) {
                        auto& buffer = animation.bone_buffer[i];
                        buffer.position = glm::vec3(0.0f);
                        for (auto& rotation : buffer.rotations) {
                            rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
                        }
                        buffer.scale = glm::vec3(0.0f);
                    }
                } break;
                case types::SkinningCommand::ResetPosition: {
                    for (std::uint32_t i = 0; i < size; ++i) {
                        auto& buffer = animation.bone_buffer[i];
                        buffer.position = glm::vec3(0.0f);
                    }
                } break;
                case types::SkinningCommand::ResetRotate: {
                    for (std::uint32_t i = 0; i < size; ++i) {
                        auto& buffer = animation.bone_buffer[i];
                        for (auto& rotation : buffer.rotations) {
                            rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
                        }
                    }
                } break;
                case types::SkinningCommand::ResetScale: {
                    for (std::uint32_t i = 0; i < size; ++i) {
                        auto& buffer = animation.bone_buffer[i];
                        buffer.scale = glm::vec3(0.0f);
                    }
                } break;

                case types::SkinningCommand::UpdateGlobal: {
                    for (std::uint32_t i = 0; i < size; ++i) {
                        AnimationPlayer::update_global(animation, model, i);
                    }
                } break;
                case types::SkinningCommand::UpdateLocal: {
                    for (std::uint32_t i = 0; i < size; ++i) {
                        AnimationPlayer::update_local(animation, i);
                    }
                } break;

                case types::SkinningCommand::WriteBoneMatrices: {
                    for (std::uint32_t i = 0; i < size; ++i) {
                        AnimationPlayer::global_to_bone_matrices(animation, i);
                    }
                } break;
                case types::SkinningCommand::ReadBoneMatrices: {
                    for (std::uint32_t i = 0; i < size; ++i) {
                        AnimationPlayer::bone_matrices_to_global(animation, i);
                    }
                } break;

                default:
                    break;
            }
        }
        */
    }
} // namespace enishi::core