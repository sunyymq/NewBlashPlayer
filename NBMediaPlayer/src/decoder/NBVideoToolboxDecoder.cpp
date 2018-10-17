//
//  NBVideoToolboxDecoder.cpp
//  iOSNBMediaPlayer
//
//  Created by liu enbao on 19/09/2018.
//  Copyright © 2018 liu enbao. All rights reserved.
//

#include "NBVideoToolboxDecoder.h"
#include "NBVideoFrame.h"
#include "foundation/NBFoundation.h"
#include <NBList.h>

#include <NBLog.h>

#ifdef __cplusplus
extern "C"
{
#endif
    
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavcodec/h264_parse.h>
#include <libavcodec/h2645_parse.h>
#include <libavcodec/bytestream.h>
#include <libavutil/intreadwrite.h>

#ifdef __cplusplus
}
#endif

#define LOG_TAG "NBVideoToolboxDecoder"

struct SortedQueueItem {
    struct list_head list;
    AVFrame frame;
};

typedef CMBlockBufferRef (*EncapsulateSampleBuffer)(HWAccelCtx* hwAccelCtx, AVPacket* videoPacket);

struct HWAccelCtx {
    int is_avc;
    CMVideoCodecType cm_codec_type;
    H264ParamSets h264ps;
    EncapsulateSampleBuffer encapsulateFunc;
};

static void DictSetString(CFMutableDictionaryRef dict, CFStringRef key, const char * value) {
    CFStringRef string;
    string = CFStringCreateWithCString(NULL, value, kCFStringEncodingASCII);
    CFDictionarySetValue(dict, key, string);
    CFRelease(string);
}

static void DictSetBoolean(CFMutableDictionaryRef dict, CFStringRef key, bool value) {
    CFDictionarySetValue(dict, key, value ? kCFBooleanTrue: kCFBooleanFalse);
}

static void DictSetObject(CFMutableDictionaryRef dict, CFStringRef key, CFTypeRef *value) {
    CFDictionarySetValue(dict, key, value);
}

//static void DictSetData(CFMutableDictionaryRef dict, CFStringRef key, AVCodecContext *avctx) {
//    CFDataRef data;
////    data = CFDataCreate(NULL, value, (CFIndex)length);
//    data = ff_videotoolbox_avcc_extradata_create(avctx);
//    CFDictionarySetValue(dict, key, data);
//    CFRelease(data);
//}

static void DictSetI32(CFMutableDictionaryRef dict, CFStringRef key, int32_t value) {
    CFNumberRef number;
    number = CFNumberCreate(NULL, kCFNumberSInt32Type, &value);
    CFDictionarySetValue(dict, key, number);
    CFRelease(number);
}

#define AV_W8(p, v) *(p) = (v)

/* --------------------- h264 begin ----------------------- */
CFDataRef NBVideoToolboxDecoder::ff_videotoolbox_avcc_extradata_create(AVCodecContext *avctx)
{
    int nal_length_size = 0;

    ff_h264_decode_extradata(avctx->extradata, avctx->extradata_size, &mHWAccelCtx->h264ps, &mHWAccelCtx->is_avc, &nal_length_size, 0, avctx);

    CFDataRef data = NULL;
    
    if (mHWAccelCtx->is_avc) {
        data = CFDataCreate(kCFAllocatorDefault, avctx->extradata, avctx->extradata_size);
    } else {
        /* currently active parameters sets */
        const PPS *pps = (const PPS*)mHWAccelCtx->h264ps.pps_list[0]->data;
        const SPS *sps = (const SPS*)mHWAccelCtx->h264ps.sps_list[0]->data;
        
        uint8_t *p;
        int vt_extradata_size = 6 + 2 + sps->data_size + 3 + pps->data_size;
        uint8_t *vt_extradata = (uint8_t*)av_malloc(vt_extradata_size);
        if (!vt_extradata)
            return NULL;

        p = vt_extradata;

        AV_W8(p + 0, 1); /* version */
        AV_W8(p + 1, sps->data[1]); /* profile */
        AV_W8(p + 2, sps->data[2]); /* profile compat */
        AV_W8(p + 3, sps->data[3]); /* level */
        AV_W8(p + 4, 0xff); /* 6 bits reserved (111111) + 2 bits nal size length - 3 (11) */
        AV_W8(p + 5, 0xe1); /* 3 bits reserved (111) + 5 bits number of sps (00001) */
        AV_WB16(p + 6, sps->data_size);
        memcpy(p + 8, sps->data, sps->data_size);
        p += 8 + sps->data_size;
        AV_W8(p + 0, 1); /* number of pps */
        AV_WB16(p + 1, pps->data_size);
        memcpy(p + 3, pps->data, pps->data_size);

        p += 3 + pps->data_size;
        av_assert0(p - vt_extradata == vt_extradata_size);

        data = CFDataCreate(kCFAllocatorDefault, vt_extradata, vt_extradata_size);
        av_free(vt_extradata);
    }
    
    return data;
}

