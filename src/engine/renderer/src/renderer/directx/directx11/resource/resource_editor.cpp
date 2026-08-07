#include "resource_editor.h"

namespace enishi::renderer::directx {
    foundation::VoidResult<DirectXError> ResourceEditor::make_shader_reflection(
        const types::HandleId handle_id, std::shared_ptr<types::ShaderData> shader_data) {
        auto reflection = std::make_shared<ShaderReflection>();
        if (!bool(reflection)) {
            return foundation::Error(
                DirectXError::ShaderReflectionError, "メモリを確保できませんでした");
        }

        auto result = reflection->load(shader_data);
        if (result.is_err()) {
            return result;
        }

        this->handle_to_index[ResourceIndex{
            .type = ResourceType::ShaderReflection,
            .handle_id = handle_id,
        }] = this->shader_refections.size();
        this->shader_refections.emplace_back(reflection);

        return {};
    }

    void ResourceEditor::add_render_target(std::shared_ptr<platform::IRenderTargetView> rtv) {
        const auto handle = rtv->get_handle();

        this->handle_to_index[ResourceIndex{
            .type = ResourceType::RenderTarget,
            .handle_id = handle.id,
        }] = this->render_targets.size();

        this->render_targets.emplace_back(std::move(rtv));
    }

    void ResourceEditor::make_draw_args(const types::HandleId handle_id, types::DrawArgs&& args) {
        this->handle_to_index[ResourceIndex{
            .type = ResourceType::DrawArgs,
            .handle_id = handle_id,
        }] = this->draw_args.size();

        this->draw_args.emplace_back(std::move(args));
    }

    foundation::Option<std::shared_ptr<platform::IRenderTargetView>>
    ResourceEditor::get_render_target(const types::HandleId handle) const {
        const auto& opt_index = this->get_index(ResourceIndex{
            .type = ResourceType::RenderTarget,
            .handle_id = handle,
        });
        if (opt_index.is_none()) {
            return {};
        }
        const auto& index = opt_index.unwrap();
        auto& vec = this->render_targets;

        if (vec.size() < index + 1) {
            return {};
        }

        auto& rtv = vec.at(index);
        if (!bool(rtv)) {
            return {};
        }

        return rtv;
    }

    const std::vector<std::shared_ptr<platform::IRenderTargetView>>&
    ResourceEditor::get_render_targets(void) const {
        return this->render_targets;
    }

    foundation::Option<const types::DrawArgs&> ResourceEditor::get_draw_args(
        const types::HandleId handle) const {
        const auto& opt_index = this->get_index(ResourceIndex{
            .type = ResourceType::RenderTarget,
            .handle_id = handle,
        });
        if (opt_index.is_none()) {
            return {};
        }
        const auto& index = opt_index.unwrap();
        auto& vec = this->draw_args;

        if (vec.size() < index + 1) {
            return {};
        }

        return vec.at(index);
    }

    foundation::Option<types::DrawArgs&> ResourceEditor::get_draw_args(
        const types::HandleId handle) {
        const auto& opt_index = this->get_index(ResourceIndex{
            .type = ResourceType::DrawArgs,
            .handle_id = handle,
        });
        if (opt_index.is_none()) {
            return {};
        }
        const auto& index = opt_index.unwrap();
        auto& vec = this->draw_args;

        if (vec.size() < index + 1) {
            return {};
        }

        return vec.at(index);
    }

    foundation::Option<std::shared_ptr<ShaderReflection>> ResourceEditor::get_shader_reflection(
        const types::HandleId handle) const {
        const auto& opt_index = this->get_index(ResourceIndex{
            .type = ResourceType::ShaderReflection,
            .handle_id = handle,
        });
        if (opt_index.is_none()) {
            return {};
        }
        const auto& index = opt_index.unwrap();
        auto& vec = this->shader_refections;

        if (vec.size() < index + 1) {
            return {};
        }

        return vec.at(index);
    }

    foundation::Option<std::size_t> ResourceEditor::get_index(const ResourceIndex& index) const {
        const auto& iter = this->handle_to_index.find(index);
        if (iter == this->handle_to_index.end()) {
            return {};
        }
        return iter->second;
    }
} // namespace enishi::renderer::directx