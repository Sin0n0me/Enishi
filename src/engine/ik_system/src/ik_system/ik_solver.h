#pragma once
#include <engine_types/assets/model/addons/ik.h>
#include <sub_system/ik/interface_ik_bone_view_list.h>
#include <sub_system/skinning_system/interface_bone_updater.h>

namespace enishi::ik {
    class IKSolver {
      public:
        static void apply_ik(const types::IK& ik,
            sub_system::IIKBoneViewList* ik_view_list,
            sub_system::IBoneUpdater* updater,
            types::BoneIndex index);

      private:
        static void ccd_ik(sub_system::IIKBoneViewList& views,
            sub_system::IBoneUpdater& updater,
            const types::CCDIK& ik);
        static glm::quat rotation(glm::vec3 from, glm::vec3 to, float limit);
        static glm::quat axis_rotation(glm::vec3 from, glm::vec3 to, glm::vec3 axis, float limit);
    };
} // namespace enishi::ik