CMBlockBufferRef H264EncapsulateSampleBuffer(HWAccelCtx* hwAccelCtx, AVPacket* videoPacket) {
//    CMSampleBufferRef sampleBuffer = NULL;
    CMBlockBufferRef blockBuffer = NULL;
    
    if (!hwAccelCtx->is_avc) {
        H2645Packet pkt = { 0 };
        int i, ret = 0;
        
        ret = ff_h2645_packet_split(&pkt, videoPacket->data, videoPacket->size, NULL, hwAccelCtx->is_avc, 2, AV_CODEC_ID_H264, 1);
        if (ret < 0) {
            ret = 0;
        }
        
        for (int i = 0; i < pkt.nb_nals; ++i) {
            switch (pkt.nals[i].type) {
                case H264_NAL_IDR_SLICE:
                case H264_NAL_SLICE:
                {
                    uint8_t* raw_data = (uint8_t*)pkt.nals[i].raw_data - 4;
                    size_t raw_size = pkt.nals[i].raw_size + 4;
                    AV_WB32(raw_data, raw_size - 4);
                    if (CMBlockBufferCreateWithMemoryBlock(kCFAllocatorDefault, raw_data, raw_size, kCFAllocatorNull, NULL, 0, raw_size, 0, &blockBuffer) != noErr) {
                        return NULL;
                    }
                }
                    break;
                default:
                    NBLOG_INFO(LOG_TAG, "The nal type is : %d", pkt.nals[i].type);
                    break;
            }
        }
        
        ff_h2645_packet_uninit(&pkt);
    } else {
        const size_t sampleSize = videoPacket->size;
        
        if (CMBlockBufferCreateWithMemoryBlock(NULL, videoPacket->data, videoPacket->size, kCFAllocatorNull, NULL, 0, videoPacket->size, 0, &blockBuffer) != noErr) {
            return NULL;
        }
    }
    
    return blockBuffer;
}
/* --------------------- h264 end ----------------------- */

/* --------------------- mpeg4 begin ----------------------- */
#define VIDEOTOOLBOX_ESDS_EXTRADATA_PADDING  12

static void videotoolbox_write_mp4_descr_length(PutByteContext *pb, int length)
{
    int i;
    uint8_t b;
    
    for (i = 3; i >= 0; i--) {
        b = (length >> (i * 7)) & 0x7F;
        if (i != 0)
            b |= 0x80;
        
        bytestream2_put_byteu(pb, b);
    }
}

static CFDataRef videotoolbox_esds_extradata_create(AVCodecContext *avctx)
{
    CFDataRef data;
    uint8_t *rw_extradata;
    PutByteContext pb;
    int full_size = 3 + 5 + 13 + 5 + avctx->extradata_size + 3;
    // ES_DescrTag data + DecoderConfigDescrTag + data + DecSpecificInfoTag + size + SLConfigDescriptor
    int config_size = 13 + 5 + avctx->extradata_size;
    int s;
    
    if (!(rw_extradata = (uint8_t*)av_mallocz(full_size + VIDEOTOOLBOX_ESDS_EXTRADATA_PADDING)))
        return NULL;
    
    bytestream2_init_writer(&pb, rw_extradata, full_size + VIDEOTOOLBOX_ESDS_EXTRADATA_PADDING);
    bytestream2_put_byteu(&pb, 0);        // version
    bytestream2_put_ne24(&pb, 0);         // flags
    
    // elementary stream descriptor
    bytestream2_put_byteu(&pb, 0x03);     // ES_DescrTag
    videotoolbox_write_mp4_descr_length(&pb, full_size);
    bytestream2_put_ne16(&pb, 0);         // esid
    bytestream2_put_byteu(&pb, 0);        // stream priority (0-32)
    
    // decoder configuration descriptor
    bytestream2_put_byteu(&pb, 0x04);     // DecoderConfigDescrTag
    videotoolbox_write_mp4_descr_length(&pb, config_size);
    bytestream2_put_byteu(&pb, 32);       // object type indication. 32 = AV_CODEC_ID_MPEG4
    bytestream2_put_byteu(&pb, 0x11);     // stream type
    bytestream2_put_ne24(&pb, 0);         // buffer size
    bytestream2_put_ne32(&pb, 0);         // max bitrate
    bytestream2_put_ne32(&pb, 0);         // avg bitrate
    
    // decoder specific descriptor
    bytestream2_put_byteu(&pb, 0x05);     ///< DecSpecificInfoTag
    videotoolbox_write_mp4_descr_length(&pb, avctx->extradata_size);
    
    bytestream2_put_buffer(&pb, avctx->extradata, avctx->extradata_size);
    
    // SLConfigDescriptor
    bytestream2_put_byteu(&pb, 0x06);     // SLConfigDescrTag
    bytestream2_put_byteu(&pb, 0x01);     // length
    bytestream2_put_byteu(&pb, 0x02);     //
    
    s = bytestream2_size_p(&pb);
    
    data = CFDataCreate(kCFAllocatorDefault, rw_extradata, s);
    
    av_freep(&rw_extradata);
    return data;
}

