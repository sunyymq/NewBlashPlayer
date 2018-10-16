#include "NBMediaCodecVDecoder.h"
#include "foundation/NBMetaData.h"
#include "NBMediaBuffer.h"
#include "NBVideoFrame.h"
#include "AndroidAVCUtils.h"
#include "AndroidSurfaceTexture.h"
#include "AndroidSurface.h"
#include "foundation/NBFoundation.h"

#include <NBLog.h>

#ifdef __cplusplus
extern "C"
{
#endif

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/pixdesc.h"

#ifdef __cplusplus
}
#endif


#define LOG_TAG "NBMediaCodecVDecoder"

#define MIN_HEVCC_LENGTH 23

#define SEC_2_MICROSEC 1000

NBMediaCodecVDecoder::NBMediaCodecVDecoder(NBMediaSource* videoTrack, void* surface)
    :mVideoStream(NULL)
    ,mSwsContext(NULL)
    ,mSwscaledFrame(NULL)
    ,mStartTime(0)
    ,mInputAformat(NULL)
    ,mOutputAformat(NULL)
    ,mAMediaCodec(NULL)
    ,mCacheMediaBuffer(NULL)
    ,mOutputBufferIndex(-1)
    ,mLastFramePTS(0)
    ,mReachTheEnd(false)
    ,mQueuedLastBuffer(false)
    ,mSurface(surface)
    ,mBitStreamFilterCtx(NULL) {
    mVideoTrack = videoTrack;
    // mMediaCodecName = mediaCodecName;
}

NBMediaCodecVDecoder::~NBMediaCodecVDecoder() {

}

nb_status_t NBMediaCodecVDecoder::start(NBMetaData *params) {
    NBLOG_INFO(LOG_TAG, "enter prepareDecodeContext");

    mMetaData.setCString(kKeyDecoderComponent, MEDIA_DECODE_COMPONENT_MEDIACODEC);

    NBMetaData* videoMeta = mVideoTrack->getFormat();
    if(!videoMeta->findPointer(kKeyFFmpegStream, (void**)&mVideoStream)) {
        NBLOG_ERROR(LOG_TAG, "Can not find the video stream");
        return UNKNOWN_ERROR;
    }
    videoMeta->findInt64(kKeyStartTime, &mStartTime);

    // FPINFO("bobmarshall The start time is : %lld", mStartTime);

    if (mVideoStream == NULL) {
        NBLOG_ERROR(LOG_TAG, "Can not find the video stream");
        return UNKNOWN_ERROR;
    }

    if (!openVideoCodecContext(mVideoStream->codec, videoMeta)) {
        NBLOG_ERROR(LOG_TAG, "Open video Context error");
        return UNKNOWN_ERROR;
    }

    if (AMediaCodec_start(mAMediaCodec) != AMEDIA_OK) {
        NBLOG_ERROR(LOG_TAG, "Open AMediaCodec_start error");
        return UNKNOWN_ERROR;
    }

    mVideoSkipStream = &mVideoStream->discard;

    mSwscaledFrame = av_frame_alloc();

    mSeekable = true;
    mLastFramePTS = 0;
    mReachTheEnd = false;
    mQueuedLastBuffer = false;

    return mVideoTrack->start();
}

// bool NBMediaCodecVDecoder::h264_extradata_to_annexb(AVCodecContext *avctx, const int padding, uint8_t** pOutData, int *outSize)
// {
//     const uint8_t *extradata1            = avctx->extradata;
//     // for(int i = 0 ; i < 4; ++i){
//         FPINFO("extradata = %02x",extradata1[0]);
//         FPINFO("extradata = %02x",extradata1[1]);
//         FPINFO("extradata = %02x",extradata1[2]);
//         FPINFO("extradata = %02x",extradata1[3]);
//     // }
//     uint16_t unit_size;
//     uint64_t total_size                 = 0;
//     uint8_t *out                        = NULL, unit_nb, sps_done = 0,
//              sps_seen                   = 0, pps_seen = 0;
//     const uint8_t *extradata            = avctx->extradata + 4;
//     static const uint8_t nalu_header[4] = { 0, 0, 0, 1 };
//     int length_size = (*extradata++ & 0x3) + 1; // retrieve length coded size

