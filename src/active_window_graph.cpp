#include "basw/active_window_graph.hpp"

#include <stdexcept>
#include <utility>

namespace basw {

std::vector<TemporalEvent> ActiveWindowGraph::validate_events(
    std::size_t vertex_count,
    std::vector<TemporalEvent> events) {
    for (const TemporalEvent& event : events) {
        if (event.edge.u >= vertex_count || event.edge.v >= vertex_count) {
            throw std::out_of_range(
                "temporal event endpoint is outside the configured graph");
        }
    }
    return events;
}

ActiveWindowGraph::ActiveWindowGraph(
    std::size_t vertex_count,
    std::vector<TemporalEvent> events,
    Timestamp window_width,
    Timestamp slide_interval,
    Timestamp initial_time)
    : window_engine_(
          validate_events(vertex_count, std::move(events)),
          window_width,
          slide_interval,
          initial_time),
      edge_index_(window_engine_.initial_events()),
      graph_(vertex_count) {
    for (const TemporalEvent& event : window_engine_.initial_events()) {
        graph_.add_edge(event.edge.u, event.edge.v);
    }
}

const Graph& ActiveWindowGraph::graph() const noexcept {
    return graph_;
}

const ActiveEdgeIndex& ActiveWindowGraph::edge_index() const noexcept {
    return edge_index_;
}

Timestamp ActiveWindowGraph::current_time() const noexcept {
    return window_engine_.current_time();
}

bool ActiveWindowGraph::has_pending_events() const noexcept {
    return window_engine_.has_pending_events();
}

WindowTransitionResult ActiveWindowGraph::advance() {
    WindowTransition event_transition = window_engine_.advance();
    TopologyChanges topology_changes = edge_index_.apply(event_transition);

    for (const Edge& edge : topology_changes.deletions) {
        if (!graph_.remove_edge(edge.u, edge.v)) {
            throw std::logic_error(
                "multiplicity index deleted an edge absent from the active graph");
        }
    }
    for (const Edge& edge : topology_changes.insertions) {
        if (!graph_.add_edge(edge.u, edge.v)) {
            throw std::logic_error(
                "multiplicity index inserted an edge already in the active graph");
        }
    }

    return {std::move(event_transition), std::move(topology_changes)};
}

}  // namespace basw
