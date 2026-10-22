#include <array>
#include <renderer/common/converter/skinned_vertices.h>
#include <renderer/vulkan/pipeline/vulkan_pipeline.h>

namespace enishi::renderer::vulkan {
    namespace {
        VkBlendFactor blend_factor(types::BlendFactor value) {
            switch (value) {
                case types::BlendFactor::Zero:
                    return VK_BLEND_FACTOR_ZERO;
                case types::BlendFactor::One:
                    return VK_BLEND_FACTOR_ONE;
                case types::BlendFactor::SrcColor:
                    return VK_BLEND_FACTOR_SRC_COLOR;
                case types::BlendFactor::OneMinusSrcColor:
                    return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
                case types::BlendFactor::DstColor:
                    return VK_BLEND_FACTOR_DST_COLOR;
                case types::BlendFactor::OneMinusDstColor:
                    return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
                case types::BlendFactor::SrcAlpha:
                    return VK_BLEND_FACTOR_SRC_ALPHA;
                case types::BlendFactor::OneMinusSrcAlpha:
                    return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                case types::BlendFactor::DstAlpha:
                    return VK_BLEND_FACTOR_DST_ALPHA;
                case types::BlendFactor::OneMinusDstAlpha:
                    return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
                case types::BlendFactor::ConstantColor:
                    return VK_BLEND_FACTOR_CONSTANT_COLOR;
                case types::BlendFactor::OneMinusConstantColor:
                    return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
                case types::BlendFactor::ConstantAlpha:
                    return VK_BLEND_FACTOR_CONSTANT_ALPHA;
                case types::BlendFactor::OneMinusConstantAlpha:
                    return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
                case types::BlendFactor::SrcAlphaSaturate:
                    return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
                default:
                    return VK_BLEND_FACTOR_ZERO; // Dual-source factors are rejected at creation.
            }
        }
        VkBlendOp blend_op(types::BlendOperator value) {
            switch (value) {
                case types::BlendOperator::Add:
                    return VK_BLEND_OP_ADD;
                case types::BlendOperator::Subtract:
                    return VK_BLEND_OP_SUBTRACT;
                case types::BlendOperator::ReverseSubtract:
                    return VK_BLEND_OP_REVERSE_SUBTRACT;
                case types::BlendOperator::Min:
                    return VK_BLEND_OP_MIN;
                case types::BlendOperator::Max:
                    return VK_BLEND_OP_MAX;
            }
            return VK_BLEND_OP_ADD;
        }
        VkCompareOp compare_op(types::CompareOperator value) {
            switch (value) {
                case types::CompareOperator::Never:
                    return VK_COMPARE_OP_NEVER;
                case types::CompareOperator::Less:
                    return VK_COMPARE_OP_LESS;
                case types::CompareOperator::Equal:
                    return VK_COMPARE_OP_EQUAL;
                case types::CompareOperator::LessEqual:
                    return VK_COMPARE_OP_LESS_OR_EQUAL;
                case types::CompareOperator::Greater:
                    return VK_COMPARE_OP_GREATER;
                case types::CompareOperator::NotEqual:
                    return VK_COMPARE_OP_NOT_EQUAL;
                case types::CompareOperator::GreaterEqual:
                    return VK_COMPARE_OP_GREATER_OR_EQUAL;
                case types::CompareOperator::Always:
                    return VK_COMPARE_OP_ALWAYS;
            }
            return VK_COMPARE_OP_ALWAYS;
        }
        VkPipelineColorBlendAttachmentState make_blend(
            const types::RenderTargetBlendState& source) {
            VkPipelineColorBlendAttachmentState blend{};
            blend.blendEnable = source.enabled;
            blend.srcColorBlendFactor = blend_factor(source.src_color);
            blend.dstColorBlendFactor = blend_factor(source.dst_color);
            blend.colorBlendOp = blend_op(source.color_operator);
            blend.srcAlphaBlendFactor = blend_factor(source.src_alpha);
            blend.dstAlphaBlendFactor = blend_factor(source.dst_alpha);
            blend.alphaBlendOp = blend_op(source.alpha_operator);
            blend.colorWriteMask = source.write_mask;
            return blend;
        }
        std::array<VkVertexInputAttributeDescription, 9> model_attributes() {
            // Attribute offsets are taken from the CPU struct, never inferred from shader packing.
            return {{{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(SkinnedVertex, position)},
                {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(SkinnedVertex, normal)},
                {2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(SkinnedVertex, uv)},
                {3, 0, VK_FORMAT_R32G32B32A32_UINT, offsetof(SkinnedVertex, bones)},
                {4, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(SkinnedVertex, weights)},
                {5, 0, VK_FORMAT_R32_UINT, offsetof(SkinnedVertex, method)},
                {6, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(SkinnedVertex, center)},
                {7, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(SkinnedVertex, anchor0)},
                {8, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(SkinnedVertex, anchor1)}}};
        }
    } // namespace
    platform::RenderResult<VkPipeline> VulkanPipeline::active_pipeline(VulkanRendererState& state) {
        const std::array key{state.vertex_shader.id.handle_id,
            state.pixel_shader.id.handle_id,
            state.rasterizer.id.handle_id,
            state.depth.id.handle_id,
            state.blend.id.handle_id,
            static_cast<std::uint32_t>(state.context->shadow_recording)};
        const auto cached = state.pipelines.find(key);
        if (cached != state.pipelines.end()) {
            return cached->second;
        }
        if (!state.shaders.contains(state.vertex_shader) ||
            !state.shaders.contains(state.pixel_shader)) {
            return foundation::Error(platform::RenderError::MakeError,
                "Vulkan draw requires vertex and fragment shaders");
        }
        std::array<VkPipelineShaderStageCreateInfo, 2> stages{};
        for (auto& stage : stages) {
            stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            stage.pName = "main";
        }
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = state.shaders.at(state.vertex_shader).module;
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module = state.shaders.at(state.pixel_shader).module;
        const VkVertexInputBindingDescription binding{
            0, sizeof(SkinnedVertex), VK_VERTEX_INPUT_RATE_VERTEX};
        const auto attributes = model_attributes();
        VkPipelineVertexInputStateCreateInfo vertex{
            VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
        vertex.vertexBindingDescriptionCount = 1;
        vertex.pVertexBindingDescriptions = &binding;
        vertex.vertexAttributeDescriptionCount = static_cast<std::uint32_t>(attributes.size());
        vertex.pVertexAttributeDescriptions = attributes.data();
        VkPipelineInputAssemblyStateCreateInfo assembly{
            VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
        assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkPipelineViewportStateCreateInfo viewport{
            VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
        viewport.viewportCount = 1;
        viewport.scissorCount = 1;
        VkPipelineRasterizationStateCreateInfo raster{
            VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
        raster.polygonMode = VK_POLYGON_MODE_FILL;
        raster.lineWidth = 1.0f;
        if (state.rasterizers.contains(state.rasterizer)) {
            const auto& source = state.rasterizers.at(state.rasterizer);
            if (source.cull_mode == types::CullMode::Back) {
                raster.cullMode = VK_CULL_MODE_BACK_BIT;
            }
            if (source.cull_mode == types::CullMode::Front) {
                raster.cullMode = VK_CULL_MODE_FRONT_BIT;
            }
            raster.frontFace = source.front_face == types::FrontFace::CounterClockwise
                                   ? VK_FRONT_FACE_COUNTER_CLOCKWISE
                                   : VK_FRONT_FACE_CLOCKWISE;
            raster.depthBiasEnable = source.depth_bias.enable;
            raster.depthBiasConstantFactor = source.depth_bias.constant_factor;
            raster.depthBiasSlopeFactor = source.depth_bias.slope_factor;
        }
        VkPipelineMultisampleStateCreateInfo multisample{
            VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
        multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        VkPipelineDepthStencilStateCreateInfo depth{
            VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
        if (state.depths.contains(state.depth)) {
            const auto& source = state.depths.at(state.depth).depth;
            depth.depthTestEnable = source.enabled;
            depth.depthWriteEnable = source.write_enabled;
            depth.depthCompareOp = compare_op(source.compare_operator);
        }
        auto blend_attachment = make_blend(types::RenderTargetBlendState::default_blend_state());
        if (state.blends.contains(state.blend)) {
            blend_attachment = make_blend(state.blends.at(state.blend).render_targets[0]);
        }
        VkPipelineColorBlendStateCreateInfo blend{
            VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
        blend.attachmentCount = state.context->shadow_recording ? 0 : 1;
        blend.pAttachments = &blend_attachment;
        const std::array dynamic_states{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamic{
            VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
        dynamic.dynamicStateCount = static_cast<std::uint32_t>(dynamic_states.size());
        dynamic.pDynamicStates = dynamic_states.data();
        VkGraphicsPipelineCreateInfo info{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
        info.stageCount = static_cast<std::uint32_t>(stages.size());
        info.pStages = stages.data();
        info.pVertexInputState = &vertex;
        info.pInputAssemblyState = &assembly;
        info.pViewportState = &viewport;
        info.pRasterizationState = &raster;
        info.pMultisampleState = &multisample;
        info.pDepthStencilState = &depth;
        info.pColorBlendState = &blend;
        info.pDynamicState = &dynamic;
        info.layout = state.pipeline_layout;
        info.renderPass = state.context->shadow_recording ? state.active_shadow->render_pass
                                                          : state.context->render_pass;
        VkPipeline pipeline = VK_NULL_HANDLE;
        auto result =
            check_vk(vkCreateGraphicsPipelines(
                         state.context->device, VK_NULL_HANDLE, 1, &info, nullptr, &pipeline),
                "vkCreateGraphicsPipelines");
        if (result.is_err()) {
            return std::move(result).take_err();
        }
        state.pipelines.emplace(key, pipeline);
        return pipeline;
    }
} // namespace enishi::renderer::vulkan
