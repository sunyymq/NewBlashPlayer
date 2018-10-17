//
// Created by parallels on 9/9/18.
//

#include "NBFFmpegVDecoder.h"
#include "NBVideoFrame.h"
#include "foundation/NBFoundation.h"

#ifdef __cplusplus
extern "C"
{
#endif

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>

#ifdef __cplusplus
}
#endif

// enum AVPixelFormat NBFFmpegVDecoder::get_format(AVCodecContext *s, const enum AVPixelFormat *pix_fmts)
// {
//     NBFFmpegVDecoder* vDecoder = (NBFFmpegVDecoder*)s->opaque;
//     const enum AVPixelFormat *p;
//     int ret;
    
//     for (p = pix_fmts; *p != -1; p++) {
//         const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(*p);
        
//         if (!(desc->flags & AV_PIX_FMT_FLAG_HWACCEL))
//             break;
        
//         if (*p == AV_PIX_FMT_VIDEOTOOLBOX) {
//             if (av_videotoolbox_default_init(s) == 0)
//                 break;
//         }
//     }
    
//     if (*p == AV_PIX_FMT_VIDEOTOOLBOX) {
//         vDecoder->mMetaData.setCString(kKeyDecoderComponent, MEDIA_DECODE_COMPONENT_VIDEOTOOLBOX);
//     } else {
//         vDecoder->mMetaData.setCString(kKeyDecoderComponent, MEDIA_DECODE_COMPONENT_FFMPEG);
//     }
    
//     return *p;
// }

NBFFmpegVDecoder::NBFFmpegVDecoder(NBMediaSource* mediaTrack)
    :mMediaTrack(mediaTrack)
    ,mVCodecCxt(NULL)
    ,mSwsContext(NULL)
    ,mCachedBuffer(NULL)
    ,mDecoderReorderPts(DRP_AUTO) {

}

NBFFmpegVDecoder::~NBFFmpegVDecoder() {

}

nb_status_t NBFFmpegVDecoder::start(NBMetaData *params) {
    NBMetaData* videoMeta = mMediaTrack->getFormat();
    if(!videoMeta->findPointer(kKeyFFmpegStream, (void**)&mVStream)) {
        return UNKNOWN_ERROR;
    }
    videoMeta->findInt64(kKeyStartTime, &mStartTime);

    if (mVStream == NULL) {
        return UNKNOWN_ERROR;
    }

    AVCodecParameters* codecpar = mVStream->codecpar;

    if (!openVideoCodecContext(codecpar)) {
        return UNKNOWN_ERROR;
    }

    mDecodedFrame = av_frame_alloc();
    if (mDecodedFrame == NULL) {
        return UNKNOWN_ERROR;
    }
    
    if (mVCodecCxt->pix_fmt != AV_PIX_FMT_YUV420P
        && mVCodecCxt->pix_fmt != AV_PIX_FMT_YUVJ420P) {
        mScaledFrame = av_frame_alloc();
        mScaledFrame->width = mVCodecCxt->width;
        mScaledFrame->height = mVCodecCxt->height;
        mScaledFrame->format = AV_PIX_FMT_YUV420P;
    }

    mCachedBuffer = NULL;
    
    mMetaData.setInt32(kKeyWidth, mVStream->codecpar->width);
    mMetaData.setInt32(kKeyHeight, mVStream->codecpar->height);

    //start the video track
    mMediaTrack->start();

    return OK;
}

