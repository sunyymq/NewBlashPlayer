#include "DECurlHttpTask.h"
#include "network/DENetwork.h"
#include "network/DESocketHelper.h"
#include "common/DELooper.h"

#include <NBLog.h>

#define LOG_TAG "DECurlHttpTask"

DECurlHttpTask::DECurlHttpTask() {

}

DECurlHttpTask::~DECurlHttpTask() {

}

void DECurlHttpTask::startNetworkProcess(DE_NETWORK_TASK_PRIV* task_priv) {

    DECurlHttpTask* task_item = (DECurlHttpTask*)task_priv->parent;

    CURLMcode rc;

    CURLcode res;

    task_priv->easy_handle = curl_easy_init();
    if(!task_priv->easy_handle) {
        // FPERROR("curl_easy_init() failed, exiting!");
        return ;
    }

    //Set debug funcion and so on
    // DECurlTask::startNetworkProcess(network, task_priv);

    if (!mEffectiveUrl.empty()) {
        curl_easy_setopt(task_priv->easy_handle, CURLOPT_URL, mEffectiveUrl.string());
    } else {
        curl_easy_setopt(task_priv->easy_handle, CURLOPT_URL, task_item->task_url);
    }

    curl_easy_setopt(task_priv->easy_handle, CURLOPT_HEADERFUNCTION, &DECurlHttpTask::httpHeaderCallback);
    curl_easy_setopt(task_priv->easy_handle, CURLOPT_HEADERDATA, task_priv);
    curl_easy_setopt(task_priv->easy_handle, CURLOPT_WRITEFUNCTION, &DEBaseTask::processOutputData);
    curl_easy_setopt(task_priv->easy_handle, CURLOPT_WRITEDATA, task_priv);

    curl_easy_setopt(task_priv->easy_handle, CURLOPT_FAILONERROR, 1L);

    curl_easy_setopt(task_priv->easy_handle, CURLOPT_ERRORBUFFER, task_priv->error);
    curl_easy_setopt(task_priv->easy_handle, CURLOPT_PRIVATE, task_priv);
    // curl_easy_setopt(task_priv->easy_handle, CURLOPT_FRESH_CONNECT, 1);

    curl_easy_setopt(task_priv->easy_handle, CURLOPT_TCP_NODELAY, 1L);
    curl_easy_setopt(task_priv->easy_handle, CURLOPT_TCP_KEEPALIVE, 1L);

    curl_easy_setopt(task_priv->easy_handle, CURLOPT_CONNECTTIMEOUT, 30L);

    // if less than 2000 bytes/sec during 30 seconds, abort transfer
    curl_easy_setopt(task_priv->easy_handle, CURLOPT_LOW_SPEED_LIMIT, 2000L);
    curl_easy_setopt(task_priv->easy_handle, CURLOPT_LOW_SPEED_TIME, 30L);

    task_priv->custom_header =  curl_slist_append(task_priv->custom_header, "Connection: Keep-Alive");
    curl_easy_setopt(task_priv->easy_handle, CURLOPT_HTTPHEADER, task_priv->custom_header);

    curl_easy_setopt(task_priv->easy_handle, CURLOPT_USERAGENT, "DownloadEngine 1.0");
    curl_easy_setopt(task_priv->easy_handle, CURLOPT_FOLLOWLOCATION, 1);

    //Socket Support
    if (de_looper_get_open_socket() != NULL && de_looper_get_close_socket() != NULL) {
        /* call this function to get a socket */
        curl_easy_setopt(task_priv->easy_handle, CURLOPT_OPENSOCKETFUNCTION, opensocket);
        curl_easy_setopt(task_priv->easy_handle, CURLOPT_OPENSOCKETDATA, mTaskLooper);

        /* call this function to close a socket */
        curl_easy_setopt(task_priv->easy_handle, CURLOPT_CLOSESOCKETFUNCTION, close_socket);
        curl_easy_setopt(task_priv->easy_handle, CURLOPT_CLOSESOCKETDATA, mTaskLooper);
    }

    char range_format[64] = {0};
    int64_t begin_off = task_priv->task_begin_off;
    int64_t end_off = task_priv->task_end_off;
    if (task_item->mBeginOff != INVALID_OFFSET && task_item->mEndOff != INVALID_OFFSET) {
        begin_off += task_item->mBeginOff;
        end_off += task_item->mBeginOff;
    }

    sprintf(range_format, "%lld-%lld", begin_off, end_off - 1);

//    NBLOG_ERROR(LOG_TAG, "The Request range is : %s", range_format);

    curl_easy_setopt(task_priv->easy_handle, CURLOPT_RANGE, range_format);

//    //Share the all sub task with the same memory
//    curl_easy_setopt(task_priv->easy_handle, CURLOPT_SHAREDBUFFER, mSharedBuffer);

//    //if use CURLOPT_SHAREDBUFFER , must call after it
//    curl_easy_setopt(task_priv->easy_handle, CURLOPT_BUFFERSIZE, CURL_MAX_READ_SIZE);

    rc = curl_multi_add_handle(mNetwork->getCurlMulti(), task_priv->easy_handle);
    checkMultiCode("startNetworkProcess: curl_multi_add_handle", rc, task_item);
    // curl_multi_socket_all(network->curl_multi, &network->still_running);

    int still_running = 0;
    rc = curl_multi_socket_action(mNetwork->getCurlMulti(), CURL_SOCKET_TIMEOUT, 0, &still_running);
    checkMultiCode("startNetworkProcess: curl_multi_socket_action", rc, task_item);

    // FPINFO("de_start_network_process end range_format : %s", range_format);
}