CMBlockBufferRef Mpeg4EncapsulateSampleBuffer(HWAccelCtx* hwAccelCtx, AVPacket* videoPacket) {
    CMBlockBufferRef blockBuffer = NULL;
    
    const size_t sampleSize = videoPacket->size;
    
    if (CMBlockBufferCreateWithMemoryBlock(NULL, videoPacket->data, videoPacket->size, kCFAllocatorNull, NULL, 0, videoPacket->size, 0, &blockBuffer) != noErr) {
        return NULL;
    }
    
    return blockBuffer;
}

/* --------------------- mpeg4 end ----------------------- */

CMFormatDescriptionRef NBVideoToolboxDecoder::CreateFormatDescriptionFromCodecData(CMVideoCodecType codec_type, int width, int height, AVCodecContext* codecCtx, uint32_t atom) {
    CMFormatDescriptionRef fmt_desc = NULL;
    OSStatus status;
    
    CFMutableDictionaryRef par = CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);
    CFMutableDictionaryRef atoms = CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);
    CFMutableDictionaryRef extensions = CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    
    /* CVPixelAspectRatio dict */
    DictSetI32(par, CFSTR ("HorizontalSpacing"), 0);
    DictSetI32(par, CFSTR ("VerticalSpacing"), 0);
    
    /* SampleDescriptionExtensionAtoms dict */
    CFDataRef data;
    switch (codec_type) {
        case kCMVideoCodecType_MPEG4Video :
            data = videotoolbox_esds_extradata_create(codecCtx);
            if (data)
                CFDictionarySetValue(atoms, CFSTR("esds"), data);
            mHWAccelCtx->encapsulateFunc = Mpeg4EncapsulateSampleBuffer;
            break;
        case kCMVideoCodecType_H264 :
            data = ff_videotoolbox_avcc_extradata_create(codecCtx);
            if (data)
                CFDictionarySetValue(atoms, CFSTR("avcC"), data);
            mHWAccelCtx->encapsulateFunc = H264EncapsulateSampleBuffer;
            break;
//        case kCMVideoCodecType_HEVC :
//            data = ff_videotoolbox_hvcc_extradata_create(avctx);
//            if (data)
//                CFDictionarySetValue(atoms, CFSTR("hvcC"), data);
//            break;
        default:
            break;
    }
    CFRelease(data);
    
//    //    data = CFDataCreate(NULL, value, (CFIndex)length);
//    data = ff_videotoolbox_avcc_extradata_create(codecCtx);
//    mHWAccelCtx->encapsulateFunc = H264EncapsulateSampleBuffer;
//    CFDictionarySetValue(atoms, CFSTR ("avcC"), data);
//    CFRelease(data);
//    DictSetData(atoms, CFSTR ("avcC"), codecCtx);
    
    /* Extensions dict */
    DictSetString(extensions, CFSTR ("CVImageBufferChromaLocationBottomField"), "left");
    DictSetString(extensions, CFSTR ("CVImageBufferChromaLocationTopField"), "left");
    DictSetBoolean(extensions, CFSTR("FullRangeVideo"), FALSE);
    DictSetObject(extensions, CFSTR ("CVPixelAspectRatio"), (CFTypeRef *) par);
    
    DictSetObject(extensions, kCMFormatDescriptionExtension_SampleDescriptionExtensionAtoms, (CFTypeRef *) atoms);
    
    status = CMVideoFormatDescriptionCreate(NULL, codec_type, width, height, extensions, &fmt_desc);
    
    CFRelease(extensions);
    CFRelease(atoms);
    CFRelease(par);
    
    if (status != 0)
        return NULL;
        
    return fmt_desc;
}

