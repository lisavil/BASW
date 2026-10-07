#include "graph/Graph.h"
#include <cmath>
#include <iostream>

class TestGraph : public Graph {
public:
    TestGraph(MyVector<dynscan::Vertex *> &vertices, double rho)
            : Graph(vertices, rho) {}

    void add_test_instance(dynscan::Vertex *v1, dynscan::Vertex *v2) {
        const int dt_index = dtManager.get_size();
        const int union_lower_bound =
                1 + std::max(v1->getDegree(), v2->getDegree());
        DTInstance *instance = new DTInstance(
                (1 - omega) * rho * rho, union_lower_bound,
                v1->getCnt(), v2->getCnt(), v1->id, v2->id, dt_index);
        dtManager.insertInstance(instance);
        const int bucket = instance->get_exp();
        v1->addDTBucketElement(bucket, instance->get_element1(), v1->getCnt());
        v2->addDTBucketElement(bucket, instance->get_element2(), v2->getCnt());
        v1->set_instance_index_map_by_neighbor_id(v2->id, dt_index);
        v2->set_instance_index_map_by_neighbor_id(v1->id, dt_index);
    }

    void tick(dynscan::Vertex *vertex) {
        checkVertexDTBucket(vertex);
    }

    int first_tau() {
        return dtManager.get_instance(0)->get_tau();
    }
};

int main() {
    const double rho = 0.02;
    dynscan::Vertex *v1 = new dynscan::Vertex(1);
    dynscan::Vertex *v2 = new dynscan::Vertex(2);

    const int edge_index_1 = dynscan::Vertex::allocate_intersection_cnt_index();
    const int edge_index_2 = dynscan::Vertex::allocate_intersection_cnt_index();
    v1->setIntersectionCnt(2, edge_index_1);
    v2->setIntersectionCnt(2, edge_index_2);
    v1->insertNeighbor(2, edge_index_1, 1.0f);
    v2->insertNeighbor(1, edge_index_2, 1.0f);

    for (int i = 0; i < 9999; ++i) {
        v1->insertNeighbor(100 + i,
                           dynscan::Vertex::allocate_intersection_cnt_index(),
                           0.0f);
        v2->insertNeighbor(20000 + i,
                           dynscan::Vertex::allocate_intersection_cnt_index(),
                           0.0f);
    }

    MyVector<dynscan::Vertex *> input;
    input.push_back(v1);
    input.push_back(v2);
    TestGraph graph(input, rho);
    graph.add_test_instance(v1, v2);

    const int initial_tau = graph.first_tau();
    graph.tick(v1);
    const int before_maturity_tau = graph.first_tau();
    graph.tick(v1);
    const int reset_tau = graph.first_tau();

    const int union_lower_bound = 1 + std::max(v1->getDegree(), v2->getDegree());
    const int expected_tau = (int) std::floor(
            ((1 - omega) * rho * rho) * union_lower_bound / 2) + 1;
    const int raw_rho_tau = (int) std::floor(rho * union_lower_bound / 2) + 1;
    const bool pass = initial_tau == expected_tau &&
                      before_maturity_tau == 1 &&
                      reset_tau == expected_tau &&
                      reset_tau != raw_rho_tau;

    std::cout << "initial_tau=" << initial_tau << "\n";
    std::cout << "before_maturity_tau=" << before_maturity_tau << "\n";
    std::cout << "reset_tau=" << reset_tau << "\n";
    std::cout << "raw_rho_tau=" << raw_rho_tau << "\n";
    std::cout << "affordability_reset_verification="
              << (pass ? "PASS" : "FAIL") << "\n";
    return pass ? 0 : 1;
}
