#include "graph/Graph.h"
#include <iostream>

class TestGraph : public Graph {
public:
    TestGraph(MyVector<dynscan::Vertex *> &vertices, double rho)
            : Graph(vertices, rho) {}

    void add_test_instance(dynscan::Vertex *v1, dynscan::Vertex *v2) {
        const int dtIndex = dtManager.get_size();
        DTInstance *instance = new DTInstance(
                0.0002, 10000, v1->getCnt(), v2->getCnt(),
                v1->id, v2->id, dtIndex);
        dtManager.insertInstance(instance);
        const int bucket = instance->get_exp();
        v1->addDTBucketElement(bucket, instance->get_element1(), v1->getCnt());
        v2->addDTBucketElement(bucket, instance->get_element2(), v2->getCnt());
        v1->set_instance_index_map_by_neighbor_id(v2->id, dtIndex);
        v2->set_instance_index_map_by_neighbor_id(v1->id, dtIndex);
    }

    int manager_size() const { return dtManager.get_size(); }

    DTInstance *instance_at(int index) { return dtManager.get_instance(index); }
};

int main() {
    MyVector<dynscan::Vertex *> vertices;
    for (int id = 1; id <= 4; ++id) {
        vertices.push_back(new dynscan::Vertex(id));
    }
    dynscan::Vertex *v1 = vertices[0];
    dynscan::Vertex *v2 = vertices[1];
    dynscan::Vertex *v3 = vertices[2];
    dynscan::Vertex *v4 = vertices[3];

    v1->insertNeighbor(2, dynscan::Vertex::allocate_intersection_cnt_index(), 0.5f);
    v2->insertNeighbor(1, dynscan::Vertex::allocate_intersection_cnt_index(), 0.5f);
    v3->insertNeighbor(4, dynscan::Vertex::allocate_intersection_cnt_index(), 0.5f);
    v4->insertNeighbor(3, dynscan::Vertex::allocate_intersection_cnt_index(), 0.5f);

    TestGraph graph(vertices, 0.02);
    graph.add_test_instance(v1, v2);
    graph.add_test_instance(v3, v4);

    const int remove_status = graph.removeEdge(1, 2);
    DTInstance *moved = graph.instance_at(0);
    const bool pass = remove_status == 0 && graph.manager_size() == 1 &&
                      v1->get_instance_index_by_neighbor_id(2) == -1 &&
                      v2->get_instance_index_by_neighbor_id(1) == -1 &&
                      v3->get_instance_index_by_neighbor_id(4) == 0 &&
                      v4->get_instance_index_by_neighbor_id(3) == 0 &&
                      moved->get_element1()->get_dt_index() == 0 &&
                      moved->get_element2()->get_dt_index() == 0;
    std::cout << "graph_swap_delete_verification=" << (pass ? "PASS" : "FAIL") << "\n";

    graph.removeEdge(3, 4);
    return pass ? 0 : 1;
}
