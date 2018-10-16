//
//  NBBeastSource.cpp
//  NBMediaPlayer
//
//  Created by liu enbao on 27/09/2018.
//  Copyright © 2018 liu enbao. All rights reserved.
//

#include "NBBeastSource.h"

#include <NBLog.h>
#include <download_engine.h>

#ifdef __cplusplus
extern "C" {
#endif
    
#include <libavformat/avio.h>
#include <libavformat/avformat.h>
#include <libavutil/time.h>
    
#ifdef __cplusplus
}
#endif

#define LOG_TAG "NBFFmpegSource"

NBBeastSource::NBBeastSource(const NBString& uri)
:mUri(uri)
,mFormatCtx(NULL)
,mIOCtx(NULL)
,mCacheBuffer(NULL) {
    // download engine init
    download_engine_init(NULL, 0);
}

NBBeastSource::~NBBeastSource() {
    NBLOG_INFO(LOG_TAG, "nb data source closing : %p", mFormatCtx);
    if (mIOCtx != NULL) {
        avio_context_free(&mIOCtx);
    }
    
    if (mFormatCtx != NULL) {
        avformat_close_input(&mFormatCtx);
    }
    
    // download engine uninit
    download_engine_uninit();
}

void NBBeastSource::RegisterSource() {
    
}

void NBBeastSource::UnRegisterSource() {
    
}

int NBBeastSource::CheckUserInterupted(void* opaque) {
    NBBeastSource* dataSource = (NBBeastSource*)opaque;
    if (dataSource->mUserInterrupted) {
        return 1;
    }
    return 0;
}

int NBBeastSource::BeastReadPacket(void *opaque, uint8_t *buf, int buf_size) {
    NBBeastSource *c = (NBBeastSource*)opaque;
    int r;
    buf_size = FFMIN(buf_size, c->mCacheBufferLen);
    
    if (c->task_id == INVALID_TASK_ID) {
        NBLOG_ERROR(LOG_TAG, "beast_read invaid task_id!");
        return AVERROR(EIO);
    }
    
    do {
        if (CheckUserInterupted(opaque)) {
            return AVERROR_EXIT;
        }
        
        if (c->error) {
            NBLOG_ERROR(LOG_TAG, "beast_read error: %s(%d)", av_err2str(c->error), c->error);
            return c->error;
        }
        
        r = download_engine_read_task(c->task_id, buf, buf_size);
        
        if (r == 0) {
            av_usleep(5000);
        }
    } while (r == 0);
    
//    NBLOG_INFO(LOG_TAG, "beast_read r : %d", r);
    
    return (-1 == r) ? AVERROR_EOF : r;
}

int NBBeastSource::BeastWritePacket(void *opaque, uint8_t *buf, int buf_size) {
    return 0;
}

int64_t NBBeastSource::BeastSeek(void *opaque, int64_t offset, int whence) {
    NBBeastSource *c = (NBBeastSource*)opaque;
    int64_t ret;

    if (c->task_id == INVALID_TASK_ID) {
        NBLOG_ERROR(LOG_TAG, "beast_seek invaid task_id!");
        return AVERROR(EIO);
    }

    c->error = 0;

    if (whence == AVSEEK_SIZE) {
        return c->task_size;
    }

    ret = download_engine_seek_task(c->task_id, offset, whence);

    return ret < 0 ? AVERROR(ret) : ret;
}

// void beast_task_create_complete(int task_id, int result, void* user_data) {
//     BeastContext *c = (BeastContext *)user_data;
//     c->create_result = result;
// }

int NBBeastSource::bp_http_averror(int status_code, int default_averror) {
    switch (status_code) {
        case 400:
            return AVERROR_HTTP_BAD_REQUEST;
        case 401:
            return AVERROR_HTTP_UNAUTHORIZED;
        case 403:
            return AVERROR_HTTP_FORBIDDEN;
        case 404:
            return AVERROR_HTTP_NOT_FOUND;
        default:
            break;
    }
    
    if (status_code >= 400 && status_code <= 499)
        return AVERROR_HTTP_OTHER_4XX;
    else if (status_code >= 500)
        return AVERROR_HTTP_SERVER_ERROR;
    else
        return default_averror;
}

void NBBeastSource::process_callback_info(int what, int extra, void* user_data) {
    NBBeastSource *c = (NBBeastSource *)user_data;
    
    // LOGI("process_callback_info what=%d, extra=%d)", what, extra);
    
    switch (what) {
        case DL_INFO_TASK_CREATE_COMPLETE:
            c->create_result = extra;
            break;
        case DL_INFO_DOWNLOAD_SPEED:
            c->download_speed = extra;
            break;
        default:
            NBLOG_ERROR(LOG_TAG, "unknown info(%d, %d)", what, extra);
            break;
    }
}

