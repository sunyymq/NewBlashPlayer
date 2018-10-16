#include "DELooper.h"
#include <ev.h>
#include <pthread.h>
#include <NBList.h>

// Looper Class begin
typedef struct DE_LOOPER_COMMAND {
    struct list_head list;
    
    const ParamsType* params;

    de_looper_command_func cmd_func;
    void* user_data;

    pthread_cond_t command_cond;
    bool command_need_signal;

    DE_LOOPER_COMMAND() {
        pthread_cond_init(&command_cond, NULL);
        command_need_signal = false;
        cmd_func = NULL;
        user_data = NULL;
        params = NULL;
        INIT_LIST_HEAD(&list);
    }

    ~DE_LOOPER_COMMAND() {
        pthread_cond_destroy(&command_cond);
    }
}DE_LOOPER_COMMAND;

typedef struct DE_LOOPER_CONTEXT {
    //The loopers name
    NBString name;

    //The uv objects
    struct ev_loop* looper;
    pthread_t looper_thread;
    ev_async looper_postman;

    pthread_mutex_t looper_mutex;
    pthread_cond_t looper_cond;

    bool looper_need_signal;

    struct list_head looper_cmd_queue;

    //Life cycle observer
    de_looper_init_func init_func;
    void* init_user_data;

    de_looper_uninit_func uninit_func;
    void* uninit_user_data;

    bool is_running;
} DE_LOOPER_CONTEXT;

static void de_looper_process_one_step(DE_LOOPER_CONTEXT* looper) {
    while (looper->is_running) {
        DE_LOOPER_COMMAND* command = NULL;

        pthread_mutex_lock(&looper->looper_mutex);
        
        if (list_empty(&looper->looper_cmd_queue)) {
            pthread_mutex_unlock(&looper->looper_mutex);
            break;
        }

        command = list_first_entry(&looper->looper_cmd_queue, DE_LOOPER_COMMAND, list);
        list_del(&command->list);
        
        command->cmd_func(command->user_data, command->params);

        if (command->command_need_signal) {
            pthread_cond_signal(&command->command_cond);
        }

        delete command;
        
        pthread_mutex_unlock(&looper->looper_mutex);
    }
}

static void de_looper_postman_async_cb(EV_P_ struct ev_async *handle, int revents) {
    DE_LOOPER_CONTEXT* looper = (DE_LOOPER_CONTEXT*)handle->data;
    de_looper_process_one_step(looper);
}

static void* looper_thread_callback(void* arg) {
    DE_LOOPER_CONTEXT* looper = (DE_LOOPER_CONTEXT*)arg;
#ifdef BUILD_TARGET_IOS
    pthread_setname_np(looper->name.string());
#else
#endif

    if (looper == NULL) {
        return NULL;
    }

    looper->is_running = true;

    if (looper->init_func != NULL) {
        looper->init_func(looper->init_user_data);
    }

    pthread_mutex_lock(&looper->looper_mutex);
    if (looper->looper_need_signal) {
        pthread_cond_signal(&looper->looper_cond);
    }
    pthread_mutex_unlock(&looper->looper_mutex);

    ev_run(looper->looper, 0);

    if (looper->uninit_func != NULL) {
        looper->uninit_func(looper->uninit_user_data);
    }

    pthread_mutex_lock(&looper->looper_mutex);
    if (looper->looper_need_signal) {
        pthread_cond_signal(&looper->looper_cond);
    }
    pthread_mutex_unlock(&looper->looper_mutex);

    return NULL;
}

DELooper::DELooper(DE_LOOPER_CONTEXT* privCtx)
    :mPrivCtx(privCtx) {

}

DELooper::~DELooper() {

}

DELooper* DELooper::Create(const NBString& looper_name,
                                de_looper_init_func init_func,
                                void* init_user_data,
                                de_looper_uninit_func uninit_func,
                                void* uninit_user_data) {
    DE_LOOPER_CONTEXT* privCtx = new DE_LOOPER_CONTEXT();
    if (privCtx == NULL) {
        return NULL;
    }

    privCtx->name.append(looper_name);

    privCtx->init_func = init_func;
    privCtx->init_user_data = init_user_data;
    privCtx->uninit_func = uninit_func;
    privCtx->uninit_user_data = uninit_user_data;

    privCtx->looper_need_signal = false;

    INIT_LIST_HEAD(&privCtx->looper_cmd_queue);
    
//    uint32_t supported = ev_supported_backends();
//    FPINFO("bobmarshall de_looper_create supported : %d", supported);

    privCtx->looper = ev_loop_new();

    ev_async_init(&privCtx->looper_postman, de_looper_postman_async_cb);
    privCtx->looper_postman.data = privCtx;

    pthread_mutex_init(&privCtx->looper_mutex, NULL);
    pthread_cond_init(&privCtx->looper_cond, NULL);

    return new DELooper(privCtx);
}

