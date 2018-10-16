#include "DECurlTask.h"
#include "filesystem/DEDataManager.h"
#include "network/DENetwork.h"
#include "common/DELooper.h"

#include <errno.h>

#include <NBLog.h>

#define LOG_TAG "DECurlTask"

static int DECurlTaskCallback(CURL *handle, curl_infotype type, char *data, size_t size, void *userptr) {
    switch(type) {
    case CURLINFO_TEXT:
        NBLOG_INFO(LOG_TAG, "== Info: %s", data);
        break;
    case CURLINFO_HEADER_OUT:
        // text = "=> Send header";
        NBLOG_INFO(LOG_TAG, "== header out: %s", data);
        break;
    case CURLINFO_DATA_OUT:
        // text = "=> Send data";
        break;
    case CURLINFO_SSL_DATA_OUT:
        // text = "=> Send SSL data";
        break;
    case CURLINFO_HEADER_IN:
        // text = "<= Recv header";
        NBLOG_INFO(LOG_TAG, "== Header in: %s", data);
        break;
    case CURLINFO_DATA_IN:
        // text = "<= Recv data";
        break;
    case CURLINFO_SSL_DATA_IN:
        // text = "<= Recv SSL data";
        break;
    }
    return 0;
}

DECurlTask::DECurlTask() {

}

DECurlTask::~DECurlTask() {

}

DE_NETWORK_TASK_PRIV* DECurlTask::createTaskPriv(int64_t download_begin, int64_t download_end) {

    DE_NETWORK_TASK_PRIV* task_priv = new DE_NETWORK_TASK_PRIV();
    task_priv->parent = this;
    task_priv->task_download_write_pos = download_begin;
    task_priv->task_begin_off = download_begin;
    task_priv->task_end_off = download_end;

    // task_priv->received_buf = new uint8_t[taskConfig.defaultBufferSize];

//    NBLOG_INFO(LOG_TAG, "createTaskPriv: download_begin:%lld, download_end%lld", download_begin, download_end);

    if ((download_end % mTaskConfig.defaultBufferSize)) {
        task_priv->needSpace = (download_end - download_begin + mTaskConfig.defaultBufferSize) / mTaskConfig.defaultBufferSize;
    } else {
        task_priv->needSpace = (download_end - download_begin) / mTaskConfig.defaultBufferSize;
    }

    task_priv->file_write_pos = mDataManager->acquireBuffer(mTaskConfig.defaultBufferSize, &task_priv->received_buf);
    task_priv->received_index = 0;
    -- task_priv->needSpace;

    return task_priv;
}

void DECurlTask::destroyTaskPriv(DE_NETWORK_TASK_PRIV* task_priv) {
    // delete []task_priv->received_buf;
    delete task_priv;
}

void DECurlTask::calculateDownloadSpeed() {
    double totalSpeed = 0;

    DE_NETWORK_TASK_PRIV* task_priv = NULL;
    list_for_each_entry(task_priv, &(mTaskPriv), list) {
        CURL* easy_handle = task_priv->easy_handle;
        if (easy_handle != NULL) {
            double speed = 0;
            curl_easy_getinfo(easy_handle, CURLINFO_SPEED_DOWNLOAD, &speed);
            totalSpeed += speed;
        }
    }
    
    mSpeedArray[mSpeedIndex] = totalSpeed;
    mSpeedIndex = (mSpeedIndex + 1) % MAX_SPEED_COUNT;

//    if (speed_array.size() >= 2) {
//        speed_array.erase(speed_array.begin());
//    }
//
//    speed_array.push_back(totalSpeed);
}

void DECurlTask::processIOEventCallback(DE_IO_CONTEXT* io_context, int fd, int action, int revents, void* user_data) {
    DECurlTask* task_item = (DECurlTask*)user_data;
    task_item->processIOEvent(io_context, fd, action, revents);
}

void DECurlTask::processIOEvent(DE_IO_CONTEXT* io_context, int fd, int action, int revents) {
    CURLMcode rc;
    int still_running = 0;

    int event = (action & DE_READ ? CURL_POLL_IN : 0) |(action & DE_WRITE ? CURL_POLL_OUT : 0);

    rc = curl_multi_socket_action(mNetwork->getCurlMulti(), fd, event, &still_running);
    checkMultiCode("processIOEvent: curl_multi_socket_action", rc, this);

    // rc = curl_multi_socket_all(network->curl_multi, &network->still_running);

    mNetwork->checkMultiInfo(still_running);

    // FPINFO("network_event_cb fd : %d action : %d rc : %d", fd, action, rc);
}

