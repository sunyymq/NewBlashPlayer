//
// Created by parallels on 9/9/18.
//

#include "NBFFmpegExtractor.h"
#include "datasource/NBDataSource.h"
#include "foundation/NBMetaData.h"
#include "NBBufferQueue.h"
#include "NBAVPacket.h"
#include "decoder/NBFFmpegMediaTrack.h"

#include <NBLog.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <libavformat/avio.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>

#ifdef __cplusplus
}
#endif

#define LOG_TAG "NBFFmpegExtractor"

#define MIN_FRAMES 25

struct FFExtractorEvent : public NBTimedEventQueue::Event {
    FFExtractorEvent(
            NBFFmpegExtractor *extractor,
            void (NBFFmpegExtractor::*method)())
            : mExtractor(extractor),
              mMethod(method) {
    }

protected:
    virtual ~FFExtractorEvent() {}

    virtual void fire(NBTimedEventQueue *queue, int64_t /* now_us */) const {
        (mExtractor->*mMethod)();
    }

private:
    NBFFmpegExtractor *mExtractor;
    void (NBFFmpegExtractor::*mMethod)();

    FFExtractorEvent(const FFExtractorEvent &);
    FFExtractorEvent &operator=(const FFExtractorEvent &);
};

NBFFmpegExtractor::NBFFmpegExtractor(INBMediaSeekListener* seekListener, NBDataSource* dataSource)
    :NBMediaExtractor(seekListener)
    ,mDataSource(dataSource)
    ,mFormatContext(NULL)
    ,mQueueStarted(false) {

    mQueue.setQueueName(NBString("FFmpegExtractor"));

    //create the extractor event
    mExtractorEvent = new FFExtractorEvent(this, &NBFFmpegExtractor::onExtractorEvent);
    mExtractorEventPending = false;

    //init the flash packet
    NBFFmpegAVPacket::FlashPacket.data = (uint8_t*)&NBFFmpegAVPacket::FlashPacket;
}

NBFFmpegExtractor::~NBFFmpegExtractor() {
    NBLOG_INFO(LOG_TAG, "We are cloing ffmpeg format context : %p", mFormatContext);
    
    if (mExtractorEvent != NULL) {
        delete mExtractorEvent;
        mExtractorEvent = NULL;
    }
}

nb_status_t NBFFmpegExtractor::sniffMedia() {
    int rc = 0;
    AVDictionary* openDict = NULL;
    //clear the metadata
    memset(mMetaDatas, 0, sizeof(mMetaDatas));

    //try to open it
    mFormatContext = (AVFormatContext*)mDataSource->getContext();
    
    //Tell the ffmpeg rtsp use tcp connection
    av_dict_set(&openDict, "rtsp_transport", "tcp", 0);
    av_dict_set(&openDict, "stimeout", "10000000", 0);
    
    // Open video file
    if ((rc = avformat_open_input(&mFormatContext,
                                       mDataSource->getUri().string(),
                                       NULL,
                                       &openDict))) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE] = { 0 };
        NBLOG_ERROR(LOG_TAG, "The error string is is : %s", av_make_error_string(errbuf, AV_ERROR_MAX_STRING_SIZE, rc));
        return UNKNOWN_ERROR;
    }

    if ((rc = avformat_find_stream_info(mFormatContext, NULL)) < 0) {
        NBLOG_ERROR(LOG_TAG, "avformat_find_stream_info failed with code : %d", rc);
        return UNKNOWN_ERROR;
    }

//    av_dump_format(mFormatContext, 0, mDataSource->getUri().string(), 0);

    bool isAudioFormat = false;
