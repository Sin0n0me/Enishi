#pragma once
#include <engine_types/collider/obb.h>
#include <engine_types/collider/ray.h>
#include <engine_types/renderer/camera.h>
#include <memory>

namespace enishi::collider {
    class Collider {
      private:
        const std::shared_ptr<types::Camera> camera;
        const std::shared_ptr<IOBBMapGetter> obb_map_getter;

        float client_x;
        float client_y;
        int16_t hit_index;
        bool is_hit;

      private:
        void on_collision_check(void);

      public:
        static bool hit_model(const Ray& ray, const OBB& obb);

      public:
        explicit Collider(const std::shared_ptr<types::Camera>& camera,
            const std::shared_ptr<IOBBMapGetter> obb_map_getter);

        int16_t get_hit_index(void) const noexcept;

        bool is_hit_model(void) const noexcept;

        void set_client_position(const float x, const float y);
    };
} // namespace enishi::collider