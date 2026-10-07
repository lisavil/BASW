#include "basw/event_parser.hpp"
#include "basw/sequential_experiment.hpp"

#include <charconv>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

template <typename Integer>
Integer parse_integer(std::string_view text, const char* name) {
    Integer value{};
    const char* begin = text.data();
    const char* end = text.data() + text.size();
    const auto [position, error] = std::from_chars(begin, end, value);
    if (error != std::errc{} || position != end) {
        throw std::invalid_argument(std::string("invalid ") + name + ": " +
                                    std::string(text));
    }
    return value;
}

void print_usage(const char* program) {
    std::cerr
        << "Usage: " << program
        << " INPUT OUTPUT W DELTA INITIAL_TIME EPS_NUM EPS_DEN MU MAX_SLIDES"
           " [METRICS_CSV]\n";
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 10 && argc != 11) {
        print_usage(argv[0]);
        return 2;
    }
    try {
        const std::filesystem::path input_path = argv[1];
        const std::filesystem::path output_path = argv[2];
        const basw::Timestamp window_width =
            parse_integer<basw::Timestamp>(argv[3], "window width");
        const basw::Timestamp slide_interval =
            parse_integer<basw::Timestamp>(argv[4], "slide interval");
        const basw::Timestamp initial_time =
            parse_integer<basw::Timestamp>(argv[5], "initial time");
        const std::uint64_t epsilon_numerator =
            parse_integer<std::uint64_t>(argv[6], "epsilon numerator");
        const std::uint64_t epsilon_denominator =
            parse_integer<std::uint64_t>(argv[7], "epsilon denominator");
        const std::uint64_t mu =
            parse_integer<std::uint64_t>(argv[8], "mu");
        const std::uint64_t max_slides_raw =
            parse_integer<std::uint64_t>(argv[9], "max slides");
        if (max_slides_raw > std::numeric_limits<std::size_t>::max()) {
            throw std::invalid_argument("max slides exceeds the platform range");
        }

        const basw::ParsedEventStream parsed = basw::parse_event_file(input_path);
        if (!output_path.parent_path().empty()) {
            std::filesystem::create_directories(output_path.parent_path());
        }
        std::ofstream output(output_path);
        if (!output) {
            throw std::runtime_error(
                "cannot open output file: " + output_path.string());
        }

        std::ofstream metrics;
        std::ostream* metrics_output = nullptr;
        if (argc == 11) {
            const std::filesystem::path metrics_path = argv[10];
            if (!metrics_path.parent_path().empty()) {
                std::filesystem::create_directories(metrics_path.parent_path());
            }
            metrics.open(metrics_path);
            if (!metrics) {
                throw std::runtime_error(
                    "cannot open metrics file: " + metrics_path.string());
            }
            metrics_output = &metrics;
        }

        const basw::SequentialExperimentConfig config{
            window_width,
            slide_interval,
            initial_time,
            basw::RationalThreshold(epsilon_numerator, epsilon_denominator),
            mu,
            static_cast<std::size_t>(max_slides_raw),
        };
        const basw::SequentialExperimentSummary summary =
            basw::run_sequential_experiment(
                parsed, config, output, metrics_output);
        std::cout << "vertices=" << parsed.original_vertex_ids.size()
                  << " events=" << parsed.events.size()
                  << " self_loops_skipped=" << parsed.stats.self_loops_skipped
                  << " snapshots=" << summary.snapshots_written
                  << " slides=" << summary.slides_processed
                  << " raw_event_changes=" << summary.raw_event_changes
                  << " effective_insertions=" << summary.effective_insertions
                  << " effective_deletions=" << summary.effective_deletions
                  << " affected_pairs=" << summary.affected_pairs
                  << " changed_similarity_edges="
                  << summary.changed_similarity_edges
                  << " transition_ns=" << summary.total_transition_ns
                  << " sequential_update_ns="
                  << summary.total_sequential_update_ns
                  << " truncated="
                  << (summary.truncated_by_slide_limit ? 1 : 0) << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
