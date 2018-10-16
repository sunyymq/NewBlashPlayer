#ifndef DE_CURL_HTTP_TASK_H
#define DE_CURL_HTTP_TASK_H

#include "DECurlTask.h"

class DECurlHttpTask : public DECurlTask {
public:
    DECurlHttpTask();
    virtual ~DECurlHttpTask();

public:
    virtual void startNetworkProcess(DE_NETWORK_TASK_PRIV* task_priv);
    virtual void processHeaderScheme(void* privHandle);

private:
    static size_t httpHeaderCallback(char *buffer, size_t size, size_t nitems, void *userdata);
};

#endif
