//
//  download_engine.h
//  DownloadEngine
//
//  Created by liuenbao on 10/29/15.
//  Copyright (c) 2015 xb. All rights reserved.
//

#ifndef download_engine_h
#define download_engine_h

#include "common_def.h"

#ifdef __cplusplus
extern "C" {
#endif

int download_engine_init(const char* config_path, size_t config_path_size);

void download_engine_uninit();

//定义创建任务的参数
typedef struct CREATE_TASK_PARAMS
{
    //创建任务的URL
    char task_url[DOWNLOAD_ENGINE_MAX_URL];
        
    //确保文件的路经，并且有足够的空间
    char save_path[DOWNLOAD_ENGINE_MAX_PATH];
        
    //保存文件的名字
    char save_name[DOWNLOAD_ENGINE_MAX_PATH];
        
    //保存任务的唯一ID，可以用于查找使用，比如使用URL的md5值等等。确保唯一
    char unique_id[DOWNLOAD_ENGINE_MAX_ID];

    //For begin offset in the single file
    int64_t begin_off;

    //For end offset in the single file
    int64_t end_off;
}CREATE_TASK_PARAMS;

int download_engine_create_task(const CREATE_TASK_PARAMS* task_params, download_engine_callback callback, void* user_data);

void download_engine_destroy_task(int task_id, download_engine_callback callback);

int download_engine_start_task(int task_id);

int download_engine_stop_task(int task_id);

int64_t download_engine_get_task_size(int task_id);

int64_t download_engine_seek_task(int task_id, int64_t off, int whence);

ssize_t download_engine_read_task(int task_id, void* buf, size_t size);

#endif /* download_engine_h */

#ifdef __cplusplus
}
#endif
