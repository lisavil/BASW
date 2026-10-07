#include "basw/sequential_exact_runner.hpp"

#include <chrono>
#include <stdexcept>
#include <utility>

namespace basw {

std::vector<TemporalEvent> SequentialExactRunner::validate_events(
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

Graph SequentialExactRunner::build_initial_graph(
    std::size_t vertex_count,
    const std::vector<TemporalEvent>& initial_events) {
    Graph graph(vertex_count);
    for (const TemporalEvent& event : initial_events) {
        graph.add_edge(event.edge.u, event.edge.v);
    }
    return graph;
}

SequentialExactRunner::SequentialExactRunner(
    std::size_t vertex_count,
    std::vector<TemporalEvent> events,
    Timestamp window_width,
    Timestamp slide_interval,
    Timestamp initial_time,
    RationalThreshold epsilon,
    std::uint64_t mu,
    SequentialSnapshotMode snapshot_mode,
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
          timing_mode),
      snapshot_mode_(snapshot_mode) {}

ClusteringSnapshot SequentialExactRunner::current_snapshot() const {
    return clustering_state_.snapshot(window_engine_.current_time());
}

bool SequentialExactRunner::has_pending_events() const noexcept {
    return window_engine_.has_pending_events();
}

SequentialExactStep SequentialExactRunner::advance() {
    const auto transition_start = std::chrono::steady_clock::now();
    WindowBatch event_batch = window_engine_.advance();
    EffectiveTopologyBatch topology_batch = edge_index_.apply(event_batch);
    const auto transition_end = std::chrono::steady_clock::now();
    const auto transition_ns =
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            transition_end - transition_start).count();
    std::vector<ClusteringSnapshot> toggle_snapshots;
    if (snapshot_mode_ == SequentialSnapshotMode::RetainIntermediate) {
        toggle_snapshots.reserve(
            topology_batch.deletions.size() + topology_batch.insertions.size());
    }
    IncrementalUpdateStats incremental_work;

    std::uint64_t update_ns = 0;
    for (const Edge& edge : topology_batch.deletions) {
        const auto update_start = std::chrono::steady_clock::now();
        const IncrementalUpdateStats update =
            clustering_state_.apply_deletion(edge);
        const auto update_end = std::chrono::steady_clock::now();
        update_ns += static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                update_end - update_start).count());
        incremental_work.affected_pairs += update.affected_pairs;
        incremental_work.changed_similarity_edges +=
            update.changed_similarity_edges;
        incremental_work.core_promotions += update.core_promotions;
        incremental_work.core_demotions += update.core_demotions;
        if (snapshot_mode_ == SequentialSnapshotMode::RetainIntermediate) {
            toggle_snapshots.push_back(
                clustering_state_.snapshot(topology_batch.new_time));
        }
    }
    for (const Edge& edge : topology_batch.insertions) {
        const auto update_start = std::chrono::steady_clock::now();
        const IncrementalUpdateStats update =
            clustering_state_.apply_insertion(edge);
        const auto update_end = std::chrono::steady_clock::now();
        update_ns += static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                update_end - update_start).count());
        incremental_work.affected_pairs += update.affected_pairs;
        incremental_work.changed_similarity_edges +=
            update.changed_similarity_edges;
        incremental_work.core_promotions += update.core_promotions;
        incremental_work.core_demotions += update.core_demotions;
        if (snapshot_mode_ == SequentialSnapshotMode::RetainIntermediate) {
            toggle_snapshots.push_back(
                clustering_state_.snapshot(topology_batch.new_time));
        }
    }

    ClusteringSnapshot final_snapshot;
    std::uint64_t snapshot_materialization_ns = 0;
    if (snapshot_mode_ != SequentialSnapshotMode::None) {
        const auto snapshot_start = std::chrono::steady_clock::now();
        if (snapshot_mode_ == SequentialSnapshotMode::RetainIntermediate &&
            !toggle_snapshots.empty()) {
            final_snapshot = toggle_snapshots.back();
        } else {
            final_snapshot = clustering_state_.snapshot(topology_batch.new_time);
        }
        const auto snapshot_end = std::chrono::steady_clock::now();
        snapshot_materialization_ns = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                snapshot_end - snapshot_start).count());
    }
    return {
        std::move(event_batch),
        std::move(topology_batch),
        std::move(toggle_snapshots),
        std::move(final_snapshot),
        incremental_work,
        static_cast<std::uint64_t>(transition_ns),
        update_ns,
        snapshot_materialization_ns,
    };
}

}  // namespace basw
