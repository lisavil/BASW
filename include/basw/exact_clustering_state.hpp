#pragma once

#include "basw/batch_exact_clustering_state.hpp"
#include "basw/clustering_snapshot.hpp"
#include "basw/graph.hpp"
#include "basw/types.hpp"

#include <cstddef>
#include <cstdint>

namespace basw {

struct IncrementalUpdateStats {
    std::size_t affected_pairs{};
    std::size_t changed_similarity_edges{};
    std::size_t core_promotions{};
    std::size_t core_demotions{};
};

class ExactClusteringState {
public:
    ExactClusteringState(
        Graph graph,
        RationalThreshold epsilon,
        std::uint64_t mu,
        MaintenanceTimingMode timing_mode =
            MaintenanceTimingMode::Instrumented);

    [[nodiscard]] const Graph& graph() const noexcept;
    [[nodiscard]] ClusteringSnapshot snapshot(Timestamp time) const;

    IncrementalUpdateStats apply_insertion(const Edge& edge);
    IncrementalUpdateStats apply_deletion(const Edge& edge);

private:
    IncrementalUpdateStats apply_toggle(const Edge& edge, bool insertion);
    BatchExactClusteringState state_;
};

}  // namespace basw
