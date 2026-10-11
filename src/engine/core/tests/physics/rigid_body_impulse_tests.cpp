#include <BulletCollision/CollisionShapes/btSphereShape.h>
#include <cstdlib>
#include <iostream>
#include <physics/bullet3/rigid_body/rigid_body_impulse.h>

using namespace enishi;
namespace {
    void check(bool condition, const char* message) {
        if (!condition) {
            std::cerr << message << '\n';
            std::exit(EXIT_FAILURE);
        }
    }
} // namespace

void rigid_body_impulse_tests() {
    constexpr btScalar TOLERANCE = 0.0001f;
    constexpr btScalar MASS = 2;
    btSphereShape shape(1);
    btVector3 inertia;
    shape.calculateLocalInertia(MASS, inertia);
    btRigidBody body(btRigidBody::btRigidBodyConstructionInfo(MASS, nullptr, &shape, inertia));
    auto transform = btTransform::getIdentity();
    transform.setRotation(btQuaternion(btVector3(0, 0, 1), SIMD_HALF_PI));
    body.setWorldTransform(transform);
    body.updateInertiaTensor();
    body.setLinearVelocity(btVector3(10, 20, 30));
    body.setAngularVelocity(btVector3(1, 2, 3));
    body.setInterpolationLinearVelocity(btVector3(1, 2, 3));
    body.setInterpolationAngularVelocity(btVector3(1, 2, 3));
    body.applyCentralForce(btVector3(1, 2, 3));
    types::RigidBodyImpulse impulse;
    impulse.reset_velocity = true;
    impulse.world_velocity = {2, 0, 2};
    impulse.local_velocity = {2, 0, 0};
    impulse.world_torque = {0, 0, 2};
    impulse.local_torque = {2, 0, 0};
    physics::bullet3::apply_rigid_body_impulse(body, impulse);
    check((body.getLinearVelocity() - btVector3(1, 1, -1)).length() < TOLERANCE,
        "reset once then apply mass-scaled world and rotated local impulse");
    const auto expected_angular = body.getInvInertiaTensorWorld() * btVector3(0, -2, 2);
    check((body.getAngularVelocity() - expected_angular).length() < TOLERANCE,
        "torque uses axial reflection and current body orientation");
    check(body.getInterpolationLinearVelocity().isZero() &&
              body.getInterpolationAngularVelocity().isZero() && body.getTotalForce().isZero(),
        "reset clears interpolated velocities and accumulated force");
    impulse = {};
    impulse.world_velocity = {-2, 0, 0};
    physics::bullet3::apply_rigid_body_impulse(body, impulse);
    check((body.getLinearVelocity() - btVector3(0, 1, -1)).length() < TOLERANCE,
        "subsequent impulses preserve existing velocity");
}