//     // ctx->sps_offset = ctx->pps_offset = -1;

//     /* retrieve sps and pps unit(s) */
//     unit_nb = *extradata++ & 0x1f; /* number of sps unit(s) */
//     if (!unit_nb) {
//         goto pps;
//     } else {
//         // ctx->sps_offset = 0;
//         sps_seen = 1;
//     }

//     while (unit_nb--) {
//         int err;

//         unit_size   = AV_RB16(extradata);
//         total_size += unit_size + 4;
//         if (total_size > INT_MAX - padding) {
//             FPERROR("Too big extradata size, corrupted stream or invalid MP4/AVCC bitstream\n");
//             av_free(out);
//             return false;
//         }
//         if (extradata + 2 + unit_size > avctx->extradata + avctx->extradata_size) {
//             FPERROR("Packet header is not contained in global extradata, "
//                    "corrupted stream or invalid MP4/AVCC bitstream");
//             av_free(out);
//             return false;
//         }
//         if ((err = av_reallocp(&out, total_size + padding)) < 0)
//             return err;
//         memcpy(out + total_size - unit_size - 4, nalu_header, 4);
//         memcpy(out + total_size - unit_size, extradata + 2, unit_size);
//         extradata += 2 + unit_size;
// pps:
//         if (!unit_nb && !sps_done++) {
//             unit_nb = *extradata++; /* number of pps unit(s) */
//             if (unit_nb) {
//                 // ctx->pps_offset = (extradata - 1) - (avctx->extradata + 4);
//                 pps_seen = 1;
//             }
//         }
//     }

//     if (out)
//         memset(out + total_size, 0, padding);

//     if (!sps_seen) {
//         FPERROR("Warning: SPS NALU missing or invalid. "
//                "The resulting stream may not play.");

//         if (out)
//             av_free(out);

//         return false;
//     }

//     if (!pps_seen) {
//         FPERROR("Warning: PPS NALU missing or invalid. "
//                "The resulting stream may not play.");

//         if (out)
//             av_free(out);

//         return false;
//     }

//     *pOutData      = out;
//     *outSize = total_size;

//     return true;
// }

// int NBMediaCodecVDecoder::hevc_extradata_to_annexb(AVCodecContext *avctx, uint8_t** out_extradata, int* out_extradata_size) {
//     GetByteContext gb;
//     int length_size, num_arrays, i, j;
//     int ret = 0;

//     uint8_t *new_extradata = NULL;
//     size_t   new_extradata_size = 0;

//     bytestream2_init(&gb, avctx->extradata, avctx->extradata_size);

//     bytestream2_skip(&gb, 21);
//     length_size = (bytestream2_get_byte(&gb) & 3) + 1;
//     num_arrays  = bytestream2_get_byte(&gb);

//     for (i = 0; i < num_arrays; i++) {
//         int type = bytestream2_get_byte(&gb) & 0x3f;
//         int cnt  = bytestream2_get_be16(&gb);

//         if (!(type == NAL_VPS || type == NAL_SPS || type == NAL_PPS ||
//               type == NAL_SEI_PREFIX || type == NAL_SEI_SUFFIX)) {
//             av_log(avctx, AV_LOG_ERROR, "Invalid NAL unit type in extradata: %d\n",
//                    type);
//             ret = AVERROR_INVALIDDATA;
//             goto fail;
//         }

//         for (j = 0; j < cnt; j++) {
//             int nalu_len = bytestream2_get_be16(&gb);

//             if (4 + AV_INPUT_BUFFER_PADDING_SIZE + nalu_len > SIZE_MAX - new_extradata_size) {
//                 ret = AVERROR_INVALIDDATA;
//                 goto fail;
//             }
//             ret = av_reallocp(&new_extradata, new_extradata_size + nalu_len + 4 + AV_INPUT_BUFFER_PADDING_SIZE);
//             if (ret < 0)
//                 goto fail;

//             AV_WB32(new_extradata + new_extradata_size, 1); // add the startcode
//             bytestream2_get_buffer(&gb, new_extradata + new_extradata_size + 4, nalu_len);
//             new_extradata_size += 4 + nalu_len;
//             memset(new_extradata + new_extradata_size, 0, AV_INPUT_BUFFER_PADDING_SIZE);
//         }
//     }

