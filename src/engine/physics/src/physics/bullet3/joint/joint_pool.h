#pragma once
#include "interface_native_joint_accessor.h"
#include "joint.h"
#include <engine_types/handle/handle_mapper.h>
#include <foundation/pool/resource_pool.h>
#include <memory>
#include <vector>

namespace enishi::physics::bullet3 {
    class JointPool : public INativeJointAccessor {
      private:
        struct JointHandle {
            std::size_t index;
        };

      private:
        types::ResourceMapper<JointHandle> handle_mapper;
        foundation::ResourcePool<NativeJoint> native_joints;

      public:
        std::tuple<types::HandleId, NativeJoint&> emplace_native_joint(
            NativeJoint&& joint) noexcept override;
        std::tuple<types::HandleId, NativeJoint&> make_native_joint(void) noexcept override;
        void remove_native_joint(const types::HandleId handle) noexcept override;

        foundation::Option<NativeJoint&> get_native_joint(
            const types::HandleId handle) noexcept override;
        foundation::Option<const NativeJoint&> get_native_joint(
            const types::HandleId handle) const noexcept override;

        std::span<const NativeJoint&> get_native_joints(void) const noexcept override;
    };
} // namespace enishi::physics::bullet3
