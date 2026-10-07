#include "basw/batch_exact_clustering_state.hpp"

#include "basw/jaccard.hpp"
#include "basw/static_clustering.hpp"

#include <algorithm>
#include <chrono>
#include <limits>
#include <queue>
#include <set>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace basw {
namespace {

bool edge_less(const Edge& left, const Edge& right) noexcept {
    return left.u < right.u || (left.u == right.u && left.v < right.v);
}

}  // namespace

const char* batch_maintenance_mode_name(
    BatchMaintenanceMode mode) noexcept {
    switch (mode) {
        case BatchMaintenanceMode::Local:
            return "local";
        case BatchMaintenanceMode::GlobalRepair:
            return "global_repair";
        case BatchMaintenanceMode::GlobalRole:
            return "global_role";
        case BatchMaintenanceMode::GlobalRepairRole:
            return "global_repair_role";
    }
    return "unknown";
}

BatchMaintenanceMode parse_batch_maintenance_mode(std::string_view text) {
    if (text == "local") {
        return BatchMaintenanceMode::Local;
    }
    if (text == "global_repair") {
        return BatchMaintenanceMode::GlobalRepair;
    }
    if (text == "global_role") {
        return BatchMaintenanceMode::GlobalRole;
    }
    if (text == "global_repair_role") {
        return BatchMaintenanceMode::GlobalRepairRole;
    }
    throw std::invalid_argument("unknown batch maintenance mode");
}

BatchExactClusteringState::BatchExactClusteringState(
    Graph graph,
    RationalThreshold epsilon,
    std::uint64_t mu,
    BatchMaintenanceMode maintenance_mode,
    bool allow_all_dirty_global_fallback,
    MaintenanceTimingMode timing_mode)
    : graph_(std::move(graph)),
      epsilon_(epsilon),
      mu_(mu),
      maintenance_mode_(maintenance_mode),
      allow_all_dirty_global_fallback_(allow_all_dirty_global_fallback),
      timing_mode_(timing_mode) {
    const StaticClusteringResult initial =
        recompute_static_clustering(graph_, epsilon_, mu_);
    edge_states_ = initial.core_state.edge_states;
    similar_neighbor_count_ = initial.core_state.similar_neighbor_count;
    is_core_ = initial.core_state.is_core;
    core_component_ = initial.core_state.core_component;
    rebuild_component_membership_index();
    memberships_ = initial.memberships;
    roles_ = initial.roles;
}

const Graph& BatchExactClusteringState::graph() const noexcept {
    return graph_;
}

ClusteringSnapshot BatchExactClusteringState::snapshot(Timestamp time) const {
    ClusteringSnapshot result;
    result.time = time;
    result.edges.reserve(edge_states_.size());
    for (const auto& [edge, state] : edge_states_) {
        result.edges.push_back({edge, state.fraction, state.similar});
    }
    std::sort(
        result.edges.begin(),
        result.edges.end(),
        [](const EdgeSnapshot& left, const EdgeSnapshot& right) {
            return edge_less(left.edge, right.edge);
        });

    result.vertices.reserve(graph_.vertex_count());
    for (VertexId vertex = 0; vertex < graph_.vertex_count(); ++vertex) {
        result.vertices.push_back({
            vertex,
            similar_neighbor_count_[vertex],
            is_core_[vertex],
            core_component_[vertex],
            memberships_[vertex],
            roles_[vertex],
        });
    }
    return result;
}

BatchUpdateStats BatchExactClusteringState::apply_batch(
    const std::vector<Edge>& deletions,
    const std::vector<Edge>& insertions) {
    return apply_changes(deletions, insertions);
}

BatchUpdateStats BatchExactClusteringState::apply_single_toggle(
    const Edge& edge, bool insertion) {
    const std::span<const Edge> one(&edge, 1);
    return insertion ? apply_changes({}, one) : apply_changes(one, {});
}