void DECurlTask::checkMultiCode(const char* where, CURLMcode code, DECurlTask* task_item) {
    if (CURLM_OK == code) {
        return;
    }

    const char *s;
    switch (code) {
        case CURLM_BAD_HANDLE:
            s = "CURLM_BAD_HANDLE";
            break;
        case CURLM_BAD_EASY_HANDLE:
            s = "CURLM_BAD_EASY_HANDLE";
            break;
        case CURLM_OUT_OF_MEMORY:
            s = "CURLM_OUT_OF_MEMORY";
            break;
        case CURLM_INTERNAL_ERROR:
            s = "CURLM_INTERNAL_ERROR";
            break;
        case CURLM_UNKNOWN_OPTION:
            s = "CURLM_UNKNOWN_OPTION";
            break;
        case CURLM_LAST:
            s = "CURLM_LAST";
            break;
        case CURLM_BAD_SOCKET:
            s = "CURLM_BAD_SOCKET";
            NBLOG_ERROR(LOG_TAG, "checkMultiCode ERROR: %s returns %s\n", where, s);
            /* ignore this error */
            return;
        default:
            s = "CURLM_unknown";
            break;
    }

    NBLOG_ERROR(LOG_TAG, "checkMultiCode ERROR: %s returns %s\n", where, s);

    notifyTaskListeners(DL_EVENT_ERROR, DL_ERROR_UNKNOWN, code);
}

void DECurlTask::handleError(CURLcode code, DE_NETWORK_TASK_PRIV* task_priv) {
    CURL* easy_handle = task_priv->easy_handle;
    int http_response_code = 200;
    int os_errno = 0;

    if (easy_handle != NULL) {
        curl_easy_getinfo(easy_handle, CURLINFO_RESPONSE_CODE, &http_response_code);
        curl_easy_getinfo(easy_handle, CURLINFO_OS_ERRNO, &os_errno);
    }

    NBLOG_WARN(LOG_TAG, "%s: %s(%d), http_response_code:%d, os_errno:%s(%d)",
        __func__, curl_easy_strerror(code), code, http_response_code, strerror(os_errno), os_errno);

//    NBLOG_INFO(LOG_TAG, "%s: network->still_running:%d, network_priv.size:%d, errRetryCount:%d",
//        __func__, network->still_running, network_priv.size(), task_item->errRetryCount);

    if (mTaskSize < 0) {
        notifyTaskListeners(DL_EVENT_ERROR, DL_ERROR_UNKNOWN, code);
        return;
    }

    //Reset the download position if needed
    if (mCurrWritePosition > task_priv->task_download_write_pos) {
        mCurrWritePosition = task_priv->task_download_write_pos;
    }

    int error_type = DL_ERROR_UNKNOWN;
    int error_extra = code;
    switch (code) {
        case CURLE_HTTP_RETURNED_ERROR:
            if (http_response_code == 503) {
                // task_item->taskConfig.maxParallelSize = 0x400000;
                // task_item->taskConfig.maxParallelNum = 1;
                return;
            }
            if (http_response_code == 504) {
//                task_item->errRetryCount = networkPriv->still_running > 0 ? task_item->errRetryCount / networkPriv->still_running : task_item->errRetryCount;
//                if (task_item->errRetryCount < MAX_ERROR_RETRY) {
//                    ++ task_item->errRetryCount;
//                    return;
//                }
            }
            error_type = DL_ERROR_HTTP_RETURN;
            error_extra = http_response_code;
            break;

        // if retry max times, report error
        case CURLE_PARTIAL_FILE:
        case CURLE_OPERATION_TIMEDOUT:
        case CURLE_AGAIN:
        case CURLE_RECV_ERROR:
        case CURLE_COULDNT_RESOLVE_PROXY:
        case CURLE_COULDNT_RESOLVE_HOST:
        case CURLE_COULDNT_CONNECT:
            if (os_errno == ECONNREFUSED) {
                return;
            }
//            task_item->errRetryCount = networkPriv->still_running > 0 ? task_item->errRetryCount / networkPriv->still_running : task_item->errRetryCount;
//            if (task_item->errRetryCount < MAX_ERROR_RETRY) {
//                ++ task_item->errRetryCount;
//                return;
//            }
            NBLOG_ERROR(LOG_TAG, "%s: retry max error times, task_item->errRetryCount:%d", __func__, mErrRetryCount);
            error_type = DL_ERROR_UNKNOWN;
            error_extra = code;
            break;

        default:
            error_type = DL_ERROR_UNKNOWN;
            error_extra = code;
            break;
    }

    notifyTaskListeners(DL_EVENT_ERROR, error_type, error_extra);
}

