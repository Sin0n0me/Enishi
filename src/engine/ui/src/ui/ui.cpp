#include "ui.h"
#include <SDL3/SDL.h>
#include <backends/imgui_impl_sdl3.h>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <utility>
#if defined(USE_OPENGL40)
#include <backends/imgui_impl_opengl3.h>
#else
#include <backends/imgui_impl_dx11.h>
#endif

namespace enishi::ui {
    void ImGuiContextDeleter::operator()(ImGuiContext* context) const noexcept {
        ImGui::DestroyContext(context);
    }

    UI::UI(std::shared_ptr<platform::IWindow> window, std::shared_ptr<platform::IRenderer> renderer)
        : window(std::move(window))
        , renderer(std::move(renderer)) {
    }

    UI::~UI(void) noexcept {
        this->shutdown();
    }

    foundation::VoidResult<UIError> UI::create_context(void) {
        if (this->context != nullptr) {
            return foundation::Error(UIError::InitializationFailed, "UI is already initialized");
        }
        IMGUI_CHECKVERSION();
        this->context.reset(ImGui::CreateContext());
        if (this->context == nullptr) {
            return foundation::Error(
                UIError::InitializationFailed, "ImGui context creation failed");
        }
        ImGui::SetCurrentContext(this->context.get());
        ImGui::GetIO().IniFilename = nullptr;
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        ImGui::StyleColorsDark();
        return {};
    }

    foundation::VoidResult<UIError> UI::init_opengl(SDL_Window* window, void* gl_context) {
#if defined(USE_OPENGL40)
        if (window == nullptr || gl_context == nullptr) {
            return foundation::Error(
                UIError::InitializationFailed, "UI requires an OpenGL window and context");
        }
        auto result = this->create_context();
        if (result.is_err()) {
            return result;
        }
        this->platform_initialized = ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
        if (this->platform_initialized) {
            this->renderer_initialized = ImGui_ImplOpenGL3_Init("#version 400 core");
        }
        if (!this->renderer_initialized) {
            this->shutdown();
            return foundation::Error(
                UIError::InitializationFailed, "ImGui SDL3/OpenGL initialization failed");
        }
        return {};
#else
        return foundation::Error(UIError::InitializationFailed, "OpenGL UI backend is not enabled");
#endif
    }

    foundation::VoidResult<UIError> UI::init_directx(
        SDL_Window* window, ID3D11Device* device, ID3D11DeviceContext* device_context) {
#if !defined(USE_OPENGL40)
        if (window == nullptr || device == nullptr || device_context == nullptr) {
            return foundation::Error(UIError::InitializationFailed,
                "UI requires an SDL window and D3D11 device/context");
        }
        auto result = this->create_context();
        if (result.is_err()) {
            return result;
        }
        this->platform_initialized = ImGui_ImplSDL3_InitForD3D(window);
        if (this->platform_initialized) {
            this->renderer_initialized = ImGui_ImplDX11_Init(device, device_context);
        }
        if (!this->renderer_initialized) {
            this->shutdown();
            return foundation::Error(
                UIError::InitializationFailed, "ImGui SDL3/D3D11 initialization failed");
        }
        return {};
#else
        return foundation::Error(UIError::InitializationFailed, "D3D11 UI backend is not enabled");
#endif
    }

    void UI::shutdown(void) noexcept {
        if (this->context == nullptr) {
            return;
        }
        ImGui::SetCurrentContext(this->context.get());
        if (this->renderer_initialized) {
#if defined(USE_OPENGL40)
            ImGui_ImplOpenGL3_Shutdown();
#else
            ImGui_ImplDX11_Shutdown();
#endif
        }
        if (this->platform_initialized) {
            ImGui_ImplSDL3_Shutdown();
        }
        this->renderer_initialized = false;
        this->platform_initialized = false;
        this->frame_ready = false;
        this->context.reset();
    }

    void UI::process_event(const SDL_Event& event) {
        if (this->platform_initialized) {
            ImGui::SetCurrentContext(this->context.get());
            ImGui_ImplSDL3_ProcessEvent(&event);
        }
    }

    void UI::begin_frame(void) {
        ImGui::SetCurrentContext(this->context.get());
        this->frame_ready = false;
#if defined(USE_OPENGL40)
        ImGui_ImplOpenGL3_NewFrame();
#else
        ImGui_ImplDX11_NewFrame();
#endif
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
    }

    void UI::end_frame(void) {
        ImGui::SetCurrentContext(this->context.get());
        ImGui::Render();
        this->frame_ready = true;
    }

    void UI::render(void) {
        if (!this->frame_ready) {
            return;
        }
        ImGui::SetCurrentContext(this->context.get());
#if defined(USE_OPENGL40)
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#else
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
#endif
        this->frame_ready = false;
    }

    bool UI::begin_window(const char* title) {
        ImGui::SetCurrentContext(this->context.get());
        return ImGui::Begin(title);
    }

    void UI::end_window(void) {
        ImGui::SetCurrentContext(this->context.get());
        ImGui::End();
    }

    void UI::text(std::string_view value) {
        ImGui::SetCurrentContext(this->context.get());
        if (value.empty()) {
            ImGui::TextUnformatted("");
            return;
        }
        ImGui::TextUnformatted(value.data(), value.data() + value.size());
    }

    bool UI::button(const char* label) {
        ImGui::SetCurrentContext(this->context.get());
        return ImGui::Button(label);
    }

    bool UI::input_text(const char* label, std::string& value) {
        ImGui::SetCurrentContext(this->context.get());
        return ImGui::InputText(label, &value);
    }

    bool UI::wants_capture_mouse(void) const {
        if (!this->platform_initialized) {
            return false;
        }
        ImGui::SetCurrentContext(this->context.get());
        return ImGui::GetIO().WantCaptureMouse;
    }

    bool UI::wants_capture_keyboard(void) const {
        if (!this->platform_initialized) {
            return false;
        }
        ImGui::SetCurrentContext(this->context.get());
        return ImGui::GetIO().WantCaptureKeyboard;
    }
} // namespace enishi::ui
