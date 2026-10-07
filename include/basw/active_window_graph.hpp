#pragma once

#include "basw/active_edge_index.hpp"
#include "basw/graph.hpp"
#include "basw/window_engine.hpp"

#include <cstddef>
#include <vector>

namespace basw {

struct WindowTransitionResult {
    WindowBatch event_batch;
    EffectiveTopologyBatch topology_batch;
};

class ActiveWindowGraph {
public:
    ActiveWindowGraph(
        std::size_t vertex_count,
        std::vector<TemporalEvent> events,
        Timestamp window_width,
        Timestamp slide_interval,
        Timestamp initial_time);

    [[nodiscard]] const Graph& graph() const noexcept;
    [[nodiscard]] const ActiveEdgeIndex& edge_index() const noexcept;
    [[nodiscard]] Timestamp current_time() const noexcept;
    [[nodiscard]] bool has_pending_events() const noexcept;

    WindowTransitionResult advance();

private:
    static std::vector<TemporalEvent> validate_events(
        std::size_t vertex_count,
        std::vector<TemporalEvent> events);

    WindowEngine window_engine_;
    ActiveEdgeIndex edge_index_;
    Graph graph_;
};

}  // namespace basw
