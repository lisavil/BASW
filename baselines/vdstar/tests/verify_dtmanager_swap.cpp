#include "dt/DTManager.h"
#include <iostream>

int main() {
    DTManager manager;
    DTInstance *first = new DTInstance(0.0002, 10000, 0, 0, 1, 2, 0);
    DTInstance *second = new DTInstance(0.0002, 10000, 0, 0, 3, 4, 1);
    manager.insertInstance(first);
    manager.insertInstance(second);

    DTInstance *moved = manager.removeInstance(0);
    const bool pass = moved == second && manager.get_size() == 1 &&
                      manager.get_instance(0) == second &&
                      second->get_element1()->get_dt_index() == 0 &&
                      second->get_element2()->get_dt_index() == 0;
    std::cout << "manager_swap_verification=" << (pass ? "PASS" : "FAIL") << "\n";
    manager.removeInstance(0);
    return pass ? 0 : 1;
}