void NBVideoToolboxDecoder::DecompressionSessionCallback(void *decompressionOutputRefCon,
                                  void *sourceFrameRefCon,
                                  OSStatus status,
                                  VTDecodeInfoFlags infoFlags,
                                  CVImageBufferRef imageBuffer,
                                  CMTime presentationTimeStamp,
                                  CMTime presentationDuration) {
    NBVideoToolboxDecoder* videoDecoder = (NBVideoToolboxDecoder*)decompressionOutputRefCon;
    
    int64_t pts = CMTimeGetSeconds(presentationTimeStamp) * 1000;
    
    if (infoFlags & kVTDecodeInfo_FrameDropped) {
        return ;
    }
    
    {
        NBAutoMutex autoMutex(videoDecoder->mSortedLock);
        SortedQueueItem* dummyItem = NULL;
        list_for_each_entry(dummyItem, &(videoDecoder->mSortedHead), list) {
            if (dummyItem->frame.pts == pts) {
                dummyItem->frame.data[3] = (uint8_t*)CVBufferRetain(imageBuffer);
                break;
            }
        }
    }
    
    NBLOG_DEBUG(LOG_TAG, "DecompressionSessionCallback pts : %lld status : %d infoFlags : %d", pts, status, infoFlags);
}

NBVideoToolboxDecoder::NBVideoToolboxDecoder(NBMediaSource* mediaTrack)
    :mMediaTrack(mediaTrack)
    ,mDecoderReorderPts(DRP_AUTO)
    ,mFormatDescription(NULL)
    ,mDecompressSession(NULL)
    ,mHWAccelCtx(NULL) {
    
}

NBVideoToolboxDecoder::~NBVideoToolboxDecoder() {
    
}