bool NBFFmpegVDecoder::openVideoCodecContext(AVCodecParameters* codecpar) {
    int ret = true;
//    AVDictionary* pOpenDict = NULL;
    AVDictionary *opts = NULL;
    AVDictionaryEntry *t = NULL;

    mVCodecCxt = avcodec_alloc_context3(NULL);

    ret = avcodec_parameters_to_context(mVCodecCxt, codecpar);
    if (ret < 0) {
        return false;
    }
    av_codec_set_pkt_timebase(mVCodecCxt, mVStream->time_base);

//    //Make the codec full cpu run
//    mVCodecCxt->thread_count = 16;
    
//    mVCodecCxt->get_format = get_format;
//    mVCodecCxt->opaque = this;
//    mVCodecCxt->thread_safe_callbacks = 1;

    mMetaData.setCString(kKeyDecoderComponent, MEDIA_DECODE_COMPONENT_FFMPEG);
    
    int fast = 0;
//    int lowres = 0;
    
    int stream_lowres = 0;

    AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
    if(stream_lowres > av_codec_get_max_lowres(codec)){
        printf("The maximum value for lowres supported by the decoder is %d\n",
               av_codec_get_max_lowres(codec));
        stream_lowres = av_codec_get_max_lowres(codec);
    }
    av_codec_set_lowres(mVCodecCxt, stream_lowres);
    
#if FF_API_EMU_EDGE
    if(stream_lowres) mVCodecCxt->flags |= CODEC_FLAG_EMU_EDGE;
#endif
    if (fast)
        mVCodecCxt->flags2 |= AV_CODEC_FLAG2_FAST;
#if FF_API_EMU_EDGE
    if(codec->capabilities & AV_CODEC_CAP_DR1)
        mVCodecCxt->flags |= CODEC_FLAG_EMU_EDGE;
#endif
    
//    opts = filter_codec_opts(codec_opts, mVCodecCxt->codec_id, ic, ic->streams[stream_index], codec);
    if (!av_dict_get(opts, "threads", NULL, 0))
        av_dict_set(&opts, "threads", "auto", 0);
    if (stream_lowres)
        av_dict_set_int(&opts, "lowres", stream_lowres, 0);
    if (mVCodecCxt->codec_type == AVMEDIA_TYPE_VIDEO)
        av_dict_set(&opts, "refcounted_frames", "1", 0);
    if ((ret = avcodec_open2(mVCodecCxt, codec, &opts)) < 0) {
        return false;
    }
    if ((t = av_dict_get(opts, "", NULL, AV_DICT_IGNORE_SUFFIX))) {
        printf("Option %s not found.\n", t->key);
        ret =  AVERROR_OPTION_NOT_FOUND;
        return false;
    }

    return true;
}

nb_status_t NBFFmpegVDecoder::stop() {
    if (mMediaTrack != NULL) {
        mMediaTrack->stop();
    }
    
    av_frame_free(&mScaledFrame);
    if (mVCodecCxt->pix_fmt != AV_PIX_FMT_YUV420P
        && mVCodecCxt->pix_fmt != AV_PIX_FMT_YUVJ420P) {
        av_frame_free(&mDecodedFrame);
    }

    if (mCachedBuffer != NULL) {
        delete mCachedBuffer;
        mCachedBuffer = NULL;
    }
    
//    if (mVCodecCxt != NULL) {
//        avcodec_close(mVCodecCxt);
//    }
    
    avcodec_free_context(&mVCodecCxt);
    
    return OK;
}

NBMetaData* NBFFmpegVDecoder::getFormat() {
    return &mMetaData;
}

// Discard nonref ref and looperfilter if video is far delay audio
void NBFFmpegVDecoder::discardNonRef() {
    if (mVCodecCxt->skip_frame == AVDISCARD_DEFAULT) {
        mVCodecCxt->skip_frame = AVDISCARD_NONREF;
    }
    
    if (mVCodecCxt->skip_loop_filter == AVDISCARD_DEFAULT) {
        mVCodecCxt->skip_loop_filter = AVDISCARD_ALL;
    }
}

// restore the default discard state
void NBFFmpegVDecoder::restoreDiscard() {
    if (mVCodecCxt->skip_frame == AVDISCARD_NONREF) {
        mVCodecCxt->skip_frame = AVDISCARD_DEFAULT;
    }
    if (mVCodecCxt->skip_loop_filter == AVDISCARD_ALL) {
        mVCodecCxt->skip_loop_filter = AVDISCARD_DEFAULT;
    }
}

