//
// Created by parallels on 9/10/18.
//

#include "NBALAudioRenderer.h"
#include "decoder/NBMediaSource.h"
#include "foundation/NBMetaData.h"
#include "foundation/NBFoundation.h"

#include <NBLog.h>

#if defined(BUILD_TARGET_LINUX64) || defined(BUILD_TARGET_ANDROID)
#include "AL/al.h"
#include "AL/alc.h"
#include "AL/alext.h"
#elif BUILD_TARGET_IOS
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"

static const int64_t AudioBufferTime = 20;
//static const int64_t AudioBufferTotalTime = 800;

#ifdef __cplusplus
}
#endif

#define LOG_TAG "NBALAudioRenderer"

NBALAudioRenderer::NBALAudioRenderer()
    :mCurrentPts(0) {
    mALCDev = alcOpenDevice(NULL);
    mBufferIdx = 0;
}

NBALAudioRenderer::~NBALAudioRenderer() {
    alcCloseDevice(mALCDev);
}

nb_status_t NBALAudioRenderer::start(NBMetaData *metaData) {
    //Init the openal context
    mALCCtx = alcCreateContext(mALCDev, NULL);
    if(!mALCCtx || alcMakeContextCurrent(mALCCtx) == ALC_FALSE) {
        NBLOG_ERROR(LOG_TAG, "Failed to set up audio device\n");
        if(mALCCtx)
            alcDestroyContext(mALCCtx);
        return UNKNOWN_ERROR;
    }

    const ALCchar *name = NULL;
    if(alcIsExtensionPresent(mALCDev, "ALC_ENUMERATE_ALL_EXT"))
        name = alcGetString(mALCDev, ALC_ALL_DEVICES_SPECIFIER);
    if(!name || alcGetError(mALCDev) != AL_NO_ERROR)
        name = alcGetString(mALCDev, ALC_DEVICE_SPECIFIER);
    NBLOG_DEBUG(LOG_TAG, "Opened aldevice %s", name);

    ALenum fmt;

    int codecFormat = 0;
    metaData->findInt32(kKeyAudioFromat, &codecFormat);

    uint64_t codecChLayout = 0;
    metaData->findInt64(kKeyAudioChLayout, (int64_t*)&codecChLayout);

    int codecSampleRate = 0;
    metaData->findInt32(kKeySampleRate, &codecSampleRate);

    int codecChannels = 0;
    metaData->findInt32(kKeyChannelCount, &codecChannels);

    /* Find a suitable format for OpenAL. */
    mDstChanLayout = 0;
    if(codecFormat == AV_SAMPLE_FMT_U8 || codecFormat == AV_SAMPLE_FMT_U8P)
    {
        mDstSampleFmt = AV_SAMPLE_FMT_U8;
        mFrameSize = 1;
        if(codecChLayout == AV_CH_LAYOUT_7POINT1 &&
           alIsExtensionPresent("AL_EXT_MCFORMATS") &&
           (fmt=alGetEnumValue("AL_FORMAT_71CHN8")) != AL_NONE && fmt != -1)
        {
            mDstChanLayout = codecChLayout;
            mFrameSize *= 8;
            mFormat = fmt;
        }
        if((codecChLayout == AV_CH_LAYOUT_5POINT1 ||
                codecChLayout == AV_CH_LAYOUT_5POINT1_BACK) &&
           alIsExtensionPresent("AL_EXT_MCFORMATS") &&
           (fmt=alGetEnumValue("AL_FORMAT_51CHN8")) != AL_NONE && fmt != -1)
        {
            mDstChanLayout = codecChLayout;
            mFrameSize *= 6;
            mFormat = fmt;
        }
        if(codecChLayout == AV_CH_LAYOUT_MONO)
        {
            mDstChanLayout = codecChLayout;
            mFrameSize *= 1;
            mFormat = AL_FORMAT_MONO8;
        }
        if(!mDstChanLayout)
        {
            mDstChanLayout = AV_CH_LAYOUT_STEREO;
            mFrameSize *= 2;
            mFormat = AL_FORMAT_STEREO8;
        }
    }
    if(!mDstChanLayout)
    {
        mDstSampleFmt = AV_SAMPLE_FMT_S16;
        mFrameSize = 2;
        if(codecChLayout == AV_CH_LAYOUT_7POINT1 &&
           alIsExtensionPresent("AL_EXT_MCFORMATS") &&
           (fmt=alGetEnumValue("AL_FORMAT_71CHN16")) != AL_NONE && fmt != -1)
        {
            mDstChanLayout = codecChLayout;
            mFrameSize *= 8;
            mFormat = fmt;
        }
        if((codecChLayout == AV_CH_LAYOUT_5POINT1 ||
                codecChLayout == AV_CH_LAYOUT_5POINT1_BACK) &&
           alIsExtensionPresent("AL_EXT_MCFORMATS") &&
           (fmt=alGetEnumValue("AL_FORMAT_51CHN16")) != AL_NONE && fmt != -1)
        {
            mDstChanLayout = codecChLayout;
            mFrameSize *= 6;
            mFormat = fmt;
        }
        if(codecChLayout == AV_CH_LAYOUT_MONO)
        {
            mDstChanLayout = codecChLayout;
            mFrameSize *= 1;
            mFormat = AL_FORMAT_MONO16;
        }
        if(!mDstChanLayout)
        {
            mDstChanLayout = AV_CH_LAYOUT_STEREO;
            mFrameSize *= 2;
            mFormat = AL_FORMAT_STEREO16;
        }
    }

    //generate al buffers
    alGenBuffers(MAX_AL_BUFFER_COUNT, mALBuffers);
    alGenSources(1, &mALSource);

    mBufferLen = codecSampleRate * AudioBufferTime / 1000.0 * mFrameSize;
    mBuffer = new uint8_t[mBufferLen];

    mDstChannels = codecChannels;
    mDstSampleRate = codecSampleRate;

    return OK;
}

