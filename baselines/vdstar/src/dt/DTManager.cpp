#include "DTManager.h"

DTManager::~DTManager() {
    for (int i = 0; i < dtInstanceList.size(); ++i) {
        delete dtInstanceList[i];
    }
    dtInstanceList.release_space();
}

DTInstance *DTManager::removeInstance(int _indexToDel) {
    int length = dtInstanceList.size();
    DTInstance *removedInstance = dtInstanceList[_indexToDel];
    DTInstance *movedInstance = nullptr;
    if (_indexToDel != length - 1) {
        movedInstance = dtInstanceList[length - 1];
        dtInstanceList[_indexToDel] = movedInstance;
        movedInstance->set_dt_index(_indexToDel);
    }
    dtInstanceList.pop_back();
    delete removedInstance;
    return movedInstance;
}
