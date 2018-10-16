//
// Created by liuenbao on 18-9-21.
//

#include "NBLog.h"
#include "NBLogPriv.h"
#include "NBLogAppender.h"
#include "NBLogMFileAppender.h"
#include "NBLogConsoleAppender.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sys/time.h>

#define DEFAULT_LOG_PRIORITY NBLOG_PRI_DEFAULT

//NBLOG_UNKNOWN = 0,
//NBLOG_DEFAULT,    /* only for SetMinPriority() */
//NBLOG_VERBOSE,
//NBLOG_DEBUG,
//NBLOG_INFO,
//NBLOG_WARN,
//NBLOG_ERROR,
//NBLOG_FATAL,
//NBLOG_SILENT,     /* only for SetMinPriority(); must be last */

const char* arrPriorityLabel[] = {
        "UNKNOWN",
        "DEFAULT",
        "VERBOSE",
        "DEBUG",
        "INFO",
        "WARN",
        "ERROR",
        "FATAL",
        "SLIENT"
};

/**
 * NBLog init function
 * @param configure file path
 * @return
 */
NBLogCtx* nb_log_init(const char* log_root, unsigned int mode) {
    NBLogCtx* ctx = malloc(sizeof(NBLogCtx));
    if (ctx == NULL) {
        return ctx;
    }
    memset(ctx, 0, sizeof(struct NBLogCtx));
    ctx->logPriority = DEFAULT_LOG_PRIORITY;

    if (log_root != NULL) {
        ctx->log_root = strdup(log_root);
    }

    if (mode & NBLOG_MODE_MFILE) {
        if (log_root == NULL) {
            return NULL;
        }
        nb_log_reg_appender(ctx, "mfile", &logMFileAppender);
    }

    if (mode & NBLOG_MODE_CONSOLE) {
        nb_log_reg_appender(ctx, "console", &logConsoleAppender);
    }

    return ctx;
}

/**
 * NBLog get log root
 */
const char* nb_log_get_root(NBLogCtx* ctx) {
    return ctx->log_root;
}

/**
 * NBLog deinit function
 */
void nb_log_deinit(NBLogCtx* ctx) {
    if (ctx == NULL) {
        return ;
    }

    for (NBLogAppender* currAppender = ctx->appender_list;
         currAppender != NULL; currAppender = currAppender->next) {

        //Open the appender if needed
        if (currAppender->isOpened) {
            logAppenderClose(currAppender);
        }

        nb_log_unreg_appender(ctx, currAppender->name);
    }

    if (ctx->log_root != NULL) {
        free((void*)ctx->log_root);
    }

    free(ctx);
}

/**
 * Set the log priority
 * @param ctx
 * @param logPriority
 */
void nb_log_set_priority(NBLogCtx* ctx, NBLogPriority logPriority) {
    ctx->logPriority = logPriority;
}

/*
 * Send a simple string to the log.
 */
int nb_log_write(NBLogCtx* ctx, int prio, const char *tag, const char *text) {
    return nb_log_print(ctx, prio, tag, "%s", text);
}

/*
 * Send a formatted string to the log, used like printf(fmt,...)
 */
int nb_log_print(NBLogCtx* ctx, int prio, const char *tag,  const char *fmt, ...) {
    int rc = 0;
    va_list arg;
    va_start(arg, fmt);
    rc = nb_log_vprint(ctx, prio, tag, fmt, arg);
    va_end(arg);
    return rc;
}

/*
 * A variant of __android_log_print() that takes a va_list to list
 * additional parameters.
 */
int nb_log_vprint(NBLogCtx* ctx, int prio, const char *tag,
                  const char *fmt, va_list ap) {
    // Do not need print it
    if (prio < ctx->logPriority) {
        return -1;
    }

    LogMessage message = {0};

    // get current message time
    gettimeofday(&message.msg_time, NULL);

    int act_size = nb_vscprintf(fmt, ap);
    char* msg = alloca(act_size + 1);
    vsnprintf(msg, act_size + 1, fmt, ap);
    msg[act_size] = '\0';
    message.msg_size = act_size;
    message.msg = msg;
    message.priority = prio;
    message.tag = tag;

    for (NBLogAppender* currAppender = ctx->appender_list;
                currAppender != NULL; currAppender = currAppender->next) {

        INBLogAppender* iAppender = &currAppender->logAppender;

        //Open the appender if needed
        if (!currAppender->isOpened) {
            logAppenderOpen(ctx, currAppender);
        }

        iAppender->append(currAppender->appenderData, &message);
    }
    return 0;
}


/*
 * Log an assertion failure and SIGTRAP the process to have a chance
 * to inspect it, if a debugger is attached. This uses the FATAL priority.
 */
void nb_log_assert(NBLogCtx* ctx, const char *cond, const char *tag,
                   const char *fmt, ...) {
    va_list arg;
    va_start(arg, fmt);
    nb_log_vprint(ctx, NBLOG_PRI_FATAL, tag, fmt, arg);
    va_end(arg);

    assert(0);
}
