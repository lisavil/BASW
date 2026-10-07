#include "basw/active_edge_index.hpp"

#include <algorithm>
#include <stdexcept>

namespace basw {
namespace {

struct EventCounts {
    std::uint64_t expired{};
    std::uint64_t arrived{};
};

bool edge_less(const Edge& left, const Edge& right) noexcept {
    return left.u < right.u || (left.u == right.u && left.v < right.v);
}

}  // namespace

ActiveEdgeIndex::ActiveEdgeIndex(
    const std::vector<TemporalEvent>& initial_events) {
    for (const TemporalEvent& event : initial_events) {
        ++multiplicities_[event.edge];
    }
}

std::uint64_t ActiveEdgeIndex::multiplicity(const Edge& edge) const noexcept {
    const auto found = multiplicities_.find(edge);
    return found == multiplicities_.end() ? 0 : found->second;
}

bool ActiveEdgeIndex::is_active(const Edge& edge) const noexcept {
    return multiplicity(edge) > 0;
}

std::size_t ActiveEdgeIndex::active_edge_count() const noexcept {
    return multiplicities_.size();
}

TopologyChanges ActiveEdgeIndex::apply(const WindowTransition& transition) {
    std::unordered_map<Edge, EventCounts, EdgeHash> counts;
    counts.reserve(transition.expired.size() + transition.arrived.size());
    for (const TemporalEvent& event : transition.expired) {
        ++counts[event.edge].expired;
    }
    for (const TemporalEvent& event : transition.arrived) {
        ++counts[event.edge].arrived;
    }

    // Validate the complete transition before mutating multiplicities_. Arrivals
    // cannot compensate for expiration records absent from the old window.
    for (const auto& [edge, event_counts] : counts) {
        if (event_counts.expired > multiplicity(edge)) {
            throw std::invalid_argument(
                "transition expires more copies of an edge than are active");
        }
    }

    TopologyChanges result{
        transition.old_time,
        transition.new_time,
        transition.expired.size() + transition.arrived.size(),
        counts.size(),
        {},
        {}};

    for (const auto& [edge, event_counts] : counts) {
        const std::uint64_t old_count = multiplicity(edge);
        const std::uint64_t final_count =
            old_count - event_counts.expired + event_counts.arrived;

        if (old_count == 0 && final_count > 0) {
            result.insertions.push_back(edge);
        } else if (old_count > 0 && final_count == 0) {
            result.deletions.push_back(edge);
        }

        if (final_count == 0) {
            multiplicities_.erase(edge);
        } else {
            multiplicities_[edge] = final_count;
        }
    }

    std::sort(result.insertions.begin(), result.insertions.end(), edge_less);
    std::sort(result.deletions.begin(), result.deletions.end(), edge_less);
    return result;
}

}  // namespace basw
