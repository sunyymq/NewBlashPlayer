//
// Created by parallels on 9/9/18.
//

#include "NBFFmpegADecoder.h"
#include "foundation/NBMetaData.h"
#include "NBAudioFrame.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "libswresample/swresample.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/opt.h"

#ifdef __cplusplus
}
#endif

NBFFmpegADecoder::NBFFmpegADecoder(NBMediaSource* mediaTrack)
    :mMediaTrack(mediaTrack)
    ,mACodecCtx(NULL)
    ,mCachedBuffer(NULL) {

}

NBFFmpegADecoder::~NBFFmpegADecoder() {

}

nb_status_t NBFFmpegADecoder::start(NBMetaData *params) {
    NBMetaData* audioMeta = mMediaTrack->getFormat();

    if (!audioMeta->findPointer(kKeyFFmpegStream, (void**)&mAStream)) {
        return UNKNOWN_ERROR;
    }
    audioMeta->findInt64(kKeyStartTime, &mStartTime);

    AVCodecParameters* codecpar = mAStream->codecpar;

    //Start the audio track
    mMediaTrack->start();

    if (!openAudioCodecContext(codecpar)) {
        return UNKNOWN_ERROR;
    }

    mMetaData.setInt32(kKeyAudioFromat, mACodecCtx->sample_fmt);
    mMetaData.setInt64(kKeyAudioChLayout, mACodecCtx->channel_layout);
    mMetaData.setInt32(kKeySampleRate, mACodecCtx->sample_rate);
    mMetaData.setInt32(kKeyChannelCount, mACodecCtx->channels);

    mDecodedFrame = av_frame_alloc();

    mCachedBuffer = NULL;

    //Define audio time scale
    if (1) {
        mStartPts = mAStream->start_time;
        mStartPtsTb = mAStream->time_base;
    } else {
        mStartPts = AV_NOPTS_VALUE;
    }

    return OK;
}

bool NBFFmpegADecoder::openAudioCodecContext(AVCodecParameters* codecpar) {
    AVDictionary* pOpenDict = NULL;

    mACodecCtx = avcodec_alloc_context3(NULL);
    if (mACodecCtx == NULL) {
        return false;
    }

    if (avcodec_parameters_to_context(mACodecCtx, codecpar) < 0) {
        return false;
    }
    av_codec_set_pkt_timebase(mACodecCtx, mAStream->time_base);

    AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
    av_dict_set(&pOpenDict, "refcounted_frames", "1", 0);
    if (!codec || (avcodec_open2(mACodecCtx, codec, &pOpenDict) < 0)) {
        avcodec_free_context(&mACodecCtx);
        return false;
    }

    return true;
}

nb_status_t NBFFmpegADecoder::stop() {
    
    if (mMediaTrack != NULL) {
        mMediaTrack->stop();
        mMediaTrack = NULL;
    }
    
    av_frame_free(&mDecodedFrame);
    
    if (mCachedBuffer != NULL) {
        delete mCachedBuffer;
        mCachedBuffer = NULL;
    }
    
//    if (mACodecCtx != NULL) {
//        avcodec_close(mACodecCtx);
//    }
    
    avcodec_free_context(&mACodecCtx);
    
    return OK;
}

NBMetaData* NBFFmpegADecoder::getFormat() {
    return &mMetaData;
}

nb_status_t NBFFmpegADecoder::read(NBMediaBuffer **buffer, ReadOptions *options) {
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
        ret = avcodec_receive_frame(mACodecCtx, mDecodedFrame);
        if (ret >= 0) {
            AVRational tb = (AVRational) {1, mDecodedFrame->sample_rate};
            if (mDecodedFrame->pts != AV_NOPTS_VALUE)
                mDecodedFrame->pts = av_rescale_q(mDecodedFrame->pts, av_codec_get_pkt_timebase(mACodecCtx), tb);
            else if (mNextPts != AV_NOPTS_VALUE)
                mDecodedFrame->pts = av_rescale_q(mNextPts, mNextPtsTb, tb);
            if (mDecodedFrame->pts != AV_NOPTS_VALUE) {
                mNextPts = mDecodedFrame->pts + mDecodedFrame->nb_samples;
                mNextPtsTb = tb;
            }

            if (mDecodedFrame->pts != AV_NOPTS_VALUE) {
                mDecodedFrame->pts = mDecodedFrame->pts * av_q2d(tb) * 1000;
            } else {
                mDecodedFrame->pts = 0;
            }

            *buffer = new NBAudioFrame(mDecodedFrame);
            return OK;
        } else if (ret == AVERROR_EOF) {
            avcodec_flush_buffers(mACodecCtx);
            return ERROR_END_OF_STREAM;
        }

        if (mCachedBuffer == NULL)
            ret = mMediaTrack->read(&mCachedBuffer, options);

        if (ret != OK) {
            if (ret == INFO_DISCONTINUITY) {
                avcodec_flush_buffers(mACodecCtx);

                mNextPts = mStartPts;
                mNextPtsTb = mNextPtsTb;

                if (mCachedBuffer != NULL) {
                    delete mCachedBuffer;
                    mCachedBuffer = NULL;
                }
            }
            return ret;
        }

        if (avcodec_send_packet(mACodecCtx, (AVPacket*)mCachedBuffer->data()) == AVERROR(EAGAIN)) {
            printf("Receive_frame and send_packet both returned EAGAIN, which is an API violation.\n");
            continue;
        }

//        av_packet_unref((AVPacket*)mCachedBuffer->data());
        delete mCachedBuffer;
        mCachedBuffer = NULL;
    }
}