BatchUpdateStats BatchExactClusteringState::apply_changes(
    std::span<const Edge> deletions,
    std::span<const Edge> insertions) {
    using Clock = std::chrono::steady_clock;
    BatchUpdateStats stats;
    const bool instrumented =
        timing_mode_ == MaintenanceTimingMode::Instrumented;
    Clock::time_point affected_start;
    if (instrumented) {
        affected_start = Clock::now();
    }
    std::unordered_set<Edge, EdgeHash> batch_edges;
    batch_edges.reserve(deletions.size() + insertions.size());
    for (const Edge& edge : deletions) {
        if (!batch_edges.insert(edge).second) {
            throw std::invalid_argument("duplicate edge in topology batch");
        }
        if (!graph_.has_edge(edge.u, edge.v)) {
            throw std::invalid_argument("cannot delete an inactive edge");
        }
    }
    for (const Edge& edge : insertions) {
        if (!batch_edges.insert(edge).second) {
            throw std::invalid_argument("edge appears more than once in topology batch");
        }
        if (graph_.has_edge(edge.u, edge.v)) {
            throw std::invalid_argument("cannot insert an already active edge");
        }
    }

    std::unordered_set<VertexId> touched;
    touched.reserve(2 * batch_edges.size());
    for (const Edge& edge : batch_edges) {
        touched.insert(edge.u);
        touched.insert(edge.v);
    }

    std::unordered_set<Edge, EdgeHash> affected;
    for (const VertexId vertex : touched) {
        for (const VertexId neighbor : graph_.neighbors(vertex)) {
            affected.emplace(vertex, neighbor);
        }
    }

    std::unordered_map<Edge, bool, EdgeHash> old_similar;
    old_similar.reserve(affected.size());
    for (const Edge& edge : affected) {
        const auto state = edge_states_.find(edge);
        if (state == edge_states_.end()) {
            throw std::logic_error("active graph edge has no similarity state");
        }
        old_similar.emplace(edge, state->second.similar);
    }

    for (const Edge& edge : deletions) {
        if (!graph_.remove_edge(edge.u, edge.v)) {
            throw std::logic_error("validated batch deletion did not change graph");
        }
    }
    for (const Edge& edge : insertions) {
        if (!graph_.add_edge(edge.u, edge.v)) {
            throw std::logic_error("validated batch insertion did not change graph");
        }
    }

    for (const VertexId vertex : touched) {
        for (const VertexId neighbor : graph_.neighbors(vertex)) {
            affected.emplace(vertex, neighbor);
        }
    }

    std::vector<Edge> ordered_affected(affected.begin(), affected.end());
    std::sort(ordered_affected.begin(), ordered_affected.end(), edge_less);
    std::unordered_set<VertexId> core_candidates = touched;
    stats.touched_vertices = touched.size();
    stats.affected_pairs = ordered_affected.size();
    if (instrumented) {
        const auto affected_end = Clock::now();
        stats.affected_discovery_ns = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                affected_end - affected_start).count());
    }

    std::unordered_set<std::int64_t> dirty_components;
    std::vector<Edge> new_core_connection_candidates;
    std::unordered_set<VertexId> role_seed_vertices = touched;
    Clock::time_point similarity_start;
    if (instrumented) {
        similarity_start = Clock::now();
    }
    for (const Edge& edge : ordered_affected) {
        const auto old = old_similar.find(edge);
        const bool was_similar =
            old == old_similar.end() ? false : old->second;
        bool now_similar = false;
        if (graph_.has_edge(edge.u, edge.v)) {
            const SimilarityFraction fraction =
                exact_jaccard(graph_, edge.u, edge.v);
            now_similar = meets_threshold(fraction, epsilon_);
            edge_states_[edge] = {fraction, now_similar};
        } else {
            edge_states_.erase(edge);
        }

        if (was_similar == now_similar) {
            continue;
        }
        role_seed_vertices.insert(edge.u);
        role_seed_vertices.insert(edge.v);
        if (now_similar) {
            new_core_connection_candidates.push_back(edge);
        }
        if (was_similar && !now_similar && is_core_[edge.u] &&
            is_core_[edge.v]) {
            if (core_component_[edge.u] >= 0) {
                dirty_components.insert(core_component_[edge.u]);
            }
            if (core_component_[edge.v] >= 0) {
                dirty_components.insert(core_component_[edge.v]);
            }
        }
        ++stats.changed_similarity_edges;
        core_candidates.insert(edge.u);
        core_candidates.insert(edge.v);
        if (now_similar) {
            ++similar_neighbor_count_[edge.u];
            ++similar_neighbor_count_[edge.v];
        } else {
            if (similar_neighbor_count_[edge.u] == 0 ||
                similar_neighbor_count_[edge.v] == 0) {
                throw std::logic_error("similar-neighbor counter underflow");
            }
            --similar_neighbor_count_[edge.u];
            --similar_neighbor_count_[edge.v];
        }
    }
    if (instrumented) {
        const auto similarity_end = Clock::now();
        stats.similarity_ns = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                similarity_end - similarity_start).count());
    }

    Clock::time_point core_start;
    if (instrumented) {
        core_start = Clock::now();
    }
    std::vector<VertexId> promoted_vertices;
    std::unordered_set<VertexId> core_changed_vertices;
    for (const VertexId vertex : core_candidates) {
        const bool old_core = is_core_[vertex];
        const bool new_core = similar_neighbor_count_[vertex] >= mu_;
        if (old_core && !new_core && core_component_[vertex] >= 0) {
            dirty_components.insert(core_component_[vertex]);
        }
        is_core_[vertex] = new_core;
        stats.core_promotions += !old_core && new_core ? 1U : 0U;
        stats.core_demotions += old_core && !new_core ? 1U : 0U;
        if (!old_core && new_core) {
            promoted_vertices.push_back(vertex);
        }
        if (old_core != new_core) {
            core_changed_vertices.insert(vertex);
            role_seed_vertices.insert(vertex);
        }
    }
    stats.dirty_components = dirty_components.size();
    if (instrumented) {
        const auto core_end = Clock::now();
        stats.core_ns = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                core_end - core_start).count());
    }

    if (!batch_edges.empty()) {
        std::unordered_set<VertexId> component_changed_vertices;
        Clock::time_point repair_start;
        if (instrumented) {
            repair_start = Clock::now();
        }
        repair_components(
            dirty_components,
            promoted_vertices,
            new_core_connection_candidates,
            stats,
            component_changed_vertices);
        if (instrumented) {
            const auto repair_end = Clock::now();
            stats.repair_ns = static_cast<std::uint64_t>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    repair_end - repair_start).count());
        }

        std::unordered_set<VertexId> core_influence_vertices =
            core_changed_vertices;
        core_influence_vertices.insert(
            component_changed_vertices.begin(),
            component_changed_vertices.end());
        role_seed_vertices.insert(
            component_changed_vertices.begin(),
            component_changed_vertices.end());

        Clock::time_point role_start;
        if (instrumented) {
            role_start = Clock::now();
        }
        if (maintenance_mode_ == BatchMaintenanceMode::GlobalRole ||
            maintenance_mode_ == BatchMaintenanceMode::GlobalRepairRole) {
            rebuild_roles_globally(stats);
        } else {
            refresh_roles(role_seed_vertices, core_influence_vertices, stats);
        }
        if (instrumented) {
            const auto role_end = Clock::now();
            stats.role_ns = static_cast<std::uint64_t>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    role_end - role_start).count());
        }
    }
    return stats;
}

