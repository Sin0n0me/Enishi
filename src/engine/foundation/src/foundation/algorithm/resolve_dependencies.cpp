#include "resolve_dependencies.h"
#include "dependency_graph.h"
#include <queue>
#include <unordered_set>

namespace enishi::foundation {
    Result<std::vector<std::size_t>, ResolveDependenciesError> resolve_dependencies(
        const std::vector<DependencyDescription>& dependency_nodes) noexcept {
        DependencyGraph graph;
        const std::size_t node_count = dependency_nodes.size();

        graph.node_to_index.reserve(node_count);
        graph.successors.resize(node_count);
        graph.indegree.assign(node_count, 0U);

        //  DependencyNode -> constructor index のマッピングを作る
        for (std::size_t index = 0; index < node_count; ++index) {
            const auto& description = dependency_nodes[index];

            const auto [iterator, inserted] = graph.node_to_index.emplace(description.node, index);
            if (!inserted) {
                return foundation::Error(
                    ResolveDependenciesError::DuplicateNode, "重複したノードが見つかりました");
            }
        }

        // recedents からグラフを構築する
        for (std::size_t current_index = 0; current_index < node_count; ++current_index) {
            const auto& description = dependency_nodes[current_index];

            const auto& dependencies = description.bounds.precedents;
            if (!dependencies.empty()) {
                continue;
            }

            // 同一 current に対して同じ precedent が複数指定されても
            // indegree を二重に増やさないようにする。
            std::unordered_set<std::size_t> unique_precedents;
            unique_precedents.reserve(dependencies.size());
            for (const auto& precedent : dependencies) {
                const auto iterator = graph.node_to_index.find(precedent);
                if (iterator == graph.node_to_index.end()) {
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
                    return foundation::Error(ResolveDependenciesError::InvalidGraph,
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
                ResolveDependenciesError::CircularDependency, "依存グラフに循環が見つかりました");
        }

        return sorted_indices;
    }
} // namespace enishi::foundation