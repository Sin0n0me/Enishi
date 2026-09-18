#include "Collider.h"

namespace enishi::collider {
    Collider::Collider(
        const std::shared_ptr<Camera>& camera, const std::shared_ptr<IOBBMapGetter> obb_map_getter)
        : camera(camera)
        , obb_map_getter(obb_map_getter) {
        this->is_hit = false;
        this->client_x = 0;
        this->client_y = 0;
        this->hit_index = -1;
    }

    void Collider::on_collision_check(void) {
        const Ray ray = Ray::make_ray_from_screen(this->client_x, this->client_y, *this->camera);

        this->is_hit = false;
        this->hit_index = -1;
        for (const auto& [index, obb] : this->obb_map_getter->get_obb_map()) {
            if (this->hit_model(ray, obb)) {
                this->is_hit = true;
                this->hit_index = index;
                break;
            }
        }
    }

    bool Collider::hit_model(const Ray& ray, const OBB& obb) {
        const DirectX::XMVECTOR ray_origin = DirectX::XMLoadFloat3(&ray.origin);
        const DirectX::XMVECTOR ray_dir = DirectX::XMLoadFloat3(&ray.direction);
        const DirectX::XMVECTOR obb_center = DirectX::XMLoadFloat3(&obb.center);
        const DirectX::XMVECTOR delta = DirectX::XMVectorSubtract(obb_center, ray_origin);

        float t_min = 0.0f;
        float t_max = FLT_MAX;
        for (int axis_index = 0; axis_index < 3; ++axis_index) {
            const DirectX::XMVECTOR axis = DirectX::XMLoadFloat3(&obb.axis[axis_index]);
            const float e = DirectX::XMVectorGetX(DirectX::XMVector3Dot(axis, delta));
            const float f = DirectX::XMVectorGetX(DirectX::XMVector3Dot(axis, ray_dir));
            // xのアドレスを取ってindexによるx, y, zの順序で値を取得
            const float extent = (&obb.half_extent.x)[axis_index];

            if (fabsf(f) > 1e-6f) {
                float t1 = (e + extent) / f;
                float t2 = (e - extent) / f;

                if (t1 > t2) {
                    const float temp = t1;
                    t1 = t2;
                    t2 = temp;
                }

                t_min = t1 > t_min ? t1 : t_min;
                t_max = t2 < t_max ? t2 : t_max;

                if (t_min > t_max) {
                    return false;
                }
            } else {
                // レイがスラブと平行
                if (-e - extent > 0.0f || -e + extent < 0.0f) {
                    return false;
                }
            }
        }

        return true;
    }

    int16_t Collider::get_hit_index(void) const noexcept {
        return this->hit_index;
    }

    bool Collider::is_hit_model(void) const noexcept {
        return this->is_hit;
    }

    void Collider::set_client_position(const float x, const float y) {
        this->client_x = x;
        this->client_y = y;

        this->on_collision_check();
    }
} // namespace enishi::collider