//     // if (!ctx->private_spspps) {
//     //     av_freep(&avctx->extradata);
//     //     avctx->extradata      = new_extradata;
//     //     avctx->extradata_size = new_extradata_size;
//     // }
//     // ctx->spspps_buf  = new_extradata;
//     // ctx->spspps_size = new_extradata_size;

//     if (out_extradata != NULL) {
//         *out_extradata = new_extradata;
//     } else {
//         FPWARN("The extradata output buffer is NULL");
//         av_freep(&new_extradata);
//     }

//     if (out_extradata_size != NULL) {
//         *out_extradata_size = new_extradata_size;
//     }

//     if (!new_extradata_size)
//         FPWARN("No parameter sets in the extradata");

//     return length_size;
// fail:
//     av_freep(&new_extradata);
//     return ret;
// }

bool NBMediaCodecVDecoder::openVideoCodecContext(AVCodecContext* codecContext, NBMetaData* metaData) {

    NBString mime_type("video/avc");

    // NBLOG_INFO(LOG_TAG, "The video mediaCodecName is : %s  width : %d height : %d", mMediaCodecName.c_str(), codecContext->width, codecContext->height);
     mAMediaCodec = AMediaCodecJava_createDecoderByType(getJNIEnv(), mime_type.string());
    if (mAMediaCodec == NULL) {
        NBLOG_ERROR(LOG_TAG, "AMediaCodecJava_createByCodecName failed");
        return false;
    }

    uint8_t * dummyData = NULL;
    int sizeDummyData = 0;

    int videoWidth = 0;
    int videoHeight = 0;
//    bool isNeedFilter = false;

    int32_t rotationDegrees;
    if (!mVideoTrack->getFormat()->findInt32(
                                             kKeyRotation, &rotationDegrees)) {
        rotationDegrees = 0;
    }

    if (rotationDegrees == 90 || rotationDegrees == 270) {
        videoWidth = codecContext->height;
        videoHeight = codecContext->width;
    } else {
        videoWidth = codecContext->width;
        videoHeight = codecContext->height;
    }

    NBLOG_ERROR(LOG_TAG, "codecContext->codec_id = %d and AV_CODEC_ID_H264 = %d, AV_CODEC_ID_HEVC = %d",codecContext->codec_id,AV_CODEC_ID_H264,AV_CODEC_ID_HEVC);

    if (codecContext->codec_id == AV_CODEC_ID_H264) {

        //ffmpeg读取mp4中的H264数据，并不能直接得到NALU，文件中也没有储存0x00000001的分隔符
        //前4个字0x000032ce表示的是nalu的长度
        const uint8_t *extradata = codecContext->extradata;

//        if(!(extradata[0] == 00 &&
//             extradata[1] == 00 &&
//             extradata[2] == 00 &&
//             extradata[3] == 01 )){
//
//            isNeedFilter = true;
//            if (!h264_extradata_to_annexb(codecContext, FF_INPUT_BUFFER_PADDING_SIZE, &dummyData,
//                                      &sizeDummyData)) {
//                return false;
//            }
//
//            if (sizeDummyData == 0) {
//                return false;
//            }
//        }


        mBitStreamFilterCtx = av_bitstream_filter_init("h264_mp4toannexb");

        mInputAformat = AMediaFormatJava_createVideoFormat(getJNIEnv(), mime_type.string(), videoWidth, videoHeight);
        if (mInputAformat == NULL) {
            NBLOG_ERROR(LOG_TAG, "AMediaFormatJava_createVideoFormat failed");
            return false;
        }

        NBLOG_INFO(LOG_TAG, "We created with h264");

    } else if (codecContext->codec_id == AV_CODEC_ID_HEVC) {

        // NBLOG_INFO(LOG_TAG, "avctx->extradata_size = %d, AV_RB24(avctx->extradata) = %d AV_RB32(avctx->extradata) = %d ",codecContext->extradata_size,AV_RB24(codecContext->extradata),AV_RB32(codecContext->extradata));
        // if(!(codecContext->extradata_size < MIN_HEVCC_LENGTH ||
        //     AV_RB24(codecContext->extradata) == 1           ||
        //     AV_RB32(codecContext->extradata) == 1)) {

        //         isNeedFilter = true;
        //         if (!hevc_extradata_to_annexb(codecContext, &dummyData,
        //                               &sizeDummyData)) {
        //             return false;
        //         }

        //         if (sizeDummyData == 0) {
        //             return false;
        //         }
        // }

        mBitStreamFilterCtx = av_bitstream_filter_init("hevc_mp4toannexb");

//        std::string mime_type = "video/hevc";
//
//        mInputAformat = AMediaFormatJava_createVideoFormat(getJNIEnv(), mime_type.c_str(), videoWidth, videoHeight);
//        if (mInputAformat == NULL) {
//            NBLOG_ERROR(LOG_TAG, "AMediaFormatJava_createVideoFormat failed");
//            return false;
//        }
    } else if(codecContext->codec_id == AV_CODEC_ID_MPEG4){
//        std::string mime_type = "video/mp4v-es";
//
//        mInputAformat = AMediaFormatJava_createVideoFormat(getJNIEnv(), mime_type.c_str(), videoWidth, videoHeight);
//        if (mInputAformat == NULL) {
//            NBLOG_ERROR(LOG_TAG, "AMediaFormatJava_createVideoFormat failed");
//            return false;
//        }
    }

    if(codecContext->codec_id == AV_CODEC_ID_MPEG4){
        AMediaFormat_setBuffer(mInputAformat, "csd-0", codecContext->extradata, codecContext->extradata_size);
    } else {
        int i = 0;
        for(i = 0; i < sizeDummyData - 3; i += 4) {
            NBLOG_INFO(LOG_TAG, "csd-0[%d]: %02x%02x%02x%02x\n",
                sizeDummyData,
                (int)dummyData[i+0],
                (int)dummyData[i+1],
                (int)dummyData[i+2],
                (int)dummyData[i+3]);
        }
        while (i < sizeDummyData) {
            NBLOG_INFO(LOG_TAG, "csd-0[%d]: %02x", sizeDummyData, dummyData[i]);
            ++i;
        }

        AMediaFormat_setBuffer(mInputAformat, "csd-0", dummyData, sizeDummyData);

        NBLOG_INFO(LOG_TAG, "The codecContext extradata_size : %d", sizeDummyData);

        av_free(dummyData);

    }

    AMediaFormat_setInt32(mInputAformat, "stride", codecContext->width);
    AMediaFormat_setInt32(mInputAformat, "slice-height", codecContext->height);
    AMediaFormat_setInt32(mInputAformat, "push-blank-buffers-on-shutdown", 0);

    // void* extHandler = getExtHandler();

    NBLOG_INFO(LOG_TAG, "The Surface is : %p texName : %d", mSurface);
    //configure is called
    amedia_status_t amc_ret = AMediaCodec_configure_surface(getJNIEnv(), mAMediaCodec, mInputAformat, (jobject)mSurface, NULL, 0);
    if (amc_ret != AMEDIA_OK) {
        NBLOG_ERROR(LOG_TAG, "%s:configure_surface: failed\n", __func__);
        // freeExtHandler();
        return false;
    }

    // mVideoCodecContext = codecContext;

    mVideoCodecContext = avcodec_alloc_context3(NULL);
    avcodec_copy_context(mVideoCodecContext, codecContext);

    // CodecContext* mediaCodecContext = new CodecContext();
    // mediaCodecContext->extHandler = mSurfaceTextureJava;

    // INFO("mSurfaceTextureJava = %p", mSurfaceTextureJava);

    // mediaCodecContext->texName = mRenderTextureId;

    // mCodecContext = this;

    // mExtData = new MediaBufferExtData();

    NBLOG_INFO(LOG_TAG, "Create HDA success");

    return true;
}

