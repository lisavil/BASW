#include "basw/jaccard.hpp"

#include <algorithm>
#include <stdexcept>

namespace basw {

SimilarityFraction exact_jaccard(const Graph& graph, VertexId u, VertexId v) {
    if (!graph.has_edge(u, v)) {
        throw std::invalid_argument("structural similarity is defined only for active edges");
    }

    const auto& u_neighbors = graph.neighbors(u);
    const auto& v_neighbors = graph.neighbors(v);
    const auto* smaller = &u_neighbors;
    const auto* larger = &v_neighbors;
    if (smaller->size() > larger->size()) {
        std::swap(smaller, larger);
    }

    std::uint64_t common_open_neighbors = 0;
    for (const VertexId candidate : *smaller) {
        if (candidate != u && candidate != v && larger->contains(candidate)) {
            ++common_open_neighbors;
        }
    }

    const std::uint64_t intersection = common_open_neighbors + 2;
    const std::uint64_t union_size =
        graph.degree(u) + graph.degree(v) + 2 - intersection;
    return {intersection, union_size};
}

bool meets_threshold(
    const SimilarityFraction& similarity,
    const RationalThreshold& epsilon) {
    if (similarity.union_size == 0) {
        throw std::invalid_argument("Jaccard union cannot be empty");
    }

    // The graph size in the intended experiments is far below the range where
    // these products can overflow uint64_t. A wider integer can be substituted
    // later if the supported input limits are increased.
    return similarity.intersection * epsilon.denominator >=
           epsilon.numerator * similarity.union_size;
}

}  // namespace basw
