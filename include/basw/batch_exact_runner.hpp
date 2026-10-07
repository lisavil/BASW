#pragma once

#include "basw/active_edge_index.hpp"
#include "basw/batch_exact_clustering_state.hpp"
#include "basw/clustering_snapshot.hpp"
#include "basw/window_engine.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace basw {

struct BatchExactStep {
    WindowBatch event_batch;
    EffectiveTopologyBatch topology_batch;
    ClusteringSnapshot final_snapshot;
    BatchUpdateStats batch_work;
    std::uint64_t transition_ns{};
    std::uint64_t batch_update_ns{};
    std::uint64_t snapshot_materialization_ns{};
};

class BatchExactRunner {
public:
    BatchExactRunner(
        std::size_t vertex_count,
        std::vector<TemporalEvent> events,
        Timestamp window_width,
        Timestamp slide_interval,
        Timestamp initial_time,
        RationalThreshold epsilon,
        std::uint64_t mu,
        BatchMaintenanceMode maintenance_mode = BatchMaintenanceMode::Local,
        SnapshotMaterialization snapshot_materialization =
            SnapshotMaterialization::Enabled,
        MaintenanceTimingMode timing_mode =
            MaintenanceTimingMode::Instrumented);

    [[nodiscard]] ClusteringSnapshot current_snapshot() const;
    [[nodiscard]] bool has_pending_events() const noexcept;
    BatchExactStep advance();

private:
    static std::vector<TemporalEvent> validate_events(
        std::size_t vertex_count,
        std::vector<TemporalEvent> events);
    static Graph build_initial_graph(
        std::size_t vertex_count,
        const std::vector<TemporalEvent>& initial_events);

    WindowEngine window_engine_;
    ActiveEdgeIndex edge_index_;
    BatchExactClusteringState clustering_state_;
    SnapshotMaterialization snapshot_materialization_;
};

}  // namespace basw
