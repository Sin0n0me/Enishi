#include "resource_editor.h"

namespace enishi::renderer::directx {
    ResourceEditor::ResourceEditor(std::shared_ptr<types::HandleAllocator> handle_allocator)
        : handle_allocator(handle_allocator) {
    }

    foundation::Result<types::RenderHandle, DirectXError> ResourceEditor::make_shader_reflection(
        const types::ShaderData& shader_data) {
        const auto iter = this->hash_to_shader_refection.find(shader_data.hash());
        if (iter != this->hash_to_shader_refection.end()) {
            // TODO:
        }

        auto reflection = std::make_shared<ShaderReflection>();
        if (!bool(reflection)) {
            return foundation::Error(DirectXError::ShaderReflectionError);
        }

        auto result =
            reflection->load(shader_data).add_message("シェーダーの読み込みに失敗しました");
        if (result.is_err()) {
            return result.propagation(DirectXError::ShaderReflectionError);
        }

        const auto handle_id = this->handle_allocator->create();
        this->handle_to_index[ResourceIndex{
            .type = ResourceType::ShaderReflection,
            .handle_id = handle_id,
        }] = this->shader_refections.size();

        this->shader_refections.emplace_back(reflection);

        return types::RenderHandle{
            .id = handle_id,
            .type = types::RenderHandleType::ShaderReflection,
        };
    }

    void ResourceEditor::add_render_target(std::shared_ptr<platform::IRenderTargetView> rtv) {
        const auto handle = rtv->get_handle();

        this->handle_to_index[ResourceIndex{
            .type = ResourceType::RenderTarget,
            .handle_id = handle.id,
        }] = this->render_targets.size();

        this->render_targets.emplace_back(std::move(rtv));
    }

    foundation::Result<types::RenderHandle, DirectXError> ResourceEditor::make_draw_args(
        types::DrawArgs&& args) {
        const auto handle_id = this->handle_allocator->create();
        this->handle_to_index[ResourceIndex{
            .type = ResourceType::DrawArgs,
            .handle_id = handle_id,
        }] = this->draw_args.size();

        this->draw_args.emplace_back(std::move(args));

        return types::RenderHandle{
            .id = handle_id,
            .type = types::RenderHandleType::Draw,
        };
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

    foundation::Option<std::size_t> ResourceEditor::get_index(const ResourceIndex& index) const {
        const auto& iter = this->handle_to_index.find(index);
        if (iter == this->handle_to_index.end()) {
            return {};
        }
        return iter->second;
    }

    foundation::Option<const IShaderReflection<DirectXError>*>
    ResourceEditor::get_shader_reflection(const types::HandleId handle) const {
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

        return vec.at(index).get();
    }

    foundation::Option<IShaderReflection<DirectXError>*> ResourceEditor::get_shader_reflection(
        const types::HandleId handle) {
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

        return vec.at(index).get();
    }
} // namespace enishi::renderer::directx