#pragma once

#include "basw/types.hpp"

#include <cstddef>
#include <vector>

namespace basw {

struct WindowTransition {
    Timestamp old_time{};
    Timestamp new_time{};
    std::vector<TemporalEvent> expired;
    std::vector<TemporalEvent> arrived;

    bool operator==(const WindowTransition&) const = default;
};

class WindowEngine {
public:
    WindowEngine(
        std::vector<TemporalEvent> events,
        Timestamp window_width,
        Timestamp slide_interval,
        Timestamp initial_time);

    [[nodiscard]] Timestamp current_time() const noexcept;
    [[nodiscard]] Timestamp window_width() const noexcept;
    [[nodiscard]] Timestamp slide_interval() const noexcept;

    // Events in the initial window (initial_time-W, initial_time].
    [[nodiscard]] std::vector<TemporalEvent> initial_events() const;

    // True while a future event can arrive or a currently active event can expire.
    [[nodiscard]] bool has_pending_events() const noexcept;

    // Advances exactly one slide and returns only events crossing a window boundary.
    WindowTransition advance();

private:
    std::vector<TemporalEvent> events_;
    Timestamp window_width_{};
    Timestamp slide_interval_{};
    Timestamp current_time_{};
    std::size_t active_begin_{};
    std::size_t active_end_{};
};

}  // namespace basw
