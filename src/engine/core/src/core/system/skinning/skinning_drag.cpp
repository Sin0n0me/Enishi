#include "skinning_system.h"
#include <platform/window/interface_window.h>

namespace enishi::core {
    bool SkinningSystem::begin_model_drag(types::ClientMousePosition cursor) {
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
        const auto inverse = glm::inverse(camera.world);
        const CursorRay local{{glm::vec3(inverse * glm::vec4(ray.origin, 1)),
                                  glm::normalize(glm::mat3(inverse) * ray.direction)},
            glm::normalize(glm::mat3(inverse) * glm::vec3(glm::inverse(camera.view)[2]))};
        CursorPose* selected = nullptr;
        const component::AnimationComponent* pose = nullptr;
        std::size_t side = 0;
        float distance = 0;
        for (auto [entity, animation, model] :
            this->registry->view<component::AnimationComponent, component::ModelComponent>()) {
            const auto found = this->model_bones.find(entity);
            if (!model.visible || found == this->model_bones.end() ||
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

    void SkinningSystem::end_model_drag() {
        for (auto& [entity, bones] : this->model_bones) {
            if (bones->cursor_pose != nullptr) {
                bones->cursor_pose->end_hanging();
            }
        }
    }
} // namespace enishi::core
