#include "basw/clustering_snapshot.hpp"

#include <algorithm>
#include <sstream>

namespace basw {
namespace {

bool edge_snapshot_less(
    const EdgeSnapshot& left, const EdgeSnapshot& right) noexcept {
    return left.edge.u < right.edge.u ||
           (left.edge.u == right.edge.u && left.edge.v < right.edge.v);
}

}  // namespace

ClusteringSnapshot make_clustering_snapshot(
    const Graph& graph,
    Timestamp time,
    const StaticClusteringResult& clustering) {
    ClusteringSnapshot snapshot;
    snapshot.time = time;
    snapshot.edges.reserve(clustering.core_state.edge_states.size());
    for (const auto& [edge, state] : clustering.core_state.edge_states) {
        snapshot.edges.push_back({edge, state.fraction, state.similar});
    }
    std::sort(snapshot.edges.begin(), snapshot.edges.end(), edge_snapshot_less);

    snapshot.vertices.reserve(graph.vertex_count());
    for (VertexId vertex = 0; vertex < graph.vertex_count(); ++vertex) {
        snapshot.vertices.push_back({
            vertex,
            clustering.core_state.similar_neighbor_count[vertex],
            clustering.core_state.is_core[vertex],
            clustering.core_state.core_component[vertex],
            clustering.memberships[vertex],
            clustering.roles[vertex],
        });
    }
    return snapshot;
}

ClusteringSnapshot make_clustering_snapshot(
    const Graph& graph,
    Timestamp time,
    const RationalThreshold& epsilon,
    std::uint64_t mu) {
    const StaticClusteringResult clustering =
        recompute_static_clustering(graph, epsilon, mu);
    return make_clustering_snapshot(graph, time, clustering);
}

std::string serialize_snapshot(const ClusteringSnapshot& snapshot) {
    std::ostringstream output;
    output << "time\t" << snapshot.time << '\n';
    output << "[edges]\n";
    for (const EdgeSnapshot& edge : snapshot.edges) {
        output << edge.edge.u << '\t'
               << edge.edge.v << '\t'
               << edge.similarity.intersection << '\t'
               << edge.similarity.union_size << '\t'
               << (edge.similar ? 1 : 0) << '\n';
    }

    output << "[vertices]\n";
    for (const VertexSnapshot& vertex : snapshot.vertices) {
        output << vertex.vertex << '\t'
               << vertex.similar_neighbor_count << '\t'
               << (vertex.core ? 1 : 0) << '\t'
               << vertex.core_component << '\t'
               << role_name(vertex.role) << '\t';
        if (vertex.memberships.empty()) {
            output << '-';
        } else {
            for (std::size_t index = 0; index < vertex.memberships.size(); ++index) {
                if (index > 0) {
                    output << ',';
                }
                output << vertex.memberships[index];
            }
        }
        output << '\n';
    }
    return output.str();
}

}  // namespace basw
