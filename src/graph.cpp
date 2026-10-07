#include "basw/graph.hpp"

#include <stdexcept>

namespace basw {

Graph::Graph(std::size_t vertex_count) : adjacency_(vertex_count) {}

std::size_t Graph::vertex_count() const noexcept {
    return adjacency_.size();
}

std::size_t Graph::edge_count() const noexcept {
    return edge_count_;
}

void Graph::check_vertex(VertexId vertex) const {
    if (vertex >= adjacency_.size()) {
        throw std::out_of_range("vertex id is outside the graph");
    }
}

void Graph::check_edge_endpoints(VertexId u, VertexId v) const {
    check_vertex(u);
    check_vertex(v);
    if (u == v) {
        throw std::invalid_argument("self-loops are not supported");
    }
}

std::size_t Graph::degree(VertexId vertex) const {
    check_vertex(vertex);
    return adjacency_[vertex].size();
}

bool Graph::has_edge(VertexId u, VertexId v) const {
    check_edge_endpoints(u, v);
    return adjacency_[u].contains(v);
}

const std::unordered_set<VertexId>& Graph::neighbors(VertexId vertex) const {
    check_vertex(vertex);
    return adjacency_[vertex];
}

std::vector<Edge> Graph::edges() const {
    std::vector<Edge> result;
    result.reserve(edge_count_);
    for (VertexId u = 0; u < adjacency_.size(); ++u) {
        for (const VertexId v : adjacency_[u]) {
            if (u < v) {
                result.emplace_back(u, v);
            }
        }
    }
    return result;
}

bool Graph::add_edge(VertexId u, VertexId v) {
    check_edge_endpoints(u, v);
    if (adjacency_[u].contains(v)) {
        return false;
    }
    adjacency_[u].insert(v);
    adjacency_[v].insert(u);
    ++edge_count_;
    return true;
}

bool Graph::remove_edge(VertexId u, VertexId v) {
    check_edge_endpoints(u, v);
    if (!adjacency_[u].contains(v)) {
        return false;
    }
    adjacency_[u].erase(v);
    adjacency_[v].erase(u);
    --edge_count_;
    return true;
}

}  // namespace basw
