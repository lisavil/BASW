#pragma once

#include "basw/event_parser.hpp"
#include "basw/types.hpp"

#include <cstddef>
#include <cstdint>
#include <iosfwd>

namespace basw {

struct StaticExperimentConfig {
    Timestamp window_width{};
    Timestamp slide_interval{};
    Timestamp initial_time{};
    RationalThreshold epsilon;
    std::uint64_t mu{};
    std::size_t max_slides{};
};

struct StaticExperimentSummary {
    std::size_t snapshots_written{};
    std::size_t slides_processed{};
    std::size_t raw_event_changes{};
    std::size_t effective_insertions{};
    std::size_t effective_deletions{};
    std::uint64_t total_transition_ns{};
    std::uint64_t total_static_recompute_ns{};
    bool truncated_by_slide_limit{false};
};

[[nodiscard]] StaticExperimentSummary run_static_experiment(
    const ParsedEventStream& parsed,
    const StaticExperimentConfig& config,
    std::ostream& output,
    std::ostream* metrics_output = nullptr);

}  // namespace basw
