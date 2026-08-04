#pragma once
#include "../../../errors/errors.h"
#include "../shader/shader_type.h"
#include <d3d11.h>
#include <engine_types/handle/handle_type.h>
#include <engine_types/renderer/description/image_view_description.h>
#include <foundation/option/option.h>
#include <foundation/result/result.h>
#include <memory>
#include <unordered_map>
#include <vector>
#include <wrl/client.h>

namespace enishi::renderer::directx {
    namespace {
        struct ViewInfo {
            types::ImageViewType view_type;
            std::uint32_t index;
        };
    } // namespace

    class ViewPool {
      private:
        std::unordered_map<types::HandleId, ViewInfo> handle_map;
        std::vector<Microsoft::WRL::ComPtr<ID3D11DepthStencilView>> depth_stencils;
        std::vector<Microsoft::WRL::ComPtr<ID3D11RenderTargetView>> render_targets;
        std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> shader_resources;
        std::vector<Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>> unordered_accesses;

      private:
        [[nodiscard]] foundation::Option<const ViewInfo&> get_view_info(
            const types::HandleId id) const noexcept;

      public:
        [[nodiscard]] foundation::VoidResult<DirectXError> create(
            const types::HandleId id, const types::ImageViewType view_type) noexcept;

        [[nodiscard]] foundation::Option<types::ImageViewType> get_view_type(
            const types::HandleId id) const noexcept;

        [[nodiscard]] foundation::Option<Microsoft::WRL::ComPtr<ID3D11DepthStencilView>>
        get_depth_stencil_view(const types::HandleId id) const noexcept;
        [[nodiscard]] foundation::Option<Microsoft::WRL::ComPtr<ID3D11RenderTargetView>>
        get_render_target_view(const types::HandleId id) const noexcept;
        [[nodiscard]] foundation::Option<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>>
        get_shader_resource_view(const types::HandleId id) const noexcept;
        [[nodiscard]] foundation::Option<Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>>
        get_unordered_access_view(const types::HandleId id) const noexcept;

      private:
    };
} // namespace enishi::renderer::directx