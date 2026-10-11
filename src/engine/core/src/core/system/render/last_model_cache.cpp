#include "last_model_cache.h"
#include <fstream>
#include <iomanip>

namespace enishi::core {
    foundation::Result<foundation::Option<std::string>, ModelCacheError> load_last_model(
        const std::filesystem::path& cache_path) {
        std::error_code error;
        const bool exists = std::filesystem::exists(cache_path, error);
        if (error.value() != 0) {
            return foundation::Error(ModelCacheError::ReadFailed, error.message());
        }
        if (!exists) {
            return foundation::Option<std::string>{};
        }
        std::ifstream input(cache_path, std::ios::binary);
        std::string id;
        if (!input.is_open() || !(input >> std::quoted(id)) || id.empty()) {
            return foundation::Error(
                ModelCacheError::ReadFailed, "Failed to read the last model cache");
        }
        input >> std::ws;
        if (input.bad() || !input.eof()) {
            return foundation::Error(
                ModelCacheError::ReadFailed, "Invalid last model cache contents");
        }
        return foundation::Option<std::string>{std::move(id)};
    }

    foundation::VoidResult<ModelCacheError> save_last_model(
        const std::filesystem::path& cache_path, const std::string& id) {
        std::error_code error;
        if (!cache_path.parent_path().empty()) {
            std::filesystem::create_directories(cache_path.parent_path(), error);
        }
        if (error.value() != 0) {
            return foundation::Error(ModelCacheError::WriteFailed, error.message());
        }
        std::ofstream output(cache_path, std::ios::binary | std::ios::trunc);
        if (!output.is_open()) {
            return foundation::Error(
                ModelCacheError::WriteFailed, "Failed to open the last model cache for writing");
        }
        output << std::quoted(id) << '\n';
        output.close();
        if (output.fail()) {
            return foundation::Error(
                ModelCacheError::WriteFailed, "Failed to write the last model cache");
        }
        return {};
    }
} // namespace enishi::core
