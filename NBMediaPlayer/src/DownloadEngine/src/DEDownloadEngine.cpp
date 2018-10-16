//
//  download_engine.c
//  DownloadEngine
//
//  Created by liu enbao on 12/1/16.
//  Copyright © 2016 xb. All rights reserved.
//

#include "download_engine.h"
#include "common/DELooper.h"
#include "common/DEInternal.h"
#include "network/DENetwork.h"
#include "filesystem/DEDataManager.h"
#include "task/DECurlHttpTask.h"

#include <list>
#include <sstream>

#include <pthread.h>

#include <NBLog.h>

#define LOG_TAG "DEDownloadEngine"

typedef struct DOWNLOAD_ENGINE {
    DELooper* main_looper;
    TaskMapType task_map;   //For fast find
//    TaskVectorType all_task;    //For sequence traversal

    int running_task_count;

    const char* config_path;
    size_t config_path_size;

    //Network
    DENetwork* network;
} DOWNLOAD_ENGINE;

DOWNLOAD_ENGINE* g_download_engine = NULL;

DEBaseTask* download_engine_get_task_by_id(DOWNLOAD_ENGINE* download_engine, int task_id) {
    TaskMapConstIterator cit = download_engine->task_map.find(task_id);
    if (cit == download_engine->task_map.end()) {
        return NULL;
    }
    //access the real task_item
    return cit->second;
}

static void download_engine_remove_task_by_id(DOWNLOAD_ENGINE* download_engine, int task_id) {
    TaskMapConstIterator iter = download_engine->task_map.find(task_id);
    if (iter != download_engine->task_map.end()) {
        download_engine->task_map.erase(iter);
    }
}

//DEBaseTask* download_engine_get_task_by_create_params(DOWNLOAD_ENGINE* download_engine, CREATE_TASK_PARAMS* create_params) {
//    for(TaskVectorConstIterator cit = download_engine->all_task.begin();
//        cit != download_engine->all_task.end();
//        ++ cit) {
//        DEBaseTask* curr = *cit;
//        if (!strcmp(create_params->unique_id, curr->unique_id)) {
//            return curr;
//        }
//    }
//    return NULL;
//}

void download_filesystem_network_delegate(void* user_data, const ParamsType* params) {
    DOWNLOAD_ENGINE* download_engine = (DOWNLOAD_ENGINE*)user_data;
}

void download_engine_main_looper_init(void* userp) {
    NBLOG_INFO(LOG_TAG, "download_engine_main_looper_init");

    DOWNLOAD_ENGINE* download_engine = (DOWNLOAD_ENGINE*)userp;

    download_engine->running_task_count = 0;

    download_engine->network = DENetwork::Create(download_engine->main_looper);
}

void download_engine_main_looper_uninit(void* userp) {
    DOWNLOAD_ENGINE* download_engine = (DOWNLOAD_ENGINE*)userp;

    DENetwork::Destroy(download_engine->network);

    NBLOG_INFO(LOG_TAG, "download_engine_main_looper_uninit");
}

int download_engine_init(const char* config_path, size_t config_path_size) {

    NBLOG_INFO(LOG_TAG, "download_engine_init");

    if (g_download_engine != NULL) {
        return FAILURE;
    }

    g_download_engine = new DOWNLOAD_ENGINE();

    if (config_path != NULL)
        g_download_engine->config_path = strdup(config_path);

    g_download_engine->config_path_size = config_path_size;

    g_download_engine->main_looper = DELooper::Create(NBString("main_looper"),
                                                      download_engine_main_looper_init,
                                                      g_download_engine,
                                                      download_engine_main_looper_uninit,
                                                      g_download_engine);
    
    return g_download_engine->main_looper->start();
}

void download_engine_uninit() {
    if (g_download_engine == NULL) {
        return;
    }

    g_download_engine->main_looper->stop();

    DELooper::Destroy(g_download_engine->main_looper);

    if (g_download_engine->config_path != NULL) {
        free((char*)g_download_engine->config_path);
        g_download_engine->config_path = NULL;
    }

    delete g_download_engine;
    g_download_engine = NULL;

    NBLOG_INFO(LOG_TAG, "download_engine_uninit");
}

