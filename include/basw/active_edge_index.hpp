#pragma once

#include "basw/types.hpp"
#include "basw/window_engine.hpp"

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace basw {

struct EffectiveTopologyBatch {
    Timestamp old_time{};
    Timestamp new_time{};
    std::size_t raw_event_changes{};
    std::size_t logical_edges_touched{};
    std::vector<Edge> insertions;
    std::vector<Edge> deletions;

    bool operator==(const EffectiveTopologyBatch&) const = default;
};

class ActiveEdgeIndex {
public:
    ActiveEdgeIndex() = default;
    explicit ActiveEdgeIndex(const std::vector<TemporalEvent>& initial_events);

    [[nodiscard]] std::uint64_t multiplicity(const Edge& edge) const noexcept;
    [[nodiscard]] bool is_active(const Edge& edge) const noexcept;
    [[nodiscard]] std::size_t active_edge_count() const noexcept;

    // Applies one complete slide atomically. If an expiration is inconsistent
    // with the current active multiset, no multiplicity is changed.
    EffectiveTopologyBatch apply(const WindowBatch& batch);

private:
    std::unordered_map<Edge, std::uint64_t, EdgeHash> multiplicities_;
};

}  // namespace basw
