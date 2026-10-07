#pragma once

#include "basw/active_edge_index.hpp"
#include "basw/clustering_snapshot.hpp"
#include "basw/exact_clustering_state.hpp"
#include "basw/window_engine.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace basw {

enum class SequentialSnapshotMode {
    RetainIntermediate,
    FinalOnly,
    None,
};

struct SequentialExactStep {
    WindowTransition event_transition;
    TopologyChanges topology_changes;
    std::vector<ClusteringSnapshot> toggle_snapshots;
    ClusteringSnapshot final_snapshot;
    IncrementalUpdateStats incremental_work;
    std::uint64_t transition_ns{};
    std::uint64_t sequential_update_ns{};
    std::uint64_t snapshot_materialization_ns{};
};

class SequentialExactRunner {
public:
    SequentialExactRunner(
        std::size_t vertex_count,
        std::vector<TemporalEvent> events,
        Timestamp window_width,
        Timestamp slide_interval,
        Timestamp initial_time,
        RationalThreshold epsilon,
        std::uint64_t mu,
        SequentialSnapshotMode snapshot_mode =
            SequentialSnapshotMode::RetainIntermediate,
        MaintenanceTimingMode timing_mode =
            MaintenanceTimingMode::Instrumented);

    [[nodiscard]] ClusteringSnapshot current_snapshot() const;
    [[nodiscard]] bool has_pending_events() const noexcept;
    SequentialExactStep advance();

private:
    static std::vector<TemporalEvent> validate_events(
        std::size_t vertex_count,
        std::vector<TemporalEvent> events);
    static Graph build_initial_graph(
        std::size_t vertex_count,
        const std::vector<TemporalEvent>& initial_events);

    WindowEngine window_engine_;
    ActiveEdgeIndex edge_index_;
    ExactClusteringState clustering_state_;
    SequentialSnapshotMode snapshot_mode_;
};

}  // namespace basw
