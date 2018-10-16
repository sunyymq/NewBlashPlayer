#include "DEDataManager.h"
#include "DEMemDataManager.h"

DEDataManager::DEDataManager()
    :mSegmentBuffer(NULL) {

}

DEDataManager::~DEDataManager() {

}

int DEDataManager::initContext() {
    return FAILURE;
}

void DEDataManager::uninitContext() {
    
}

DEDataManager* DEDataManager::Create() {
    DEDataManager* dataManager = new DEMemDataManager();
    dataManager->initContext();
    return dataManager;
}

void DEDataManager::Destroy(DEDataManager* dataManager) {
    if (dataManager == NULL) {
        return ;
    }
    dataManager->uninitContext();
    delete dataManager;
}
