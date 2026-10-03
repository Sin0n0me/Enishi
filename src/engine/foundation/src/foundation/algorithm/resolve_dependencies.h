#pragma once
#include "dependency_bounds.h"
#include <cstdint>
#include <foundation/result/result.h>

namespace enishi::foundation {
    enum class ResolveDependenciesError {
        DuplicateNode,
        InvalidGraph,
        CircularDependency,
    };

    [[nodiscard]] Result<std::vector<std::size_t>, ResolveDependenciesError> resolve_dependencies(
        const std::vector<DependencyDescription>& dependency_nodes) noexcept;
} // namespace enishi::foundation