void NBMediaCodecVDecoder::closeVideoCodecContext() {
    if (mBitStreamFilterCtx != NULL) {
        av_bitstream_filter_close(mBitStreamFilterCtx);
        mBitStreamFilterCtx = NULL;
    }

    if (mAMediaCodec != NULL) {

        NBLOG_INFO(LOG_TAG, "media codec stop");
        AMediaCodec_stop(mAMediaCodec);

        NBLOG_INFO(LOG_TAG, "media codec AMediaCodec_decreaseReference");
        AMediaCodec_decreaseReferenceP(&mAMediaCodec);

        mAMediaCodec = NULL;
    }

    if (mVideoCodecContext != NULL) {
        avcodec_free_context(&mVideoCodecContext);
        mVideoCodecContext = NULL;
    }

    // if (mExtData != NULL) {
    //     delete mExtData;
    //     mExtData = NULL;
    // }

}

nb_status_t NBMediaCodecVDecoder::stop() {
    NBLOG_INFO(LOG_TAG, "video decode stop");
    closeVideoCodecContext();

    av_frame_free(&mSwscaledFrame);

    NBLOG_INFO(LOG_TAG, "video decode stop over");
    return OK;
}

NBMetaData* NBMediaCodecVDecoder::getFormat() {
    return &mMetaData;
}