void download_engine_create_command(void* user_data, const ParamsType* params) {
    NBLOG_INFO(LOG_TAG, "download_engine_create_command begin");
    DEBaseTask* task_item = NULL;
    DOWNLOAD_ENGINE* download_engine = (DOWNLOAD_ENGINE*)user_data;

    int* p_task_id = AnyCast<int*>(paramsGetAtIndex(params, 0));
    CREATE_TASK_PARAMS* task_params = AnyCast<CREATE_TASK_PARAMS*>(paramsGetAtIndex(params, 1));
    download_engine_callback callback = AnyCast<download_engine_callback>(paramsGetAtIndex(params, 2));
    void* callback_opaque = AnyCast<void*>(paramsGetAtIndex(params, 3));

    // task_item = download_engine_get_task_by_create_params(download_engine, task_params);
    // //If all realy in task map
    // if (task_item != NULL) {
    //     de_filesystem_open_task(download_engine->filesystem, task_item);
    //     task_item->callback.callback = callback;
    //     task_item->callback.opaque = callback_opaque;
    //     *p_task_id = task_item->task_id;
    //     callback(task_item->task_id, DL_EVENT_INFO, DL_INFO_TASK_CREATE_COMPLETE, 0, callback_opaque);
    //     return ;
    // }

    task_item = DEBaseTask::Create(task_params);

    task_item->setLooper(download_engine->main_looper);

    TaskCallback callbackItem;
    callbackItem.callback = callback;
    callbackItem.opaque = callback_opaque;
    task_item->registerTaskCallback(callbackItem);

    task_item->setNetwork(download_engine->network);

    task_item->prepare();

    download_engine->task_map[task_item->getTaskId()] = task_item;
//    download_engine->all_task.push_back(task_item);

    *p_task_id = task_item->getTaskId();

    if (task_item->getTaskSize() > 0) {
        task_item->notifyTaskListeners(DL_EVENT_INFO, DL_INFO_TASK_CREATE_COMPLETE, 0);
    }

    delete task_params;
    
    paramsDestroy(params);
}

//Download engine interface
int download_engine_create_task(const CREATE_TASK_PARAMS* task_params, download_engine_callback callback, void* user_data) {

    if (g_download_engine == NULL) {
        return -1;
    }

    NBLOG_INFO(LOG_TAG, "download_engine_create_task begin");

    int task_id;

    ParamsType* params = paramsCreate(4);
    paramsSetAtIndex(params, 0, DEAny(&task_id));
    paramsSetAtIndex(params, 1, DEAny(new CREATE_TASK_PARAMS(*task_params)));
    paramsSetAtIndex(params, 2, DEAny(callback));
    paramsSetAtIndex(params, 3, DEAny(user_data));

    g_download_engine->main_looper->postCommandSyncImediately(params, download_engine_create_command, g_download_engine);

    NBLOG_INFO(LOG_TAG, "download_engine_create_task end");

    return task_id;
}

void download_engine_destroy_command(void* user_data, const ParamsType* params) {
    DOWNLOAD_ENGINE* download_engine = (DOWNLOAD_ENGINE*)user_data;

    const DEAny& anyValue = paramsGetAtIndex(params, 0);

    int task_id = AnyCast<int>(anyValue);

    DEBaseTask* task_item = download_engine_get_task_by_id(download_engine, task_id);

    download_engine->task_map.erase(task_id);

    download_engine_remove_task_by_id(download_engine, task_id);

    task_item->unregisterTaskCallback();

    DEBaseTask::Destroy(task_item);

//    NBLOG_INFO(LOG_TAG, "download_engine_destroy_command size download_engine->task_map %d: download_engine->all_task : %d", download_engine->task_map.size(), download_engine->all_task.size());
    
    paramsDestroy(params);
}

void download_engine_destroy_task(int task_id) {
    if (g_download_engine == NULL) {
        return;
    }

    ParamsType* params = paramsCreate(2);
    paramsSetAtIndex(params, 0, DEAny(task_id));

    g_download_engine->main_looper->postCommandSyncImediately(params, download_engine_destroy_command, g_download_engine);
}

void download_engine_start_command(void* user_data, const ParamsType* params) {
    DOWNLOAD_ENGINE* download_engine = (DOWNLOAD_ENGINE*)user_data;

    const DEAny& anyValue = paramsGetAtIndex(params, 0);

    int task_id = AnyCast<int>(anyValue);

    DEBaseTask* task_item = download_engine_get_task_by_id(download_engine, task_id);

    task_item->start();
    
    paramsDestroy(params);
}

int download_engine_start_task(int task_id) {
    if (g_download_engine == NULL) {
        return -1;
    }

    ParamsType* params = paramsCreate(1);
    paramsSetAtIndex(params, 0, DEAny(task_id));

    return g_download_engine->main_looper->postCommandAsyncImediately(params, download_engine_start_command, g_download_engine);
}

