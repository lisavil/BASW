#include "basw/event_parser.hpp"

#include <charconv>
#include <fstream>
#include <limits>
#include <optional>
#include <sstream>
#include <string_view>
#include <unordered_map>

namespace basw {
namespace {

Timestamp parse_timestamp(
    const std::string& token,
    std::size_t line_number) {
    Timestamp value{};
    const char* begin = token.data();
    const char* end = token.data() + token.size();
    const auto [position, error] = std::from_chars(begin, end, value);
    if (error != std::errc{} || position != end) {
        throw EventParseError(line_number, "invalid signed 64-bit timestamp");
    }
    return value;
}

bool is_ignored_line(const std::string& line) {
    const std::size_t first = line.find_first_not_of(" \t\r\n");
    return first == std::string::npos || line[first] == '#';
}

}  // namespace

EventParseError::EventParseError(
    std::size_t line_number,
    const std::string& message)
    : std::runtime_error(
          "event parse error on line " + std::to_string(line_number) +
          ": " + message),
      line_number_(line_number) {}

std::size_t EventParseError::line_number() const noexcept {
    return line_number_;
}

ParsedEventStream parse_event_stream(std::istream& input) {
    ParsedEventStream result;
    std::unordered_map<std::string, VertexId> dense_ids;
    std::optional<Timestamp> previous_timestamp;
    std::string line;

    while (std::getline(input, line)) {
        ++result.stats.total_lines;
        if (is_ignored_line(line)) {
            ++result.stats.ignored_lines;
            continue;
        }

        std::istringstream fields(line);
        std::string timestamp_token;
        std::string source;
        std::string destination;
        std::string extra;
        if (!(fields >> timestamp_token >> source >> destination) ||
            (fields >> extra)) {
            throw EventParseError(
                result.stats.total_lines,
                "expected exactly: timestamp source destination");
        }

        const Timestamp timestamp =
            parse_timestamp(timestamp_token, result.stats.total_lines);
        if (previous_timestamp.has_value() &&
            timestamp < previous_timestamp.value()) {
            throw EventParseError(
                result.stats.total_lines,
                "timestamps must be nondecreasing");
        }
        previous_timestamp = timestamp;
        ++result.stats.data_lines;

        if (source == destination) {
            ++result.stats.self_loops_skipped;
            continue;
        }

        const auto get_dense_id = [&](const std::string& original) {
            const auto found = dense_ids.find(original);
            if (found != dense_ids.end()) {
                return found->second;
            }
            if (result.original_vertex_ids.size() >
                std::numeric_limits<VertexId>::max()) {
                throw EventParseError(
                    result.stats.total_lines,
                    "vertex count exceeds the uint32 dense-id range");
            }
            const VertexId dense =
                static_cast<VertexId>(result.original_vertex_ids.size());
            result.original_vertex_ids.push_back(original);
            dense_ids.emplace(original, dense);
            return dense;
        };

        const VertexId u = get_dense_id(source);
        const VertexId v = get_dense_id(destination);
        result.events.emplace_back(timestamp, u, v);
    }

    if (input.bad()) {
        throw std::runtime_error("I/O error while reading temporal events");
    }
    return result;
}

ParsedEventStream parse_event_file(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error(
            "cannot open temporal event file: " + path.string());
    }
    return parse_event_stream(input);
}

}  // namespace basw