//    if (strcmp(mFormatContext->iformat->name, "mp3") == 0) {
//        isAudioFormat = true;
//    }

    for (int i = 0; i < mFormatContext->nb_streams; i++) {
        AVStream* stream = mFormatContext->streams[i];

        NBMetaData* trackMetaData = getMetaData();
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO
            || stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO
            || stream->codecpar->codec_type == AVMEDIA_TYPE_SUBTITLE) {
            int64_t totalDuration = 0;
            if (mFormatContext->duration != AV_NOPTS_VALUE) {
                totalDuration = mFormatContext->duration;
            }else if(stream->duration != AV_NOPTS_VALUE) {
                totalDuration = stream->duration;
            }else {
                totalDuration = 0;
            }

//            if (totoDuration == 0) {
//                FPINFO("duration is: %ld, We can not seek", totoDuration);
//                mFlags &= (~CAN_SEEK);
//            }

            NBLOG_DEBUG(LOG_TAG, "the mediaType : %d duration is : %lld", stream->codecpar->codec_type, totalDuration);

            trackMetaData->setInt64(kKeyDuration, totalDuration / 1000);
            trackMetaData->setInt64(kKeyStartTime, mFormatContext->start_time);
            trackMetaData->setInt32(kKeyMediaType, stream->codecpar->codec_type);

            //Fix some mp3 have picture
            if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO
                && !isAudioFormat
                && mHasVideoStream == false) {

                NBBufferQueue* mediaBufferQueue = new NBBufferQueue();

                int32_t frameRate = 0;
                AVRational rational = stream->r_frame_rate;
                frameRate = av_q2d(rational);

                trackMetaData->setCString(kKeyMIMEType, MEDIA_MIMETYPE_VIDEO_FFMPEG);
                trackMetaData->setPointer(kKeyFFmpegStream, stream);
                trackMetaData->setPointer(kKeyMediaBufferQueue, mediaBufferQueue);
                trackMetaData->setInt32(kKeyTrackActive, 0);
                trackMetaData->setInt32(kKeyDisplayWidth, stream->codecpar->width);
                trackMetaData->setInt32(kKeyDisplayHeight, stream->codecpar->height);
                trackMetaData->setInt32(kKeyWidth, stream->codecpar->width);
                trackMetaData->setInt32(kKeyHeight, stream->codecpar->height);
                trackMetaData->setInt32(kKeyFrameRate, frameRate);

                AVDictionaryEntry* rotateEntry = av_dict_get(stream->metadata, "rotate", NULL, 0);
                if (rotateEntry != NULL && rotateEntry->value != NULL) {
                    int rotate = atoi(rotateEntry->value);
                    if (rotate == 90
                        || rotate == 270) {

                    }
                    NBLOG_DEBUG(LOG_TAG, "rotate = %d",rotate);
                    trackMetaData->setInt32(kKeyRotation, rotate);
                }

                // trackMetaData->setInt32(kKeyIOSHVDAActive, 1);

                // if (strcmp(mFormatContext->iformat->name, "mov,mp4,m4a,3gp,3g2,mj2") == 0) {
                //     MOVContext* movContext  = (MOVContext*)mFormatContext->priv_data;
                //     if (movContext->isom) {
                //         trackMetaData->setInt32(kKeyAndroidHVDAActivie, 1);
                //     }else {
                //         trackMetaData->setInt32(kKeyAndroidHVDAActivie, 0);
                //     }
                // }

#if BUILD_TARGET_ANDROID
                if (stream->codec->height >= 320 && stream->codec->width >= 480) {
                        trackMetaData->setInt32(kKeyAndroidHVDAActivie, 1);
                    }else {
                        trackMetaData->setInt32(kKeyAndroidHVDAActivie, 0);
                    }
#else
                //iOSH264HVDAChoose
                if (stream->codecpar
                    && stream->codecpar->height >= 480
                    && stream->codecpar->codec_id == AV_CODEC_ID_H264
                    && stream->codecpar->extradata_size > 0
                    && (stream->codecpar->format == AV_PIX_FMT_YUV420P
                        || stream->codecpar->format == AV_PIX_FMT_YUVJ420P)) {
                    trackMetaData->setInt32(kKeyIOSHVDAActive, 1);
                }else {
                    trackMetaData->setInt32(kKeyIOSHVDAActive, 0);
                }
#endif

                mMetaDatas[stream->index] = trackMetaData;
                ++mMetaDataIndex;

//                //If the video resolution is 4k , reduce the buffer packet count .
//                if (stream->codec->width > 1920 && stream->codec->height > 1080) {
//                    mMaxVideoBufferPacketCount = 300;
//                }else {
//                    mMaxVideoBufferPacketCount = 300;
//                }

                NBLOG_DEBUG(LOG_TAG, "we got video codec context, iformat->name:%s\n", mFormatContext->iformat->name);

                //if not seek by bytes, We try to seek to 0 percent position , we can not seek if fail
                bool seek_by_bytes = !!(mFormatContext->iformat->flags & AVFMT_TS_DISCONT) && strcmp("ogg", mFormatContext->iformat->name);
//                if (seek_by_bytes) {
//                    mSeekFlag |= AVSEEK_FLAG_BYTE;
//                } else if (strcmp("hls,applehttp", mFormatContext->iformat->name) && (avformat_seek_file(mFormatContext, -1, INT64_MIN, 0, INT64_MAX, 0) < 0)) {
//                    //Current we can not seek
//                    mFlags &= (~CAN_SEEK);
//                }

                mHasVideoStream = true;
            } else if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && mHasAudioStream == false) {
                NBBufferQueue* mediaBufferQueue = new NBBufferQueue();
                trackMetaData->setCString(kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_FFMPEG);
                trackMetaData->setPointer(kKeyFFmpegStream, stream);
                trackMetaData->setPointer(kKeyMediaBufferQueue, mediaBufferQueue);
                trackMetaData->setInt32(kKeyTrackActive, 0);

//                if (stream->codec->codec_id == AV_CODEC_ID_AC3
//                    || stream->codec->codec_id == AV_CODEC_ID_EAC3
//                    || stream->codec->codec_id == AV_CODEC_ID_MLP) {
//                    trackMetaData->setInt32(kKeyDoblyAudioCodecs, 1);
//                }else {
//                    trackMetaData->setInt32(kKeyDoblyAudioCodecs, 0);
//                }
//
//                // if (stream->codec->codec_id == AV_CODEC_ID_DTS) {
//                //     trackMetaData->setInt32(kKeyDTSAudioCodecs, 1);
//                // }else {
//                //     trackMetaData->setInt32(kKeyDTSAudioCodecs, 0);
//                // }
//
                mMetaDatas[stream->index] = trackMetaData;
                ++mMetaDataIndex;

//
//                FPINFO("we got audio codec context");
//
//                mMaxAudioBufferPacketCount = 1000;

                mHasAudioStream = true;
            } else if (stream->codecpar->codec_type == AVMEDIA_TYPE_SUBTITLE)  {
                stream->discard = AVDISCARD_ALL;
                NBBufferQueue* mediaBufferQueue = new NBBufferQueue();
                trackMetaData->setCString(kKeyMIMEType, MEDIA_MIMETYPE_SUBTITLE_FFMPEG);
                trackMetaData->setPointer(kKeyFFmpegStream, stream);
                trackMetaData->setPointer(kKeyMediaBufferQueue, mediaBufferQueue);
                trackMetaData->setInt32(kKeyTrackActive, 0);
                trackMetaData->setInt32(kKeyTrackID, i);

//                if (stream->metadata != NULL) {
//                    AVDictionaryEntry *lang = NULL;
//                    lang = av_dict_get(stream->metadata, "language", NULL, 0);
//                    if (lang != NULL && lang->value != NULL) {
//                        const char* lang_iso639_2_t = av_convert_lang_to(lang->value, AV_LANG_ISO639_2_TERM);
//                        trackMetaData->setCString(kKeyMediaLanguage, lang_iso639_2_t);
//                        FPINFO("subtitle language: %s, iso639_2_t: %s", lang->value, lang_iso639_2_t);
//                    }
//                }

                const char* codecName = avcodec_get_name(stream->codecpar->codec_id);
                trackMetaData->setCString(kKeyCodecName, codecName);
//                FPINFO("subtitle codecName: %s", codecName);

                mMetaDatas[mMetaDataIndex++] = trackMetaData;

//                FPINFO("we got subtitle codec context, index:%d", i);
            } else {
                stream->discard = AVDISCARD_ALL;
                trackMetaData->setCString(kKeyMIMEType, "unpported");
                mMetaDatas[mMetaDataIndex++] = trackMetaData;
            }
        }// end of if (stream->codec->codec_type == AVMEDIA_TYPE_VIDEO
    }// end of for (int i = 0; i < mFormatContext->nb_streams; i++)

    // start the media play
    start(NULL);
    
    return OK;
}

