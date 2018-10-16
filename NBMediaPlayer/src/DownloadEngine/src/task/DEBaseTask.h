#ifndef DE_BASE_TASK_H
#define DE_BASE_TASK_H

#include "common_def.h"
#include <NBString.h>
#include <NBList.h>

struct CREATE_TASK_PARAMS;

typedef enum TaskStatus {
    TaskStatusUnknow = -1,
    TaskStatusInit,
    TaskStatusRunning,
    TaskStatusPaused,
    TaskStatusFailed,
} TaskStatus;

typedef struct TaskCallback {
    void (*callback)(int, int, int, int, void*);
    void *opaque;
} TaskCallback;

typedef struct  TaskConfig {
    int maxParallelSize;
    int maxParallelNum;
    int maxSegmentNum;
    int maxCacheSegmentNum;
    int maxNetworkCacheSize;
    int defaultBufferSize;
} TaskConfig;

struct DE_TIMER_CONTEXT;
class DETimerContext;
class DELooper;
class DEDataManager;
class DENetwork;

class DEBaseTask {
public:
    DEBaseTask();
    virtual ~DEBaseTask();

public:
    static DEBaseTask* Create(const CREATE_TASK_PARAMS* task_params);
    static void Destroy(DEBaseTask* taskItem);

public:
    void reportDownloadSpeed();
    bool isProcessingPosition(int64_t postion);
    void notifyTaskListeners(int task_event, int arg1, int arg2);
    int registerTaskCallback(const TaskCallback& callback);
    void unregisterTaskCallback();

    virtual void calculateDownloadSpeed();
    virtual void launchNewSegment();
    virtual void processHeaderScheme(void* privHandle);
    virtual void queryNextDownload(int64_t pos, int64_t* download_begin, int64_t* download_end);

    virtual int prepare();
    virtual int64_t seek(int64_t off, int whence);
    // read&write data
    virtual ssize_t read(void* buf, size_t buf_size);
    virtual size_t write(void *buf, size_t trunck_size, uint32_t trunck_index, int64_t offs);

    // Life cycle control
    virtual int start();
    virtual void stop();
    
protected:
    virtual void purgeTasks();
    
    static void TaskObserverTimer(DE_TIMER_CONTEXT* timer_context, int revents, void* user_data);
    
    static size_t processOutputData(void *ptr, size_t size, size_t nmemb, void *data);

public:
    void setErrRetryCount(int retryCount) {
        mErrRetryCount = retryCount;
    }
    
    int getErrRetryCount() {
        return mErrRetryCount;
    }
    
    void setLooper(DELooper* looper) {
        mTaskLooper = looper;
    }
    
    DELooper* getLooper() {
        return mTaskLooper;
    }
    
    int getTaskId() {
        return mTaskId;
    }
    
    int64_t getTaskSize() {
        return mTaskSize;
    }
    
    void setTaskSize(int64_t taskSize) {
        mTaskSize = taskSize;
    }
    
    bool isGettingSize() {
        return mIsGettingSize;
    }
    
    void setNetwork(DENetwork* network) {
        mNetwork = network;
    }
    
    void setDataManager(DEDataManager* dataManager) {
        mDataManager = dataManager;
    }
    
    virtual int acquireBuffer(size_t reqLen, uint8_t** pOutBuf);
    virtual void releaseBuffer(int trunck_index);
    
protected:
    // download engine unique id
    int mTaskId;
    
    // Current task write position
    int64_t mCurrWritePosition;
    
    // Current task read position
    int64_t mCurrReadPosition;
    
    // Total task size
    int64_t mTaskSize;
    
    // The request with range support
    int64_t mBeginOff;
    int64_t mEndOff;
    
    // task status listener
    TaskCallback mTaskCallback;
    
    // task config eth : cache buffer, download size and so on
    TaskConfig mTaskConfig;

    // Whether size is readed
    bool mIsGettingSize;
    
    //Netowrk context
    DENetwork* mNetwork;
    
    //Manage how to process data
    DEDataManager* mDataManager;
    
    // For restart sub range tasks
    DETimerContext* mTimerCtx;
    
    // The task status
    TaskStatus mStatus;
    
    //The segment network download
    struct list_head mTaskPriv;
    
    // The task event looper
    DELooper* mTaskLooper;
    
    //创建任务的URL
    char task_url[DOWNLOAD_ENGINE_MAX_URL];
    
    //确保文件的路经，并且有足够的空间
    char save_path[DOWNLOAD_ENGINE_MAX_PATH];
    
    //保存文件的名字
    char save_name[DOWNLOAD_ENGINE_MAX_PATH];
    
    //保存任务的唯一ID，可以用于查找使用，比如使用URL的md5值等等。确保唯一
    char unique_id[DOWNLOAD_ENGINE_MAX_ID];
    
#define MAX_SPEED_COUNT 4
    double mSpeedArray[MAX_SPEED_COUNT];
    int mSpeedIndex;
    
    //The error retry count
    int mErrRetryCount;
    
private:
    static int task_id_pool;
    
    static int download_engine_next_id() {
        if (++ task_id_pool <= 0) {
            task_id_pool = 1;
        }
        return task_id_pool;
    }
};

#endif