void DECurlHttpTask::processHeaderScheme(void* privHandle) {
    //If we already get the file size
    DE_NETWORK_TASK_PRIV* task_priv = (DE_NETWORK_TASK_PRIV*)privHandle;

    // double downloadLength = 0;
    // curl_easy_getinfo(task_priv->easy_handle, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &downloadLength);
    // task_size = downloadLength;

//    NBLOG_INFO(LOG_TAG, "processHeaderScheme: %d", is_get_size);
    
    if (task_priv->task_end_off == mTaskSize) {
        NBLOG_ERROR(LOG_TAG, "task_item->curr_write_pos purge current curr_write_pos : %lld", mCurrWritePosition);
        mCurrWritePosition = 0;
    }

    if (mIsGettingSize == false) {
        return ;
    }

//    NBLOG_INFO(LOG_TAG, "processHeaderScheme Content-length download : %lld", task_size);

    char* effectiveUrl = NULL;
    curl_easy_getinfo(task_priv->easy_handle, CURLINFO_EFFECTIVE_URL, &effectiveUrl);

    if (effectiveUrl != NULL) {
        mEffectiveUrl = effectiveUrl;

//        NBLOG_INFO(LOG_TAG, "processHeaderScheme effectiveUrl : %s", mEffectiveUrl.string());
    }

    notifyTaskListeners(DL_EVENT_INFO, DL_INFO_TASK_CREATE_COMPLETE, 0);
}

size_t DECurlHttpTask::httpHeaderCallback(char *buffer, size_t size, size_t nitems, void *userdata) {
    DE_NETWORK_TASK_PRIV* task_priv = (DE_NETWORK_TASK_PRIV*)userdata;
    DEBaseTask* task_item = (DEBaseTask*)task_priv->parent;

//    NBLOG_INFO(LOG_TAG, "header_callback buffer : %s", buffer);

    if (!task_item->isGettingSize()) {
        return size * nitems;
    }

    // if (!strcmp(buffer, "\r\n")) {

    //     // if (task_item->mBeginOff != INVALID_OFFSET && task_item->mEndOff != INVALID_OFFSET) {
    //     //     task_item->task_size = task_item->mEndOff - task_item->mBeginOff;
    //     // }

    //     FPERROR("The task_item->task_size is : %lld", task_item->task_size);

    //     if (task_item->is_get_size && task_item->task_size > 0) {

    //         task_item->notifyTaskListeners(DL_EVENT_INFO, DL_INFO_TASK_CREATE_COMPLETE, 0);

    //         task_item->is_get_size = false;
    //         // FPINFO("header_callback got task_size:%lld", task_item->task_size);
    //     }
    // }

    int64_t dummy = 0;
    int64_t content_length = 0;

    if (sscanf(buffer, "Content-Range: bytes %lld-%lld/%lld\r\n", &dummy, &dummy, &content_length)) {
        NBLOG_INFO(LOG_TAG, "The content length is : %lld", content_length);
        task_item->setTaskSize(content_length);
    } else if (sscanf(buffer, "Content-Length: %lld\r\n", &content_length)) {
        NBLOG_INFO(LOG_TAG, "The content length is : %lld", content_length);
        if (task_item->getTaskSize() == -1) {
            task_item->setTaskSize(content_length);
        }
    }

    return size * nitems;
}
