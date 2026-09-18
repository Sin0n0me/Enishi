#pragma once
#include "dependency_bounds.h"
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace enishi::foundation {
    struct DependencyGraph {
        std::unordered_map<DependencyNode, std::size_t> node_to_index;
        std::vector<std::vector<std::size_t>> successors;
        std::vector<std::size_t> indegree;
    };
} // namespace enishi::foundation