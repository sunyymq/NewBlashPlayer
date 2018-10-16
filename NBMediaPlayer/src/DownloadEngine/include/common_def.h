//
//  common_def.h
//  DownloadEngine
//
//  Created by liu enbao on 12/1/16.
//  Copyright © 2016 xb. All rights reserved.
//

#ifndef common_def_h
#define common_def_h

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef SUCCESS
#define SUCCESS 0
#endif

#ifndef FAILURE
#define FAILURE -1
#endif

#define DOWNLOAD_ENGINE_MAX_URL 1024

#define DOWNLOAD_ENGINE_MAX_PATH 1024

#define DOWNLOAD_ENGINE_MAX_ID 128

#define INVALID_TASK_ID -1

//The error never reach this value
#define INVALID_DATA -20121220

#define INVALID_OFFSET -1

enum download_engine_event {
    DL_EVENT_NOP            = 0, // interface test message
    DL_EVENT_ERROR          = 100,
    DL_EVENT_INFO           = 200,
};

enum download_engine_error_type {
    DL_ERROR_UNKNOWN         = 1,
    DL_ERROR_HTTP_RETURN     = 2,
};

enum download_engine_info_type {
    DL_INFO_UNKNOWN                 = 1,
    DL_INFO_TASK_CREATE_COMPLETE    = 2,
    DL_INFO_DOWNLOAD_SPEED          = 3,
};


typedef void (*download_engine_callback) (int task_id, int what, int arg1, int arg2, void* user_data);

#endif /* common_def_h */
