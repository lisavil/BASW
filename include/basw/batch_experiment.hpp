#pragma once

#include "basw/batch_exact_clustering_state.hpp"
#include "basw/event_parser.hpp"
#include "basw/types.hpp"

#include <cstddef>
#include <cstdint>
#include <iosfwd>

namespace basw {

struct BatchExperimentConfig {
    Timestamp window_width{};
    Timestamp slide_interval{};
    Timestamp initial_time{};
    RationalThreshold epsilon;
    std::uint64_t mu{};
    std::size_t max_slides{};
    BatchMaintenanceMode batch_maintenance_mode{BatchMaintenanceMode::Local};
};

struct BatchExperimentSummary {
    std::size_t snapshots_written{};
    std::size_t slides_processed{};
    std::size_t raw_event_changes{};
    std::size_t effective_insertions{};
    std::size_t effective_deletions{};
    std::size_t touched_vertices{};
    std::size_t batch_affected_pairs{};
    std::size_t sequential_affected_pairs{};
    std::size_t dirty_components{};
    std::size_t repair_vertices{};
    std::size_t repair_edges{};
    std::size_t repair_adjacency_checks{};
    std::size_t role_boundary_vertices{};
    std::size_t role_adjacency_checks{};
    std::uint64_t total_transition_ns{};
    std::uint64_t total_batch_update_ns{};
    std::uint64_t total_affected_discovery_ns{};
    std::uint64_t total_similarity_ns{};
    std::uint64_t total_core_ns{};
    std::uint64_t total_repair_ns{};
    std::uint64_t total_role_ns{};
    bool truncated_by_slide_limit{false};
};

[[nodiscard]] BatchExperimentSummary run_batch_experiment(
    const ParsedEventStream& parsed,
    const BatchExperimentConfig& config,
    std::ostream& output,
    std::ostream* metrics_output = nullptr);

}  // namespace basw
