#pragma once
#include <filesystem>
#include <foundation/option/option.h>
#include <foundation/result/result.h>
#include <string>

namespace enishi::core {
    enum class ModelCacheError { ReadFailed, WriteFailed };

    [[nodiscard]] foundation::Result<foundation::Option<std::string>, ModelCacheError>
    load_last_model(const std::filesystem::path& cache_path);

    [[nodiscard]] foundation::VoidResult<ModelCacheError> save_last_model(
        const std::filesystem::path& cache_path, const std::string& id);
} // namespace enishi::core