void NBFFmpegExtractor::start(NBMetaData* metaData) {
    NBAutoMutex autoLock(mLock);
    if (!mQueueStarted) {
        mQueue.start();
        mQueueStarted = true;
    }
    mReachedEnd = false;

//    postExtractorEvent_l();
}

void NBFFmpegExtractor::stop() {
    {
        // Cancel the extractor
        NBAutoMutex autoLock(mLock);
        cancelExtractorEvent_l();
    }

    if (mQueueStarted) {
        mQueue.stop(true);
        mQueueStarted = false;
    }
    
    for (int i = 0; i < mMetaDataIndex; ++i) {
        NBMetaData* metaData = mMetaDatas[i];
        if (metaData == NULL) {
            continue;
        }
        
        NBBufferQueue* mediaBufferQueue = NULL;
        metaData->findPointer(kKeyMediaBufferQueue, (void**)&mediaBufferQueue);
        
        if (mediaBufferQueue != NULL) {
            mediaBufferQueue->flush();
            delete mediaBufferQueue;
            mediaBufferQueue = NULL;
        }
        
        delete metaData;
        
        mMetaDatas[i] = NULL;
    }

    mReachedEnd = true;
}

NBMediaSource* NBFFmpegExtractor::getTrack(size_t index) {
    return new NBFFmpegMediaTrack(getTrackMetaData(index), this);
}

