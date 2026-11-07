#include "render_pass_tests.h"
#include <test_support/test_runner.h>
#include <array>
#include <render_pass/constructor/pipeline_description_builder.h>
#include <render_pass/render_pass.h>

namespace enishi::tests {
    namespace {
        const types::RenderHandle TARGET{types::HandleId{1}, types::RenderHandleType::View};
        const types::RenderHandle DEPTH{types::HandleId{2}, types::RenderHandleType::View};
        const types::RenderHandle STATE{types::HandleId{3}, types::RenderHandleType::State};
        const types::RenderHandle SHADER{types::HandleId{4}, types::RenderHandleType::Shader};
        const types::RenderHandle MESH{types::HandleId{5}, types::RenderHandleType::Mesh};
        const types::RenderHandle REPLACEMENT{types::HandleId{6}, types::RenderHandleType::Mesh};
    } // namespace

    void pipeline_contract_tests() {
        render_pass::PipelineDescriptionConstructer builder(nullptr, nullptr);
        check(builder.add_topology(types::PrimitiveTopology::TriangleList) == &builder,
            "topology setter preserves fluent builder identity");
        check(builder.add_render_target_view(TARGET) == &builder &&
                  builder.add_depth_stencil_view(DEPTH) == &builder &&
                  builder.add_rasterizer_state(STATE) == &builder &&
                  builder.add_blend_state(STATE) == &builder &&
                  builder.add_depth_stencil_state(STATE) == &builder &&
                  builder.add_sampler_state(STATE) == &builder,
            "every handle setter returns this");
        check(builder.set_sampled_depth(DEPTH) == &builder &&
                  builder.add_shader_resource(TARGET) == &builder &&
                  builder.add_shader_resource(types::RenderHandle{}) == &builder,
            "optional resources ignore invalid handles");
        const auto result = builder.build();
        check(result.is_ok(), "valid description does not require a GPU");
        const auto& value = result.unwrap();
        check(value.topology == types::PrimitiveTopology::TriangleList &&
                  value.render_target_view == TARGET && value.depth_stencil_view == DEPTH &&
                  value.rasterizer_state == STATE && value.blend_state == STATE &&
                  value.depth_stencil_state == STATE && value.sampler_state == STATE &&
                  value.sampled_depth == DEPTH && value.shader_resources == std::vector{TARGET},
            "description preserves all supplied bindings");
        using Setter = render_pass::PipelineDescriptionConstructer* (
            render_pass::PipelineDescriptionConstructer::*)(types::RenderHandle) noexcept;
        const std::array<Setter, 6> setters{
            &render_pass::PipelineDescriptionConstructer::add_render_target_view,
            &render_pass::PipelineDescriptionConstructer::add_depth_stencil_view,
            &render_pass::PipelineDescriptionConstructer::add_rasterizer_state,
            &render_pass::PipelineDescriptionConstructer::add_blend_state,
            &render_pass::PipelineDescriptionConstructer::add_depth_stencil_state,
            &render_pass::PipelineDescriptionConstructer::add_sampler_state};
        for (const auto setter : setters) {
            render_pass::PipelineDescriptionConstructer invalid(nullptr, nullptr);
            check((invalid.*setter)(types::RenderHandle{}) == &invalid,
                "invalid binding preserves fluent identity");
            const auto error = invalid.build();
            check(error.is_err() &&
                      error.unwrap_err().get_error() == render_pass::ConstructError::Construct,
                "required invalid binding cannot silently build");
        }
    }

    void command_contract_tests() {
        types::PipelineDescription description{};
        description.topology = types::PrimitiveTopology::TriangleList;
        description.render_target_view = TARGET;
        description.depth_stencil_view = DEPTH;
        description.sampled_depth = DEPTH;
        description.shaders = {SHADER};
        render_pass::RenderPass pass;
        check(pass.make_from_description(description, "test", {7}, {{3}}).is_ok(),
            "construct a backend-independent render pass");
        const auto initial = pass.get_commands();
        check(initial.size() == 4 && initial[0].handle == TARGET && initial[1].handle == DEPTH &&
                  initial[2].handle.type == types::RenderHandleType::Topology &&
                  initial[3].handle == SHADER,
            "targets and topology are bound before shaders; invalid optional state is omitted");
        check(pass.get_render_target() == TARGET && pass.get_sampled_depth() == DEPTH &&
                  pass.get_node() == foundation::DependencyNode{7} &&
                  pass.get_dependencies().precedents == std::vector<foundation::DependencyNode>{3},
            "pass exposes its render graph dependencies and depth input");
        const auto base_count = initial.size();
        pass.add_mesh(MESH);
        pass.add_mesh("duplicate", MESH);
        pass.add_mesh(REPLACEMENT);
        pass.remove_mesh(TARGET);
        check(pass.get_commands().size() == base_count + 3,
            "mesh removal cannot remove a target binding");
        pass.remove_mesh(MESH);
        check(pass.get_commands().size() == base_count + 1 &&
                  pass.get_commands().back().handle == REPLACEMENT,
            "all occurrences of removed mesh disappear without disturbing other commands");
        pass.remove_mesh(MESH);
        pass.set_shadow_receiver(MESH);
        pass.set_shadow_receiver(REPLACEMENT);
        check(pass.get_shadow_receiver() == REPLACEMENT, "shadow receiver can be replaced");
        for (const auto& command : pass.get_commands()) {
            check(command.handle != MESH, "old shadow receiver is no longer drawn");
        }
        pass.add_updater(nullptr);
        pass.update();
    }
} // namespace enishi::tests

int main() {
    enishi::tests::run_test("pipeline_contract_tests", enishi::tests::pipeline_contract_tests);
    enishi::tests::run_test("command_contract_tests", enishi::tests::command_contract_tests);
    return EXIT_SUCCESS;
}