// Returns a new buffer of data. Call blocks until a
// buffer is available, an error is encountered of the end of the stream
// is reached.
// End of stream is signalled by a result of ERROR_END_OF_STREAM.
// A result of INFO_FORMAT_CHANGED indicates that the format of this
// MediaSource has changed mid-stream, the client can continue reading
// but should be prepared for buffers of the new configuration.
nb_status_t NBFFmpegVDecoder::read(
        NBMediaBuffer **buffer, ReadOptions *options) {
    int ret = OK;
    int64_t seekTimeUs = 0;
    ReadOptions::SeekMode seekMode = ReadOptions::SeekMode::SEEK_CLOSEST;
    
    if (options != NULL && options->getSeekTo(&seekTimeUs, &seekMode)) {
        if (mCachedBuffer != NULL) {
            delete mCachedBuffer;
            mCachedBuffer = NULL;
        }
    }

    for (;;) {
        ret = avcodec_receive_frame(mVCodecCxt, mDecodedFrame);
        
//        printf("The ret is : %d eagain : %d\n", ret, AVERROR(EAGAIN));
        
        if (ret >= 0) {
            if (mDecodedFrame->format == AV_PIX_FMT_YUV420P
                || mDecodedFrame->format == AV_PIX_FMT_YUVJ420P) {
                mScaledFrame = mDecodedFrame;
            } else {
                mSwsContext = sws_getCachedContext(mSwsContext,
                                                   mVCodecCxt->width,
                                                   mVCodecCxt->height,
                                                   mVCodecCxt->pix_fmt,
                                                   mVCodecCxt->width,
                                                   mVCodecCxt->height,
                                                   AV_PIX_FMT_YUV420P,
                                                   SWS_BICUBIC,
                                                   NULL,
                                                   NULL,
                                                   NULL);
//                avpicture_alloc((AVPicture*)mSwscaledFrame, AV_PIX_FMT_YUV420P, mVideoCodecContext->width, mVideoCodecContext->height);
                
                av_frame_get_buffer(mScaledFrame, 0);
                
                if (mSwsContext != NULL) {
                    sws_scale(mSwsContext,
                              mDecodedFrame->data,
                              mDecodedFrame->linesize,
                              0,
                              mVCodecCxt->height,
                              mScaledFrame->data,
                              mScaledFrame->linesize);
                }
            }
            
            if (mDecoderReorderPts == DRP_AUTO) {
                mScaledFrame->pts = mDecodedFrame->best_effort_timestamp;
            } else if (mDecoderReorderPts != DRP_ON){
                mScaledFrame->pts = mDecodedFrame->pkt_dts;
            }

            //Calculate the display time
            if (mScaledFrame->pts != AV_NOPTS_VALUE)
                mScaledFrame->pts = av_q2d(mVStream->time_base) * mScaledFrame->pts * 1000;
            else
                mScaledFrame->pts = 0;

            *buffer = new NBVideoFrame(mScaledFrame, 0, false);
            return OK;
        } else if (ret == AVERROR_EOF) {
            return ERROR_END_OF_STREAM;
        }

        if (mCachedBuffer == NULL)
            ret = mMediaTrack->read(&mCachedBuffer, options);

        if (ret != OK) {
            if (ret == INFO_DISCONTINUITY) {
                avcodec_flush_buffers(mVCodecCxt);

//                if (mCachedBuffer != NULL) {
//                    delete mCachedBuffer;
//                    mCachedBuffer = NULL;
//                }
            }
            return ret;
        }

        AVPacket* videoPacket = (AVPacket*)mCachedBuffer->data();
        
//        printf("The videoPacket dts : %lld pts : %lld\n", videoPacket->dts, videoPacket->pts);
        
        if (avcodec_send_packet(mVCodecCxt, videoPacket) == AVERROR(EAGAIN)) {
            printf("Receive_frame and send_packet both returned EAGAIN, which is an API violation.\n");
            continue;
        }

        delete mCachedBuffer;
        mCachedBuffer = NULL;
    }
}
