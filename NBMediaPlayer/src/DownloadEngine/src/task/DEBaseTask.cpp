#include "DEBaseTask.h"
#include "DECurlHttpTask.h"
#include "filesystem/DEDataManager.h"
#include "common/DELooper.h"
#include "download_engine.h"

#define LOG_TAG "DEBaseTask"

int DEBaseTask::task_id_pool = -1;

DEBaseTask::DEBaseTask() {
        memset((void*)mSpeedArray, 0, sizeof(mSpeedArray));
        mSpeedIndex = 0;
        
        memset(&mTaskCallback, 0, sizeof(mTaskCallback));
        
        INIT_LIST_HEAD(&mTaskPriv);
}

DEBaseTask::~DEBaseTask() {

}

DEBaseTask* DEBaseTask::Create(const CREATE_TASK_PARAMS* task_params) {
    DEBaseTask* task_item = new DECurlHttpTask();
    strcpy(task_item->task_url, task_params->task_url);
    strcpy(task_item->save_path, task_params->save_path);
    strcpy(task_item->save_name, task_params->save_name);
    task_item->mBeginOff = task_params->begin_off;
    task_item->mEndOff = task_params->end_off;

    task_item->mStatus = TaskStatusUnknow;
    task_item->mTaskSize = task_item->mEndOff - task_item->mBeginOff;

    task_item->mCurrReadPosition = 0;
    task_item->mCurrWritePosition = 0;
    
    task_item->mTaskId = download_engine_next_id();

    task_item->mTaskConfig.maxParallelSize = MAX_PARALLEL_TASK_SIZE;
    task_item->mTaskConfig.maxParallelNum = MAX_PARALLEL_TASK_NUM;
    task_item->mTaskConfig.maxSegmentNum = MAX_SEGMENT_NUM;
    task_item->mTaskConfig.maxCacheSegmentNum = MAX_CACHE_SEGMENT_NUM;
    task_item->mTaskConfig.maxNetworkCacheSize = MAX_NETWORK_CACHE_SIZE;
    task_item->mTaskConfig.defaultBufferSize = DEFAULT_BUFFER_SIZE;

    task_item->mTimerCtx = NULL;

    return task_item;
}

void DEBaseTask::Destroy(DEBaseTask* task_item) {
    delete task_item;
}

void DEBaseTask::reportDownloadSpeed() {

    calculateDownloadSpeed();

    double sum = 0;
    double averSpeed = 0;
    
    for (int idx = 0; idx < MAX_SPEED_COUNT; ++idx) {
        sum += mSpeedArray[idx];
    }
    averSpeed = sum / MAX_SPEED_COUNT;

     NBLOG_INFO(LOG_TAG, "%s : averSpeed:%lf", __func__, averSpeed);

    notifyTaskListeners(DL_EVENT_INFO, DL_INFO_DOWNLOAD_SPEED, averSpeed);
}

bool DEBaseTask::isProcessingPosition(int64_t postion) {
    DE_NETWORK_TASK_PRIV* task_priv = NULL;
    list_for_each_entry(task_priv, &(mTaskPriv), list) {
        if (postion >= task_priv->task_begin_off && postion < task_priv->task_end_off) {
            return true;
        }
    }
    
    return false;
}

int DEBaseTask::registerTaskCallback(const TaskCallback& callback) {
    mTaskCallback = callback;
    return 0;
}

void DEBaseTask::unregisterTaskCallback() {
    memset(&mTaskCallback, 0, sizeof(mTaskCallback));
}

void DEBaseTask::notifyTaskListeners(int task_event, int arg1, int arg2) {
    if (mTaskCallback.callback != NULL) {
        mTaskCallback.callback(mTaskId, task_event, arg1, arg2, mTaskCallback.opaque);
    }
}

void DEBaseTask::calculateDownloadSpeed() {

}

void DEBaseTask::launchNewSegment() {

}

void DEBaseTask::processHeaderScheme(void* privHandle) {

}

int DEBaseTask::prepare() {
    mDataManager = DEDataManager::Create();
    mDataManager->open(&mTaskConfig);
    launchNewSegment();
    return 0;
}

int64_t DEBaseTask::seek(int64_t off, int whence) {
    int64_t target_pos = -1;
    switch (whence) {
        case SEEK_SET:
        {
            target_pos = off;
        }
            break;
        case SEEK_CUR:
        {
            target_pos = mCurrReadPosition + off;
        }
            break;
        case SEEK_END:
        {
            target_pos = mTaskSize + off;
        }
            break;
        default:
            break;
    }
    
    NBLOG_INFO(LOG_TAG, "download_engine_seek_command off : %lld whence : %d off_at_begin : %lld", off, whence, target_pos);
    
//    *p_seeked_off = off_at_begin;
    
    if (target_pos < 0) {
        return target_pos;
    }
    
    mCurrReadPosition = target_pos;
        
    int64_t off_at_begin = target_pos / mTaskConfig.defaultBufferSize * mTaskConfig.defaultBufferSize;
        
    if (mDataManager->isPositionCached(mTaskConfig.defaultBufferSize, off_at_begin)) {
        NBLOG_INFO(LOG_TAG, "We got seek cache index : %d", off_at_begin / mTaskConfig.defaultBufferSize);
        return target_pos;
    }
    
//    NBLOG_INFO(LOG_TAG, "Seek command index : %lld timer_context : %p", off_at_begin / taskConfig.defaultBufferSize, timer_context);
    
    mCurrWritePosition = off_at_begin;
    
    //free the cache buffer if needed
    mDataManager->purgeBuffers();
    
    //stop all task first
    purgeTasks();
    
    launchNewSegment();
    
    return target_pos;
}