void download_engine_stop_command(void* user_data, const ParamsType* params) {
    DOWNLOAD_ENGINE* download_engine = (DOWNLOAD_ENGINE*)user_data;

    int task_id = AnyCast<int>(paramsGetAtIndex(params, 0));

    DEBaseTask* task_item = download_engine_get_task_by_id(download_engine, task_id);

    task_item->stop();
    
    paramsDestroy(params);
}

int download_engine_stop_task(int task_id) {
    if (g_download_engine == NULL) {
        return -1;
    }

    ParamsType* params = paramsCreate(1);
    paramsSetAtIndex(params, 0, DEAny(task_id));

    g_download_engine->main_looper->postCommandSyncImediately(params, download_engine_stop_command, g_download_engine);

    return 0;
}

void download_engine_get_task_size_command(void* user_data, const ParamsType* params) {
    DOWNLOAD_ENGINE* download_engine = (DOWNLOAD_ENGINE*)user_data;

    int64_t* p_task_size = AnyCast<int64_t*>(paramsGetAtIndex(params, 1));

    DEBaseTask* task_item = download_engine_get_task_by_id(download_engine, AnyCast<int>(paramsGetAtIndex(params, 0)));

    *p_task_size = task_item->getTaskSize();
    
    paramsDestroy(params);
}

int64_t download_engine_get_task_size(int task_id) {
    if (g_download_engine == NULL) {
        return -1;
    }

    int64_t task_size = -1;

    ParamsType* params = paramsCreate(2);
    paramsSetAtIndex(params, 0, DEAny(task_id));
    paramsSetAtIndex(params, 1, DEAny(&task_size));

    g_download_engine->main_looper->postCommandSyncImediately(params, download_engine_get_task_size_command, g_download_engine);

    return task_size;
}

void download_engine_seek_command(void* user_data, const ParamsType* params) {
    DOWNLOAD_ENGINE* download_engine = (DOWNLOAD_ENGINE*)user_data;

    int64_t *p_seeked_off = AnyCast<int64_t*>(paramsGetAtIndex(params, 3));

    int whence = AnyCast<int>(paramsGetAtIndex(params, 2));

    DEBaseTask* task_item = download_engine_get_task_by_id(download_engine, AnyCast<int>(paramsGetAtIndex(params, 0)));

    int64_t off_at_begin = -1;
    int64_t off = AnyCast<int64_t>(paramsGetAtIndex(params, 1));
    
    *p_seeked_off = task_item->seek(off, whence);
    
    paramsDestroy(params);
}

int64_t download_engine_seek_task(int task_id, int64_t off, int whence) {
    if (g_download_engine == NULL) {
        return -1;
    }

    int64_t seeked_off = -1;

    ParamsType* params = paramsCreate(4);
    paramsSetAtIndex(params, 0, DEAny(task_id));
    paramsSetAtIndex(params, 1, DEAny(off));
    paramsSetAtIndex(params, 2, DEAny(whence));
    paramsSetAtIndex(params, 3, DEAny(&seeked_off));

    g_download_engine->main_looper->postCommandSyncImediately(params, download_engine_seek_command, g_download_engine);

    return seeked_off;
}

void download_engine_read_command(void* user_data, const ParamsType* params) {

    DOWNLOAD_ENGINE* download_engine = (DOWNLOAD_ENGINE*)user_data;

    ssize_t* p_read_size = AnyCast<ssize_t*>(paramsGetAtIndex(params, 1));
    void* buf = AnyCast<void*>(paramsGetAtIndex(params, 2));
    size_t buf_size = AnyCast<size_t>(paramsGetAtIndex(params, 3));

    DEBaseTask* task_item = download_engine_get_task_by_id(download_engine, AnyCast<int>(paramsGetAtIndex(params, 0)));

    int read_size = task_item->read(buf, buf_size);

    *p_read_size = read_size;
    
    paramsDestroy(params);
}

ssize_t download_engine_read_task(int task_id, void* buf, size_t size) {
    if (g_download_engine == NULL) {
        return -1;
    }

    ssize_t read_size = -1;

    ParamsType* params = paramsCreate(4);
    paramsSetAtIndex(params, 0, DEAny(task_id));
    paramsSetAtIndex(params, 1, DEAny(&read_size));
    paramsSetAtIndex(params, 2, DEAny(buf));
    paramsSetAtIndex(params, 3, DEAny(size));

    g_download_engine->main_looper->postCommandSyncImediately(params, download_engine_read_command, g_download_engine);

    return read_size;
}
