#include "bone.h"

namespace enishi::types {
    bool BoneNode::has_parent(void) const {
        return this->parent != INVALID_BONE_INDEX;
    }
} // namespace enishi::types