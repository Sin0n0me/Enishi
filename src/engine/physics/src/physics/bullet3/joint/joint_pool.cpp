#include "joint_pool.h"

namespace enishi::physics::bullet3 {
    std::tuple<types::HandleId, JointPool::NativeJoint&> JointPool::emplace_native_joint(
        NativeJoint&& joint) noexcept {
        return this->handle_mapper.make_from(
            this->native_joints.emplace(std::move(joint)), [](const std::size_t index) {
                return decltype(handle_mapper)::ValueType{
                    .index = index,
                };
            });
    }

    void JointPool::remove_native_joint(const types::HandleId handle) noexcept {
    }

    foundation::Option<JointPool::NativeJoint&> JointPool::get_native_joint(
        const types::HandleId handle) noexcept {
        return this->handle_mapper.get(handle).and_then(
            [this](const decltype(handle_mapper)::ValueType& mapped) {
                return this->native_joints.get(mapped.index);
            });
    }

    foundation::Option<const JointPool::NativeJoint&> JointPool::get_native_joint(
        const types::HandleId handle) const noexcept {
        return this->handle_mapper.get(handle).and_then(
            [this](const decltype(handle_mapper)::ValueType& mapped) {
                return this->native_joints.get(mapped.index);
            });
    }

    std::span<const JointPool::NativeJoint> JointPool::get_native_joints(void) const noexcept {
        return this->native_joints.get_all();
    }
} // namespace enishi::physics::bullet3