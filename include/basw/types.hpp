#pragma once

#include <cstdint>
#include <functional>
#include <stdexcept>

namespace basw {

using VertexId = std::uint32_t;
using Timestamp = std::int64_t;

struct Edge {
    VertexId u{};
    VertexId v{};

    Edge(VertexId first, VertexId second)
        : u(first < second ? first : second),
          v(first < second ? second : first) {
        if (first == second) {
            throw std::invalid_argument("self-loops are not supported");
        }
    }

    bool operator==(const Edge&) const = default;
};

struct EdgeHash {
    std::size_t operator()(const Edge& edge) const noexcept {
        const auto high = static_cast<std::uint64_t>(edge.u) << 32U;
        return std::hash<std::uint64_t>{}(high | edge.v);
    }
};

struct TemporalEvent {
    Timestamp timestamp{};
    Edge edge;

    TemporalEvent(Timestamp time, VertexId first, VertexId second)
        : timestamp(time), edge(first, second) {}

    bool operator==(const TemporalEvent&) const = default;
};

struct RationalThreshold {
    std::uint64_t numerator{};
    std::uint64_t denominator{1};

    RationalThreshold(std::uint64_t p, std::uint64_t q)
        : numerator(p), denominator(q) {
        if (q == 0 || p > q) {
            throw std::invalid_argument(
                "epsilon must satisfy 0 <= numerator <= denominator and denominator > 0");
        }
    }

    bool operator==(const RationalThreshold&) const = default;
};

struct SimilarityFraction {
    std::uint64_t intersection{};
    std::uint64_t union_size{};

    bool operator==(const SimilarityFraction&) const = default;
};

}  // namespace basw
