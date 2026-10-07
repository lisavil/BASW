#include "graph/Vertex.h"
#include <iostream>

int main() {
    dynscan::Vertex v(1);
    const int i2 = dynscan::Vertex::allocate_intersection_cnt_index();
    const int i3 = dynscan::Vertex::allocate_intersection_cnt_index();
    const int i4 = dynscan::Vertex::allocate_intersection_cnt_index();
    v.insertNeighbor(2, i2, 0.2f);
    v.insertNeighbor(3, i3, 0.3f);
    v.insertNeighbor(4, i4, 0.4f);
    v.setIntersectionCnt(20, i2);
    v.setIntersectionCnt(30, i3);
    v.setIntersectionCnt(40, i4);

    v.deleteNeighbor(2);
    const int moved_index = v.getAdjacentIndex(4);
    const bool ok = v.getDegree() == 2 &&
                    v.getAdjacentIndex(2) == -1 &&
                    moved_index == 0 &&
                    v.getIntersectionCnt(moved_index) == 40 &&
                    v.NOPtr->size() == 2;
    std::cout << "degree=" << v.getDegree() << "\n";
    std::cout << "moved_neighbor_index=" << moved_index << "\n";
    std::cout << "moved_intersection_count="
              << v.getIntersectionCnt(moved_index) << "\n";
    std::cout << "sorted_neighbor_size=" << v.NOPtr->size() << "\n";
    std::cout << "verification=" << (ok ? "PASS" : "FAIL") << "\n";
    return ok ? 0 : 1;
}
