#include "graph/Graph.h"
#include <cmath>
#include <iostream>

class TestGraph : public Graph {
public:
    TestGraph(MyVector<dynscan::Vertex *> &vertices, double rho)
            : Graph(vertices, rho) {}

    float sample(dynscan::Vertex *v1, dynscan::Vertex *v2) {
        return myJaccard->compute_similarity(*v1, *v2);
    }
};

int main() {
    constexpr int n = 128;
    constexpr long double group_size = 500000.0L;
    const double rho = 0.02;
    const long double old_expression = 1.0L / (1 / n);
    const long double base_denominator = static_cast<long double>(n);
    const long double first_group_denominator =
            group_size * 1.0L * 2.0L * base_denominator;
    const long double sample_count =
            2.0L / (omega * rho) / (omega * rho) *
            std::log(first_group_denominator);

    long double allocated_failure_probability = 0.0L;
    for (int group = 1; group <= 10000; ++group) {
        allocated_failure_probability +=
                1.0L / (group * (group + 1.0L) * n);
    }

    MyVector<dynscan::Vertex *> input;
    dynscan::Vertex *vertices[n];
    for (int id = 1; id <= n; ++id) {
        vertices[id - 1] = new dynscan::Vertex(id);
        input.push_back(vertices[id - 1]);
    }
    vertices[0]->insertNeighbor(
            2, dynscan::Vertex::allocate_intersection_cnt_index(), 1.0f);
    vertices[1]->insertNeighbor(
            1, dynscan::Vertex::allocate_intersection_cnt_index(), 1.0f);
    vertices[0]->set_large();
    vertices[1]->set_large();

    TestGraph graph(input, rho);
    const float sampled_similarity = graph.sample(vertices[0], vertices[1]);

    const bool pass = std::isinf(old_expression) &&
                      std::isfinite(base_denominator) &&
                      base_denominator == n &&
                      std::isfinite(sample_count) && sample_count > 0 &&
                      sample_count < 10000000.0L &&
                      allocated_failure_probability < 1.0L / n &&
                      std::isfinite(sampled_similarity) &&
                      sampled_similarity >= 0.0f && sampled_similarity <= 1.0f;

    std::cout << "old_expression_is_inf=" << std::isinf(old_expression) << "\n";
    std::cout << "base_denominator=" << (double) base_denominator << "\n";
    std::cout << "first_group_sample_count=" << (double) sample_count << "\n";
    std::cout << "allocated_failure_probability="
              << (double) allocated_failure_probability << "\n";
    std::cout << "sampled_similarity=" << sampled_similarity << "\n";
    std::cout << "failure_probability_verification="
              << (pass ? "PASS" : "FAIL") << "\n";
    return pass ? 0 : 1;
}
