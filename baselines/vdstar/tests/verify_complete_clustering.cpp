#include "graph/Graph.h"
#include <algorithm>
#include <iostream>
#include <vector>

static void add_edge(dynscan::Vertex *u, dynscan::Vertex *v, float similarity) {
    u->insertNeighbor(v->id, dynscan::Vertex::allocate_intersection_cnt_index(),
                      similarity);
    v->insertNeighbor(u->id, dynscan::Vertex::allocate_intersection_cnt_index(),
                      similarity);
}

static void add_clique(dynscan::Vertex **vertices, int first, int last) {
    for (int u = first; u <= last; ++u) {
        for (int v = u + 1; v <= last; ++v) {
            add_edge(vertices[u], vertices[v], 0.90f);
        }
    }
}

int main() {
    constexpr int n = 12;
    dynscan::Vertex *by_id[n + 1] = {};
    MyVector<dynscan::Vertex *> input;
    for (int id = 1; id <= n; ++id) {
        by_id[id] = new dynscan::Vertex(id);
        input.push_back(by_id[id]);
    }

    add_clique(by_id, 1, 4);
    add_clique(by_id, 5, 8);
    add_edge(by_id[9], by_id[1], 0.90f);
    add_edge(by_id[10], by_id[2], 0.90f);
    add_edge(by_id[10], by_id[5], 0.90f);
    add_edge(by_id[11], by_id[3], 0.10f);
    add_edge(by_id[11], by_id[6], 0.10f);
    add_edge(by_id[12], by_id[4], 0.10f);

    Graph graph(input, 0.02);
    const ClusteringResult result = graph.materializeClustering(0.50, 3);

    bool pass = result.core_components ==
                std::vector<std::vector<int>>{{1, 2, 3, 4}, {5, 6, 7, 8}};
    for (int id = 1; id <= 8; ++id) {
        pass = pass && result.core_flags[id] == 1 &&
               result.roles[id] == VertexRole::Core;
    }
    for (int id = 9; id <= 12; ++id) {
        pass = pass && result.core_flags[id] == 0;
    }
    pass = pass && result.memberships[9] == std::vector<int>{0};
    pass = pass && result.memberships[10] == std::vector<int>({0, 1});
    pass = pass && result.memberships[11].empty();
    pass = pass && result.memberships[12].empty();
    pass = pass && result.roles[9] == VertexRole::Border;
    pass = pass && result.roles[10] == VertexRole::Border;
    pass = pass && result.roles[11] == VertexRole::Hub;
    pass = pass && result.roles[12] == VertexRole::Outlier;

    int similar_count = 0;
    for (const SimilarEdgeLabel &edge : result.similar_edge_labels) {
        similar_count += edge.similar ? 1 : 0;
    }
    pass = pass && result.similar_edge_labels.size() == 18;
    pass = pass && similar_count == 15;

    std::cout << "core_components=" << result.core_components.size() << "\n";
    std::cout << "overlap_memberships=" << result.memberships[10].size() << "\n";
    std::cout << "similar_edges=" << similar_count << "/"
              << result.similar_edge_labels.size() << "\n";
    std::cout << "complete_clustering_verification="
              << (pass ? "PASS" : "FAIL") << "\n";
    return pass ? 0 : 1;
}
