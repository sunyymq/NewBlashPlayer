#ifndef DE_LOOPER_H
#define DE_LOOPER_H

#include "DEInternal.h"

typedef DEAny ParamsType;

static ParamsType* paramsCreate(int numVal) {
    return new ParamsType[numVal];
}

static void paramsDestroy(const ParamsType* params) {
    if (params == NULL) {
        return ;
    }
    delete []params;
}

static void paramsSetAtIndex(ParamsType* params, int index, const ParamsType& param) {
    params[index] = param;
}

static const ParamsType& paramsGetAtIndex(const ParamsType* params, int index) {
    return params[index];
}

//Looper Class
typedef void (*de_looper_init_func)(void* userp);
typedef void (*de_looper_uninit_func)(void* userp);

typedef void (*de_looper_command_func)(void* user_data, const ParamsType* params);

typedef struct DE_LOOPER_CONTEXT DE_LOOPER_CONTEXT;
typedef struct DE_IO_CONTEXT DE_IO_CONTEXT;
typedef struct DE_TIMER_CONTEXT DE_TIMER_CONTEXT;

class DELooper {
public:
    static DELooper* Create(const NBString& looper_name,
                                de_looper_init_func init_func,
                                void* init_user_data,
                                de_looper_uninit_func uninit_func,
                                void* uninit_user_data);

    static void Destroy(DELooper* looper);

public:
    int start();
    void stop();
    void suspend();
    void resume();

    int postCommandAsyncImediately(const ParamsType* params, de_looper_command_func cmd_func, void* user_data);
    int postCommandSyncImediately(const ParamsType* params, de_looper_command_func cmd_func, void* user_data);

public:
    DE_LOOPER_CONTEXT* mPrivCtx;

    friend struct DE_IO_CONTEXT;
    friend struct DE_TIMER_CONTEXT;

private:
    DELooper(DE_LOOPER_CONTEXT* privCtx);
    ~DELooper();
};

//IO Class
#define DE_NONE     0x00
#define DE_READ     0x01
#define DE_WRITE    0x02

typedef void (*de_io_event_func)(DE_IO_CONTEXT* io_context, int fd, int action, int revents, void* user_data);

class DEIOContext {
public:
    static DEIOContext* Create(DELooper* looper, int fd, de_io_event_func io_func, void* user_data);
    static void Destroy(DEIOContext* ioContext);

public:
    int start(int action);
    void stop();

private:
    DE_IO_CONTEXT* mPrivCtx;

private:
    DEIOContext(DE_IO_CONTEXT* privCtx);
    ~DEIOContext();
};

//Timer Class

typedef void (*de_timer_func)(DE_TIMER_CONTEXT* timer_context, int revents, void* user_data);

class DETimerContext {
public:
    static DETimerContext* Create(DELooper* looper, int64_t after, int64_t repeat, de_timer_func timer_func, void* user_data);
    static void Destroy(DETimerContext* timerContext);

public:
    DE_TIMER_CONTEXT* getPrivCtx() {
        return mPrivCtx;
    }
    
public:
    int start();
    void reset(int64_t after, int64_t repeat);
    int again();
    void stop();

private:
    DE_TIMER_CONTEXT* mPrivCtx;

private:
    DETimerContext(DE_TIMER_CONTEXT* privCtx);
    ~DETimerContext();
};

//Socket support
typedef int de_socket_t;

typedef de_socket_t (*de_open_socket_func)(DELooper* looper, void *user_data);
typedef void (*de_close_socket_func)(DELooper* looper, de_socket_t sockfd);

de_open_socket_func de_looper_get_open_socket();
de_close_socket_func de_looper_get_close_socket();

#endif