NBMetaData* NBFFmpegExtractor::getTrackMetaData(
        size_t index, uint32_t flags) {
    int realIndex = 0;
    if (index >= mMetaDataIndex) {
        return NULL;
    }

    while (realIndex < MAX_META_DATA_COUNT) {
        if (mMetaDatas[realIndex] != 0
            && index-- == 0) {
            break;
        }
        ++realIndex;
    }

    return mMetaDatas[realIndex];
}

void NBFFmpegExtractor::notifyContinueRead() {
    NBAutoMutex autoLock(mLock);

//    if (!mQueueStarted) {
//        mQueue.start();
//        mQueueStarted = true;
//    }

    postExtractorEvent_l();
}

bool NBFFmpegExtractor::getReachedEnd() {
    NBAutoMutex autoLock(mLock);
    return mReachedEnd;
}

void NBFFmpegExtractor::postExtractorEvent_l(int64_t delayUs) {
    if (mExtractorEventPending) {
        return ;
    }
    mExtractorEventPending = true;
    mQueue.postEventWithDelay(mExtractorEvent, delayUs < 0 ? INT64_MIN : delayUs);
}

void NBFFmpegExtractor::cancelExtractorEvent_l() {
    if (!mExtractorEventPending) {
        return ;
    }
    mQueue.cancelEvent(mExtractorEvent->eventID());
    mExtractorEventPending = false;
}

