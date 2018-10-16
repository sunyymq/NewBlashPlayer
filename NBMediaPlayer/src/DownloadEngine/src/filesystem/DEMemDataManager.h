#ifndef DE_MEM_DATAMANAGER_H
#define DE_MEM_DATAMANAGER_H

#include "DEDataManager.h"

class DEMemDataManager : public DEDataManager {
public:
    virtual int open(const TaskConfig* taskConfig);
    virtual void close();

    // read&write data
    virtual ssize_t read(const TaskConfig* taskConfig, void* buf, size_t buf_size, int64_t curr_read_pos);
    virtual size_t write(const TaskConfig* taskConfig, void *buf, size_t trunck_size, uint32_t trunck_index, int64_t offs);
    
    // free space is enough
    virtual bool isEnoughFreespace(int needSpaceCount);
    
    // whether the request pos is cached or not
    virtual bool isPositionCached(size_t bufferSize, int64_t pos);

    // seek support, operation on cache buffers
    virtual int acquireBuffer(size_t reqLen, uint8_t** pOutBuf);
    virtual void releaseBuffer(int trunck_index);
    virtual void purgeBuffers();

public:
    DEMemDataManager();
    virtual ~DEMemDataManager();

    virtual int initContext();
    virtual void uninitContext();
};

#endif
