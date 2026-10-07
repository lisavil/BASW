#include "basw/static_clustering.hpp"

#include <set>
#include <stdexcept>
#include <utility>

namespace basw {

StaticClusteringResult recompute_static_clustering(
    const Graph& graph,
    const RationalThreshold& epsilon,
    std::uint64_t mu) {
    StaticClusteringResult result;
    result.core_state = recompute_static_core(graph, epsilon, mu);
    result.memberships.resize(graph.vertex_count());
    result.roles.assign(graph.vertex_count(), VertexRole::Outlier);

    // Core memberships are known directly. Non-core memberships depend only
    // on epsilon-similar edges to core vertices and can therefore be computed
    // before hub/outlier classification.
    for (VertexId vertex = 0; vertex < graph.vertex_count(); ++vertex) {
        if (result.core_state.is_core[vertex]) {
            result.memberships[vertex].push_back(
                result.core_state.core_component[vertex]);
            result.roles[vertex] = VertexRole::Core;
            continue;
        }

        std::set<std::int64_t> adjacent_core_components;
        for (const VertexId neighbor : graph.neighbors(vertex)) {
            if (!result.core_state.is_core[neighbor]) {
                continue;
            }
            const auto state = result.core_state.edge_states.find(Edge(vertex, neighbor));
            if (state != result.core_state.edge_states.end() && state->second.similar) {
                adjacent_core_components.insert(
                    result.core_state.core_component[neighbor]);
            }
        }

        result.memberships[vertex].assign(
            adjacent_core_components.begin(), adjacent_core_components.end());
        if (!result.memberships[vertex].empty()) {
            result.roles[vertex] = VertexRole::Border;
        }
    }

    // A vertex with no membership is a hub when its ordinary neighbors touch
    // at least two clusters. Neighbor memberships include both core and border
    // vertices and have already been fully materialized above.
    for (VertexId vertex = 0; vertex < graph.vertex_count(); ++vertex) {
        if (result.core_state.is_core[vertex] || !result.memberships[vertex].empty()) {
            continue;
        }

        std::set<std::int64_t> neighboring_components;
        for (const VertexId neighbor : graph.neighbors(vertex)) {
            for (const std::int64_t component : result.memberships[neighbor]) {
                neighboring_components.insert(component);
                if (neighboring_components.size() >= 2) {
                    break;
                }
            }
            if (neighboring_components.size() >= 2) {
                break;
            }
        }
        result.roles[vertex] = neighboring_components.size() >= 2
                                   ? VertexRole::Hub
                                   : VertexRole::Outlier;
    }

    return result;
}

std::string_view role_name(VertexRole role) noexcept {
    switch (role) {
        case VertexRole::Core:
            return "core";
        case VertexRole::Border:
            return "border";
        case VertexRole::Hub:
            return "hub";
        case VertexRole::Outlier:
            return "outlier";
    }
    return "unknown";
}

}  // namespace basw
