#ifndef DE_DATAMANGER_H
#define DE_DATAMANGER_H

#include "common/DEInternal.h"
#include "utils/DELRUCache.h"
#include "task/DEBaseTask.h"

#include <NBLog.h>
#include <NBSet.h>

typedef struct DE_SEGMENT_ITEM {
    //The offset index in the whole file
    uint32_t trunck_index;
    //The offset index in downloaded cache position
    int offset_begin_index;
    //The segment item size
    size_t trunck_size;
    
//    DE_SEGMENT_ITEM() {
//        trunck_index = -1;
//        offset_begin_index = 0;
//        trunck_size = 0;
//    }
} DE_SEGMENT_ITEM;

class DEDataManager {
public:
    static DEDataManager* Create();
    static void Destroy(DEDataManager* dataManager);

public:
    virtual int open(const TaskConfig* taskConfig) = 0;
    virtual void close() = 0;

    // read&write data
    virtual ssize_t read(const TaskConfig* taskConfig, void* buf, size_t buf_size, int64_t curr_read_pos) = 0;
    virtual size_t write(const TaskConfig* taskConfig, void *buf, size_t trunck_size, uint32_t trunck_index, int64_t offs) = 0;
    
    // free space is enough
    virtual bool isEnoughFreespace(int needSpaceCount) = 0;
    
    // whether the request pos is cached or not
    virtual bool isPositionCached(size_t bufferSize, int64_t pos) = 0;

    // seek support, operations on cached buffers
    virtual int acquireBuffer(size_t reqLen, uint8_t** pOutBuf) = 0;
    virtual void releaseBuffer(int trunck_index) = 0;
    virtual void purgeBuffers() = 0;

protected:
    DEDataManager();
    virtual ~DEDataManager();

    virtual int initContext();
    virtual void uninitContext();
    
protected:
    uint8_t* mSegmentBuffer;
    
    NBMap<int, DE_SEGMENT_ITEM> mDownloadedSegments;
    
    NBSet<int> mFreeSegmentIndex;
    
    DELRUCache<int>* mReadCache;
};

#endif
