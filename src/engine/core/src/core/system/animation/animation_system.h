#pragma once
#include <animation/errors/errors.h>
#include <animation/morph/morph_name_dictionary.h>
#include <animation/playback/interface_animation_controller.h>
#include <component/animation_component.h>
#include <component/model_component.h>
#include <core/system/interface_system.h>
#include <ecs/registry.h>
#include <engine_types/assets/model/addons/bone.h>
#include <filesystem>
#include <memory>
#include <unordered_map>

namespace enishi::core {
    class AnimationSystem : public ISystem {
      private:
        std::shared_ptr<ecs::Registry> registry;
        animation::MorphNameDictionary morph_names;
        std::unordered_map<types::HandleId, std::shared_ptr<animation::IAnimationController>>
            controllers;
        struct PlaybackState {
            const animation::AnimationClipData* clip = nullptr;
            float time = 0;
            bool playing = false;
            bool initialized = false;
        };
        std::unordered_map<types::HandleId, PlaybackState> playback_states;

      public:
        explicit AnimationSystem(const std::shared_ptr<ecs::Registry> registry);

        void set_morph_names(const animation::MorphNameDictionary& dictionary);
        [[nodiscard]] types::MorphIndex find_morph(
            types::HandleId entity, animation::MorphRole role) const;

        void set_controller(const types::HandleId entity,
            std::shared_ptr<animation::IAnimationController> controller);
        void remove_controller(const types::HandleId& entity);
        [[nodiscard]] foundation::Result<void, animation::AnimationError> play_vmd(
            types::HandleId entity, const std::filesystem::path& path, bool looping = true);
        [[nodiscard]] std::shared_ptr<animation::IAnimationController> get_controller(
            const types::HandleId& entity) const;

        void pre_update(void) override;
        void post_update(void) override;
        void update(const types::DeltaTime& delta_time) override;
        void render(void) const override;
    };
} // namespace enishi::core
