#include "basw/static_experiment.hpp"

#include "basw/clustering_snapshot.hpp"
#include "basw/static_window_runner.hpp"

#include <chrono>
#include <iomanip>
#include <ostream>
#include <stdexcept>
#include <vector>

namespace basw {
namespace {

std::size_t active_vertex_count(const ClusteringSnapshot& snapshot) {
    std::vector<bool> active(snapshot.vertices.size(), false);
    for (const EdgeSnapshot& edge : snapshot.edges) {
        active[edge.edge.u] = true;
        active[edge.edge.v] = true;
    }
    std::size_t count = 0;
    for (const bool present : active) {
        count += present ? 1U : 0U;
    }
    return count;
}

void write_metrics_header(std::ostream& output) {
    output << "snapshot_index,window_time,active_vertices,active_edges,"
              "raw_event_changes,logical_edges_touched,effective_insertions,"
              "effective_deletions,alpha,transition_ns,static_recompute_ns,total_ns\n";
}

void write_metrics_row(
    std::ostream& output,
    std::size_t snapshot_index,
    const ClusteringSnapshot& snapshot,
    std::size_t raw_event_changes,
    std::size_t logical_edges_touched,
    std::size_t effective_insertions,
    std::size_t effective_deletions,
    std::uint64_t transition_ns,
    std::uint64_t static_recompute_ns) {
    const std::size_t effective_toggles =
        effective_insertions + effective_deletions;
    const double alpha = raw_event_changes == 0
                             ? 0.0
                             : static_cast<double>(effective_toggles) /
                                   static_cast<double>(raw_event_changes);
    output << snapshot_index << ','
           << snapshot.time << ','
           << active_vertex_count(snapshot) << ','
           << snapshot.edges.size() << ','
           << raw_event_changes << ','
           << logical_edges_touched << ','
           << effective_insertions << ','
           << effective_deletions << ','
           << std::fixed << std::setprecision(9) << alpha << ','
           << transition_ns << ','
           << static_recompute_ns << ','
           << transition_ns + static_recompute_ns << '\n';
}

}  // namespace

StaticExperimentSummary run_static_experiment(
    const ParsedEventStream& parsed,
    const StaticExperimentConfig& config,
    std::ostream& output,
    std::ostream* metrics_output) {
    if (config.max_slides == 0) {
        throw std::invalid_argument("max_slides must be positive");
    }

    StaticWindowRunner runner(
        parsed.original_vertex_ids.size(),
        parsed.events,
        config.window_width,
        config.slide_interval,
        config.initial_time,
        config.epsilon,
        config.mu);

    output << "[metadata]\n"
           << "vertices\t" << parsed.original_vertex_ids.size() << '\n'
           << "events\t" << parsed.events.size() << '\n'
           << "self_loops_skipped\t" << parsed.stats.self_loops_skipped << '\n'
           << "window_width\t" << config.window_width << '\n'
           << "slide_interval\t" << config.slide_interval << '\n'
           << "initial_time\t" << config.initial_time << '\n'
           << "epsilon\t" << config.epsilon.numerator << '/'
           << config.epsilon.denominator << '\n'
           << "mu\t" << config.mu << '\n'
           << "[vertex_map]\n";
    for (std::size_t dense = 0; dense < parsed.original_vertex_ids.size(); ++dense) {
        output << dense << '\t' << parsed.original_vertex_ids[dense] << '\n';
    }

    StaticExperimentSummary summary;
    const auto initial_static_start = std::chrono::steady_clock::now();
    const ClusteringSnapshot initial_snapshot = runner.current_snapshot();
    const auto initial_static_end = std::chrono::steady_clock::now();
    const auto initial_static_ns =
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            initial_static_end - initial_static_start).count();
    summary.total_static_recompute_ns =
        static_cast<std::uint64_t>(initial_static_ns);

    output << "=== snapshot 0 ===\n"
           << serialize_snapshot(initial_snapshot);
    ++summary.snapshots_written;
    if (metrics_output != nullptr) {
        write_metrics_header(*metrics_output);
        write_metrics_row(
            *metrics_output,
            0,
            initial_snapshot,
            0,
            0,
            0,
            0,
            0,
            static_cast<std::uint64_t>(initial_static_ns));
    }

    while (runner.has_pending_events() &&
           summary.slides_processed < config.max_slides) {
        const StaticWindowStep step = runner.advance();
        ++summary.slides_processed;
        ++summary.snapshots_written;
        summary.raw_event_changes +=
            step.transition.topology_batch.raw_event_changes;
        summary.effective_insertions +=
            step.transition.topology_batch.insertions.size();
        summary.effective_deletions +=
            step.transition.topology_batch.deletions.size();
        summary.total_transition_ns += step.transition_ns;
        summary.total_static_recompute_ns += step.static_recompute_ns;

        output << "=== snapshot " << summary.slides_processed << " ===\n"
               << serialize_snapshot(step.snapshot);
        if (metrics_output != nullptr) {
            write_metrics_row(
                *metrics_output,
                summary.slides_processed,
                step.snapshot,
                step.transition.topology_batch.raw_event_changes,
                step.transition.topology_batch.logical_edges_touched,
                step.transition.topology_batch.insertions.size(),
                step.transition.topology_batch.deletions.size(),
                step.transition_ns,
                step.static_recompute_ns);
        }
    }

    summary.truncated_by_slide_limit = runner.has_pending_events();
    if (!output) {
        throw std::runtime_error("failed while writing static experiment output");
    }
    if (metrics_output != nullptr && !*metrics_output) {
        throw std::runtime_error("failed while writing static experiment metrics");
    }
    return summary;
}

}  // namespace basw