void NBFFmpegExtractor::onExtractorEvent() {
    int64_t seekTimeUs = -1;
    {
        NBAutoMutex autoLock(mLock);

        seekTimeUs = mSeekTimeUs;

        if (seekTimeUs == -1) {

            int fullCount = 0;
            for (int index = 0; index < mMetaDataIndex; ++index) {
                NBMetaData *metaData = mMetaDatas[index];
                if (metaData == NULL) {
                    continue;
                }

                NBBufferQueue *mediaBufferQueue = NULL;
                metaData->findPointer(kKeyMediaBufferQueue, (void **) &mediaBufferQueue);
                if (mediaBufferQueue == NULL) {
                    continue;
                }

                if (mediaBufferQueue->getCount() < MIN_FRAMES) {
                    // we not all full, need read more
                    break;
                }

                ++fullCount;
            }

            //The all packet queue is full
            if (fullCount == mMetaDataIndex) {
//                NBLOG_VERBOSE(LOG_TAG, "queue is full\n");
                mExtractorEventPending = false;
                return;
            }
        }
    }

//    NBLOG_VERBOSE(LOG_TAG, "seekTimeUs is : %lld\n", seekTimeUs);

    //Do the real seek if needed
    if (seekTimeUs != -1) {
        int ret = avformat_seek_file(mFormatContext, -1, INT64_MIN, seekTimeUs * 1000, INT64_MAX, 0);
        if (ret < 0) {
            mReachedEnd = true;

//            mSeekListener->onMediaSeeked(-1, UNKNOWN_ERROR);

        }else {
            for (int index = 0; index < mMetaDataIndex; ++index) {
                NBMetaData *metaData = mMetaDatas[index];
                if (metaData == NULL) {
                    continue;
                }

                int32_t trackActive = 0;
                metaData->findInt32(kKeyTrackActive, &trackActive);
                if (trackActive) {
                    NBBufferQueue* mediaBufferQueue = NULL;
                    metaData->findPointer(kKeyMediaBufferQueue, (void**)&mediaBufferQueue);
                    mediaBufferQueue->flush();

                    NBLOG_DEBUG(LOG_TAG, "We push flush packet at stream index : %d\n", index);

                    mediaBufferQueue->pushPacket(new NBFFmpegAVPacket(NBFFmpegAVPacket::FlashPacket, 0));
                }
            }
        }

        NBLOG_DEBUG(LOG_TAG, "We seek a traget : %lld seekTimeUs : %lld\n", mSeekTimeUs, seekTimeUs);

        NBAutoMutex autoLock(mLock);

        //restore when we only have no other seek
        if (seekTimeUs == mSeekTimeUs) {
            mSeekListener->onMediaSeeked(mSeekTimeUs, OK);
            mSeekTimeUs = -1;
        }

        mExtractorEventPending = false;
        //do not start the extractor
        return ;
    }

    //Do real read packet from ffmpeg
    AVPacket readPkt;
    av_init_packet(&readPkt);
    int ret;

    // FPINFO("read av_read_frame");
    if ((ret = av_read_frame(mFormatContext, &readPkt)) < 0) {
        if (ret == AVERROR_EOF || avio_feof(mFormatContext->pb)) {
            // av_usleep(10000);

            if (!mDataSource->isUserInterrupted()) {
                NBLOG_DEBUG(LOG_TAG, "set mReachEnd true");
                NBAutoMutex autoLock(mLock);
                mReachedEnd = true;
            }

            NBLOG_DEBUG(LOG_TAG, "The data is read end of file, AVERROR_EOF: %d, avio_feof:%d\n", (ret == AVERROR_EOF), avio_feof(mFormatContext->pb));
        }else{
            NBLOG_ERROR(LOG_TAG, "read data return : %d\n", ret);
        }

        NBAutoMutex autoLock(mLock);
        mExtractorEventPending = false;
    }

    if (ret < 0) {
        return ;
    }

//    NBLOG_DEBUG(LOG_TAG, "We got on packet stream_index : %d readPkt.pts : %lld readPkt.dts : %lld\n", readPkt.stream_index, readPkt.pts, readPkt.dts);

    //some source ... Stream discovered after head already parsed
    if (readPkt.stream_index >= 0) {

//        NBLOG_DEBUG(LOG_TAG, "Write Packet stream index is : %d\n", readPkt.stream_index);

        NBMetaData* metaData = mMetaDatas[readPkt.stream_index];

        if (metaData == NULL) {
            NBLOG_ERROR(LOG_TAG, "The meta data is null of stream index : %d", readPkt.stream_index);
        }

        int32_t trackActive = 0;
        if (metaData != NULL
            && metaData->findInt32(kKeyTrackActive, &trackActive)
            && trackActive == 1) {
//            if(readPkt.data == NULL
//               || av_dup_packet(&readPkt) < 0) {
//                NBAutoMutex _lock(mLock);
//                if (!mExtractorEventPending) {
//                    return ;
//                }
//                mExtractorEventPending = false;
//                postExtractorEvent_l();
//                return ;
//            }

            int64_t packetDuration = 0;
            AVStream* stream = NULL;
            NBBufferQueue* mediaBufferQueue = NULL;
            metaData->findPointer(kKeyMediaBufferQueue, (void**)&mediaBufferQueue);

            metaData->findPointer(kKeyFFmpegStream, (void**)&stream);
            packetDuration = av_rescale_q(readPkt.duration, stream->time_base, AV_TIME_BASE_Q);

            //Fix the duration equal to 0
            if (packetDuration == 0) {
                packetDuration = 1 / av_q2d(stream->avg_frame_rate) * AV_TIME_BASE;
            }

            int mediaType = AVMEDIA_TYPE_UNKNOWN;
            metaData->findInt32(kKeyMediaType, &mediaType);
            // if (mediaType == AVMEDIA_TYPE_AUDIO) {
            //     FPINFO("type:%d, flag:%d, pts:%lld, queue count:%d", mediaType, readPkt.flags, readPkt.pts, mediaBufferQueue->getCount());
            // }

//            if (mDropToKeyFrame && (!(readPkt.flags & AV_PKT_FLAG_KEY) || (mediaType == AVMEDIA_TYPE_AUDIO))) {
//                // FPINFO("dropToKeyFrame");
//                av_free_packet(&readPkt);
//            } else {
//                // mediaBufferQueue->incrimentDuration(packetDuration);
//                if (mDropToKeyFrame && (mediaType == AVMEDIA_TYPE_VIDEO)) {
//                    mDropToKeyFrame = false;
//                }

            mediaBufferQueue->pushPacket(new NBFFmpegAVPacket(readPkt, packetDuration));
//            }
        }else {
            av_packet_unref(&readPkt);
        }
    }else {
        av_packet_unref(&readPkt);
    }

    //Read more data if needed
    NBAutoMutex _lock(mLock);
    if (!mExtractorEventPending) {
        return ;
    }
    mExtractorEventPending = false;
    postExtractorEvent_l();
}

nb_status_t NBFFmpegExtractor::seekTo(int64_t millisec) {
    NBAutoMutex autoMutex(mLock);
    mSeekTimeUs = millisec;
    postExtractorEvent_l();
    return OK;
}
