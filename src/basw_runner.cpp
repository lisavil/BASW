#include "basw/basw_runner.hpp"

#include <chrono>
#include <stdexcept>
#include <utility>

namespace basw {

std::vector<TemporalEvent> BaswRunner::validate_events(
    std::size_t vertex_count,
    std::vector<TemporalEvent> events) {
    for (const TemporalEvent& event : events) {
        if (event.edge.u >= vertex_count || event.edge.v >= vertex_count) {
            throw std::out_of_range(
                "temporal event endpoint is outside the configured graph");
        }
    }
    return events;
}

Graph BaswRunner::build_initial_graph(
    std::size_t vertex_count,
    const std::vector<TemporalEvent>& initial_events) {
    Graph graph(vertex_count);
    for (const TemporalEvent& event : initial_events) {
        graph.add_edge(event.edge.u, event.edge.v);
    }
    return graph;
}

BaswRunner::BaswRunner(
    std::size_t vertex_count,
    std::vector<TemporalEvent> events,
    Timestamp window_width,
    Timestamp slide_interval,
    Timestamp initial_time,
    RationalThreshold epsilon,
    std::uint64_t mu,
    MaintenanceMode maintenance_mode,
    SnapshotMaterialization snapshot_materialization,
    MaintenanceTimingMode timing_mode)
    : window_engine_(
          validate_events(vertex_count, std::move(events)),
          window_width,
          slide_interval,
          initial_time),
      edge_index_(window_engine_.initial_events()),
      clustering_state_(
          build_initial_graph(vertex_count, window_engine_.initial_events()),
          epsilon,
          mu,
          maintenance_mode,
          true,
          timing_mode),
      snapshot_materialization_(snapshot_materialization) {}

ClusteringSnapshot BaswRunner::current_snapshot() const {
    return clustering_state_.snapshot(window_engine_.current_time());
}

bool BaswRunner::has_pending_events() const noexcept {
    return window_engine_.has_pending_events();
}

BaswStep BaswRunner::advance() {
    const auto transition_start = std::chrono::steady_clock::now();
    WindowTransition event_transition = window_engine_.advance();
    TopologyChanges topology_changes = edge_index_.apply(event_transition);
    const auto transition_end = std::chrono::steady_clock::now();
    const auto transition_ns =
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            transition_end - transition_start).count();

    const auto update_start = std::chrono::steady_clock::now();
    BaswUpdateStats work = clustering_state_.apply_transition(
        topology_changes.deletions, topology_changes.insertions);
    const auto update_end = std::chrono::steady_clock::now();
    const auto update_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        update_end - update_start).count();
    ClusteringSnapshot final_snapshot;
    std::uint64_t snapshot_materialization_ns = 0;
    if (snapshot_materialization_ == SnapshotMaterialization::Enabled) {
        const auto snapshot_start = std::chrono::steady_clock::now();
        final_snapshot = clustering_state_.snapshot(topology_changes.new_time);
        const auto snapshot_end = std::chrono::steady_clock::now();
        snapshot_materialization_ns = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                snapshot_end - snapshot_start).count());
    }

    return {
        std::move(event_transition),
        std::move(topology_changes),
        std::move(final_snapshot),
        work,
        static_cast<std::uint64_t>(transition_ns),
        static_cast<std::uint64_t>(update_ns),
        snapshot_materialization_ns,
    };
}

}  // namespace basw