nb_status_t NBVideoToolboxDecoder::start(NBMetaData *params) {
    
    int ret;
    
    mHWAccelCtx = new HWAccelCtx();
    
    NBMetaData* videoMeta = mMediaTrack->getFormat();
    if(!videoMeta->findPointer(kKeyFFmpegStream, (void**)&mVStream)) {
        return UNKNOWN_ERROR;
    }
    videoMeta->findInt64(kKeyStartTime, &mStartTime);
    
    if (mVStream == NULL) {
        return UNKNOWN_ERROR;
    }
    
    mMetaData.setCString(kKeyDecoderComponent, MEDIA_DECODE_COMPONENT_VIDEOTOOLBOX);
    
    mVCodecCxt = avcodec_alloc_context3(NULL);
    
    ret = avcodec_parameters_to_context(mVCodecCxt, mVStream->codecpar);
    if (ret < 0) {
        return false;
    }
    av_codec_set_pkt_timebase(mVCodecCxt, mVStream->time_base);
    
    switch( mVCodecCxt->codec_id ) {
        case AV_CODEC_ID_H263 :
            mHWAccelCtx->cm_codec_type = kCMVideoCodecType_H263;
            break;
        case AV_CODEC_ID_H264 :
            mHWAccelCtx->cm_codec_type = kCMVideoCodecType_H264;
            break;
        case AV_CODEC_ID_HEVC :
            mHWAccelCtx->cm_codec_type = kCMVideoCodecType_HEVC;
            break;
        case AV_CODEC_ID_MPEG1VIDEO :
            mHWAccelCtx->cm_codec_type = kCMVideoCodecType_MPEG1Video;
            break;
        case AV_CODEC_ID_MPEG2VIDEO :
            mHWAccelCtx->cm_codec_type = kCMVideoCodecType_MPEG2Video;
            break;
        case AV_CODEC_ID_MPEG4 :
            mHWAccelCtx->cm_codec_type = kCMVideoCodecType_MPEG4Video;
            break;
        default :
            break;
    }
    
    mFormatDescription = CreateFormatDescriptionFromCodecData(mHWAccelCtx->cm_codec_type,
                                                              mVStream->codecpar->width,
                                                              mVStream->codecpar->height,
                                                              mVCodecCxt,
                                                              0);
    
    VTDecompressionOutputCallbackRecord callBackRecord;
    callBackRecord.decompressionOutputCallback = DecompressionSessionCallback;
    callBackRecord.decompressionOutputRefCon = this;
    
    CFMutableDictionaryRef destinationImageBufferAttributes = CFDictionaryCreateMutable(NULL,
                                                                                        0,
                                                                                        &kCFTypeDictionaryKeyCallBacks,
                                                                                        &kCFTypeDictionaryValueCallBacks);
    DictSetBoolean(destinationImageBufferAttributes, kCVPixelBufferOpenGLESCompatibilityKey, true);
    DictSetI32(destinationImageBufferAttributes, kCVPixelBufferPixelFormatTypeKey, kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange);
    DictSetI32(destinationImageBufferAttributes, kCVPixelBufferWidthKey, mVStream->codecpar->width);
    DictSetI32(destinationImageBufferAttributes, kCVPixelBufferHeightKey, mVStream->codecpar->height);
    
    OSStatus status =  VTDecompressionSessionCreate(NULL, mFormatDescription, NULL,
                                                    destinationImageBufferAttributes,
                                                    &callBackRecord, &mDecompressSession);
    NBLOG_DEBUG(LOG_TAG, "Video Decompress session create: %s\n", (status == noErr) ? "Success" : "Failed");
    
    mDecodedFrame = av_frame_alloc();
    if (mDecodedFrame == NULL) {
        return UNKNOWN_ERROR;
    }
    
    INIT_LIST_HEAD(&(mSortedHead));
    mSortedCount = 0;
    
    mCachedBuffer = NULL;
    mLastPts = 0;
    
    mMaxRefFrames = mVStream->codecpar->video_delay;
    
    mMediaTrack->start();
    
    mMetaData.setInt32(kKeyWidth, mVStream->codecpar->width);
    mMetaData.setInt32(kKeyHeight, mVStream->codecpar->height);
    
    return OK;
}

nb_status_t NBVideoToolboxDecoder::stop() {
    mMediaTrack->stop();
    
    struct list_head *pos, *q;
    list_for_each_safe(pos, q, &(mSortedHead)) {
        SortedQueueItem* dummyItem = list_entry(pos, struct SortedQueueItem, list);
        if (dummyItem->frame.data[3] != NULL) {
            CVPixelBufferRelease((CVPixelBufferRef)dummyItem->frame.data[3]);
        }
        list_del(&(dummyItem->list));
        delete dummyItem;
    }
    
    if (mDecodedFrame != NULL)
        av_frame_free(&mDecodedFrame);
    
    if (mDecompressSession != NULL)
        VTDecompressionSessionInvalidate(mDecompressSession);
    
    if (mFormatDescription != NULL)
        CFRelease(mFormatDescription);
    
    return OK;
}

NBMetaData* NBVideoToolboxDecoder::getFormat() {
    return &mMetaData;
}

