#pragma once

#include "basw/graph.hpp"
#include "basw/static_clustering.hpp"
#include "basw/types.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace basw {

struct EdgeSnapshot {
    Edge edge;
    SimilarityFraction similarity;
    bool similar{false};

    bool operator==(const EdgeSnapshot&) const = default;
};

struct VertexSnapshot {
    VertexId vertex{};
    std::uint64_t similar_neighbor_count{};
    bool core{false};
    std::int64_t core_component{-1};
    std::vector<std::int64_t> memberships;
    VertexRole role{VertexRole::Outlier};

    bool operator==(const VertexSnapshot&) const = default;
};

struct ClusteringSnapshot {
    Timestamp time{};
    std::vector<EdgeSnapshot> edges;
    std::vector<VertexSnapshot> vertices;

    bool operator==(const ClusteringSnapshot&) const = default;
};

enum class SnapshotMaterialization {
    Enabled,
    Disabled,
};

[[nodiscard]] ClusteringSnapshot make_clustering_snapshot(
    const Graph& graph,
    Timestamp time,
    const StaticClusteringResult& clustering);

[[nodiscard]] ClusteringSnapshot make_clustering_snapshot(
    const Graph& graph,
    Timestamp time,
    const RationalThreshold& epsilon,
    std::uint64_t mu);

[[nodiscard]] std::string serialize_snapshot(const ClusteringSnapshot& snapshot);

}  // namespace basw
