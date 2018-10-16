#include "DENetwork.h"
#include "DESocketHelper.h"
#include <curl/curl.h>
#include "common/DELooper.h"
#include "filesystem/DEDataManager.h"
#include "task/DEBaseTask.h"
#include "task/DECurlTask.h"

#include <NBLog.h>

#define LOG_TAG "DENetwork"

static int de_network_socket_cb(CURL *easy_handle, curl_socket_t s, int what, void *cbp, void *sockp) {

//    NBLOG_INFO(LOG_TAG, "de_network_socket_cb socket : %d what : %d", s, what);
    DENetwork* network = (DENetwork*)cbp;
    
    DEIOContext *io_context = (DEIOContext*) sockp;

    if(what == CURL_POLL_REMOVE) {
        if (io_context != NULL) {
            io_context->stop();
            DEIOContext::Destroy(io_context);
        }
        curl_multi_assign(network->getCurlMulti(), s, NULL);
    } else {
        int action = (what & CURL_POLL_IN ? DE_READ : 0)|( what & CURL_POLL_OUT ? DE_WRITE : 0);

        // FPINFO("de_network_socket_cb action : %d what & CURL_POLL_IN : %d action & CURL_POLL_OUT : %d", action, what & CURL_POLL_IN, action & CURL_POLL_OUT);

        if(!io_context) {
            DE_NETWORK_TASK_PRIV* task_priv = NULL;
            curl_easy_getinfo(easy_handle, CURLINFO_PRIVATE, &task_priv);

            DEBaseTask* task_item = (DEBaseTask*)task_priv->parent;

            io_context = DEIOContext::Create(task_item->getLooper(), s, &DECurlTask::processIOEventCallback, task_item);
            curl_multi_assign(network->getCurlMulti(), s, io_context);

            if (action > 0) {
                io_context->start(action);
            }
        } else {
            io_context->stop();
            if (action > 0) {
                io_context->start(action);
            }
        }
    }

    return 0;
}

static void de_network_task_observer_timer_callback(DE_TIMER_CONTEXT* timer_context, int revents, void* user_data) {
    DENetwork* network = (DENetwork*)user_data;

    NBLOG_INFO(LOG_TAG, "de_task_observer_timer_callback is ok");
    int still_running = 0;

    CURLMcode rc = curl_multi_socket_action(network->getCurlMulti(), CURL_SOCKET_TIMEOUT, 0, &still_running);

//    task_item->checkMultiCode("de_network_task_observer_timer_callback: curl_multi_socket_action", rc, task_item);
    network->checkMultiInfo(still_running);

//    task_item->reportDownloadSpeed();
//
//    if (task_item->taskConfig.maxParallelNum > 1) {
//        task_item->launchNewSegment();
//        task_item->launchNewSegment();
//    }
}

static int de_network_multi_timer_callback(CURLM* multi, long timeout_us, DENetwork* network) {
    network->timer_context->stop();
    
    if (timeout_us > 0) {
        network->timer_context->reset(timeout_us, 0);
        network->timer_context->start();
    } else if (timeout_us == 0)
        de_network_task_observer_timer_callback(network->timer_context->getPrivCtx(), 0, network);
    return 0;
}

DENetwork::DENetwork(DELooper* task_looper)
    :task_looper(task_looper) {

}

DENetwork::~DENetwork() {

}

DENetwork* DENetwork::Create(DELooper* task_looper) {
    DENetwork* network = new DENetwork(task_looper);
    network->initContext();
    return network;
}

void DENetwork::Destroy(DENetwork* network) {
    if (network == NULL) {
        return ;
    }

    network->uninitContext();

    delete network;
    NBLOG_INFO(LOG_TAG, "de_network_destory end");
}

int DENetwork::initContext() {
    NBLOG_INFO(LOG_TAG, "initContext begin");
            //Init all context
    curl_global_init(CURL_GLOBAL_DEFAULT);

    mCurlMulti = curl_multi_init();
    
    timer_context = DETimerContext::Create(task_looper, 0, 0, de_network_task_observer_timer_callback, this);

    curl_multi_setopt(mCurlMulti, CURLMOPT_MAXCONNECTS, MAX_PARALLEL_TASK_NUM);

    curl_multi_setopt(mCurlMulti, CURLMOPT_SOCKETFUNCTION, de_network_socket_cb);
    curl_multi_setopt(mCurlMulti, CURLMOPT_SOCKETDATA, this);
    curl_multi_setopt(mCurlMulti, CURLMOPT_TIMERFUNCTION, de_network_multi_timer_callback);
    curl_multi_setopt(mCurlMulti, CURLMOPT_TIMERDATA, this);
    
    NBLOG_INFO(LOG_TAG, "initContext end");
    return SUCCESS;
}

void DENetwork::uninitContext() {
    NBLOG_INFO(LOG_TAG, "uninitContext begin");
    curl_multi_cleanup(mCurlMulti);

    //Clearup the all data
    curl_global_cleanup();
    
    DETimerContext::Destroy(timer_context);

    NBLOG_INFO(LOG_TAG, "uninitContext end");
}

void DENetwork::checkMultiInfo(int stillRunning) {

    char *eff_url;
    CURLMsg *msg;
    int msgs_left;
    CURL *easy_handle;
    CURLcode res;
    int http_response_code;
    
    while((msg = curl_multi_info_read(mCurlMulti, &msgs_left))) {
        if(msg->msg == CURLMSG_DONE) {
            easy_handle = msg->easy_handle;
            res = msg->data.result;
            
            DE_NETWORK_TASK_PRIV* task_priv = NULL;
            curl_easy_getinfo(easy_handle, CURLINFO_PRIVATE, &task_priv);
            curl_easy_getinfo(easy_handle, CURLINFO_EFFECTIVE_URL, &eff_url);
            
            DECurlTask* task_item = (DECurlTask*)task_priv->parent;
            
            if (task_priv->received_index > 0) {
                // FPINFO("Not allow to happen : %d task_download_write_pos : %lld task_size : %lld", task_priv->received_index, task_priv->task_download_write_pos, task_item->task_size);
            }
            
            task_item->calculateDownloadSpeed();
            
            if (res != CURLE_OK) {
                task_item->handleError(res, task_priv);
                
                //Release the aqurired buffer
                task_item->releaseBuffer(task_priv->file_write_pos);
            }
            
            list_del(&task_priv->list);
            
            if (res == CURLE_OK) {
                task_item->setErrRetryCount(0);
                
                task_item->processHeaderScheme(task_priv);
                
//                if (task_priv->task_end_off == task_item->task_size) {
//                    NBLOG_ERROR(LOG_TAG, "task_item->curr_write_pos purge current curr_write_pos : %lld", task_item->curr_write_pos);
//                    task_item->curr_write_pos = 0;
//                }
                
                //start an new segment if needed
                task_item->launchNewSegment();
            }
            
            if (task_priv->easy_handle != NULL) {
                curl_multi_remove_handle(mCurlMulti, task_priv->easy_handle);
                curl_easy_cleanup(task_priv->easy_handle);
                curl_slist_free_all(task_priv->custom_header);
            }
            
            task_item->destroyTaskPriv(task_priv);
        }
    }
    
    // top the timer if no task exist
    if (stillRunning <= 0) {
        timer_context->stop();
    }
}