void BatchExactClusteringState::repair_components(
    const std::unordered_set<std::int64_t>& dirty_components,
    const std::vector<VertexId>& promoted_vertices,
    const std::vector<Edge>& new_core_connection_candidates,
    BatchUpdateStats& stats,
    std::unordered_set<VertexId>& component_changed_vertices) {
    if (maintenance_mode_ == BatchMaintenanceMode::GlobalRepair ||
        maintenance_mode_ == BatchMaintenanceMode::GlobalRepairRole ||
        (allow_all_dirty_global_fallback_ && !dirty_components.empty() &&
         dirty_components.size() == component_count_)) {
        rebuild_components_globally(stats, component_changed_vertices);
        return;
    }

    std::unordered_set<Edge, EdgeHash> examined_core_edges;
    std::unordered_set<VertexId> visited;
    std::vector<std::pair<std::int64_t, std::vector<VertexId>>>
        split_components;
    std::vector<std::int64_t> ordered_dirty(
        dirty_components.begin(), dirty_components.end());
    std::sort(ordered_dirty.begin(), ordered_dirty.end());

    std::size_t dirty_member_count = 0;
    for (const std::int64_t component : ordered_dirty) {
        if (component < 0 ||
            static_cast<std::size_t>(component) >= component_vertices_.size() ||
            component_vertices_[static_cast<std::size_t>(component)].empty()) {
            throw std::logic_error("dirty component is missing from membership index");
        }
        dirty_member_count +=
            component_vertices_[static_cast<std::size_t>(component)].size();
    }
    visited.reserve(dirty_member_count);

    for (const std::int64_t component : ordered_dirty) {
        const auto& old_members =
            component_vertices_[static_cast<std::size_t>(component)];
        for (const VertexId start : old_members) {
            if (!is_core_[start] || visited.contains(start)) {
                continue;
            }
            std::queue<VertexId> frontier;
            std::vector<VertexId> final_members;
            visited.insert(start);
            frontier.push(start);
            while (!frontier.empty()) {
                const VertexId u = frontier.front();
                frontier.pop();
                ++stats.repair_vertices;
                final_members.push_back(u);
                for (const VertexId v : graph_.neighbors(u)) {
                    ++stats.repair_adjacency_checks;
                    if (!is_core_[v]) {
                        continue;
                    }
                    const auto state = edge_states_.find(Edge(u, v));
                    if (state == edge_states_.end() || !state->second.similar) {
                        continue;
                    }
                    examined_core_edges.emplace(u, v);
                    if (core_component_[v] != component ||
                        visited.contains(v)) {
                        continue;
                    }
                    visited.insert(v);
                    frontier.push(v);
                }
            }
            std::sort(final_members.begin(), final_members.end());
            split_components.emplace_back(
                component, std::move(final_members));
        }
    }

    for (const std::int64_t component : ordered_dirty) {
        auto& old_members =
            component_vertices_[static_cast<std::size_t>(component)];
        for (const VertexId vertex : old_members) {
            core_component_[vertex] = -1;
        }
        old_members.clear();
        --component_count_;
    }
    for (const auto& [old_component, members] : split_components) {
        if (members.empty()) {
            throw std::logic_error("component split produced an empty component");
        }
        const VertexId component = members.front();
        if (!component_vertices_[component].empty()) {
            throw std::logic_error("component split produced a duplicate identifier");
        }
        component_vertices_[component] = members;
        for (const VertexId vertex : members) {
            core_component_[vertex] = static_cast<std::int64_t>(component);
            if (static_cast<std::int64_t>(component) != old_component) {
                component_changed_vertices.insert(vertex);
            }
        }
        ++component_count_;
    }

    std::vector<VertexId> ordered_promotions = promoted_vertices;
    std::sort(ordered_promotions.begin(), ordered_promotions.end());
    std::unordered_set<Edge, EdgeHash> merge_candidates;
    merge_candidates.reserve(
        new_core_connection_candidates.size() + promoted_vertices.size());
    for (const VertexId vertex : ordered_promotions) {
        if (!is_core_[vertex] || core_component_[vertex] >= 0 ||
            !component_vertices_[vertex].empty()) {
            throw std::logic_error("promoted core has inconsistent component metadata");
        }
        core_component_[vertex] = static_cast<std::int64_t>(vertex);
        component_vertices_[vertex].push_back(vertex);
        component_changed_vertices.insert(vertex);
        ++component_count_;
        ++stats.repair_vertices;
    }
    for (const VertexId vertex : ordered_promotions) {
        for (const VertexId neighbor : graph_.neighbors(vertex)) {
            ++stats.repair_adjacency_checks;
            if (!is_core_[neighbor]) {
                continue;
            }
            const Edge edge(vertex, neighbor);
            const auto state = edge_states_.find(edge);
            if (state != edge_states_.end() && state->second.similar) {
                examined_core_edges.insert(edge);
                merge_candidates.insert(edge);
            }
        }
    }
    for (const Edge& edge : new_core_connection_candidates) {
        if (!is_core_[edge.u] || !is_core_[edge.v]) {
            continue;
        }
        const auto state = edge_states_.find(edge);
        if (state != edge_states_.end() && state->second.similar) {
            examined_core_edges.insert(edge);
            merge_candidates.insert(edge);
        }
    }

    std::unordered_map<std::int64_t, std::int64_t> parent;
    const auto find_root = [&parent](std::int64_t component) {
        std::int64_t root = component;
        while (parent.at(root) != root) {
            root = parent.at(root);
        }
        while (parent.at(component) != component) {
            const std::int64_t next = parent.at(component);
            parent[component] = root;
            component = next;
        }
        return root;
    };
    std::vector<Edge> ordered_merge_candidates(
        merge_candidates.begin(), merge_candidates.end());
    std::sort(
        ordered_merge_candidates.begin(),
        ordered_merge_candidates.end(),
        edge_less);
    for (const Edge& edge : ordered_merge_candidates) {
        const std::int64_t left = core_component_[edge.u];
        const std::int64_t right = core_component_[edge.v];
        if (left < 0 || right < 0) {
            throw std::logic_error("final core merge edge has no component");
        }
        parent.try_emplace(left, left);
        parent.try_emplace(right, right);
        const std::int64_t left_root = find_root(left);
        const std::int64_t right_root = find_root(right);
        if (left_root != right_root) {
            const std::int64_t low = std::min(left_root, right_root);
            const std::int64_t high = std::max(left_root, right_root);
            parent[high] = low;
        }
    }

    std::unordered_map<std::int64_t, std::vector<std::int64_t>> merge_groups;
    for (const auto& [component, ignored_parent] : parent) {
        static_cast<void>(ignored_parent);
        merge_groups[find_root(component)].push_back(component);
    }
    for (auto& [root, components] : merge_groups) {
        static_cast<void>(root);
        std::sort(components.begin(), components.end());
        if (components.size() < 2) {
            continue;
        }
        const std::int64_t canonical = components.front();
        std::vector<VertexId> merged_members;
        for (const std::int64_t component : components) {
            const auto& members =
                component_vertices_[static_cast<std::size_t>(component)];
            if (members.empty()) {
                throw std::logic_error("merge component is missing from membership index");
            }
            merged_members.insert(
                merged_members.end(), members.begin(), members.end());
        }
        std::sort(merged_members.begin(), merged_members.end());
        for (const std::int64_t component : components) {
            component_vertices_[static_cast<std::size_t>(component)].clear();
        }
        component_vertices_[static_cast<std::size_t>(canonical)] =
            merged_members;
        for (const VertexId vertex : merged_members) {
            if (core_component_[vertex] != canonical) {
                component_changed_vertices.insert(vertex);
            }
            core_component_[vertex] = canonical;
            ++stats.repair_vertices;
        }
        component_count_ -= components.size() - 1;
    }
    stats.repair_edges = examined_core_edges.size();
}

