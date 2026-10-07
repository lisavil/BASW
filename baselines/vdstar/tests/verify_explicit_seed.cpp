#include "graph/Graph.h"

#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace {

std::string serialize(const ClusteringResult& result) {
    std::ostringstream out;
    for (std::size_t id = 1; id < result.core_flags.size(); ++id) {
        out << id << ':' << static_cast<int>(result.core_flags[id]) << ':'
            << result.core_component[id] << ':'
            << static_cast<int>(result.roles[id]) << ':';
        for (const int component : result.memberships[id]) {
            out << component << ',';
        }
        out << ';';
    }
    out << '|';
    for (const SimilarEdgeLabel& edge : result.similar_edge_labels) {
        out << edge.u << ',' << edge.v << ',' << edge.similar << ';';
    }
    return out.str();
}

std::vector<std::string> run(std::uint64_t seed) {
    constexpr int n = 48;
    MyVector<dynscan::Vertex*> vertices;
    for (int id = 1; id <= n; ++id) {
        vertices.push_back(new dynscan::Vertex(id));
    }
    Graph graph(vertices, 0.10, seed);
    std::vector<std::pair<int, int>> active;
    std::vector<std::string> checkpoints;
    for (int u = 1; u <= n; ++u) {
        for (int offset = 1; offset <= 8; ++offset) {
            const int v = 1 + (u - 1 + offset) % n;
            if (u < v) {
                graph.insertEdge(u, v);
                active.emplace_back(u, v);
            }
        }
    }
    checkpoints.push_back(serialize(graph.materializeClustering(0.5, 5)));
    for (std::size_t i = 0; i < active.size(); i += 3) {
        graph.removeEdge(active[i].first, active[i].second);
    }
    checkpoints.push_back(serialize(graph.materializeClustering(0.5, 5)));
    return checkpoints;
}

}  // namespace

int main() {
    const auto first = run(20250915ULL);
    const auto second = run(20250915ULL);
    const bool pass = first == second;
    std::cout << "explicit_seed_replay=" << (pass ? "PASS" : "FAIL")
              << '\n';
    return pass ? 0 : 1;
}
