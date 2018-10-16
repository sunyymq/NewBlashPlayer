//
//  NBBeastSource.hpp
//  NBMediaPlayer
//
//  Created by liu enbao on 27/09/2018.
//  Copyright © 2018 liu enbao. All rights reserved.
//

#ifndef NB_BEAST_SOURCE_H
#define NB_BEAST_SOURCE_H

#include "NBDataSource.h"

struct AVIOContext;

class NBBeastSource : public NBDataSource {
public:
    NBBeastSource(const NBString& uri);
    ~NBBeastSource();
    
public:
    virtual nb_status_t initCheck(const NBMap<NBString, NBString>* params);
    
    // return the current io context
    virtual void* getContext() {
        return mIOCtx;
    }
    
    virtual NBString getUri() {
        return mUri;
    }
    
private:
    static int BeastReadPacket(void *opaque, uint8_t *buf, int buf_size);
    static int BeastWritePacket(void *opaque, uint8_t *buf, int buf_size);
    static int64_t BeastSeek(void *opaque, int64_t offset, int whence);
    
    static int bp_http_averror(int status_code, int default_averror);
    static void process_callback_error(int what, int extra, void* user_data);
    static void process_callback_info(int what, int extra, void* user_data);
    static void beast_download_engine_callback(int task_id, int what, int arg1, int arg2, void* user_data);
    
public:
    static void RegisterSource();
    static void UnRegisterSource();
    
private:
    const NBString& mUri;
    AVIOContext* mIOCtx;
    uint8_t* mCacheBuffer;
    int mCacheBufferLen;
    int task_id;
    int error;
    int64_t task_size;
    int download_speed;
    int create_result;
    
private:
    static int CheckUserInterupted(void* opaque);
};

#endif /* NB_BEAST_SOURCE_H */