void DECurlTask::launchNewSegment() {
    if (mTaskSize > 0 && mCurrWritePosition >= mTaskSize) {
        NBLOG_INFO(LOG_TAG, "launchNewSegment task_size: %lld current_write_pos : %lld", mTaskSize, mCurrWritePosition);
        return ;
    }
    
    int privCount = 0;
    struct list_head* pos;
    list_for_each(pos, &mTaskPriv) {
        ++privCount;
    }
    
    if (privCount >= mTaskConfig.maxParallelNum) {
        NBLOG_INFO(LOG_TAG, "launchNewSegment network_priv.size() : %d taskConfig.maxParallelNum : %d", privCount, mTaskConfig.maxParallelNum);
        return ;
    }

    int64_t download_begin = -1;
    int64_t download_end = -1;

    int64_t off_at_begin = mCurrWritePosition;

    if (mTaskSize > 0) {
        queryNextDownload(off_at_begin, &download_begin, &download_end);
        mIsGettingSize = false;
    } else {
        // FPINFO("bobmarshall task_item->task_size : %lld", task_size);
        download_begin = 0;
        download_end = mTaskConfig.maxParallelSize;
        mIsGettingSize = true;
    }

//    NBLOG_INFO(LOG_TAG, "launchNewSegment download_begin : %lld download_end : %lld", download_begin, download_end);
    
    if (download_begin != -1 && download_end != -1) {

        int needSpaceCount = 0;
        
        DE_NETWORK_TASK_PRIV* task_priv = NULL;
        list_for_each_entry(task_priv, &(mTaskPriv), list) {
            needSpaceCount += task_priv->needSpace;
        }

        if ((download_end % mTaskConfig.defaultBufferSize)) {
            needSpaceCount += (download_end - download_begin + mTaskConfig.defaultBufferSize) / mTaskConfig.defaultBufferSize;
        } else {
            needSpaceCount += (download_end - download_begin) / mTaskConfig.defaultBufferSize;
        }
        
        if (!mDataManager->isEnoughFreespace(needSpaceCount)) {
//            NBLOG_INFO(LOG_TAG, "launchNewSegment de_filesystem_enough_freespace is not ok, needSpace:%d", needSpace);
            return ;
        }

        task_priv = createTaskPriv(download_begin, download_end);
        startNetworkProcess(task_priv);
        
        list_add_tail(&task_priv->list, &mTaskPriv);

        mCurrWritePosition = download_end;
    }
}

int DECurlTask::start() {
    DEBaseTask::start();
    return SUCCESS;
}

void DECurlTask::stop() {
    purgeTasks();
    
    DEBaseTask::stop();
}

void DECurlTask::purgeTasks() {
    struct list_head *pos, *q;
    
    list_for_each_safe(pos, q, &mTaskPriv){
        DE_NETWORK_TASK_PRIV* task_priv = list_entry(pos, DE_NETWORK_TASK_PRIV, list);
        list_del(pos);
        if (task_priv->easy_handle != NULL) {
            curl_multi_remove_handle(mNetwork->getCurlMulti(), task_priv->easy_handle);
            curl_easy_cleanup(task_priv->easy_handle);
            curl_slist_free_all(task_priv->custom_header);
        }
        
        mDataManager->releaseBuffer(task_priv->file_write_pos);
        
        //delte the task priv
        destroyTaskPriv(task_priv);
    }
}

void DECurlTask::startNetworkProcess(DE_NETWORK_TASK_PRIV* task_priv) {
    if (task_priv == NULL) {
        return ;
    }

    curl_easy_setopt(task_priv->easy_handle, CURLOPT_DEBUGFUNCTION, DECurlTaskCallback);
    curl_easy_setopt(task_priv->easy_handle, CURLOPT_DEBUGDATA, this);

    //disable speed meter
    curl_easy_setopt(task_priv->easy_handle, CURLOPT_NOPROGRESS, 1L);

    curl_easy_setopt(task_priv->easy_handle, CURLOPT_VERBOSE, 1L);
}
