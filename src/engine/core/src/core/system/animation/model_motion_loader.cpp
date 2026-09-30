#include "model_motion_loader.h"
#include <animation/converter/vmd/vmd_converter.h>
#include <assets_system/animation/vmd/vmd_loader.h>
#include <assets_system/model/bone/bone_resolver.h>
#include <assets_system/model/morph/morph_resolver.h>

namespace enishi::core {
    foundation::Result<animation::AnimationClipData, animation::AnimationError> bind_model_motion(
        const component::ModelComponent& model, const assets_system::VMDData& data) {
        assets_system::BoneNameMapConstructor bone_names;
        bone_names.bone_names = model.bone_names;
        assets_system::BoneResolver bones(bone_names);
        assets_system::MorphNameMapConstructor morph_names;
        for (const auto& target : model.morph_targets.targets) {
            morph_names.morph_names.push_back(target.name);
        }
        assets_system::MorphResolver morphs(morph_names);
        return animation::FrameConverter::convert_clip_data(&bones, &morphs, data);
    }

    foundation::Result<animation::AnimationClipData, animation::AnimationError> load_model_motion(
        const component::ModelComponent& model, const std::filesystem::path& path) {
        auto data = assets_system::VMDLoader::load(path);
        if (data.is_err()) {
            return data.propagation(animation::AnimationError::FailedConvert);
        }
        auto clip = bind_model_motion(model, *data.unwrap());
        if (clip.is_ok()) {
            clip.unwrap_mut().name = path.stem().string();
        }
        return clip;
    }
} // namespace enishi::core
