#include "ray.h"

namespace enishi::types {
    Ray Ray::make_ray_from_screen(const types::WindowSize& window_size,
        const types::ClientMousePosition& mouse_position,
        const types::UniformCamera& camera) {
        // スクリーン座標からNDCに
        const float ndc_x = (2.0f * mouse_position.x / window_size.width) - 1.0f;
        const float ndc_y = 1.0f - (2.0f * mouse_position.y / window_size.height);

        // クリップ空間（z = 1 は far plane）
        const glm::vec4 ray_clip = glm::vec4(ndc_x, ndc_y, 1.0f, 1.0f);

        // ビュー空間へ逆変換
        const glm::mat4 inverse_projection = glm::inverse(camera.projection);
        glm::vec4 ray_view = ray_clip * inverse_projection;
        ray_view.w = 0.0f; // 方向ベクトルなのでwを0に

        // ワールド空間へ逆変換
        const glm::mat4 inverse_view = glm::inverse(camera.view);
        glm::vec4 ray_direction = glm::normalize(ray_view * inverse_view);

        // レイの原点（カメラ位置）
        const glm::vec4 ray_origin = inverse_view[3];

        return Ray{
            .origin = ray_origin,
            .direction = ray_direction,
        };
    }
} // namespace enishi::types