nb_status_t NBALAudioRenderer::stop() {
    if (mBuffer != NULL) {
        delete mBuffer;
        mBuffer = NULL;
    }

    alDeleteSources(1, &mALSource);

    alDeleteBuffers(MAX_AL_BUFFER_COUNT, mALBuffers);

    alcMakeContextCurrent(NULL);
    alcDestroyContext(mALCCtx);
    return OK;
}


uint32_t NBALAudioRenderer::acquireBuffer(uint8_t** pPtr) {
    /* First remove any processed buffers. */
    ALint processed;
    alGetSourcei(mALSource, AL_BUFFERS_PROCESSED, &processed);
    while (processed > 0) {
        ALuint bids[4] = {0};
        int minValue = 4 < processed ? 4 : processed;
        alSourceUnqueueBuffers(mALSource, minValue,
                               bids);
        processed -= minValue;
    }

    ALint queued;
    alGetSourcei(mALSource, AL_BUFFERS_QUEUED, &queued);

    //This value is very big
    if (queued < MAX_AL_BUFFER_COUNT) {
        *pPtr = mBuffer;
    }

    return mBufferLen;
}

void NBALAudioRenderer::releaseBuffer(bool shoudRender) {
    if (!shoudRender)
        return ;

    uint32_t bufid = mALBuffers[mBufferIdx];

    if (mBuffer)
        alBufferData(bufid, mFormat, mBuffer, mBufferLen, mDstSampleRate);

    alSourceQueueBuffers(mALSource, 1, &bufid);
    mBufferIdx = (mBufferIdx + 1) % MAX_AL_BUFFER_COUNT;
}

