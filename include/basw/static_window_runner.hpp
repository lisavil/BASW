#pragma once

#include "basw/active_window_graph.hpp"
#include "basw/clustering_snapshot.hpp"
#include "basw/static_clustering.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace basw {

struct StaticWindowStep {
    WindowTransitionResult transition;
    ClusteringSnapshot snapshot;
    std::uint64_t transition_ns{};
    std::uint64_t static_recompute_ns{};
    std::uint64_t snapshot_materialization_ns{};
};

class StaticWindowRunner {
public:
    StaticWindowRunner(
        std::size_t vertex_count,
        std::vector<TemporalEvent> events,
        Timestamp window_width,
        Timestamp slide_interval,
        Timestamp initial_time,
        RationalThreshold epsilon,
        std::uint64_t mu,
        SnapshotMaterialization snapshot_materialization =
            SnapshotMaterialization::Enabled);

    [[nodiscard]] ClusteringSnapshot current_snapshot() const;
    [[nodiscard]] bool has_pending_events() const noexcept;
    StaticWindowStep advance();

private:
    ActiveWindowGraph active_window_;
    RationalThreshold epsilon_;
    std::uint64_t mu_{};
    StaticClusteringResult current_state_;
    SnapshotMaterialization snapshot_materialization_;
};

}  // namespace basw
