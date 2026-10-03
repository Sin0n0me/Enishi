#include "physics_bodies_component_factory.h"
#include <variant>

namespace enishi::core {
    foundation::Option<component::PhysicsBodiesComponent> PhysicsBodiesComponentFactory::make(
        const types::ModelData& model_data) noexcept {
        component::PhysicsBodiesComponent bodies;

        // addonsはvariantのリストであり、目的の種類(剛体/ジョイント)だけを拾い出す
        // 同じ種類のaddonがモデル中に複数存在する場合はすべて連結する
        for (const auto& addon : model_data.addons) {
            if (const auto* const rigid_bodies = std::get_if<types::AddonRigidBodies>(&addon)) {
                bodies.rigid_bodies.insert(
                    bodies.rigid_bodies.end(), rigid_bodies->begin(), rigid_bodies->end());
                continue;
            }

            if (const auto* const joints = std::get_if<types::AddonPhysicsJoints>(&addon)) {
                bodies.joints.insert(bodies.joints.end(), joints->begin(), joints->end());
                continue;
            }
        }

        // 剛体を1つも持たないモデルであれば、そもそもPhysicsBodiesComponentは不要
        if (bodies.rigid_bodies.empty()) {
            return {};
        }

        return bodies;
    }
} // namespace enishi::core