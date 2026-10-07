#pragma once

#include "basw/clustering_snapshot.hpp"
#include "basw/graph.hpp"
#include "basw/static_core.hpp"
#include "basw/types.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace basw {

enum class MaintenanceMode {
    Local,
    GlobalRepair,
    GlobalRole,
    GlobalRepairRole,
};

enum class MaintenanceTimingMode {
    Instrumented,
    Uninstrumented,
};

[[nodiscard]] const char* maintenance_mode_name(
    MaintenanceMode mode) noexcept;
[[nodiscard]] MaintenanceMode parse_maintenance_mode(
    std::string_view text);

struct BaswUpdateStats {
    std::size_t touched_vertices{};
    std::size_t affected_pairs{};
    std::size_t changed_similarity_edges{};
    std::size_t core_promotions{};
    std::size_t core_demotions{};
    std::size_t dirty_components{};
    std::size_t repair_vertices{};
    std::size_t repair_edges{};
    std::size_t repair_adjacency_checks{};
    std::size_t role_boundary_vertices{};
    std::size_t role_adjacency_checks{};
    std::uint64_t affected_discovery_ns{};
    std::uint64_t similarity_ns{};
    std::uint64_t core_ns{};
    std::uint64_t repair_ns{};
    std::uint64_t role_ns{};

    [[nodiscard]] std::uint64_t measured_phase_ns() const noexcept {
        return affected_discovery_ns + similarity_ns + core_ns + repair_ns +
               role_ns;
    }
};

class BaswClusteringState {
public:
    BaswClusteringState(
        Graph graph,
        RationalThreshold epsilon,
        std::uint64_t mu,
        MaintenanceMode maintenance_mode = MaintenanceMode::Local,
        bool allow_all_dirty_global_fallback = true,
        MaintenanceTimingMode timing_mode =
            MaintenanceTimingMode::Instrumented);

    [[nodiscard]] const Graph& graph() const noexcept;
    [[nodiscard]] ClusteringSnapshot snapshot(Timestamp time) const;

    BaswUpdateStats apply_transition(
        const std::vector<Edge>& deletions,
        const std::vector<Edge>& insertions);
    // Executes exact local maintenance for exactly one current toggle. It cannot
    // observe or coalesce a later toggle in the enclosing slide.
    BaswUpdateStats apply_single_toggle(const Edge& edge, bool insertion);

private:
    BaswUpdateStats apply_changes(
        std::span<const Edge> deletions,
        std::span<const Edge> insertions);
    void repair_components(
        const std::unordered_set<std::int64_t>& dirty_components,
        const std::vector<VertexId>& promoted_vertices,
        const std::vector<Edge>& new_core_connection_candidates,
        BaswUpdateStats& stats,
        std::unordered_set<VertexId>& component_changed_vertices);
    void rebuild_components_globally(
        BaswUpdateStats& stats,
        std::unordered_set<VertexId>& component_changed_vertices);
    void rebuild_component_membership_index();
    void refresh_roles(
        const std::unordered_set<VertexId>& role_seed_vertices,
        const std::unordered_set<VertexId>& core_influence_vertices,
        BaswUpdateStats& stats);
    void rebuild_roles_globally(BaswUpdateStats& stats);

    Graph graph_;
    RationalThreshold epsilon_;
    std::uint64_t mu_{};
    MaintenanceMode maintenance_mode_{MaintenanceMode::Local};
    bool allow_all_dirty_global_fallback_{true};
    MaintenanceTimingMode timing_mode_{MaintenanceTimingMode::Instrumented};
    std::unordered_map<Edge, EdgeSimilarityState, EdgeHash> edge_states_;
    std::vector<std::uint64_t> similar_neighbor_count_;
    std::vector<bool> is_core_;
    std::vector<std::int64_t> core_component_;
    std::vector<std::vector<VertexId>> component_vertices_;
    std::size_t component_count_{};
    std::vector<std::vector<std::int64_t>> memberships_;
    std::vector<VertexRole> roles_;
};

}  // namespace basw
