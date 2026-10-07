#include "basw/static_window_runner.hpp"

#include <chrono>
#include <utility>

namespace basw {

StaticWindowRunner::StaticWindowRunner(
    std::size_t vertex_count,
    std::vector<TemporalEvent> events,
    Timestamp window_width,
    Timestamp slide_interval,
    Timestamp initial_time,
    RationalThreshold epsilon,
    std::uint64_t mu,
    SnapshotMaterialization snapshot_materialization)
    : active_window_(
          vertex_count,
          std::move(events),
          window_width,
          slide_interval,
          initial_time),
      epsilon_(epsilon),
      mu_(mu),
      snapshot_materialization_(snapshot_materialization) {
    current_state_ =
        recompute_static_clustering(active_window_.graph(), epsilon_, mu_);
}

ClusteringSnapshot StaticWindowRunner::current_snapshot() const {
    return make_clustering_snapshot(
        active_window_.graph(), active_window_.current_time(), current_state_);
}

bool StaticWindowRunner::has_pending_events() const noexcept {
    return active_window_.has_pending_events();
}

StaticWindowStep StaticWindowRunner::advance() {
    const auto transition_start = std::chrono::steady_clock::now();
    WindowTransitionResult transition = active_window_.advance();
    const auto transition_end = std::chrono::steady_clock::now();
    // STATIC deliberately discards the prior window result and recomputes the
    // complete clustering from the final active graph. Clearing the prior state
    // is timed so neither its teardown nor a second retained generation is hidden
    // from latency and peak-memory measurements.
    current_state_ = {};
    current_state_ =
        recompute_static_clustering(active_window_.graph(), epsilon_, mu_);
    const auto static_end = std::chrono::steady_clock::now();
    ClusteringSnapshot snapshot;
    std::uint64_t snapshot_materialization_ns = 0;
    if (snapshot_materialization_ == SnapshotMaterialization::Enabled) {
        const auto snapshot_start = std::chrono::steady_clock::now();
        snapshot = current_snapshot();
        const auto snapshot_end = std::chrono::steady_clock::now();
        snapshot_materialization_ns = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                snapshot_end - snapshot_start).count());
    }
    const auto transition_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        transition_end - transition_start).count();
    const auto static_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        static_end - transition_end).count();
    return {
        std::move(transition),
        std::move(snapshot),
        static_cast<std::uint64_t>(transition_ns),
        static_cast<std::uint64_t>(static_ns),
        snapshot_materialization_ns,
    };
}

}  // namespace basw