void DEBaseTask::purgeTasks() {
    
}

void DEBaseTask::TaskObserverTimer(DE_TIMER_CONTEXT* timer_context, int revents, void* user_data) {
    DECurlTask* task_item = (DECurlTask*)user_data;
    
    task_item->reportDownloadSpeed();
    
    if (task_item->mTaskConfig.maxParallelNum > 1) {
        task_item->launchNewSegment();
        task_item->launchNewSegment();
    }
}

int DEBaseTask::start() {
    mTimerCtx = DETimerContext::Create(mTaskLooper, 100, 1000, TaskObserverTimer, this);
    mTimerCtx->start();
    return 0;
}

void DEBaseTask::stop() {
    if (mTimerCtx != NULL) {
        mTimerCtx->stop();
        DETimerContext::Destroy(mTimerCtx);
    }
    
    mDataManager->close();
    
    DEDataManager::Destroy(mDataManager);
}

void DEBaseTask::queryNextDownload(int64_t pos, int64_t* download_begin, int64_t* download_end) {
    int64_t curr_write_pos = pos;
    
    NBMap<int, DE_SEGMENT_ITEM>::const_iterator cit;
    
    do {
        if (!mDataManager->isPositionCached(mTaskConfig.defaultBufferSize, curr_write_pos) && !isProcessingPosition(curr_write_pos)) {
            break;
        }
        curr_write_pos += mTaskConfig.defaultBufferSize;
    } while (curr_write_pos < mTaskSize);
    
    if (curr_write_pos > mTaskSize) {
        return ;
    }
    
    *download_begin = curr_write_pos;
    
    do {
        if (mDataManager->isPositionCached(mTaskConfig.defaultBufferSize, curr_write_pos) || isProcessingPosition(curr_write_pos)
            || curr_write_pos - *download_begin >= mTaskConfig.maxParallelSize) {
            break;
        }
        curr_write_pos += mTaskConfig.defaultBufferSize;
    } while(curr_write_pos < mTaskSize);
    
    if (curr_write_pos > mTaskSize) {
        NBLOG_INFO(LOG_TAG, "We got an end of file");
        curr_write_pos = mTaskSize;
    }
    
    *download_end = curr_write_pos;
    
    if (*download_end <= *download_begin) {
        NBLOG_INFO(LOG_TAG, "never gone be here");
    }
}

size_t DEBaseTask::processOutputData(void *ptr, size_t size, size_t nmemb, void *data) {
    size_t buf_size = size * nmemb;
    uint8_t* buf_ptr = (uint8_t*)ptr;

    DE_NETWORK_TASK_PRIV* task_priv = (DE_NETWORK_TASK_PRIV*)data;

    DEBaseTask* task_item = (DEBaseTask*)task_priv->parent;

    while (buf_size > 0) {

        size_t copy_size = std::min((size_t)(task_item->mTaskConfig.defaultBufferSize - task_priv->received_index), buf_size);

        memcpy(task_priv->received_buf + task_priv->received_index, buf_ptr, copy_size);

        task_priv->received_index += copy_size;

        buf_ptr += copy_size;
        buf_size -= copy_size;

        //Write out the enough data
        if (task_priv->received_index >= task_item->mTaskConfig.defaultBufferSize || (task_priv->task_download_write_pos + task_priv->received_index >= task_item->getTaskSize())) {

            size_t write_size = task_item->write(task_priv->received_buf, task_priv->received_index, task_priv->task_download_write_pos / task_item->mTaskConfig.defaultBufferSize, task_priv->file_write_pos);

            //Add the current wirte task pos
            task_priv->task_download_write_pos += write_size;

            // if (task_priv->task_download_write_pos < task_priv->task_end_off) {
            //     //reset the write index
            //     task_priv->file_write_pos = task_item->dataManager->acquireBuffer(task_item, &task_priv->received_buf);
            //     task_priv->received_index = 0;
            // }

            task_priv->received_index = 0;
            
            if (task_priv->needSpace > 0) {
                task_priv->file_write_pos = task_item->acquireBuffer(task_item->mTaskConfig.defaultBufferSize, &task_priv->received_buf);
                -- task_priv->needSpace;
            }
        }

        if (task_priv->task_download_write_pos == task_priv->task_end_off) {
            assert(task_priv->received_index == 0);
        }
    }

    return size * nmemb;
}

// read&write data
ssize_t DEBaseTask::read(void* buf, size_t buf_size) {
    ssize_t read_size = -1;
    if (mCurrReadPosition >= mTaskSize) {
        NBLOG_INFO(LOG_TAG, "read reached the end");
        return -1;
    }
    
    read_size = mDataManager->read(&mTaskConfig, buf, buf_size, mCurrReadPosition);
    
    if (read_size > 0) {
        mCurrReadPosition += read_size;
    }
    return read_size;
}

size_t DEBaseTask::write(void *buf, size_t trunck_size, uint32_t trunck_index, int64_t offs) {
    return mDataManager->write(&mTaskConfig, buf, trunck_size, trunck_index, offs);
}

int DEBaseTask::acquireBuffer(size_t reqLen, uint8_t** pOutBuf) {
    return mDataManager->acquireBuffer(reqLen, pOutBuf);
}

void DEBaseTask::releaseBuffer(int trunck_index) {
    mDataManager->releaseBuffer(trunck_index);
}
