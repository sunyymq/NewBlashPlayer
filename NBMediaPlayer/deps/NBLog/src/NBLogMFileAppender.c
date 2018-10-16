//
// Created by liuenbao on 18-9-21.
//

#include "NBLogMFileAppender.h"
#include <NBLog.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

typedef struct MFileCtx {
    int logFile;
    char logDir[FILE_MAX];
    pthread_t bgThread;
} MFileCtx;

static void* BackgoundThreadLoop(void* params);

#define MAX_RETRY_CNT 3

void* mfile_appender_open(NBLogCtx* ctx) {
    char currLogName[FILE_MAX] = {0};
    MFileCtx* fileCtx = NULL;
    int lastPos = 0;
    const char* log_root = nb_log_get_root(ctx);
    if (log_root == NULL) {
        return NULL;
    }
    lastPos = strlen(log_root);

    fileCtx = (MFileCtx*)malloc(sizeof(MFileCtx));
    if (fileCtx == NULL) {
        return fileCtx;
    }
    memset(fileCtx, 0, sizeof(MFileCtx));

    strncpy(fileCtx->logDir, log_root, lastPos);
    if (fileCtx->logDir[lastPos - 1] != '/') {
        fileCtx->logDir[lastPos++] = '/';
    }

    struct timeval tv;
    gettimeofday(&tv, NULL);

    struct tm *ptm = NULL;

    time_t timep = tv.tv_sec;
    ptm = localtime(&timep);

    sprintf(&fileCtx->logDir[lastPos], "%04d_%02d_%02d",
            ptm->tm_year + 1900,
            ptm->tm_mon + 1,
            ptm->tm_mday);
    lastPos += 10;

    fileCtx->logDir[lastPos++] = '/';

    mkdir(fileCtx->logDir, S_IRWXU | S_IRWXG | S_IRWXO);

    snprintf(currLogName, FILENAME_MAX, "%s%d.log", fileCtx->logDir, ptm->tm_hour);

    fileCtx->logFile = open(currLogName, O_RDWR | O_CREAT, 0644);

    pthread_create(&fileCtx->bgThread, NULL, BackgoundThreadLoop, fileCtx);

    return fileCtx;
}

void mfile_appender_close(void* apdCtx) {
    MFileCtx* fileCtx = (MFileCtx*)apdCtx;
    if (fileCtx == NULL) {
        return ;
    }

    close(fileCtx->logFile);

    free(apdCtx);
}

static ssize_t nb_fprintf(int fd, const void *fmt, ...)
{
    ssize_t n;
    char *p = NULL;

    va_list args;
    va_start(args, fmt);
    size_t len  = nb_vscprintf(fmt, args);
    p = alloca(len + 1);
    vsnprintf(p, len + 1, fmt, args);
    p[len] = '\0';
    va_end(args);

    size_t left = len;
    size_t step = 1024 * 1024;
    int cnt = 0;
    if (fd == -1 || p == NULL || left == 0) {
        printf("%s paraments invalid, ", __func__);
        return -1;
    }
    while (left > 0) {
        if (left < step)
            step = left;
        n = write(fd, (void *)p, step);
        if (n > 0) {
            p += n;
            left -= n;
            continue;
        } else if (n == 0) {
            break;
        }
        if (errno == EINTR || errno == EAGAIN) {
            if (++cnt > MAX_RETRY_CNT) {
                printf("reach max retry count\n");
                break;
            }
            continue;
        } else {
            printf("write failed: %d\n", errno);
            break;
        }
    }
    return (len - left);
}

int mfile_appender_append(void* apdCtx, const LogMessage* message) {
    MFileCtx* fileCtx = (MFileCtx*)apdCtx;
    if (fileCtx == NULL) {
        return -1;
    }

    if (fileCtx->logFile == -1) {
        return -1;
    }

    struct tm *ptm = NULL;

    time_t timep = message->msg_time.tv_sec;
    ptm = localtime(&timep);

    nb_fprintf(fileCtx->logFile, "[%04d-%02d-%02d-%02d-%02d-%02d.%03ld] %s <%s>: %.*s\n",
            ptm->tm_year + 1900,
            ptm->tm_mon + 1,
            ptm->tm_mday,
            ptm->tm_hour,
            ptm->tm_min,
            ptm->tm_sec,
            message->msg_time.tv_usec / 1000,
            message->tag ? message->tag : "none",
            getPriorityLable(message->priority),
            message->msg_size,
            message->msg);

    return 0;
}

INBLogAppender logMFileAppender = {
        .open = mfile_appender_open,
        .append = mfile_appender_append,
        .close = mfile_appender_close,
};

static void* BackgoundThreadLoop(void* params) {
    MFileCtx* fileCtx = (MFileCtx*)params;
    return NULL;
}
