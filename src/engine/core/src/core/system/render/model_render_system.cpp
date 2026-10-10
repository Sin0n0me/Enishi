#include "model_render_system.h"
#include "last_model_cache.h"
#include <component/physics_bodies_component.h>
#include <core/system/skinning/model_pose_initializer.h>
#include <filesystem>
#include <foundation/log/logger.h>
#include <utility>
#include <vector>

namespace enishi::core {
    namespace {
        const std::filesystem::path MODEL_SEARCH_PATH = "./assets/models";
        const std::filesystem::path LAST_MODEL_CACHE_PATH = "./cache/last_model.txt";
    } // namespace

    ModelRenderSystem::ModelRenderSystem(std::shared_ptr<ecs::Registry> registry,
        std::shared_ptr<platform::IAssetSystem> asset_system,
        std::shared_ptr<platform::IRenderer> renderer,
        std::shared_ptr<platform::IRenderPass> model_render_pass)
        : model_render_pass(model_render_pass)
        , registry(std::move(registry))
        , model_controller(std::make_shared<model_controller::ModelController>(asset_system,
              std::make_shared<model_controller::ModelRenderDataBuilder>(renderer, asset_system)))
        , is_initial_models_registered(false) {
    }

    bool ModelRenderSystem::should_close(void) {
        return false;
    }

    void ModelRenderSystem::pre_update(void) {
        if (this->is_initial_models_registered) {
            return;
        }

        this->is_initial_models_registered = true;
        this->model_controller->find_model(MODEL_SEARCH_PATH);
        const auto model_names = this->model_controller->get_model_list();
        if (model_names.empty()) {
            foundation::Logger::warning("描画するモデルが見つかりません");
            return;
        }

        std::string cached_id;
        const auto cached = load_last_model(LAST_MODEL_CACHE_PATH);
        if (cached.is_err()) {
            foundation::Logger::warning(cached.unwrap_err().get_message());
        } else if (cached.unwrap().is_some()) {
            cached_id = cached.unwrap().unwrap();
            const auto restored = this->show_model(cached_id);
            if (restored.is_ok()) {
                return;
            }
            foundation::Logger::warning(restored.unwrap_err().get_message());
        }
        for (const auto& model_name : model_names) {
            if (model_name == cached_id) {
                continue;
            }
            const auto result = this->show_model(model_name);
            if (result.is_ok()) {
                break;
            }
            foundation::Logger::error(result.unwrap_err().get_message());
        }
    }

    std::vector<sub_system::ModelEntry> ModelRenderSystem::get_models(void) const {
        std::vector<sub_system::ModelEntry> entries;
        for (const auto& id : this->model_controller->get_model_list()) {
            const auto path = std::filesystem::path(std::u8string(id.begin(), id.end()));
            const auto filename = path.filename().u8string();
            const auto loaded = this->loaded_models.find(id);
            entries.push_back(sub_system::ModelEntry{
                .id = id,
                .filename = std::string(filename.begin(), filename.end()),
                .visible = loaded != this->loaded_models.end() && loaded->second.visible,
            });
        }
        return entries;
    }

    foundation::VoidResult<sub_system::ModelSelectionError> ModelRenderSystem::show_model(
        const std::string& id) {
        const auto loaded = this->loaded_models.find(id);
        if (loaded != this->loaded_models.end()) {
            if (!loaded->second.visible) {
                this->model_render_pass->add_mesh(loaded->second.mesh);
                loaded->second.visible = true;
                auto bodies =
                    this->registry->get<component::PhysicsBodiesComponent>(loaded->second.entity);
                if (bodies.is_some()) {
                    bodies.unwrap_mut().enabled = true;
                }
            }
            this->remember_model(id);
            return {};
        }
        const auto reflections = this->model_render_pass->get_shader_reflections();
        const auto reflection_list =
            std::vector<types::RenderHandle>{reflections.begin(), reflections.end()};
        auto result = this->model_controller->make_model(id, reflection_list);
        if (result.is_err()) {
            return result.propagation(sub_system::ModelSelectionError::LoadFailed);
        }
        const auto entity = this->registry->create();
        auto inserted = this->registry->insert(entity, std::move(result.unwrap_mut()));
        if (inserted.is_err()) {
            this->registry->destroy(entity);
            return inserted.propagation(sub_system::ModelSelectionError::RegistrationFailed);
        }
        const auto pose_result =
            initialize_model_pose(*this->registry, entity, inserted.unwrap_mut());
        if (pose_result.is_err()) {
            this->registry->destroy(entity);
            return pose_result.propagation(sub_system::ModelSelectionError::RegistrationFailed);
        }
        const auto mesh = inserted.unwrap_mut().render_handle;
        this->loaded_models.emplace(id, LoadedModel{entity, mesh, true});
        this->model_render_pass->add_mesh(mesh);
        this->remember_model(id);
        return {};
    }

    void ModelRenderSystem::remember_model(const std::string& id) const {
        const auto result = save_last_model(LAST_MODEL_CACHE_PATH, id);
        if (result.is_err()) {
            foundation::Logger::warning(result.unwrap_err().get_message());
        }
    }

    void ModelRenderSystem::hide_model(const std::string& id) {
        const auto loaded = this->loaded_models.find(id);
        if (loaded == this->loaded_models.end() || !loaded->second.visible) {
            return;
        }
        this->model_render_pass->remove_mesh(loaded->second.mesh);
        loaded->second.visible = false;
        auto bodies =
            this->registry->get<component::PhysicsBodiesComponent>(loaded->second.entity);
        if (bodies.is_some()) {
            bodies.unwrap_mut().enabled = false;
        }
    }

    void ModelRenderSystem::update(const types::DeltaTime& delta_time) {
    }

    void ModelRenderSystem::post_update(void) {
    }

    void ModelRenderSystem::render(void) const {
    }
} // namespace enishi::core
