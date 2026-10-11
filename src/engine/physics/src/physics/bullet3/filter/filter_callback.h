#pragma once
#include <BulletCollision/BroadphaseCollision/btOverlappingPairCache.h>
#include <unordered_set>

namespace enishi::physics::bullet3 {
    class FilterCallback : public btOverlapFilterCallback {
      protected:
        std::unordered_set<btBroadphaseProxy*> always_collide_proxies;

      public:
        explicit FilterCallback(void) = default;

        void add_proxy(btBroadphaseProxy* const proxy);

        bool needBroadphaseCollision(
            btBroadphaseProxy* proxy0, btBroadphaseProxy* proxy1) const override;
    };
} // namespace enishi::physics::bullet3