void BatchExactClusteringState::rebuild_components_globally(
    BatchUpdateStats& stats,
    std::unordered_set<VertexId>& component_changed_vertices) {
    const std::vector<std::int64_t> old_components = core_component_;
    core_component_.assign(graph_.vertex_count(), -1);

    std::vector<bool> visited(graph_.vertex_count(), false);
    for (VertexId start = 0; start < graph_.vertex_count(); ++start) {
        if (!is_core_[start] || visited[start]) {
            continue;
        }
        std::queue<VertexId> frontier;
        std::vector<VertexId> component_vertices;
        VertexId minimum_vertex = std::numeric_limits<VertexId>::max();
        visited[start] = true;
        frontier.push(start);
        while (!frontier.empty()) {
            const VertexId u = frontier.front();
            frontier.pop();
            ++stats.repair_vertices;
            component_vertices.push_back(u);
            minimum_vertex = std::min(minimum_vertex, u);
            for (const VertexId v : graph_.neighbors(u)) {
                ++stats.repair_adjacency_checks;
                if (!is_core_[v]) {
                    continue;
                }
                const auto state = edge_states_.find(Edge(u, v));
                if (state != edge_states_.end() && state->second.similar) {
                    stats.repair_edges += u < v ? 1U : 0U;
                    if (visited[v]) {
                        continue;
                    }
                    visited[v] = true;
                    frontier.push(v);
                }
            }
        }
        for (const VertexId vertex : component_vertices) {
            core_component_[vertex] = minimum_vertex;
        }
    }
    for (VertexId vertex = 0; vertex < graph_.vertex_count(); ++vertex) {
        if (core_component_[vertex] != old_components[vertex]) {
            component_changed_vertices.insert(vertex);
        }
    }
    rebuild_component_membership_index();
}

