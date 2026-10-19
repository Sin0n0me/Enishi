#include <SDL3/SDL.h>
#include <core/system/asset/asset_system.h>
#include <core/system/asset/shader/shader_data_provider.h>
#include <cstdlib>
#include <glad/gl.h>
#include <iostream>
#include <memory>
#include <render_pass/constructor/shadow/shadow_map_render_pass_constructor.h>
#include <renderer/opengl/opengl40/opengl40_renderer.h>
#include <type_traits>

using namespace enishi;

namespace {
    void require(bool value, const char* message) {
        if (!value) {
            std::cerr << message << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }
    template <class Result> auto take(Result&& result) {
        if (result.is_err()) {
            std::cerr << result.unwrap_err().get_message() << std::endl;
            std::exit(EXIT_FAILURE);
        }
        return std::move(result.unwrap_mut());
    }
    class Context final : public platform::IOpenGLContext {
        SDL_Window* window; // Borrowed; window outlives the test context.
        std::unique_ptr<std::remove_pointer_t<SDL_GLContext>, decltype(&SDL_GL_DestroyContext)>
            context;

      public:
        explicit Context(SDL_Window* window);
        ~Context() override;
        bool make_current() const noexcept override;
        void present() const noexcept override;
        void* get_proc_address(const char* name) const noexcept override;
    };
    Context::Context(SDL_Window* window)
        : window(window)
        , context(SDL_GL_CreateContext(window), SDL_GL_DestroyContext) {
        require(this->context != nullptr, SDL_GetError());
        require(this->make_current(), SDL_GetError());
        const auto load = [](const char* name) {
            return reinterpret_cast<GLADapiproc>(SDL_GL_GetProcAddress(name));
        };
        require(gladLoadGL(load) != 0, "OpenGL function loading failed");
    }
    Context::~Context() = default;
    bool Context::make_current() const noexcept {
        return SDL_GL_MakeCurrent(this->window, this->context.get());
    }
    void Context::present() const noexcept {
        SDL_GL_SwapWindow(this->window);
    }
    void* Context::get_proc_address(const char* name) const noexcept {
        return reinterpret_cast<void*>(SDL_GL_GetProcAddress(name));
    }

    types::DrawCommand bind(types::RenderHandle handle) {
        return {.handle = handle, .sub_command = types::SubCommand::Bind};
    }
    void submit(renderer::opengl::OpenGL40Renderer& renderer, const platform::IRenderPass& pass) {
        for (const auto& command : pass.get_commands()) {
            switch (command.handle.type) {
                case types::RenderHandleType::View:
                    renderer.submit_command_view(command, pass.get_render_target());
                    break;
                case types::RenderHandleType::State:
                    renderer.submit_command_state(command);
                    break;
                case types::RenderHandleType::Shader:
                    renderer.submit_command_shader(command);
                    break;
                case types::RenderHandleType::Topology:
                    renderer.submit_command_topology(command);
                    break;
                case types::RenderHandleType::VertexLayout:
                    renderer.submit_command_vertex_layout(command);
                    break;
                default:
                    break;
            }
            const auto error = glGetError();
            if (error != GL_NO_ERROR) {
                std::cerr << "Command type " << static_cast<int>(command.handle.type) << " error "
                          << error << std::endl;
                require(false, "render pass command failed");
            }
        }
    }
    void check_shadow_target(
        renderer::opengl::OpenGL40Renderer& renderer, const platform::IRenderPass& shadow) {
        renderer.setup_views();
        submit(renderer, shadow);
        require(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
            "shadow FBO incomplete");
        GLint viewport[4]{};
        glGetIntegerv(GL_VIEWPORT, viewport);
        require(viewport[2] == 2048 && viewport[3] == 2048, "shadow viewport size");
        GLint draw_buffer = 0;
        GLint read_buffer = 0;
        glGetIntegerv(GL_DRAW_BUFFER, &draw_buffer);
        glGetIntegerv(GL_READ_BUFFER, &read_buffer);
        require(draw_buffer == GL_NONE && read_buffer == GL_NONE, "depth-only framebuffer buffers");
        glClearDepth(0.25);
        glClear(GL_DEPTH_BUFFER_BIT);
        glDepthMask(GL_FALSE);
        renderer.setup_views();
        submit(renderer, shadow);
        float depth = 0;
        glReadPixels(1, 1, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
        require(depth > 0.999f, "shadow depth must clear even after a pass disabled depth writes");
    }
    void check_model_target(renderer::opengl::OpenGL40Renderer& renderer,
        const platform::IRenderPass& shadow,
        core::ShaderDataProvider& provider) {
        constexpr int WIDTH = 96;
        constexpr int HEIGHT = 80;
        const auto image = take(renderer.create_image(
            types::ImageDescription::make_swap_chain_render_target({WIDTH, HEIGHT})));
        const auto target = take(renderer.create_render_target_view(image,
            types::ImageViewDescription::make_render_target_view_description(
                types::ImageFormat::RGBA8_UNORM)));
        const auto depth_image =
            take(renderer.create_image(types::ImageDescription::make_depth_stencil(
                {WIDTH, HEIGHT}, types::ImageFormat::D32_FLOAT)));
        const auto depth = take(renderer.create_depth_stencil_view(depth_image,
            types::ImageViewDescription::make_depth_stencil_view_description(
                types::ImageFormat::D32_FLOAT)));
        glDepthMask(GL_FALSE);
        renderer.submit_command_view(bind(target->get_handle()), target->get_handle());
        renderer.submit_command_view(bind(depth->get_handle()), target->get_handle());
        require(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
            "model FBO incomplete");
        GLint viewport[4]{};
        glGetIntegerv(GL_VIEWPORT, viewport);
        require(
            viewport[2] == WIDTH && viewport[3] == HEIGHT, "model viewport restored after shadow");
        float cleared_depth = 0;
        glReadPixels(1, 1, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &cleared_depth);
        require(cleared_depth > 0.999f, "model depth must clear after the background pass");
        renderer.submit_command_view(bind(shadow.get_sampled_depth()), target->get_handle());
        glActiveTexture(GL_TEXTURE3);
        GLint texture = 0;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture);
        require(texture != 0, "shadow must be bound to texture unit 3");
        glActiveTexture(GL_TEXTURE0);
        const std::vector<std::tuple<types::ShaderKind, std::filesystem::path>> paths{
            {types::ShaderKind::Vertex, "vs_model_gl"}, {types::ShaderKind::Pixel, "ps_model_gl"}};
        for (const auto& entry : take(provider.get(paths))) {
            take(renderer.create_shader_reflection(*entry.data));
            const auto shader = take(renderer.create_shader(entry.kind, *entry.data));
            renderer.submit_command_shader(bind(shader));
        }
        GLint program = 0;
        glGetIntegerv(GL_CURRENT_PROGRAM, &program);
        const auto location = glGetUniformLocation(program, "shadow_texture");
        require(location != -1, "model shadow sampler missing");
        GLint unit = -1;
        glGetUniformiv(program, location, &unit);
        require(unit == 3, "shader sampler and render pass texture unit must agree");
        renderer.setup_views();
        glActiveTexture(GL_TEXTURE3);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture);
        require(texture == 0, "next frame must detach sampled shadow depth");
        glActiveTexture(GL_TEXTURE0);
    }
    void run() {
        require(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4), SDL_GetError());
        require(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0), SDL_GetError());
        require(SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE),
            SDL_GetError());
        const auto window = std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)>(
            SDL_CreateWindow("Shadow test", 96, 80, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN),
            SDL_DestroyWindow);
        require(window != nullptr, SDL_GetError());
        const auto context = std::make_shared<Context>(window.get());
        renderer::opengl::OpenGL40Renderer renderer(context);
        core::AssetSystem assets;
        core::ShaderDataProvider provider(assets.get_asset_system());
        render_pass::ShadowMapRenderPassConstructor constructor;
        const auto shadow = take(constructor.make(&renderer, nullptr, &provider, {}));
        check_shadow_target(renderer, *shadow);
        check_model_target(renderer, *shadow, provider);
        require(glGetError() == GL_NO_ERROR, "OpenGL error during production render pass test");
    }
} // namespace

int main() {
    require(SDL_Init(SDL_INIT_VIDEO), SDL_GetError());
    run();
    SDL_Quit();
    std::cout << "OpenGL render-pass creation, FBO clearing, viewport and texture bindings passed"
              << std::endl;
}
