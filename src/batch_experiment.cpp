#include "basw/batch_experiment.hpp"

#include "basw/batch_exact_runner.hpp"
#include "basw/clustering_snapshot.hpp"
#include "basw/sequential_exact_runner.hpp"

#include <iomanip>
#include <memory>
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
              "effective_deletions,alpha,touched_vertices,"
              "batch_affected_pairs,seq_affected_pairs,beta,"
              "changed_similarity_edges,core_promotions,core_demotions,"
              "dirty_components,repair_vertices,repair_edges,"
              "repair_adjacency_checks,role_boundary_vertices,"
              "role_adjacency_checks,affected_discovery_ns,similarity_ns,"
              "core_ns,repair_ns,role_ns,measured_phase_ns,"
              "transition_ns,batch_update_ns,total_ns\n";
}

void write_metrics_row(
    std::ostream& output,
    std::size_t snapshot_index,
    const ClusteringSnapshot& snapshot,
    const EffectiveTopologyBatch& topology,
    const BatchUpdateStats& work,
    std::size_t sequential_affected_pairs,
    std::uint64_t transition_ns,
    std::uint64_t batch_update_ns) {
    const std::size_t effective_toggles =
        topology.insertions.size() + topology.deletions.size();
    const double alpha = topology.raw_event_changes == 0
                             ? 0.0
                             : static_cast<double>(effective_toggles) /
                                   static_cast<double>(topology.raw_event_changes);
    const double beta = sequential_affected_pairs == 0
                            ? 0.0
                            : static_cast<double>(work.affected_pairs) /
                                  static_cast<double>(sequential_affected_pairs);
    output << snapshot_index << ','
           << snapshot.time << ','
           << active_vertex_count(snapshot) << ','
           << snapshot.edges.size() << ','
           << topology.raw_event_changes << ','
           << topology.logical_edges_touched << ','
           << topology.insertions.size() << ','
           << topology.deletions.size() << ','
           << std::fixed << std::setprecision(9) << alpha << ','
           << work.touched_vertices << ','
           << work.affected_pairs << ','
           << sequential_affected_pairs << ','
           << beta << ','
           << work.changed_similarity_edges << ','
           << work.core_promotions << ','
           << work.core_demotions << ','
           << work.dirty_components << ','
           << work.repair_vertices << ','
           << work.repair_edges << ','
           << work.repair_adjacency_checks << ','
           << work.role_boundary_vertices << ','
           << work.role_adjacency_checks << ','
           << work.affected_discovery_ns << ','
           << work.similarity_ns << ','
           << work.core_ns << ','
           << work.repair_ns << ','
           << work.role_ns << ','
           << work.measured_phase_ns() << ','
           << transition_ns << ','
           << batch_update_ns << ','
           << transition_ns + batch_update_ns << '\n';
}

void write_metadata(
    std::ostream& output,
    const ParsedEventStream& parsed,
    const BatchExperimentConfig& config) {
    output << "[metadata]\n"
           << "method\tBASW-BATCH\n"
           << "vertices\t" << parsed.original_vertex_ids.size() << '\n'
           << "events\t" << parsed.events.size() << '\n'
           << "self_loops_skipped\t" << parsed.stats.self_loops_skipped << '\n'
           << "window_width\t" << config.window_width << '\n'
           << "slide_interval\t" << config.slide_interval << '\n'
           << "initial_time\t" << config.initial_time << '\n'
           << "epsilon\t" << config.epsilon.numerator << '/'
           << config.epsilon.denominator << '\n'
           << "mu\t" << config.mu << '\n'
           << "batch_maintenance_mode\t"
           << batch_maintenance_mode_name(config.batch_maintenance_mode) << '\n'
           << "[vertex_map]\n";
    for (std::size_t dense = 0; dense < parsed.original_vertex_ids.size(); ++dense) {
        output << dense << '\t' << parsed.original_vertex_ids[dense] << '\n';
    }
}

}  // namespace

