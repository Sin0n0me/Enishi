#include "render_system.h"
#include <component/animation_component.h>
#include <component/model_component.h>
#include <foundation/log/logger.h>

namespace enishi::core {
    struct RenderPassGraph {
        std::unordered_map<types::DependencyNode, std::size_t> node_to_index;
        std::vector<std::vector<std::size_t>> successors;
        std::vector<std::size_t> indegree;
    };

    RenderSystem::RenderSystem(std::shared_ptr<ecs::Registory> registory,
        std::shared_ptr<platform::IRenderer> renderer,
        std::shared_ptr<platform::IRenderCommandEncoder> encoder)
        : registory(registory)
        , renderer(renderer)
        , encoder(encoder) {
    }

    foundation::VoidResult<SystemError> RenderSystem::add_render_pass_constructor(
        std::shared_ptr<IRenderPassConstructor> render_pass_constructor) {
        if (!bool(render_pass_constructor)) {
            return foundation::Error(
                SystemError::ConstructRenderPassError, "レンダーパスコンストラクタがNULLです");
        }

        this->constructors.emplace_back(render_pass_constructor);
        return {};
    }

    foundation::VoidResult<SystemError> RenderSystem::create_render_passes(
        assets_system::IAssetSystem* const asset_system) {
        RenderPassGraph graph;
        const std::size_t node_count = this->constructors.size();

        graph.node_to_index.reserve(node_count);
        graph.successors.resize(node_count);
        graph.indegree.assign(node_count, 0U);

        //  DependencyNode -> constructor index のマッピングを作る
        for (std::size_t index = 0; index < node_count; ++index) {
            const auto& constructor = this->constructors[index];
            const auto node = constructor->get_node();

            const auto [iterator, inserted] = graph.node_to_index.emplace(node, index);
            if (!inserted) {
                return foundation::Error(
                    SystemError::ConstructRenderPassError, "レンダーパスが重複しています");
            }
        }

        // recedents からグラフを構築する
        for (std::size_t current_index = 0; current_index < node_count; ++current_index) {
            const auto& constructor = this->constructors[current_index];

            const auto dependencies = constructor->get_dependencies();
            if (!dependencies.is_some()) {
                continue;
            }
            const auto& bounds = dependencies.unwrap();

            // 同一 current に対して同じ precedent が複数指定されても
            // indegree を二重に増やさないようにする。
            std::unordered_set<std::size_t> unique_precedents;
            unique_precedents.reserve(bounds.precedents.size());
            for (const auto& precedent : bounds.precedents) {
                const auto iterator = graph.node_to_index.find(precedent);
                if (iterator == graph.node_to_index.end()) {
                    foundation::Logger::warning(
                        std::format("不明な依存先が見つかりました. 現在id: {} 依存先id: {}",
                            constructor->get_node().id,
                            precedent.id));
                    continue;
                }

                const std::size_t precedent_index = iterator->second;
                if (!unique_precedents.emplace(precedent_index).second) {
                    continue;
                }

                graph.successors[precedent_index].push_back(current_index);
                ++graph.indegree[current_index];
            }
        }

        // Kahn's algorithm
        // priority_queue を使わず set of available nodes の最小 index を priority_queue で選択
        // これにより依存関係がないノードについては constructors の入力順を優先する。
        std::priority_queue<std::size_t, std::vector<std::size_t>, std::greater<std::size_t>> ready;
        for (std::size_t index = 0; index < node_count; ++index) {
            if (graph.indegree[index] == 0U) {
                ready.push(index);
            }
        }

        std::vector<std::size_t> sorted_indices;
        sorted_indices.reserve(node_count);

        while (!ready.empty()) {
            const std::size_t current = ready.top();
            ready.pop();
            sorted_indices.push_back(current);

            for (const std::size_t successor : graph.successors[current]) {
                if (graph.indegree[successor] == 0U) {
                    return foundation::Error(SystemError::ConstructRenderPassError,
                        "無効な依存グラフです"); // 通常ここには到達しない(グラフ構築時の不変条件に反するため)
                }

                --graph.indegree[successor];
                if (graph.indegree[successor] == 0U) {
                    ready.push(successor);
                }
            }
        }

        // 全ノードを取り出せなければ循環依存している
        if (sorted_indices.size() != node_count) {
            return foundation::Error(
                SystemError::ConstructRenderPassError, "依存グラフに循環が見つかりました");
        }

        // Topological order に従って RenderPass を生成
        this->render_passes.reserve(node_count);
        for (const auto& constructor_index : sorted_indices) {
            const auto& constructor = constructors[constructor_index];

            auto result = constructor->make(this->renderer.get(), asset_system)
                              .add_message("レンダーパスの構築に失敗しました");
            if (result.is_err()) {
                return result.propagation(SystemError::ConstructRenderPassError);
            }

            this->render_passes.emplace_back(std::move(result).unwrap());
        }

        return {};
    }

    bool enishi::core::RenderSystem::should_close(void) {
        return false;
    }

    void enishi::core::RenderSystem::pre_update(void) {
    }

    void RenderSystem::update(const types::DeltaTime& delta_time) {
        auto view =
            this->registory->view<component::AnimationComponent, component::ModelComponent>();

        for (auto [entity, animation, model] : view) {
        }

        // 各パイプラインに応じた描画コマンド実行
        for (const auto& pass : this->render_passes) {
            pass->update();
        }
    }

    void enishi::core::RenderSystem::post_update(void) {
    }

    void enishi::core::RenderSystem::render(void) const {
        this->submit_render_graph();
        this->present();
    }

    void RenderSystem::submit_render_graph(void) const {
        // 描画前初期化
        this->encoder->setup_viewports();
        this->encoder->setup_views();

        // 各パイプラインに応じた描画コマンド実行
        for (const auto& pass : this->render_passes) {
            for (const auto& command : pass->get_commands()) {
                switch (command.handle.type) {
                    case types::RenderHandleType::Buffer: {
                        this->encoder->submit_command_buffer(command);
                    } break;
                    case types::RenderHandleType::Shader: {
                        this->encoder->submit_command_shader(command);
                    } break;
                    case types::RenderHandleType::Mesh: {
                        this->encoder->submit_command_mesh(command);
                    } break;
                    case types::RenderHandleType::View: {
                        this->encoder->submit_command_view(command, pass->get_render_target());
                    } break;
                    case types::RenderHandleType::ViewPort: {
                        this->encoder->submit_command_viewport(command);
                    } break;
                    case types::RenderHandleType::State: {
                        this->encoder->submit_command_state(command);
                    } break;
                    case types::RenderHandleType::Image: {
                        this->encoder->submit_command_image(command);
                    } break;
                    case types::RenderHandleType::Topology: {
                        this->encoder->submit_command_topology(command);
                    } break;
                    case types::RenderHandleType::VertexLayout: {
                        this->encoder->submit_command_vertex_layout(command);
                    } break;
                    case types::RenderHandleType::Draw: {
                        this->encoder->draw(command.handle);
                    } break;
                    default:
                        break;
                }
            }
        }
    }

    void RenderSystem::present(void) const {
        this->encoder->present();
    }

    std::shared_ptr<platform::IRenderer> RenderSystem::get_renderer(void) const {
        return this->renderer;
    }
} // namespace enishi::core