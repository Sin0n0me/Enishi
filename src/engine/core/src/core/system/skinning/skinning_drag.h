#pragma once
#include "model_bones.h"
#include <component/animation_component.h>
#include <component/model_component.h>
#include <ecs/registry.h>
#include <unordered_map>
namespace enishi::core {
    class SkinningDrag final {
      public:
        static bool begin(ecs::Registry& registry,
            const std::unordered_map<types::HandleId, std::unique_ptr<ModelBones>>& model_bones,
            const animation::CursorRay& local);
        static void end(
            const std::unordered_map<types::HandleId, std::unique_ptr<ModelBones>>& model_bones);
    };
} // namespace enishi::core
