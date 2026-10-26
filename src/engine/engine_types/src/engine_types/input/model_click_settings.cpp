#include "model_click_settings.h"
#include <algorithm>

namespace enishi::types {
    bool ModelClickSettings::accepts_push(types::RigidBodyKind kind) const {
        return std::find(this->push_states.begin(), this->push_states.end(), kind) !=
               this->push_states.end();
    }
} // namespace enishi::types