void DELooper::Destroy(DELooper* looper) {
    if (looper == NULL) {
        return ;
    }

    struct list_head *pos, *q;
    
    list_for_each_safe(pos, q, &looper->mPrivCtx->looper_cmd_queue){
        DE_LOOPER_COMMAND* command = list_entry(pos, DE_LOOPER_COMMAND, list);
        list_del(pos);
        delete command;
    }

    pthread_mutex_destroy(&looper->mPrivCtx->looper_mutex);
    pthread_cond_destroy(&looper->mPrivCtx->looper_cond);

    ev_loop_destroy(looper->mPrivCtx->looper);

    delete looper->mPrivCtx;
    delete looper;
}

int DELooper::start() {
    int rtn = 0;

    ev_async_start(mPrivCtx->looper, &mPrivCtx->looper_postman);

    pthread_mutex_lock(&mPrivCtx->looper_mutex);

    mPrivCtx->looper_need_signal = true;

    rtn = pthread_create(&mPrivCtx->looper_thread, NULL, looper_thread_callback, mPrivCtx);

    pthread_cond_wait(&mPrivCtx->looper_cond, &mPrivCtx->looper_mutex);

    pthread_mutex_unlock(&mPrivCtx->looper_mutex);

    return rtn;
}

static void de_looper_command_stop(void* user_data, const ParamsType* params) {
    DE_LOOPER_CONTEXT* looper = (DE_LOOPER_CONTEXT*)user_data;
    ev_break(looper->looper);

    looper->is_running = false;
}

void DELooper::stop() {
    postCommandAsyncImediately(NULL, de_looper_command_stop, mPrivCtx);

    //Wait for the thread end
    pthread_join(mPrivCtx->looper_thread, NULL);

    ev_async_stop(mPrivCtx->looper, &mPrivCtx->looper_postman);
}

void DELooper::suspend() {
    ev_suspend(mPrivCtx->looper);
}

void DELooper::resume() {
    ev_resume(mPrivCtx->looper);
}

int DELooper::postCommandAsyncImediately(const ParamsType* params, de_looper_command_func cmd_func, void* user_data) {
    int rtn = 0;

    pthread_mutex_lock(&mPrivCtx->looper_mutex);
    DE_LOOPER_COMMAND* command = new DE_LOOPER_COMMAND();

    command->cmd_func = cmd_func;
    command->user_data = user_data;
    command->command_need_signal = false;
    // command->command_cond = PTHREAD_COND_INITIALIZER;

    command->params = params;

    list_add_tail(&command->list, &mPrivCtx->looper_cmd_queue);
    
    ev_async_send(mPrivCtx->looper, &mPrivCtx->looper_postman);
    pthread_mutex_unlock(&mPrivCtx->looper_mutex);

    return rtn;
}

int DELooper::postCommandSyncImediately(const ParamsType* params, de_looper_command_func cmd_func, void* user_data) {
    int rtn = 0;

    pthread_mutex_lock(&mPrivCtx->looper_mutex);

    DE_LOOPER_COMMAND* command = new DE_LOOPER_COMMAND();
    command->cmd_func = cmd_func;
    command->user_data = user_data;
    command->command_need_signal = true;

    command->params = params;

    list_add_tail(&command->list, &mPrivCtx->looper_cmd_queue);

    ev_async_send(mPrivCtx->looper, &mPrivCtx->looper_postman);

    pthread_cond_wait(&command->command_cond, &mPrivCtx->looper_mutex);

    pthread_mutex_unlock(&mPrivCtx->looper_mutex);

    return rtn;
}

//Looper Class end

//IO Class begin

typedef struct DE_IO_CONTEXT {
    int fd;

    de_io_event_func io_func;
    void* user_data;

    struct ev_io io;

    DELooper* looper;

} DE_IO_CONTEXT;

DEIOContext::DEIOContext(DE_IO_CONTEXT* privCtx)
    :mPrivCtx(privCtx) {

}

