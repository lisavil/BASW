#include "basw/window_engine.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace basw {

WindowEngine::WindowEngine(
    std::vector<TemporalEvent> events,
    Timestamp window_width,
    Timestamp slide_interval,
    Timestamp initial_time)
    : events_(std::move(events)),
      window_width_(window_width),
      slide_interval_(slide_interval),
      current_time_(initial_time) {
    if (window_width_ <= 0) {
        throw std::invalid_argument("window width must be positive");
    }
    if (slide_interval_ <= 0 || slide_interval_ > window_width_) {
        throw std::invalid_argument(
            "slide interval must satisfy 0 < delta <= window width");
    }
    if (!std::is_sorted(
            events_.begin(), events_.end(),
            [](const TemporalEvent& left, const TemporalEvent& right) {
                return left.timestamp < right.timestamp;
            })) {
        throw std::invalid_argument(
            "temporal events must be sorted by nondecreasing timestamp");
    }

    const Timestamp lower_bound = current_time_ - window_width_;
    while (active_begin_ < events_.size() &&
           events_[active_begin_].timestamp <= lower_bound) {
        ++active_begin_;
    }
    active_end_ = active_begin_;
    while (active_end_ < events_.size() &&
           events_[active_end_].timestamp <= current_time_) {
        ++active_end_;
    }
}

Timestamp WindowEngine::current_time() const noexcept {
    return current_time_;
}

Timestamp WindowEngine::window_width() const noexcept {
    return window_width_;
}

Timestamp WindowEngine::slide_interval() const noexcept {
    return slide_interval_;
}

std::vector<TemporalEvent> WindowEngine::initial_events() const {
    return {events_.begin() + static_cast<std::ptrdiff_t>(active_begin_),
            events_.begin() + static_cast<std::ptrdiff_t>(active_end_)};
}

bool WindowEngine::has_pending_events() const noexcept {
    return active_begin_ < active_end_ || active_end_ < events_.size();
}

WindowTransition WindowEngine::advance() {
    const Timestamp old_time = current_time_;
    const Timestamp new_time = current_time_ + slide_interval_;
    const Timestamp expiration_boundary = new_time - window_width_;

    WindowTransition transition{old_time, new_time, {}, {}};

    while (active_begin_ < active_end_ &&
           events_[active_begin_].timestamp <= expiration_boundary) {
        transition.expired.push_back(events_[active_begin_]);
        ++active_begin_;
    }

    while (active_end_ < events_.size() &&
           events_[active_end_].timestamp <= new_time) {
        // active_end_ starts at the first event strictly after old_time, so
        // arrivals implement old_time < timestamp <= new_time.
        transition.arrived.push_back(events_[active_end_]);
        ++active_end_;
    }

    current_time_ = new_time;
    return transition;
}

}  // namespace basw
