#pragma once
#include <engine_types/collider/obb.h>
#include <engine_types/collider/ray.h>
#include <memory>
#include <optional>
#include <sub_system/collision/interface_collision_handler.h>
#include <unordered_map>
#include <vector>

namespace enishi::collider {
    class Collider {
      public:
        using OBBMap = std::unordered_map<types::BoneIndex, types::OBB>;

      private:
        struct Model {
            OBBMap local_obbs;
            OBBMap world_obbs;
            std::vector<std::shared_ptr<sub_system::ICollisionHandler>> handlers;
        };
        std::unordered_map<types::CollisionModelId, Model> models;

      public:
        [[nodiscard]] bool register_model(types::CollisionModelId model);
        bool remove_model(types::CollisionModelId model);

        // Vertices are bone-local; transform maps that space into world space.
        // Invalid input leaves an existing bone unchanged.
        [[nodiscard]] bool set_bone(types::CollisionModelId model,
            types::BoneIndex bone,
            const std::vector<glm::vec3>& positions,
            const glm::mat4& transform = glm::mat4(1.0f));
        [[nodiscard]] bool update_bone(
            types::CollisionModelId model, types::BoneIndex bone, const glm::mat4& transform);
        bool remove_bone(types::CollisionModelId model, types::BoneIndex bone);
        [[nodiscard]] const OBBMap* get_obb_map(types::CollisionModelId model) const noexcept;

        [[nodiscard]] bool add_handler(types::CollisionModelId model,
            const std::shared_ptr<sub_system::ICollisionHandler>& handler);
        bool remove_handler(types::CollisionModelId model,
            const std::shared_ptr<sub_system::ICollisionHandler>& handler);

        // Call after pose updates once per step. Returns the colliding bone-pair count.
        // Notifications are snapshotted: callback mutations affect the next check.
        // Single-threaded; handlers must not recursively call check_collisions().
        std::size_t check_collisions(void);

        // Same-owner rejection precedes geometric work. Touching counts as collision.
        [[nodiscard]] static std::optional<types::OBBContact> intersect(
            types::CollisionModelId first_model,
            const types::OBB& first,
            types::CollisionModelId second_model,
            const types::OBB& second);
        [[nodiscard]] static bool hit_model(const types::Ray& ray, const types::OBB& obb);
    };
} // namespace enishi::collider