DEIOContext::~DEIOContext() {

}

static void de_io_event_cb(EV_P_ struct ev_io *w, int revents) {
    DE_IO_CONTEXT* io_context = (DE_IO_CONTEXT*)w->data;

    int action = (revents & EV_READ ? DE_READ : 0) | (revents & EV_WRITE ? DE_WRITE : 0);

    io_context->io_func(io_context, io_context->fd, action, revents, io_context->user_data);
}

DEIOContext* DEIOContext::Create(DELooper* looper, int fd, de_io_event_func io_func, void* user_data) {
    DE_IO_CONTEXT* io_context = new DE_IO_CONTEXT();
    io_context->fd = fd;
    io_context->io_func = io_func;
    io_context->user_data = user_data;

    io_context->looper = looper;

    return new DEIOContext(io_context);
}

void DEIOContext::Destroy(DEIOContext* ioContext) {
    if (ioContext == NULL) {
        return ;
    }

    delete ioContext->mPrivCtx;
    delete ioContext;
}

int DEIOContext::start(int action) {

    int events = 0;
    if (action & DE_READ)
        events |= EV_READ;
    if (action & DE_WRITE)
        events |= EV_WRITE;

//    NBLOG_INFO(LOG_TAG, "DEIOContext::start fd : %d action : %d", mPrivCtx->fd, events);

    ev_io_init(&mPrivCtx->io, de_io_event_cb, mPrivCtx->fd, events);
    mPrivCtx->io.data = mPrivCtx;

    ev_io_start(mPrivCtx->looper->mPrivCtx->looper, &mPrivCtx->io);
    
    return 0;
}

void DEIOContext::stop() {
    ev_io_stop(mPrivCtx->looper->mPrivCtx->looper, &mPrivCtx->io);
}

//IO Class end

//Timer Class begin

typedef struct DE_TIMER_CONTEXT {
    de_timer_func timer_func;
    DELooper* looper;

    int64_t after;
    int64_t repeat;

    void* user_data;

    struct ev_timer timer_handler;
}DE_TIMER_CONTEXT;

DETimerContext::DETimerContext(DE_TIMER_CONTEXT* privCtx)
    :mPrivCtx(privCtx) {

}

DETimerContext::~DETimerContext() {

}

static void de_timer_event_cb(EV_P_ struct ev_timer *handle, int revents) {
    DE_TIMER_CONTEXT* timer_context = (DE_TIMER_CONTEXT*)handle->data;
    if (timer_context->timer_func != NULL) {
        timer_context->timer_func(timer_context, revents, timer_context->user_data);
    }
}

DETimerContext* DETimerContext::Create(DELooper* looper, int64_t after, int64_t repeat, de_timer_func timer_func, void* user_data) {
    DE_TIMER_CONTEXT* timer_context = new DE_TIMER_CONTEXT();
    timer_context->looper = looper;
    timer_context->after = after;
    timer_context->repeat = repeat;
    timer_context->user_data = user_data;
    timer_context->timer_func = timer_func;

    ev_timer_init(&timer_context->timer_handler, de_timer_event_cb, (double)after / 1000, (double)repeat / 1000);
    timer_context->timer_handler.data = timer_context;

    return new DETimerContext(timer_context);
}

void DETimerContext::reset(int64_t after, int64_t repeat) {
    ev_timer_init(&mPrivCtx->timer_handler, de_timer_event_cb, (double)after / 1000, (double)repeat / 1000);
}

void DETimerContext::Destroy(DETimerContext* timerContext) {
    if (timerContext == NULL) {
        return ;
    }

    delete timerContext->mPrivCtx;
    delete timerContext;
}

int DETimerContext::start() {
    ev_timer_start(mPrivCtx->looper->mPrivCtx->looper, &mPrivCtx->timer_handler);
    return 0;
}

int DETimerContext::again() {
    ev_timer_again(mPrivCtx->looper->mPrivCtx->looper, &mPrivCtx->timer_handler);
    return 0;
}

void DETimerContext::stop() {
    ev_timer_stop(mPrivCtx->looper->mPrivCtx->looper, &mPrivCtx->timer_handler);
}

//Timer Class end

//Socket support
de_open_socket_func de_looper_get_open_socket() {
    return NULL;
}

de_close_socket_func de_looper_get_close_socket() {
    return NULL;
}