BatchExperimentSummary run_batch_experiment(
    const ParsedEventStream& parsed,
    const BatchExperimentConfig& config,
    std::ostream& output,
    std::ostream* metrics_output) {
    if (config.max_slides == 0) {
        throw std::invalid_argument("max_slides must be positive");
    }

    BatchExactRunner runner(
        parsed.original_vertex_ids.size(),
        parsed.events,
        config.window_width,
        config.slide_interval,
        config.initial_time,
        config.epsilon,
        config.mu,
        config.batch_maintenance_mode);
    std::unique_ptr<SequentialExactRunner> sequential_reference;
    if (metrics_output != nullptr) {
        sequential_reference = std::make_unique<SequentialExactRunner>(
            parsed.original_vertex_ids.size(),
            parsed.events,
            config.window_width,
            config.slide_interval,
            config.initial_time,
            config.epsilon,
            config.mu);
    }

    write_metadata(output, parsed, config);
    BatchExperimentSummary summary;
    const ClusteringSnapshot initial_snapshot = runner.current_snapshot();
    if (sequential_reference != nullptr &&
        initial_snapshot != sequential_reference->current_snapshot()) {
        throw std::logic_error("initial BASW and SEQ snapshots differ");
    }
    output << "=== snapshot 0 ===\n" << serialize_snapshot(initial_snapshot);
    ++summary.snapshots_written;
    if (metrics_output != nullptr) {
        write_metrics_header(*metrics_output);
        write_metrics_row(
            *metrics_output,
            0,
            initial_snapshot,
            EffectiveTopologyBatch{
                initial_snapshot.time, initial_snapshot.time, 0, 0, {}, {}},
            BatchUpdateStats{},
            0,
            0,
            0);
    }

    while (runner.has_pending_events() &&
           summary.slides_processed < config.max_slides) {
        BatchExactStep step = runner.advance();
        std::size_t sequential_affected_pairs = 0;
        if (sequential_reference != nullptr) {
            if (!sequential_reference->has_pending_events()) {
                throw std::logic_error("SEQ stream ended before BASW stream");
            }
            const SequentialExactStep sequential_step =
                sequential_reference->advance();
            if (step.topology_batch != sequential_step.topology_batch ||
                step.final_snapshot != sequential_step.final_snapshot) {
                throw std::logic_error("BASW and SEQ exact results differ");
            }
            sequential_affected_pairs =
                sequential_step.incremental_work.affected_pairs;
        }

        ++summary.slides_processed;
        ++summary.snapshots_written;
        summary.raw_event_changes += step.topology_batch.raw_event_changes;
        summary.effective_insertions += step.topology_batch.insertions.size();
        summary.effective_deletions += step.topology_batch.deletions.size();
        summary.touched_vertices += step.batch_work.touched_vertices;
        summary.batch_affected_pairs += step.batch_work.affected_pairs;
        summary.sequential_affected_pairs += sequential_affected_pairs;
        summary.dirty_components += step.batch_work.dirty_components;
        summary.repair_vertices += step.batch_work.repair_vertices;
        summary.repair_edges += step.batch_work.repair_edges;
        summary.repair_adjacency_checks +=
            step.batch_work.repair_adjacency_checks;
        summary.role_boundary_vertices +=
            step.batch_work.role_boundary_vertices;
        summary.role_adjacency_checks +=
            step.batch_work.role_adjacency_checks;
        summary.total_transition_ns += step.transition_ns;
        summary.total_batch_update_ns += step.batch_update_ns;
        summary.total_affected_discovery_ns +=
            step.batch_work.affected_discovery_ns;
        summary.total_similarity_ns += step.batch_work.similarity_ns;
        summary.total_core_ns += step.batch_work.core_ns;
        summary.total_repair_ns += step.batch_work.repair_ns;
        summary.total_role_ns += step.batch_work.role_ns;

        output << "=== snapshot " << summary.slides_processed << " ===\n"
               << serialize_snapshot(step.final_snapshot);
        if (metrics_output != nullptr) {
            write_metrics_row(
                *metrics_output,
                summary.slides_processed,
                step.final_snapshot,
                step.topology_batch,
                step.batch_work,
                sequential_affected_pairs,
                step.transition_ns,
                step.batch_update_ns);
        }
    }

    summary.truncated_by_slide_limit = runner.has_pending_events();
    if (!output) {
        throw std::runtime_error("failed while writing batch experiment output");
    }
    if (metrics_output != nullptr && !*metrics_output) {
        throw std::runtime_error("failed while writing batch experiment metrics");
    }
    return summary;
}

}  // namespace basw
