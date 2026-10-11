#include "buffer_interface_maker.h"
#include "updater/bone_updater.h"
#include "updater/camera_updater.h"
#include <engine_types/renderer/uniform_buffer/bones.h>
#include <engine_types/renderer/uniform_buffer/camera.h>
#include <engine_types/renderer/uniform_buffer/light.h>
#include <engine_types/renderer/uniform_buffer/material.h>

namespace enishi::renderer::directx {
    std::shared_ptr<platform::IResourceUpdater> BufferInterfaceMaker::make_uniform(
        types::OwnedRenderData&& render_data,
        const ID3D11Context::Context context,
        const INativeBufferAccessor::NativeBuffer buffer) {
        if (types::LightModelBones::UNIFORM_NAME) {
            return std::make_shared<BoneUpdater>(std::move(render_data), context, buffer);
        }
        if (types::MediumModelBones::UNIFORM_NAME) {
            return std::make_shared<BoneUpdater>(std::move(render_data), context, buffer);
        }
        if (types::HeavyModelBones::UNIFORM_NAME) {
            return std::make_shared<BoneUpdater>(std::move(render_data), context, buffer);
        }
        if (types::UniformCamera::UNIFORM_NAME) {
            return std::make_shared<CameraUpdater>(std::move(render_data), context, buffer);
        }

        return std::shared_ptr<platform::IResourceUpdater>();
    }
} // namespace enishi::renderer::directx