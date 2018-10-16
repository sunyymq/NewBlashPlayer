#ifndef DE_NETWORK_H
#define DE_NETWORK_H

#include "common/DEInternal.h"

#include <curl/curl.h>

class DEBaseTask;
class DELooper;
class DETimerContext;

typedef enum NetworkEventType {
    NetworkEventTypeUnknow = -1,
    NetworkEventTypeContentLength,
}NetworkEventType;

class DENetwork {
public:
    static DENetwork* Create(DELooper* task_looper);
    static void Destroy(DENetwork* network);
    
public:
    CURLM* getCurlMulti() {
        return mCurlMulti;
    }
    
    void checkMultiInfo(int stillRunning);
    
public:
    DETimerContext* timer_context;
    DELooper* task_looper;
    CURLM *mCurlMulti;

protected:
    DENetwork(DELooper* task_looper);
    ~DENetwork();

    virtual int initContext();
    virtual void uninitContext();
};

#endif
