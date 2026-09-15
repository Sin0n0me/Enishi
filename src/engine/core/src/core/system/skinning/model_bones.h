#pragma once
#include <memory>
#include <skinning_system/cache/animation_bone_cache.h>
#include <skinning_system/cache/bind_bone_cache.h>
#include <skinning_system/cache/ik_bone_cache.h>
#include <skinning_system/cache/physics_bone_cache.h>
#include <skinning_system/updater/animation_bones_updater.h>
#include <skinning_system/updater/ik_bones_updater.h>
#include <skinning_system/updater/physics_bones_updater.h>

namespace enishi::core {
    // 1モデル分のView/Cache/Updaterをまとめて保持する
    // Updaterは同じ構造体内のCacheへポインタで依存するため、
    // 構築後にこの構造体自体がコピー/移動されないことを前提とする
    // (呼び出し側はunordered_map<EntityID, std::unique_ptr<ModelBones>>のように
    //  アドレスが変わらない形で保持すること)
    struct ModelBones {
        std::unique_ptr<skinning_system::AnimationBonesCache> animation_cache;
        std::shared_ptr<skinning_system::PhysicsBonesCache> physics_cache;
        std::unique_ptr<skinning_system::IKBoneCache> ik_cache;
        std::unique_ptr<skinning_system::BindBonesCache> bind_cache;

        std::unique_ptr<skinning_system::AnimationBonesUpdater> animation_updater;
        std::shared_ptr<skinning_system::PhysicsBonesUpdater> physics_updater;
        std::unique_ptr<skinning_system::IKBonesUpdater> ik_updater;

        ModelBones(void) = default;

        ModelBones(const ModelBones&) = delete;
        ModelBones& operator=(const ModelBones&) = delete;
        ModelBones(ModelBones&&) = delete;
        ModelBones& operator=(ModelBones&&) = delete;
    };
} // namespace enishi::core