nb_status_t NBVideoToolboxDecoder::read(
                                        NBMediaBuffer **buffer, ReadOptions *options) {
    
    nb_status_t ret = OK;
    int retryCount = 5;
    
    for (;;) {
        OSStatus status = noErr;
        
        if (mSortedCount <= mMaxRefFrames) {
            ret = mMediaTrack->read(&mCachedBuffer, options);
        
            if (ret != OK) {
                if (ret == INFO_DISCONTINUITY) {
                    NBLOG_INFO(LOG_TAG, "decoder got seek request begin");
                    
                    VTDecompressionSessionWaitForAsynchronousFrames(mDecompressSession);
                    
                    struct list_head *pos, *q;
                    list_for_each_safe(pos, q, &(mSortedHead)) {
                        SortedQueueItem* dummyItem = list_entry(pos, struct SortedQueueItem, list);
                        if (dummyItem->frame.data[3] != NULL) {
                            CVPixelBufferRelease((CVPixelBufferRef)dummyItem->frame.data[3]);
                        }
                        list_del(&(dummyItem->list));
                        delete dummyItem;
                    }
                    
                    if (mCachedBuffer != NULL) {
                        delete mCachedBuffer;
                        mCachedBuffer = NULL;
                    }
                    
                    NBLOG_INFO(LOG_TAG, "decoder got seek request end");
                    
                    mSortedCount = 0;
                }
                return ret;
            }
        
            AVPacket* videoPacket = (AVPacket*)mCachedBuffer->data();
            CMSampleBufferRef sampleBuffer = NULL;
            CMBlockBufferRef blockBuffer = NULL;
            
            if (videoPacket->dts == AV_NOPTS_VALUE) {
                videoPacket->dts = 0;
            }
            if (videoPacket->pts == AV_NOPTS_VALUE) {
                videoPacket->pts = 0;
            }
            
            CMSampleTimingInfo frameTimingInfo;
            frameTimingInfo.decodeTimeStamp = CMTimeMake(videoPacket->dts * mVStream->time_base.num, mVStream->time_base.den);
            frameTimingInfo.duration = CMTimeMake(videoPacket->duration * mVStream->time_base.num, mVStream->time_base.den);
            frameTimingInfo.presentationTimeStamp = CMTimeMake(videoPacket->pts * mVStream->time_base.num, mVStream->time_base.den);
            
            if (mHWAccelCtx->encapsulateFunc != NULL) {
                blockBuffer = mHWAccelCtx->encapsulateFunc(mHWAccelCtx, videoPacket);
            }
            
//            if (CMSampleBufferCreate(kCFAllocatorDefault, blockBuffer, true, NULL, NULL, mFormatDescription, 1, 1, &frameTimingInfo, 1, &raw_size, &sampleBuffer) != noErr) {
//                NBLOG_ERROR(LOG_TAG, "erroe when create samplebuffer\n");
//            }
            
            size_t bufferLen = CMBlockBufferGetDataLength(blockBuffer);
            if (CMSampleBufferCreate(kCFAllocatorDefault, blockBuffer, true, NULL, NULL, mFormatDescription, 1, 1, &frameTimingInfo, 1, &bufferLen, &sampleBuffer) != noErr) {
                NBLOG_ERROR(LOG_TAG, "erroe when create samplebuffer\n");
            }
            
            //Insert this item to sorted queue
            SortedQueueItem* queueItem = new SortedQueueItem();
            int64_t pts = CMTimeGetSeconds(frameTimingInfo.presentationTimeStamp) * 1000;
            int64_t dts = CMTimeGetSeconds(frameTimingInfo.decodeTimeStamp) * 1000;
            
            {
                queueItem->frame.pts = pts;
                NBAutoMutex autoMutex(mSortedLock);
                if (list_empty(&(mSortedHead))) {
                    list_add_tail(&(queueItem->list), &(mSortedHead));
                } else {
                    SortedQueueItem* dummyItem = NULL;
                    list_for_each_entry(dummyItem, &(mSortedHead), list) {
                        if (queueItem->frame.pts < dummyItem->frame.pts) {
                            break;
                        }
                    }
                    list_add_tail(&(queueItem->list), &(dummyItem->list));
                }
            }
    
            //decode buffer and add to vieoRingBuffer
            VTDecodeFrameFlags flags = kVTDecodeFrame_EnableAsynchronousDecompression;
            VTDecodeInfoFlags flagOut;
            status = VTDecompressionSessionDecodeFrame(mDecompressSession, sampleBuffer, flags,
                                      &sampleBuffer, &flagOut);

            CFRelease(sampleBuffer);
            CFRelease(blockBuffer);
            
            delete mCachedBuffer;
            mCachedBuffer = NULL;
            
            ++ mSortedCount;
            
//            NBLOG_INFO(LOG_TAG, "The decode timestamp : %lld present timestamp : %lld mSortedCount : %d", dts, pts, mSortedCount);
        } else {
            //Wait for an async callback
            status = VTDecompressionSessionWaitForAsynchronousFrames(mDecompressSession);
            
            SortedQueueItem* leastFrame = list_first_entry(&(mSortedHead), SortedQueueItem, list);
            if (leastFrame->frame.data[3] == NULL) {
                if (--retryCount < 0) {
                    return TIMED_OUT;
                }
                continue;
            }
            
            *mDecodedFrame = leastFrame->frame;
            
            {
                NBAutoMutex autoMutex(mSortedLock);
                list_del(&(leastFrame->list));
                delete leastFrame;
            }
            --mSortedCount;
            
            *buffer = new NBVideoFrame(mDecodedFrame, 0, true);
            break;
        }
    }
    
    return OK;
}