int64_t NBALAudioRenderer::getCurrentClock() {
    /* The source-based clock is based on 4 components:
     * 1 - The timestamp of the next sample to buffer (mCurrentPts)
     * 2 - The length of the source's buffer queue
     *     (AudioBufferTime*AL_BUFFERS_QUEUED)
     * 3 - The offset OpenAL is currently at in the source (the first value
     *     from AL_SAMPLE_OFFSET_LATENCY_SOFT)
     * 4 - The latency between OpenAL and the DAC (the second value from
     *     AL_SAMPLE_OFFSET_LATENCY_SOFT)
     *
     * Subtracting the length of the source queue from the next sample's
     * timestamp gives the timestamp of the sample at the start of the source
     * queue. Adding the source offset to that results in the timestamp for the
     * sample at OpenAL's current position, and subtracting the source latency
     * from that gives the timestamp of the sample currently at the DAC.
     */
    int64_t pts = mCurrentPts;
    if(mALSource)
    {
        ALint offset[2];

        ALint queued;
        ALint status;

        ALint ioffset;
        alGetSourcei(mALSource, AL_SAMPLE_OFFSET, &ioffset);
//        offset[0] = (ALint64SOFT)ioffset << 32;
        offset[0] = ioffset;
        offset[1] = 0;

        alGetSourcei(mALSource, AL_BUFFERS_QUEUED, &queued);
        alGetSourcei(mALSource, AL_SOURCE_STATE, &status);

//        printf("The audio offset[0] : %d offset[1] : %d\n", offset[0], offset[1]);

        /* If the source is AL_STOPPED, then there was an underrun and all
         * buffers are processed, so ignore the source queue. The audio thread
         * will put the source into an AL_INITIAL state and clear the queue
         * when it starts recovery. */
        if(status != AL_STOPPED)
        {
//            printf("Audio pts before : %lld\n", pts);

            pts -= AudioBufferTime * queued;

//            printf("Audio pts middle : %lld\n", pts);

            pts += offset[0] * 1000.0f / mDstSampleRate;

//            printf("Audio pts after : %lld\n", pts);
        }

        /* Don't offset by the latency if the source isn't playing. */
        if(status == AL_PLAYING)
            pts -=  offset[1] / 1000;
    }

    return FFMAX(pts, 0);
}

nb_status_t NBALAudioRenderer::pause(bool at_eos) {
    /* Check that the source is playing. */
    ALint state;
    alGetSourcei(mALSource, AL_SOURCE_STATE, &state);
    if(state == AL_PAUSED) {
        return OK;
    }
    alSourcePause(mALSource);
    return OK;
}

nb_status_t NBALAudioRenderer::play() {
    /* Check that the source is playing. */
    ALint state;
    alGetSourcei(mALSource, AL_SOURCE_STATE, &state);

    if(state == AL_STOPPED) {
        /* AL_STOPPED means there was an underrun. Clear the buffer queue
         * since this likely means we're late, and rewind the source to get
         * it back into an AL_INITIAL state.
         */
        alSourceRewind(mALSource);
        alSourcei(mALSource, AL_BUFFER, 0);
        return UNKNOWN_ERROR;
    }

    if(state == AL_PLAYING) {
        return OK;
    }

    alSourcePlay(mALSource);
    return OK;
}

nb_status_t NBALAudioRenderer::flushALContext() {
    ALint processed = 0, queued = 0, state = AL_INITIAL;

    /* Get relevant source info */
    alGetSourcei(mALSource, AL_SOURCE_STATE, &state);

    if (state != AL_STOPPED) {
        alSourceStop(mALSource);
    }

//    alGetSourcei(mALSource, AL_BUFFERS_PROCESSED, &processed);
    alGetSourcei(mALSource, AL_BUFFERS_QUEUED, &queued);

    uint32_t unqueueCount = processed + queued;

    if (unqueueCount > 0) {
        ALuint* buffer = new ALuint[unqueueCount];
        alSourceUnqueueBuffers(mALSource, unqueueCount, buffer);
        delete[] buffer;
    }

    alSourceRewind(mALSource);
    mBufferIdx = 0;
    
    return OK;
}