nb_status_t NBMediaCodecVDecoder::read(
                NBMediaBuffer **buffer, ReadOptions *options) {
    nb_status_t err = OK;
    NBMediaBuffer* videoFrame = NULL;
    jlong timeout = 10000;
    int retryCount = 0;

    int64_t seekTimeUs;
    // ReadOptions::SeekMode seekMode;
    // if (mSeekable && options && options->getSeekTo(&seekTimeUs, &seekMode)) {
    //     NBLOG_INFO(LOG_TAG, "video got an seek");

    //     if (mCacheMediaBuffer != NULL) {
    //         mCacheMediaBuffer->freeData();
    //         delete mCacheMediaBuffer;
    //         mCacheMediaBuffer = NULL;
    //     }

    //     mReachTheEnd = false;
    //     mQueuedLastBuffer = false;

    //     for (;;) {
    //         err = mVideoTrack->read(&mCacheMediaBuffer, options);

    //         if (err == OK) {
    //             mCacheMediaBuffer->freeData();
    //             delete mCacheMediaBuffer;
    //             mCacheMediaBuffer = NULL;
    //         } else if (err == INFO_DISCONTINUITY ) {
    //             AMediaCodec_flush(mAMediaCodec);
    //             NBLOG_INFO(LOG_TAG, "flush packet, return INFO_DISCONTINUITY");
    //             return INFO_DISCONTINUITY;
    //         } else if (err == ERROR_END_OF_STREAM || err == ERROR_MEDIA_SEEK) {
    //             NBLOG_INFO(LOG_TAG, "read err: %d", err);
    //             return err;
    //         }
    //     }
    // }

    for(;;) {
        amedia_status_t    amc_ret  = AMEDIA_OK;
        ssize_t  input_buffer_index = 0;
        uint8_t* input_buffer_ptr   = NULL;
        size_t   input_buffer_size  = 0;
        size_t   copy_size          = 0;
        int64_t  time_stamp         = 0;
        ssize_t  output_buffer_index = 0;
        uint8_t* output_buffer_ptr  = NULL;
        size_t   output_buffer_size = 0;
        AVPacket* packet = NULL;
        err = OK;

        int64_t duration = 0;
        bool eos = false;
        // mVideoTrack->getCachedDuration(&duration, &eos);

        // if (mVideoTrack->isInterrupt()) {
        //     NBLOG_INFO(LOG_TAG, "We interrupt the datasource");
        //     return ERROR_IO;
        // }

        if (mCacheMediaBuffer == NULL && !mReachTheEnd)
            // remove all fake pictures
            err = mVideoTrack->read(&mCacheMediaBuffer, options);

        if (err == ERROR_END_OF_STREAM) {
            NBLOG_INFO(LOG_TAG, "We real reach the end");
            mReachTheEnd = true;
        } else if (err != OK ) {
            NBLOG_INFO(LOG_TAG, "mVideoTrack read error:%d", err);
            if (err == INFO_DISCONTINUITY ) {
                AMediaCodec_flush(mAMediaCodec);
                NBLOG_INFO(LOG_TAG, "flush packet, AMediaCodec_flush");
            }
            return err;
        }

        // if (!mReachTheEnd)
        //     mVideoTrack->decrimentCachedDuration(mCacheMediaBuffer->getDuration());

        if (!mQueuedLastBuffer) {
            //Returns the index of an input buffer to be filled with valid data 
            //or -1 if no such buffer is currently available
            input_buffer_index = AMediaCodec_dequeueInputBuffer(mAMediaCodec, 5000);

            // if (input_buffer_index < 0)
            //     NBLOG_INFO(LOG_TAG, "input_buffer_index = %d and mQueuedLastBuffer is %d", input_buffer_index, mQueuedLastBuffer);
        }

        //Queue the input buffer if needed
        if (input_buffer_index >= 0) {
            if (mReachTheEnd) {
                amc_ret = AMediaCodec_queueInputBuffer(mAMediaCodec, input_buffer_index, 0, 0, 0, 4);
                NBLOG_INFO(LOG_TAG, "queue the last input buffer with return : %d", amc_ret);
                mQueuedLastBuffer = true;
            }else {

                if(mBitStreamFilterCtx == NULL) {

                    packet = (AVPacket*)mCacheMediaBuffer->data();

                    input_buffer_ptr = AMediaCodec_getInputBuffer(mAMediaCodec, input_buffer_index, &input_buffer_size);
                    if (!input_buffer_ptr) {
                        NBLOG_ERROR(LOG_TAG, "%s: AMediaCodec_getInputBuffer failed\n", __func__);
                    }

                    copy_size = FFMIN(input_buffer_size, packet->size);
                    memcpy(input_buffer_ptr, packet->data, copy_size);

                    //INFO("The packet size is : %d", pkt.size);

                    time_stamp = packet->pts;
                    if (time_stamp == AV_NOPTS_VALUE && packet->dts != AV_NOPTS_VALUE)
                        time_stamp = packet->dts;

                    if (time_stamp != AV_NOPTS_VALUE) {
                        AVRational time_base = {1, 1};
                        time_stamp *= SEC_2_MICROSEC;
                        time_stamp = av_rescale_q(time_stamp, mVideoStream->time_base, time_base);

                        if (mStartTime > 0) {
                            time_stamp -= mStartTime;
                        }

                    } else {
                        time_stamp = 0;
                    }

                    amc_ret = AMediaCodec_queueInputBuffer(mAMediaCodec, input_buffer_index, 0, copy_size, time_stamp, (packet->flags & AV_PKT_FLAG_KEY) ? 1 : 0);

                    //FPINFO("AndroidMediaCodecVideoDecoder queueInputBuffer timestamps is : %lld", time_stamp);

                    if (amc_ret != AMEDIA_OK) {
                        NBLOG_ERROR(LOG_TAG, "Time queue input buffer error");
                    }

                    // mCacheMediaBuffer->freeData();
                    delete mCacheMediaBuffer;
                    mCacheMediaBuffer = NULL;
                } else { //for h264 and h265

                    packet = (AVPacket*)mCacheMediaBuffer->data();

                    AVPacket pkt = *packet;

                    if (packet && packet->data) {
                        int retFilter = av_bitstream_filter_filter(mBitStreamFilterCtx,
                                                   mVideoCodecContext,
                                                   NULL,
                                                   &pkt.data,
                                                   &pkt.size,
                                                   packet->data,
                                                   packet->size,
                                                   packet->flags & AV_PKT_FLAG_KEY);

                        if (retFilter < 0) {
                            //Goto the next loop
                            NBLOG_INFO(LOG_TAG, "h264 got retFilter error : %d", retFilter);

                            // drop invalid packet
                            // mCacheMediaBuffer->freeData();
                            delete mCacheMediaBuffer;
                            mCacheMediaBuffer = NULL;

                            return UNKNOWN_ERROR;
                            // input_buffer_ptr = AMediaCodec_getInputBuffer(mAMediaCodec, input_buffer_index, &input_buffer_size);
                            // if (!input_buffer_ptr) {
                            //     FPERROR("%s: AMediaCodec_getInputBuffer failed\n", __func__);
                            // }

                            // copy_size = FFMIN(input_buffer_size, packet->size);
                            // memcpy(input_buffer_ptr, packet->data, copy_size);
                        } else {
                            input_buffer_ptr = AMediaCodec_getInputBuffer(mAMediaCodec, input_buffer_index, &input_buffer_size);
                            if (!input_buffer_ptr) {
                                NBLOG_ERROR(LOG_TAG, "%s: AMediaCodec_getInputBuffer failed\n", __func__);
                            }

                            copy_size = FFMIN(input_buffer_size, pkt.size);
                            memcpy(input_buffer_ptr, pkt.data, copy_size);
                        }

                        // *packet = pkt;
                    }

                    //INFO("The packet size is : %d", pkt.size);

                    time_stamp = pkt.pts;
                    if (time_stamp == AV_NOPTS_VALUE && pkt.dts != AV_NOPTS_VALUE)
                        time_stamp = pkt.dts;
                    if (time_stamp != AV_NOPTS_VALUE) {
                        AVRational time_base = {1, 1};
                        time_stamp *= SEC_2_MICROSEC;
                        time_stamp = av_rescale_q(time_stamp, mVideoStream->time_base, time_base);

                        if (mStartTime > 0) {
                            time_stamp -= mStartTime;
                        }

                    } else {
                        time_stamp = 0;
                    }

                    amc_ret = AMediaCodec_queueInputBuffer(mAMediaCodec, input_buffer_index, 0, copy_size, time_stamp, (pkt.flags & AV_PKT_FLAG_KEY) ? 1 : 0);

                    if (amc_ret != AMEDIA_OK) {
                        NBLOG_ERROR(LOG_TAG, "Time queue input buffer error");
                    }

                    if (pkt.data) {
                        av_freep(&pkt.data);
                    }

                    // mCacheMediaBuffer->freeData();
                    delete mCacheMediaBuffer;
                    mCacheMediaBuffer = NULL;
                    
                }
            }
        }

        //Pull the output buffer if needed
        AMediaCodecBufferInfo bufferInfo = {0};
        output_buffer_index = AMediaCodec_dequeueOutputBuffer(mAMediaCodec, &bufferInfo, timeout);

        // if (output_buffer_index < 0)
        //     NBLOG_INFO(LOG_TAG, "output_buffer_index = %d", output_buffer_index);

        /*if (input_buffer_index == -1 
            && output_buffer_index == -1
            && mVideoTrack->isReachTheEnd()) {
            INFO("The Video Track is reach the end");
            return ERROR_END_OF_STREAM;
        }*/

        if (output_buffer_index == AMEDIACODEC__INFO_OUTPUT_BUFFERS_CHANGED) {
            retryCount = 0;
            NBLOG_INFO(LOG_TAG, "AMEDIACODEC__INFO_OUTPUT_BUFFERS_CHANGED");
        } else if (output_buffer_index == AMEDIACODEC__INFO_OUTPUT_FORMAT_CHANGED) {
            retryCount = 0;
            mOutputAformat = AMediaCodec_getOutputFormat(mAMediaCodec);
            if (mOutputAformat) {
                int width        = 0;
                int height       = 0;
                int color_format = 0;
                int stride       = 0;
                int slice_height = 0;
                int crop_left    = 0;
                int crop_top     = 0;
                int crop_right   = 0;
                int crop_bottom  = 0;

                AMediaFormat_getInt32(mOutputAformat, "width",          &width);
                AMediaFormat_getInt32(mOutputAformat, "height",         &height);
                AMediaFormat_getInt32(mOutputAformat, "color-format",   &color_format);

                AMediaFormat_getInt32(mOutputAformat, "stride",         &stride);
                AMediaFormat_getInt32(mOutputAformat, "slice-height",   &slice_height);
                AMediaFormat_getInt32(mOutputAformat, "crop-left",      &crop_left);
                AMediaFormat_getInt32(mOutputAformat, "crop-top",       &crop_top);
                AMediaFormat_getInt32(mOutputAformat, "crop-right",     &crop_right);
                AMediaFormat_getInt32(mOutputAformat, "crop-bottom",    &crop_bottom);

                mVideoWidth = width;
                mVideoHeight = height;
                mColorFormat = color_format;

                mSwscaledFrame->width = width;
                mSwscaledFrame->height = height;

                NBLOG_INFO(LOG_TAG,
                    "AMEDIACODEC__INFO_OUTPUT_FORMAT_CHANGED\n"
                    "    width-height: (%d x %d)\n"
                    "    color-format: (%s: 0x%x)\n"
                    "    stride:       (%d)\n"
                    "    slice-height: (%d)\n"
                    "    crop:         (%d, %d, %d, %d)\n"
                    ,
                    width, height,
                    AMediaCodec_getColorFormatName(color_format), color_format,
                    stride,
                    slice_height,
                    crop_left, crop_top, crop_right, crop_bottom);

                // //Todo use the crop for the right size
                // MediaBufferExtData* extData = (MediaBufferExtData*)mExtData;
                // extData->videoWidth = crop_right - crop_left + 1;
                // extData->videoHeight = crop_bottom - crop_top + 1;

            }
        } else if (output_buffer_index == AMEDIACODEC__INFO_TRY_AGAIN_LATER) {
            if(++retryCount > 15){
                return UNKNOWN_ERROR;
            }

            NBLOG_INFO(LOG_TAG, "AMEDIACODEC__INFO_TRY_AGAIN_LATER max times, return UNKNOWN_ERROR");

            timeout = 10 * 1000;
        } else if (output_buffer_index < 0){
            if(++retryCount > 15){
                NBLOG_INFO(LOG_TAG, "AMEDIACODEC unknown error max times, return UNKNOWN_ERROR");
                return UNKNOWN_ERROR;
            }

            NBLOG_INFO(LOG_TAG, "The output buffer is : %d", output_buffer_index);
            timeout = 10 * 1000;
        } else if (output_buffer_index >= 0) {
            retryCount = 0;
            output_buffer_ptr = AMediaCodec_getOutputBuffer(mAMediaCodec, output_buffer_index, &output_buffer_size);
            // if (output_buffer_ptr != NULL) {
            //     const AVPixFmtDescriptor *pix_desc = av_pix_fmt_desc_get(AV_PIX_FMT_YUV420P);
            //     int src_linesize[3];
            //     const uint8_t *src_data[3];

            //     src_linesize[0] = av_image_get_linesize(AV_PIX_FMT_YUV420P, mSwscaledFrame->width, 0);
            //     src_linesize[1] = av_image_get_linesize(AV_PIX_FMT_YUV420P, mSwscaledFrame->width, 1);
            //     src_linesize[2] = av_image_get_linesize(AV_PIX_FMT_YUV420P, mSwscaledFrame->width, 2);

            //     src_data[0] = output_buffer_ptr;
            //     src_data[1] = src_data[0] + src_linesize[0] * mSwscaledFrame->height;
            //     src_data[2] = src_data[1] + src_linesize[1] * -(-mSwscaledFrame->height >> pix_desc->log2_chroma_h);

            //     for (int i = 0; i < 3; ++i) {
            //         mSwscaledFrame->data[i] = src_data[i];
            //         mSwscaledFrame->linesize[i] = src_linesize[i];
            //     }
            // }

            mSwscaledFrame->pts = bufferInfo.presentationTimeUs;

            mOutputBufferIndex = output_buffer_index;

            if (mColorFormat == AMEDIACODEC__OMX_COLOR_FormatYUV420Planar) {
                mSwscaledFrame->format = AV_PIX_FMT_YUV420P;
            }else {
                // WARN("The Current format only support FormatYUV420Planar (0x13)");
            }

            if (mLastFramePTS == 0) {
                videoFrame = new NBVideoFrame(mSwscaledFrame, 0, false);
                mLastFramePTS = mSwscaledFrame->pts;
            }else {
                int64_t duration = mSwscaledFrame->pts - mLastFramePTS;
                if (duration < 0 || duration > 100000) {
                    duration = 45000;
                }
                videoFrame = new NBVideoFrame(mSwscaledFrame, duration, false);
                mLastFramePTS = mSwscaledFrame->pts;
            }

            // Reuse the channels for output buffer index
            mSwscaledFrame->data[0] = (uint8_t*)mAMediaCodec;
            mSwscaledFrame->linesize[0] = mOutputBufferIndex;

            err = OK;

            if ((bufferInfo.flags & AMEDIACODEC__BUFFER_FLAG_END_OF_STREAM) != 0) {
                return ERROR_END_OF_STREAM;
            }

            break;
        }
    }

    *buffer = videoFrame;
//    releaseDecodeContext(true);
    return err;
}