void BatchExactClusteringState::rebuild_component_membership_index() {
    component_vertices_.assign(graph_.vertex_count(), {});
    component_count_ = 0;
    for (VertexId vertex = 0; vertex < graph_.vertex_count(); ++vertex) {
        if (!is_core_[vertex]) {
            if (core_component_[vertex] != -1) {
                throw std::logic_error("non-core vertex has a component identifier");
            }
            continue;
        }
        const std::int64_t component = core_component_[vertex];
        if (component < 0 ||
            static_cast<std::size_t>(component) >= graph_.vertex_count()) {
            throw std::logic_error("core vertex has an invalid component identifier");
        }
        auto& members =
            component_vertices_[static_cast<std::size_t>(component)];
        if (members.empty()) {
            ++component_count_;
        }
        members.push_back(vertex);
    }
}

void BatchExactClusteringState::refresh_roles(
    const std::unordered_set<VertexId>& role_seed_vertices,
    const std::unordered_set<VertexId>& core_influence_vertices,
    BatchUpdateStats& stats) {
    std::unordered_set<VertexId> primary_boundary;
    std::unordered_set<VertexId> membership_changed_vertices;
    std::unordered_set<VertexId> refreshed_non_core_vertices;

    std::vector<VertexId> ordered_influence(
        core_influence_vertices.begin(), core_influence_vertices.end());
    std::sort(ordered_influence.begin(), ordered_influence.end());
    for (const VertexId vertex : ordered_influence) {
        if (is_core_[vertex]) {
            const std::vector<std::int64_t> new_memberships{
                core_component_[vertex]};
            if (memberships_[vertex] != new_memberships) {
                membership_changed_vertices.insert(vertex);
            }
            memberships_[vertex] = new_memberships;
            roles_[vertex] = VertexRole::Core;
        } else {
            primary_boundary.insert(vertex);
        }

        for (const VertexId neighbor : graph_.neighbors(vertex)) {
            ++stats.role_adjacency_checks;
            if (!is_core_[neighbor]) {
                primary_boundary.insert(neighbor);
            }
        }
    }

    for (const VertexId vertex : role_seed_vertices) {
        if (!is_core_[vertex]) {
            primary_boundary.insert(vertex);
        }
    }

    std::vector<VertexId> ordered_primary(
        primary_boundary.begin(), primary_boundary.end());
    std::sort(ordered_primary.begin(), ordered_primary.end());
    std::unordered_set<VertexId> hub_candidates;
    for (const VertexId vertex : ordered_primary) {
        const std::vector<std::int64_t> old_memberships = memberships_[vertex];
        std::set<std::int64_t> components;
        for (const VertexId neighbor : graph_.neighbors(vertex)) {
            ++stats.role_adjacency_checks;
            if (!is_core_[neighbor]) {
                continue;
            }
            const auto state = edge_states_.find(Edge(vertex, neighbor));
            if (state != edge_states_.end() && state->second.similar) {
                components.insert(core_component_[neighbor]);
            }
        }
        memberships_[vertex].assign(components.begin(), components.end());
        if (memberships_[vertex] != old_memberships) {
            membership_changed_vertices.insert(vertex);
        }
        refreshed_non_core_vertices.insert(vertex);
        if (memberships_[vertex].empty()) {
            hub_candidates.insert(vertex);
        } else {
            roles_[vertex] = VertexRole::Border;
        }
    }

    std::vector<VertexId> ordered_membership_changes(
        membership_changed_vertices.begin(),
        membership_changed_vertices.end());
    std::sort(
        ordered_membership_changes.begin(),
        ordered_membership_changes.end());
    for (const VertexId vertex : ordered_membership_changes) {
        for (const VertexId neighbor : graph_.neighbors(vertex)) {
            ++stats.role_adjacency_checks;
            if (!is_core_[neighbor] && memberships_[neighbor].empty()) {
                hub_candidates.insert(neighbor);
            }
        }
    }

    std::vector<VertexId> ordered_hub_candidates(
        hub_candidates.begin(), hub_candidates.end());
    std::sort(
        ordered_hub_candidates.begin(), ordered_hub_candidates.end());
    for (const VertexId vertex : ordered_hub_candidates) {
        if (is_core_[vertex] || !memberships_[vertex].empty()) {
            continue;
        }
        std::set<std::int64_t> neighboring_components;
        for (const VertexId neighbor : graph_.neighbors(vertex)) {
            ++stats.role_adjacency_checks;
            neighboring_components.insert(
                memberships_[neighbor].begin(), memberships_[neighbor].end());
            if (neighboring_components.size() >= 2) {
                break;
            }
        }
        roles_[vertex] = neighboring_components.size() >= 2
                             ? VertexRole::Hub
                             : VertexRole::Outlier;
        refreshed_non_core_vertices.insert(vertex);
    }
    stats.role_boundary_vertices = refreshed_non_core_vertices.size();
}

