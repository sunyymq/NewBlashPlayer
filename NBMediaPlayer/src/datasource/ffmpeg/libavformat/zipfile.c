/*
 * buffered file I/O
 * Copyright (c) 2001 Fabrice Bellard
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "libavutil/avstring.h"
#include "libavutil/internal.h"
#include "libavutil/opt.h"
#include "libavformat/avformat.h"
#if HAVE_DIRENT_H
#include <dirent.h>
#endif
#include <fcntl.h>
#if HAVE_IO_H
#include <io.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/stat.h>
#include <stdlib.h>
#include "os_support.h"
#include "url.h"
#include "zzip/zzip.h"

/* standard file protocol */

#ifdef CONFIG_ZIPFILE_PROTOCOL

typedef struct ZipFileContext {
    const AVClass *class;
    ZZIP_DIR* dir;
    ZZIP_FILE* file;
    int blocksize;
} ZipFileContext;

static const AVOption zipfile_options[] = {
    { "blocksize", "set I/O operation maximum block size", offsetof(ZipFileContext, blocksize), AV_OPT_TYPE_INT, { .i64 = INT_MAX }, 1, INT_MAX, AV_OPT_FLAG_ENCODING_PARAM },
    { NULL }
};

static const AVClass zipfile_class = {
    .class_name = "zipfile",
    .item_name  = av_default_item_name,
    .option     = zipfile_options,
    .version    = LIBAVUTIL_VERSION_INT,
};

static int zipfile_read(URLContext *h, unsigned char *buf, int size)
{
    ZipFileContext *c = h->priv_data;
    int ret;
    size = FFMIN(size, c->blocksize);
    ret = zzip_read(c->file, buf, size);
    if (ret == 0)
        return AVERROR_EOF;
    return (ret == -1) ? AVERROR(errno) : ret;
}

static int zipfile_write(URLContext *h, const unsigned char *buf, int size)
{
    return AVERROR(ENOTSUP);
}

static int zipfile_open(URLContext *h, const char *filename, int flags)
{
    ZipFileContext *c = h->priv_data;
    struct stat st;

    av_strstart(filename, "zip:", &filename);
    
    c->dir = zzip_opendir(filename);
    if (c->dir == NULL) {
        return AVERROR(EIO);
    }
    
    ZZIP_DIRENT* direct = zzip_readdir(c->dir);
    if (direct == NULL) {
        zzip_close(c->dir);
        return AVERROR(EIO);
    }
    
    c->file = zzip_file_open(c->dir, direct->d_name, 0);
    if (c->file == NULL) {
        zzip_close(c->dir);
        return AVERROR(EIO);
    }

    h->is_streamed = 0;

    /* Buffer writes more than the default 32k to improve throughput especially
     * with networked file systems */
    if (!h->is_streamed && flags & AVIO_FLAG_WRITE)
        h->min_packet_size = h->max_packet_size = 262144;

    return 0;
}

/* XXX: use llseek */
static int64_t zipfile_seek(URLContext *h, int64_t pos, int whence)
{
    ZipFileContext *c = h->priv_data;
    int64_t ret;

    if (whence == AVSEEK_SIZE) {
        
        ZZIP_STAT st = {0};
        
        ret = zzip_file_stat(c->file, &st);
        return ret < 0 ? AVERROR(ret) : st.st_size;
    }
    
    ret = zzip_seek(c->file, pos, whence);

    return ret < 0 ? AVERROR(ret) : ret;
}

static int zipfile_close(URLContext *h)
{
    ZipFileContext *c = h->priv_data;
    zzip_close(c->file);
    zzip_closedir(c->dir);
    return 0;
}

const URLProtocol ff_zipfile_protocol = {
    .name                = "zip",
    .url_open            = zipfile_open,
    .url_read            = zipfile_read,
    .url_write           = zipfile_write,
    .url_seek            = zipfile_seek,
    .url_close           = zipfile_close,
    .priv_data_size      = sizeof(ZipFileContext),
    .priv_data_class     = &zipfile_class,
    .default_whitelist   = "zipfile"
};

#endif // CONFIG_ZIPFILE_PROTOCOL
