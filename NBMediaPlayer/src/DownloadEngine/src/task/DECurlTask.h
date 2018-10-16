#ifndef DE_CURL_TASK_H
#define DE_CURL_TASK_H

#include "DEBaseTask.h"
#include <curl/curl.h>

typedef struct DE_NETWORK_CONTEXT DE_NETWORK_CONTEXT;
typedef struct DE_IO_CONTEXT DE_IO_CONTEXT;

typedef struct DE_NETWORK_TASK_PRIV {
    struct list_head list;
    
    CURL* easy_handle;
    
    struct curl_slist *custom_header;
    
    char error[CURL_ERROR_SIZE];

    uint8_t* received_buf;
    uint32_t received_index;

    int64_t task_download_write_pos;

    int64_t task_begin_off;
    int64_t task_end_off;

    int64_t file_write_pos;

    //While the task priv belong to
    DEBaseTask* parent;

    int needSpace;

} DE_NETWORK_TASK_PRIV;

class DECurlTask : public DEBaseTask {
public:
    DECurlTask();
    virtual ~DECurlTask();

public:
    virtual void calculateDownloadSpeed();
    virtual void launchNewSegment();

    virtual int start();
    virtual void stop();

    virtual void startNetworkProcess(DE_NETWORK_TASK_PRIV* task_priv);
    void checkMultiCode(const char *where, CURLMcode code, DECurlTask* task_item);

    void handleError(CURLcode code, DE_NETWORK_TASK_PRIV* task_priv);
    void destroyTaskPriv(DE_NETWORK_TASK_PRIV* task_priv);

protected:
    virtual void purgeTasks();
    
    DE_NETWORK_TASK_PRIV* createTaskPriv(int64_t download_begin, int64_t download_end);
    void processIOEvent(DE_IO_CONTEXT* io_context, int fd, int action, int revents);
    
public:
    static void processIOEventCallback(DE_IO_CONTEXT* io_context, int fd, int action, int revents, void* user_data);
    
protected:
    NBString mEffectiveUrl;
};

#endif
