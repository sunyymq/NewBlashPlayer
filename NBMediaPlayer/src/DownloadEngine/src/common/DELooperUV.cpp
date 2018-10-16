#include "DELooper.h"
#include <uv.h>

#include <queue>

#include <pthread.h>

#include "logger.h"

using namespace fasterplayer;

FPLOG_DEFINE("DOWNLOAD_ENGINE");

namespace downloadengine {

    // Looper Class begin

    typedef struct DE_LOOPER_COMMAND {
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
        }

        ~DE_LOOPER_COMMAND() {
            pthread_cond_destroy(&command_cond);
        }
    }DE_LOOPER_COMMAND;

    typedef struct DE_LOOPER_CONTEXT {
        //The loopers name
        std::string name;

        //The uv objects
        uv_loop_t looper;
        pthread_t looper_thread;
        uv_async_t looper_postman;

        pthread_mutex_t looper_mutex;
        pthread_cond_t looper_cond;

        bool looper_need_signal;

        std::queue<DE_LOOPER_COMMAND*> looper_cmd_queue;

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

            if (looper->looper_cmd_queue.empty()) {
                pthread_mutex_unlock(&looper->looper_mutex);
                break;
            }

            command = looper->looper_cmd_queue.front();
            looper->looper_cmd_queue.pop();

            // pthread_mutex_unlock(&looper->looper_mutex);

            command->cmd_func(command->user_data, command->params);

            // pthread_mutex_lock(&looper->looper_mutex);

            if (command->command_need_signal) {
                pthread_cond_signal(&command->command_cond);
            }
            pthread_mutex_unlock(&looper->looper_mutex);

            if (command->params != NULL) {
                delete command->params;
            }
            delete command;
        }
    }

    static void de_looper_postman_async_cb(uv_async_t* handle) {
        DE_LOOPER_CONTEXT* looper = (DE_LOOPER_CONTEXT*)handle->data;
        de_looper_process_one_step(looper);
    }

    static void* looper_thread_callback(void* arg) {
        DE_LOOPER_CONTEXT* looper = (DE_LOOPER_CONTEXT*)arg;

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

        uv_run(&looper->looper, UV_RUN_DEFAULT);

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

    DELooper* DELooper::Create(const std::string& looper_name,
                                    de_looper_init_func init_func,
                                    void* init_user_data,
                                    de_looper_uninit_func uninit_func,
                                    void* uninit_user_data) {
        DE_LOOPER_CONTEXT* privCtx = new DE_LOOPER_CONTEXT();
        if (privCtx == NULL) {
            return NULL;
        }

        privCtx->name = looper_name;

        privCtx->init_func = init_func;
        privCtx->init_user_data = init_user_data;
        privCtx->uninit_func = uninit_func;
        privCtx->uninit_user_data = uninit_user_data;

        privCtx->looper_need_signal = false;

        // uint32_t supported = ev_supported_backends();
        // FPINFO("bobmarshall de_looper_create supported : %d", supported);

        uv_loop_init(&privCtx->looper);

        uv_async_init(&privCtx->looper, &privCtx->looper_postman, de_looper_postman_async_cb);
        privCtx->looper_postman.data = privCtx;

        pthread_mutex_init(&privCtx->looper_mutex, NULL);
        pthread_cond_init(&privCtx->looper_cond, NULL);

        return new DELooper(privCtx);
    }

    void DELooper::Destroy(DELooper* looper) {
        if (looper == NULL) {
            return ;
        }

        while (!looper->mPrivCtx->looper_cmd_queue.empty()) {
            DE_LOOPER_COMMAND* command = looper->mPrivCtx->looper_cmd_queue.front();
            if (command->params != NULL) {
                delete command->params;
            }
            delete command;
            looper->mPrivCtx->looper_cmd_queue.pop();
        }

        pthread_mutex_destroy(&looper->mPrivCtx->looper_mutex);
        pthread_cond_destroy(&looper->mPrivCtx->looper_cond);

        uv_loop_close(&looper->mPrivCtx->looper);

        delete looper->mPrivCtx;
        delete looper;
    }

    int DELooper::start() {
        int rtn = 0;

        // ev_async_start(mPrivCtx->looper, &mPrivCtx->looper_postman);

        pthread_mutex_lock(&mPrivCtx->looper_mutex);

        mPrivCtx->looper_need_signal = true;

        rtn = pthread_create(&mPrivCtx->looper_thread, NULL, looper_thread_callback, mPrivCtx);

        pthread_cond_wait(&mPrivCtx->looper_cond, &mPrivCtx->looper_mutex);

        pthread_mutex_unlock(&mPrivCtx->looper_mutex);

        return rtn;
    }

    static void de_looper_command_stop(void* user_data, const ParamsType* params) {
        DE_LOOPER_CONTEXT* looper = (DE_LOOPER_CONTEXT*)user_data;
        uv_stop(&looper->looper);

        looper->is_running = false;
    }

    void DELooper::stop() {
        postCommandAsyncImediately(NULL, de_looper_command_stop, mPrivCtx);

        //Wait for the thread end
        pthread_join(mPrivCtx->looper_thread, NULL);

        // ev_async_stop(mPrivCtx->looper, &mPrivCtx->looper_postman);
    }

    void DELooper::suspend() {
        // ev_suspend(mPrivCtx->looper);
    }

    void DELooper::resume() {
        // ev_resume(mPrivCtx->looper);
    }

    int DELooper::postCommandAsyncImediately(const ParamsType* params, de_looper_command_func cmd_func, void* user_data) {
        int rtn = 0;

        pthread_mutex_lock(&mPrivCtx->looper_mutex);

        DE_LOOPER_COMMAND* command = new DE_LOOPER_COMMAND();

        command->cmd_func = cmd_func;
        command->user_data = user_data;
        command->command_need_signal = false;
        // command->command_cond = PTHREAD_COND_INITIALIZER;

        if (params != NULL) {
            command->params = new ParamsType(*params);
        }

        mPrivCtx->looper_cmd_queue.push(command);

        uv_async_send(&mPrivCtx->looper_postman);

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

        if (params != NULL) {
            command->params = new ParamsType(*params);
        }

        mPrivCtx->looper_cmd_queue.push(command);

        uv_async_send(&mPrivCtx->looper_postman);

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

        uv_poll_t io;

        DELooper* looper;

    } DE_IO_CONTEXT;

    DEIOContext::DEIOContext(DE_IO_CONTEXT* privCtx)
        :mPrivCtx(privCtx) {

    }

    DEIOContext::~DEIOContext() {

    }

    static void de_io_event_cb(uv_poll_t* handle, int status, int revents) {
        DE_IO_CONTEXT* io_context = (DE_IO_CONTEXT*)handle->data;

        int action = (revents & UV_READABLE ? DE_READ : 0) | (revents & UV_WRITABLE ? DE_WRITE : 0);

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
            events |= UV_READABLE;
        if (action & DE_WRITE)
            events |= UV_WRITABLE;

        FPINFO("DEIOContext::start fd : %d action : %d", mPrivCtx->fd, events);

        // ev_io_init(&mPrivCtx->io, de_io_event_cb, mPrivCtx->fd, events);
        uv_poll_init(&mPrivCtx->looper->mPrivCtx->looper, &mPrivCtx->io, mPrivCtx->fd);
        mPrivCtx->io.data = mPrivCtx;

        uv_poll_start(&mPrivCtx->io, events, de_io_event_cb);
        // ev_io_start(mPrivCtx->looper->mPrivCtx->looper, &mPrivCtx->io);
    }

    void DEIOContext::stop() {
        // ev_io_stop(mPrivCtx->looper->mPrivCtx->looper, &mPrivCtx->io);
        uv_poll_stop(&mPrivCtx->io);
    }

    //IO Class end

    //Timer Class begin

    typedef struct DE_TIMER_CONTEXT {
        de_timer_func timer_func;
        DELooper* looper;

        int64_t after;
        int64_t repeat;

        void* user_data;

        uv_timer_t timer_handler;
    }DE_TIMER_CONTEXT;

    DETimerContext::DETimerContext(DE_TIMER_CONTEXT* privCtx)
        :mPrivCtx(privCtx) {

    }

    DETimerContext::~DETimerContext() {

    }

    static void de_timer_event_cb(uv_timer_t* handle) {
        DE_TIMER_CONTEXT* timer_context = (DE_TIMER_CONTEXT*)handle->data;
        if (timer_context->timer_func != NULL) {
            timer_context->timer_func(timer_context, 0, timer_context->user_data);
        }
    }

    DETimerContext* DETimerContext::Create(DELooper* looper, int64_t after, int64_t repeat, de_timer_func timer_func, void* user_data) {
        DE_TIMER_CONTEXT* timer_context = new DE_TIMER_CONTEXT();
        timer_context->looper = looper;
        timer_context->after = after;
        timer_context->repeat = repeat;
        timer_context->user_data = user_data;
        timer_context->timer_func = timer_func;

        uv_timer_init(&timer_context->looper->mPrivCtx->looper, &timer_context->timer_handler);
        // ev_timer_init(&timer_context->timer_handler, de_timer_event_cb, (double)after / 1000, (double)repeat / 1000);
        timer_context->timer_handler.data = timer_context;

        return new DETimerContext(timer_context);
    }

    void DETimerContext::Destroy(DETimerContext* timerContext) {
        if (timerContext == NULL) {
            return ;
        }

        delete timerContext->mPrivCtx;
        delete timerContext;
    }

    int DETimerContext::start() {
        uv_timer_start(&mPrivCtx->timer_handler, de_timer_event_cb, mPrivCtx->after, mPrivCtx->repeat);
        // ev_timer_start(mPrivCtx->looper->mPrivCtx->looper, &mPrivCtx->timer_handler);
    }

    int DETimerContext::again() {
        uv_timer_again(&mPrivCtx->timer_handler);
        // ev_timer_again(mPrivCtx->looper->mPrivCtx->looper, &mPrivCtx->timer_handler);
    }

    void DETimerContext::stop() {
        uv_timer_stop(&mPrivCtx->timer_handler);
        // ev_timer_stop(mPrivCtx->looper->mPrivCtx->looper, &mPrivCtx->timer_handler);
    }

    //Timer Class end

    //Socket support
    de_open_socket_func de_looper_get_open_socket() {
        return NULL;
    }

    de_close_socket_func de_looper_get_close_socket() {
        return NULL;
    }

}