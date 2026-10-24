#include "skinning_drag.h"

namespace enishi::core {
    bool SkinningDrag::begin(ecs::Registry& registry,
        const std::unordered_map<types::HandleId, std::unique_ptr<ModelBones>>& model_bones,
        const animation::CursorRay& local) {
        animation::CursorPose* selected = nullptr;
        const component::AnimationComponent* pose = nullptr;
        std::size_t side = 0;
        float distance = 0;
        for (auto [entity, animation, model] :
            registry.view<component::AnimationComponent, component::ModelComponent>()) {
            const auto found = model_bones.find(entity);
            if (!model.visible || found == model_bones.end() ||
                found->second->cursor_pose == nullptr) {
                continue;
            }
            const auto hit = found->second->cursor_pose->pick_hand(animation, local);
            if (hit.has_value() && (selected == nullptr || hit->second < distance)) {
                selected = found->second->cursor_pose.get();
                pose = &animation;
                side = hit->first;
                distance = hit->second;
            }
        }
        if (selected == nullptr) {
            return false;
        }
        selected->begin_hanging(side, *pose, local);
        return selected->is_hanging();
    }

    void SkinningDrag::end(
        const std::unordered_map<types::HandleId, std::unique_ptr<ModelBones>>& model_bones) {
        for (auto& [entity, bones] : model_bones) {
            if (bones->cursor_pose != nullptr) {
                bones->cursor_pose->end_hanging();
            }
        }
    }
} // namespace enishi::core
