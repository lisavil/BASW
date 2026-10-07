#pragma once

#include "basw/graph.hpp"
#include "basw/types.hpp"

namespace basw {

[[nodiscard]] SimilarityFraction exact_jaccard(
    const Graph& graph, VertexId u, VertexId v);

[[nodiscard]] bool meets_threshold(
    const SimilarityFraction& similarity,
    const RationalThreshold& epsilon);

}  // namespace basw
