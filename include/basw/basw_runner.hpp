#pragma once

#include "basw/active_edge_index.hpp"
#include "basw/basw_clustering_state.hpp"
#include "basw/clustering_snapshot.hpp"
#include "basw/window_engine.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace basw {

struct BaswStep {
    WindowTransition event_transition;
    TopologyChanges topology_changes;
    ClusteringSnapshot final_snapshot;
    BaswUpdateStats work;
    std::uint64_t transition_ns{};
    std::uint64_t update_ns{};
    std::uint64_t snapshot_materialization_ns{};
};

class BaswRunner {
public:
    BaswRunner(
        std::size_t vertex_count,
        std::vector<TemporalEvent> events,
        Timestamp window_width,
        Timestamp slide_interval,
        Timestamp initial_time,
        RationalThreshold epsilon,
        std::uint64_t mu,
        MaintenanceMode maintenance_mode = MaintenanceMode::Local,
        SnapshotMaterialization snapshot_materialization =
            SnapshotMaterialization::Enabled,
        MaintenanceTimingMode timing_mode =
            MaintenanceTimingMode::Instrumented);

    [[nodiscard]] ClusteringSnapshot current_snapshot() const;
    [[nodiscard]] bool has_pending_events() const noexcept;
    BaswStep advance();

private:
    static std::vector<TemporalEvent> validate_events(
        std::size_t vertex_count,
        std::vector<TemporalEvent> events);
    static Graph build_initial_graph(
        std::size_t vertex_count,
        const std::vector<TemporalEvent>& initial_events);

    WindowEngine window_engine_;
    ActiveEdgeIndex edge_index_;
    BaswClusteringState clustering_state_;
    SnapshotMaterialization snapshot_materialization_;
};

}  // namespace basw
