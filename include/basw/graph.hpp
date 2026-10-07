#pragma once

#include "basw/types.hpp"

#include <cstddef>
#include <unordered_set>
#include <vector>

namespace basw {

class Graph {
public:
    explicit Graph(std::size_t vertex_count);

    [[nodiscard]] std::size_t vertex_count() const noexcept;
    [[nodiscard]] std::size_t edge_count() const noexcept;
    [[nodiscard]] std::size_t degree(VertexId vertex) const;
    [[nodiscard]] bool has_edge(VertexId u, VertexId v) const;
    [[nodiscard]] const std::unordered_set<VertexId>& neighbors(VertexId vertex) const;
    [[nodiscard]] std::vector<Edge> edges() const;

    bool add_edge(VertexId u, VertexId v);
    bool remove_edge(VertexId u, VertexId v);

private:
    void check_vertex(VertexId vertex) const;
    void check_edge_endpoints(VertexId u, VertexId v) const;

    std::vector<std::unordered_set<VertexId>> adjacency_;
    std::size_t edge_count_{0};
};

}  // namespace basw