void NBBeastSource::process_callback_error(int what, int extra, void* user_data) {
    NBBeastSource *c = (NBBeastSource *)user_data;
    
    NBLOG_ERROR(LOG_TAG, "process_callback_error what=%d, extra=%d)", what, extra);
    
    switch (what) {
        case DL_ERROR_HTTP_RETURN:
            c->error = bp_http_averror(extra, AVERROR(EIO));
            break;
        default:
            c->error = AVERROR(EIO);
            break;
    }
}

void NBBeastSource::beast_download_engine_callback(int task_id, int what, int arg1, int arg2, void* user_data) {
    // LOGI("beast_download_engine_callback (%d, %d, %d)", what, arg1, arg2);
    switch(what) {
        case DL_EVENT_INFO:
            process_callback_info(arg1, arg2, user_data);
            break;
        case DL_EVENT_ERROR:
            process_callback_error(arg1, arg2, user_data);
            break;
        default:
            NBLOG_WARN(LOG_TAG, "unknown callback event(%d, %d, %d)", what, arg1, arg2);
            break;
    }
}

nb_status_t NBBeastSource::initCheck(const NBMap<NBString, NBString>* params) {
    nb_status_t rc = OK;
    AVDictionary* openDict = NULL;
    if (params != NULL) {
        //do something with params
    }
    
    AVIOInterruptCB interrupt_callback = {
        .callback = CheckUserInterupted,
        .opaque = (void*)this,
    };
    
    CREATE_TASK_PARAMS create_task_params = {0};
    strcpy(create_task_params.task_url, mUri.string());
    
//    if (c->save_path != NULL)
//        strcpy(create_task_params.save_path, c->save_path);
//
//    if (c->save_name != NULL)
//        strcpy(create_task_params.save_name, c->save_name);
    
    create_task_params.begin_off = 0;
    create_task_params.end_off = 0;
    
    NBLOG_ERROR(LOG_TAG, "beast_open : %s begin_off : %lld end_off : %lld", create_task_params.task_url, create_task_params.begin_off, create_task_params.end_off);
    
    error = 0;
    create_result = INVALID_DATA;
    task_id = download_engine_create_task(&create_task_params, beast_download_engine_callback, this);
    if (task_id == INVALID_TASK_ID) {
        NBLOG_ERROR(LOG_TAG, "beast_open download_engine_create_task failed");
        return AVERROR(EIO);
    }
    
    int retry = 0;
    do {
        if (CheckUserInterupted(this)) {
            NBLOG_WARN(LOG_TAG, "beast_open create interrupt, beast_close");
            return AVERROR_EXIT;
        }
        
        if (error) {
            NBLOG_ERROR(LOG_TAG, "beast_open error: %s(%d)", av_err2str(error), error);
            return error;
        }
        
        if (create_result == INVALID_DATA) {
            // LOGE("beast_open c->create_result: %d", c->create_result);
            av_usleep(1000);
            retry++;
        }
        
        if (retry > 15000) {
            NBLOG_WARN(LOG_TAG, "beast_open error, 15s time out!");
            return AVERROR(EIO);
        }
    } while (create_result == INVALID_DATA);
    
    NBLOG_INFO(LOG_TAG, "beast_open download_engine_start_task");
    
    //We should manual start the task
    download_engine_start_task(task_id);
    
    do {
        if (CheckUserInterupted(this)) {
            NBLOG_WARN(LOG_TAG, "beast_open create interrupt, beast_close");
            return AVERROR_EXIT;
        }
        
        if (error) {
            NBLOG_INFO(LOG_TAG, "beast_open error: %s(%d)", av_err2str(error), error);
            return error;
        }
        
        task_size = download_engine_get_task_size(task_id);
        
        if (task_size == -1) {
            av_usleep(500);
        }
    } while (task_size == -1);
    
    mCacheBufferLen = 32 * 1024;
    mCacheBuffer = (uint8_t*)av_malloc(mCacheBufferLen);
    
    mIOCtx = avio_alloc_context(mCacheBuffer, mCacheBufferLen, 0, this, BeastReadPacket, BeastWritePacket, BeastSeek);
    if (mIOCtx == NULL) {
        return UNKNOWN_ERROR;
    }
    
    mFormatCtx = avformat_alloc_context();
    mFormatCtx->pb = mIOCtx;
    mFormatCtx->interrupt_callback = interrupt_callback;
    
//    if ((rc = avio_open2(&mIOCtx, mUri.string(), AVIO_FLAG_READ, &interrupt_callback, &openDict))) {
//        char errbuf[AV_ERROR_MAX_STRING_SIZE] = { 0 };
//        NBLOG_ERROR(LOG_TAG, "The error string is is : %s", av_make_error_string(errbuf, AV_ERROR_MAX_STRING_SIZE, rc));
//        return UNKNOWN_ERROR;
//    }
    return rc;
}
