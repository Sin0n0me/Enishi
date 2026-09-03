#include "filter_callback.h"

namespace enishi::physics::bullet3 {
    void FilterCallback::add_proxy(btBroadphaseProxy* const proxy) {
        this->always_collide_proxies.insert(proxy);
    }

    bool FilterCallback::needBroadphaseCollision(
        btBroadphaseProxy* proxy0, btBroadphaseProxy* proxy1) const {
        if (this->always_collide_proxies.contains(proxy0) ||
            this->always_collide_proxies.contains(proxy1)) {
            return true;
        }

        if ((proxy0->m_collisionFilterGroup & proxy1->m_collisionFilterMask) == 0) {
            return false;
        }

        if ((proxy1->m_collisionFilterGroup & proxy0->m_collisionFilterMask) == 0) {
            return false;
        }

        return true;
    }
} // namespace enishi::physics::bullet3