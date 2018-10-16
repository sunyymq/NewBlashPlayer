#include "DEMemDataManager.h"
#include "utils/DELRUCache.h"
#include "task/DEBaseTask.h"

#include <NBLog.h>
#include <NBSet.h>

#define LOG_TAG "DEMemDataManager"

DEMemDataManager::DEMemDataManager()
    :DEDataManager() { 

    }

DEMemDataManager::~DEMemDataManager() {

}

int DEMemDataManager::initContext() {

    return 0;
}

void DEMemDataManager::uninitContext() {

}

int DEMemDataManager::open(const TaskConfig* taskConfig) {
    mSegmentBuffer = new uint8_t[DEFAULT_SEGMENT_BUFFER_SIZE];

    int maxSegmentIndex = DEFAULT_SEGMENT_BUFFER_SIZE / taskConfig->defaultBufferSize;

    for (int i = 0; i < maxSegmentIndex; ++i) {
        mFreeSegmentIndex.insert(i);
    }

    mReadCache = new DELRUCache<int>(taskConfig->maxCacheSegmentNum * taskConfig->maxParallelSize / taskConfig->defaultBufferSize);

    return 0;
}

void DEMemDataManager::close() {
    mFreeSegmentIndex.clear();
    mDownloadedSegments.clear();

    delete []mSegmentBuffer;
    delete mReadCache;

    NBLOG_INFO(LOG_TAG, "DEMemDataManager::close cache");
}

ssize_t DEMemDataManager::read(const TaskConfig* taskConfig, void* buf, size_t buf_size, int64_t curr_read_pos) {
    ssize_t read_size = 0;
    
//    if (curr_read_pos >= task_item->task_size) {
//        NBLOG_INFO(LOG_TAG, "read reached the end");
//        return -1;
//    }

    NBMap<int, DE_SEGMENT_ITEM>::const_iterator iter = mDownloadedSegments.find(curr_read_pos / taskConfig->defaultBufferSize);
    if (iter == mDownloadedSegments.end()) {
        return 0;
    }

    read_size = std::min(iter->second.trunck_size - (size_t)((curr_read_pos - iter->second.trunck_index * taskConfig->defaultBufferSize)), buf_size);

    memcpy(buf, mSegmentBuffer + iter->second.offset_begin_index * taskConfig->defaultBufferSize + (curr_read_pos - iter->second.trunck_index * taskConfig->defaultBufferSize), read_size);

    int replacedValue = mReadCache->set(iter->second.trunck_index);
    
    if (replacedValue != DELRUCache<int>::INVALID_VALUE) {
        NBMap<int, DE_SEGMENT_ITEM>::const_iterator riter = mDownloadedSegments.find(replacedValue);

        if (riter != mDownloadedSegments.end()) {
            mFreeSegmentIndex.insert(riter->second.offset_begin_index);
            mDownloadedSegments.erase(riter);
        }
    }

    return read_size;
}

size_t DEMemDataManager::write(const TaskConfig* taskConfig, void *buf, size_t trunck_size, uint32_t trunck_index, int64_t offs) {
    DE_SEGMENT_ITEM segmentItem;
    segmentItem.trunck_index = trunck_index;
    segmentItem.offset_begin_index = offs;
    segmentItem.trunck_size = trunck_size;

    mDownloadedSegments[trunck_index] = segmentItem;
    return trunck_size;
}

bool DEMemDataManager::isPositionCached(size_t bufferSize, int64_t pos) {

    NBMap<int, DE_SEGMENT_ITEM>::const_iterator cit = mDownloadedSegments.find(pos / bufferSize);

    if (cit != mDownloadedSegments.end()) {
        return true;
    }

    return false;
}

bool DEMemDataManager::isEnoughFreespace(int needSpace) {
    NBLOG_INFO(LOG_TAG, "MemDataManger freeSpace : %d Downloaded size : %d needSpace : %d", mFreeSegmentIndex.size(), mDownloadedSegments.size(), needSpace);
    return mFreeSegmentIndex.size() >= needSpace;
}

int DEMemDataManager::acquireBuffer(size_t reqLen, uint8_t** pOutBuf) {
    NBSet<int>::const_iterator iter = mFreeSegmentIndex.begin();

    if (iter == mFreeSegmentIndex.end()) {
        NBLOG_ERROR(LOG_TAG, "acquireBuffer never be here");
        *pOutBuf = NULL;
        return -1;
    }

    int offset_begin_index = *iter;

    if (pOutBuf != NULL) {
        *pOutBuf = mSegmentBuffer + offset_begin_index * reqLen; // task_item->taskConfig.defaultBufferSize;
    }

    mFreeSegmentIndex.erase(iter);

    return offset_begin_index;
}

void DEMemDataManager::releaseBuffer(int trunck_index) {
    mFreeSegmentIndex.insert(trunck_index);
}

void DEMemDataManager::purgeBuffers() {
    NBMap<int, DE_SEGMENT_ITEM>::const_iterator iter = mDownloadedSegments.begin();
    while(iter != mDownloadedSegments.end()) {
        int alreadyRead = mReadCache->get(iter->second.trunck_index);

        //remove the node we not accessed
        if (alreadyRead == DELRUCache<int>::INVALID_VALUE) {
            NBSet<int>::const_iterator cit = mFreeSegmentIndex.find(iter->second.offset_begin_index);

            mFreeSegmentIndex.insert(iter->second.offset_begin_index);

            mDownloadedSegments.erase(iter++);
        } else {
            ++iter;
        }
    }
}
