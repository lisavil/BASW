#include "basw/static_core.hpp"

#include "basw/jaccard.hpp"

#include <algorithm>
#include <limits>
#include <queue>
#include <stdexcept>

namespace basw {

StaticCoreResult recompute_static_core(
    const Graph& graph,
    const RationalThreshold& epsilon,
    std::uint64_t mu) {
    if (mu == 0) {
        throw std::invalid_argument("mu must be at least one");
    }

    StaticCoreResult result;
    result.similar_neighbor_count.assign(graph.vertex_count(), 0);
    result.is_core.assign(graph.vertex_count(), false);
    result.core_component.assign(graph.vertex_count(), -1);
    result.edge_states.reserve(graph.edge_count());

    for (const Edge& edge : graph.edges()) {
        const SimilarityFraction fraction = exact_jaccard(graph, edge.u, edge.v);
        const bool similar = meets_threshold(fraction, epsilon);
        result.edge_states.emplace(edge, EdgeSimilarityState{fraction, similar});
        if (similar) {
            ++result.similar_neighbor_count[edge.u];
            ++result.similar_neighbor_count[edge.v];
        }
    }

    for (VertexId vertex = 0; vertex < graph.vertex_count(); ++vertex) {
        result.is_core[vertex] = result.similar_neighbor_count[vertex] >= mu;
    }

    std::vector<bool> visited(graph.vertex_count(), false);
    for (VertexId start = 0; start < graph.vertex_count(); ++start) {
        if (!result.is_core[start] || visited[start]) {
            continue;
        }

        std::queue<VertexId> frontier;
        std::vector<VertexId> component_vertices;
        VertexId minimum_vertex = std::numeric_limits<VertexId>::max();
        visited[start] = true;
        frontier.push(start);

        while (!frontier.empty()) {
            const VertexId u = frontier.front();
            frontier.pop();
            component_vertices.push_back(u);
            minimum_vertex = std::min(minimum_vertex, u);

            for (const VertexId v : graph.neighbors(u)) {
                if (!result.is_core[v] || visited[v]) {
                    continue;
                }
                const auto state = result.edge_states.find(Edge(u, v));
                if (state != result.edge_states.end() && state->second.similar) {
                    visited[v] = true;
                    frontier.push(v);
                }
            }
        }

        for (const VertexId vertex : component_vertices) {
            result.core_component[vertex] = minimum_vertex;
        }
    }

    return result;
}

}  // namespace basw
