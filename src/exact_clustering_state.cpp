#include "basw/exact_clustering_state.hpp"

#include <utility>

namespace basw {

ExactClusteringState::ExactClusteringState(
    Graph graph,
    RationalThreshold epsilon,
    std::uint64_t mu,
    MaintenanceTimingMode timing_mode)
    : state_(
          std::move(graph), epsilon, mu, MaintenanceMode::Local,
          false, timing_mode) {}

const Graph& ExactClusteringState::graph() const noexcept {
    return state_.graph();
}

ClusteringSnapshot ExactClusteringState::snapshot(Timestamp time) const {
    return state_.snapshot(time);
}

IncrementalUpdateStats ExactClusteringState::apply_insertion(const Edge& edge) {
    return apply_toggle(edge, true);
}

IncrementalUpdateStats ExactClusteringState::apply_deletion(const Edge& edge) {
    return apply_toggle(edge, false);
}

IncrementalUpdateStats ExactClusteringState::apply_toggle(
    const Edge& edge, bool insertion) {
    const BaswUpdateStats update = state_.apply_single_toggle(edge, insertion);
    return {
        update.affected_pairs,
        update.changed_similarity_edges,
        update.core_promotions,
        update.core_demotions,
    };
}

}  // namespace basw