void BatchExactClusteringState::rebuild_roles_globally(
    BatchUpdateStats& stats) {
    memberships_.assign(graph_.vertex_count(), {});
    roles_.assign(graph_.vertex_count(), VertexRole::Outlier);

    for (VertexId vertex = 0; vertex < graph_.vertex_count(); ++vertex) {
        if (is_core_[vertex]) {
            memberships_[vertex].push_back(core_component_[vertex]);
            roles_[vertex] = VertexRole::Core;
            continue;
        }
        ++stats.role_boundary_vertices;
        std::set<std::int64_t> components;
        for (const VertexId neighbor : graph_.neighbors(vertex)) {
            ++stats.role_adjacency_checks;
            if (!is_core_[neighbor]) {
                continue;
            }
            const auto state = edge_states_.find(Edge(vertex, neighbor));
            if (state != edge_states_.end() && state->second.similar) {
                components.insert(core_component_[neighbor]);
            }
        }
        memberships_[vertex].assign(components.begin(), components.end());
        if (!memberships_[vertex].empty()) {
            roles_[vertex] = VertexRole::Border;
        }
    }

    for (VertexId vertex = 0; vertex < graph_.vertex_count(); ++vertex) {
        if (is_core_[vertex] || !memberships_[vertex].empty()) {
            continue;
        }
        std::set<std::int64_t> neighboring_components;
        for (const VertexId neighbor : graph_.neighbors(vertex)) {
            ++stats.role_adjacency_checks;
            neighboring_components.insert(
                memberships_[neighbor].begin(), memberships_[neighbor].end());
            if (neighboring_components.size() >= 2) {
                break;
            }
        }
        roles_[vertex] = neighboring_components.size() >= 2
                             ? VertexRole::Hub
                             : VertexRole::Outlier;
    }
}

}  // namespace basw
