#pragma once

#include "basw/types.hpp"

#include <cstddef>
#include <filesystem>
#include <istream>
#include <stdexcept>
#include <string>
#include <vector>

namespace basw {

struct EventParserStats {
    std::size_t total_lines{};
    std::size_t ignored_lines{};
    std::size_t data_lines{};
    std::size_t self_loops_skipped{};
};

struct ParsedEventStream {
    std::vector<TemporalEvent> events;

    // original_vertex_ids[dense_id] gives the input label.
    std::vector<std::string> original_vertex_ids;
    EventParserStats stats;
};

class EventParseError : public std::runtime_error {
public:
    EventParseError(std::size_t line_number, const std::string& message);

    [[nodiscard]] std::size_t line_number() const noexcept;

private:
    std::size_t line_number_{};
};

[[nodiscard]] ParsedEventStream parse_event_stream(std::istream& input);

[[nodiscard]] ParsedEventStream parse_event_file(
    const std::filesystem::path& path);

}  // namespace basw
