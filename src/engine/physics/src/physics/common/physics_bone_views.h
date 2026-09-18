#pragma once
#include <memory>
#include <sub_system/physics/bone/interface_physics_bone_view.h>
#include <sub_system/physics/bone/interface_physics_bone_view_list.h>
#include <sub_system/physics/rigid_body/interface_rigid_body.h>

namespace enishi::physics {
    struct PhysicsBoneViews {
        std::shared_ptr<sub_system::IPhysicsBoneViewList> views;
        std::shared_ptr<sub_system::IBoneUpdater> updater;
        std::shared_ptr<sub_system::IPhysicsBoneView> physics_bone_view;
    };
} // namespace enishi::physics