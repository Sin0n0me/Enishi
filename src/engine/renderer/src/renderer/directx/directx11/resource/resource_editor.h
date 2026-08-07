#pragma once
#include "../../../common/interface_gpu_resource_accessor.h"
#include "../shader/shader_refrection.h"
#include <foundation/option/option.h>
#include <memory>
#include <platform/renderer/interface_image_view.h>
#include <vector>

namespace enishi::renderer::directx {
    class ResourceEditor : public GPUResourceAccessor {
      private:
        enum class ResourceType : std::uint32_t {
            RenderTarget,
            DrawArgs,
            ShaderReflection,
        };

        struct ResourceIndex {
            ResourceType type;
            types::HandleId handle_id;

            bool operator==(const ResourceIndex& other) const {
                return this->type == other.type && this->handle_id == other.handle_id;
            }
        };

        struct KeyHash {
            std::size_t operator()(const ResourceIndex& k) const {
                std::size_t h1 = std::hash<decltype(ResourceIndex::type)>{}(k.type);
                std::size_t h2 = std::hash<decltype(ResourceIndex::handle_id)>{}(k.handle_id);
                return h1 ^ (h2 << sizeof(decltype(ResourceIndex::type)) * 8);
            }
        };

      private:
        std::unordered_map<ResourceIndex, std::size_t, KeyHash> handle_to_index;
        std::vector<types::DrawArgs> draw_args;
        std::vector<std::shared_ptr<platform::IRenderTargetView>> render_targets;
        std::vector<std::shared_ptr<ShaderReflection>> shader_refections;

      public:
        foundation::VoidResult<DirectXError> make_shader_reflection(
            const types::HandleId handle_id, std::shared_ptr<types::ShaderData> shader_data);

        void add_render_target(std::shared_ptr<platform::IRenderTargetView> rtv);
        void make_draw_args(const types::HandleId handle_id, types::DrawArgs&& args);

      public:
        foundation::Option<std::shared_ptr<platform::IRenderTargetView>> get_render_target(
            const types::HandleId handle) const override;
        const std::vector<std::shared_ptr<platform::IRenderTargetView>>& get_render_targets(
            void) const override;
        foundation::Option<const types::DrawArgs&> get_draw_args(
            const types::HandleId handle) const override;
        foundation::Option<types::DrawArgs&> get_draw_args(const types::HandleId handle) override;

        foundation::Option<std::shared_ptr<ShaderReflection>> get_shader_reflection(
            const types::HandleId handle) const;

      private:
        foundation::Option<std::size_t> get_index(const ResourceIndex& index) const;
    };
} // namespace enishi::renderer::directx
