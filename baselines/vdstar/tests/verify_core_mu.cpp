#include "graph/Vertex.h"
#include <iostream>

static void add_neighbor(dynscan::Vertex &v, int neighbor, float similarity) {
    v.insertNeighbor(neighbor,
                     dynscan::Vertex::allocate_intersection_cnt_index(),
                     similarity);
}

int main() {
    const double eps = 0.50;
    const int mu = 3;

    dynscan::Vertex insufficient(1);
    add_neighbor(insufficient, 2, 0.90f);
    add_neighbor(insufficient, 3, 0.10f);
    add_neighbor(insufficient, 4, 0.10f);
    add_neighbor(insufficient, 5, 0.10f);

    dynscan::Vertex boundary(10);
    add_neighbor(boundary, 11, 0.90f);
    add_neighbor(boundary, 12, 0.80f);
    add_neighbor(boundary, 13, 0.70f);

    dynscan::Vertex exactly_three(20);
    add_neighbor(exactly_three, 21, 0.90f);
    add_neighbor(exactly_three, 22, 0.80f);
    add_neighbor(exactly_three, 23, 0.70f);
    add_neighbor(exactly_three, 24, 0.10f);

    const int insufficient_result = insufficient.query(eps, mu);
    const int boundary_result = boundary.query(eps, mu);
    const int exactly_three_result = exactly_three.query(eps, mu);
    const bool pass = insufficient_result == 0 &&
                      boundary.getDegree() == mu && boundary_result >= mu &&
                      exactly_three_result >= mu;

    std::cout << "insufficient_result=" << insufficient_result << "\n";
    std::cout << "degree_equal_mu_result=" << boundary_result << "\n";
    std::cout << "exactly_mu_similar_result=" << exactly_three_result << "\n";
    std::cout << "core_mu_verification=" << (pass ? "PASS" : "FAIL") << "\n";
    return pass ? 0 : 1;
}
