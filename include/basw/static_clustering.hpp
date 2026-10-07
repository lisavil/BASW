#pragma once

#include "basw/static_core.hpp"

#include <string_view>
#include <vector>

namespace basw {

enum class VertexRole {
    Core,
    Border,
    Hub,
    Outlier,
};

struct StaticClusteringResult {
    StaticCoreResult core_state;
    std::vector<std::vector<std::int64_t>> memberships;
    std::vector<VertexRole> roles;
};

[[nodiscard]] StaticClusteringResult recompute_static_clustering(
    const Graph& graph,
    const RationalThreshold& epsilon,
    std::uint64_t mu);

[[nodiscard]] std::string_view role_name(VertexRole role) noexcept;

}  // namespace basw
