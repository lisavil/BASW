#include "basw/sequential_experiment.hpp"

#include "basw/clustering_snapshot.hpp"
#include "basw/sequential_exact_runner.hpp"

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
              "effective_deletions,alpha,effective_toggles,affected_pairs,"
              "changed_similarity_edges,core_promotions,core_demotions,"
              "transition_ns,sequential_update_ns,total_ns\n";
}

void write_metrics_row(
    std::ostream& output,
    std::size_t snapshot_index,
    const ClusteringSnapshot& snapshot,
    const TopologyChanges& topology,
    const IncrementalUpdateStats& work,
    std::uint64_t transition_ns,
    std::uint64_t sequential_update_ns) {
    const std::size_t effective_toggles =
        topology.insertions.size() + topology.deletions.size();
    const double alpha = topology.raw_event_changes == 0
                             ? 0.0
                             : static_cast<double>(effective_toggles) /
                                   static_cast<double>(topology.raw_event_changes);
    output << snapshot_index << ','
           << snapshot.time << ','
           << active_vertex_count(snapshot) << ','
           << snapshot.edges.size() << ','
           << topology.raw_event_changes << ','
           << topology.logical_edges_touched << ','
           << topology.insertions.size() << ','
           << topology.deletions.size() << ','
           << std::fixed << std::setprecision(9) << alpha << ','
           << effective_toggles << ','
           << work.affected_pairs << ','
           << work.changed_similarity_edges << ','
           << work.core_promotions << ','
           << work.core_demotions << ','
           << transition_ns << ','
           << sequential_update_ns << ','
           << transition_ns + sequential_update_ns << '\n';
}

void write_metadata(
    std::ostream& output,
    const ParsedEventStream& parsed,
    const SequentialExperimentConfig& config) {
    output << "[metadata]\n"
           << "method\tSEQ-EXACT\n"
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
}

}  // namespace

SequentialExperimentSummary run_sequential_experiment(
    const ParsedEventStream& parsed,
    const SequentialExperimentConfig& config,
    std::ostream& output,
    std::ostream* metrics_output) {
    if (config.max_slides == 0) {
        throw std::invalid_argument("max_slides must be positive");
    }

    SequentialExactRunner runner(
        parsed.original_vertex_ids.size(),
        parsed.events,
        config.window_width,
        config.slide_interval,
        config.initial_time,
        config.epsilon,
        config.mu);
    write_metadata(output, parsed, config);

    SequentialExperimentSummary summary;
    const ClusteringSnapshot initial_snapshot = runner.current_snapshot();
    output << "=== snapshot 0 ===\n" << serialize_snapshot(initial_snapshot);
    ++summary.snapshots_written;
    if (metrics_output != nullptr) {
        write_metrics_header(*metrics_output);
        write_metrics_row(
            *metrics_output,
            0,
            initial_snapshot,
            TopologyChanges{
                initial_snapshot.time, initial_snapshot.time, 0, 0, {}, {}},
            IncrementalUpdateStats{},
            0,
            0);
    }

    while (runner.has_pending_events() &&
           summary.slides_processed < config.max_slides) {
        SequentialExactStep step = runner.advance();
        ++summary.slides_processed;
        ++summary.snapshots_written;
        summary.raw_event_changes += step.topology_changes.raw_event_changes;
        summary.effective_insertions += step.topology_changes.insertions.size();
        summary.effective_deletions += step.topology_changes.deletions.size();
        summary.affected_pairs += step.incremental_work.affected_pairs;
        summary.changed_similarity_edges +=
            step.incremental_work.changed_similarity_edges;
        summary.core_promotions += step.incremental_work.core_promotions;
        summary.core_demotions += step.incremental_work.core_demotions;
        summary.total_transition_ns += step.transition_ns;
        summary.total_sequential_update_ns += step.sequential_update_ns;

        output << "=== snapshot " << summary.slides_processed << " ===\n"
               << serialize_snapshot(step.final_snapshot);
        if (metrics_output != nullptr) {
            write_metrics_row(
                *metrics_output,
                summary.slides_processed,
                step.final_snapshot,
                step.topology_changes,
                step.incremental_work,
                step.transition_ns,
                step.sequential_update_ns);
        }
    }

    summary.truncated_by_slide_limit = runner.has_pending_events();
    if (!output) {
        throw std::runtime_error("failed while writing sequential experiment output");
    }
    if (metrics_output != nullptr && !*metrics_output) {
        throw std::runtime_error("failed while writing sequential experiment metrics");
    }
    return summary;
}

}  // namespace basw
