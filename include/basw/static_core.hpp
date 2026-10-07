#pragma once

#include "basw/graph.hpp"
#include "basw/types.hpp"

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace basw {

struct EdgeSimilarityState {
    SimilarityFraction fraction;
    bool similar{false};
};

struct StaticCoreResult {
    std::unordered_map<Edge, EdgeSimilarityState, EdgeHash> edge_states;
    std::vector<std::uint64_t> similar_neighbor_count;
    std::vector<bool> is_core;

    // Non-core vertices have component id -1. Each core component is assigned
    // its minimum vertex id, making results independent of traversal order.
    std::vector<std::int64_t> core_component;
};

[[nodiscard]] StaticCoreResult recompute_static_core(
    const Graph& graph,
    const RationalThreshold& epsilon,
    std::uint64_t mu);

}  // namespace basw
