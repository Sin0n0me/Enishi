#pragma once
#include <component/model_component.h>
#include <engine_types/assets/model/model_data.h>

namespace enishi::model_controller {
    [[nodiscard]] component::ModelComponent make_model_component(
        const types::ModelData& data, types::RenderHandle handle);
}
