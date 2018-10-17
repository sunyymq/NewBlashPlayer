//
//  NBVideoToolboxDecoder.hpp
//  iOSNBMediaPlayer
//
//  Created by liu enbao on 19/09/2018.
//  Copyright © 2018 liu enbao. All rights reserved.
//

#ifndef NBVIDEOTOOLBOXDECODER_H
#define NBVIDEOTOOLBOXDECODER_H

#include "NBMediaSource.h"
#include <NBMutex.h>
#include <NBList.h>

#include <VideoToolbox/VideoToolbox.h>

#ifdef __cplusplus
extern "C" {
#endif
    
#include "libavutil/rational.h"
    
#ifdef __cplusplus
}
#endif

struct AVStream;
struct AVFrame;
struct AVCodecContext;
class AVCodecParameters;

typedef struct HWAccelCtx HWAccelCtx;

class NBVideoToolboxDecoder : public NBMediaSource {
public:
    NBVideoToolboxDecoder(NBMediaSource* mediaTrack);
    virtual ~NBVideoToolboxDecoder();
    
    enum DecoderRecorderPts {
        DRP_AUTO = -1,
        DRP_OFF,
        DRP_ON,
    };
    
public:
    // To be called before any other methods on this object, except
    // getFormat().
    virtual nb_status_t start(NBMetaData *params = NULL);
    
    // Any blocking read call returns immediately with a result of NO_INIT.
    // It is an error to call any methods other than start after this call
    // returns. Any buffers the object may be holding onto at the time of
    // the stop() call are released.
    // Also, it is imperative that any buffers output by this object and
    // held onto by callers be released before a call to stop() !!!
    virtual nb_status_t stop();
    
    // Returns the format of the data output by this media source.
    virtual NBMetaData* getFormat();
    
    // Returns a new buffer of data. Call blocks until a
    // buffer is available, an error is encountered of the end of the stream
    // is reached.
    // End of stream is signalled by a result of ERROR_END_OF_STREAM.
    // A result of INFO_FORMAT_CHANGED indicates that the format of this
    // MediaSource has changed mid-stream, the client can continue reading
    // but should be prepared for buffers of the new configuration.
    virtual nb_status_t read(
                             NBMediaBuffer **buffer, ReadOptions *options = NULL);
    
private:
    static void DecompressionSessionCallback(void *decompressionOutputRefCon,
                                             void *sourceFrameRefCon,
                                             OSStatus status,
                                             VTDecodeInfoFlags infoFlags,
                                             CVImageBufferRef imageBuffer,
                                             CMTime presentationTimeStamp,
                                             CMTime presentationDuration);
    
private:
    NBMediaSource* mMediaTrack;
    
    AVStream* mVStream;
    
    AVFrame* mDecodedFrame;
    
    int64_t mStartTime;
    
    int64_t next_pts;
    AVRational next_pts_tb;
    
    int64_t mLastPts;
    
    NBMediaBuffer* mCachedBuffer;
    
    //let decoder recorder the pts
    DecoderRecorderPts mDecoderReorderPts;
    AVCodecContext *mVCodecCxt;
    
    CMVideoFormatDescriptionRef mFormatDescription;
    VTDecompressionSessionRef mDecompressSession;
    
    struct list_head mSortedHead;
    int mSortedCount;
    NBMutex mSortedLock;
    
    int mMaxRefFrames;
    
    HWAccelCtx* mHWAccelCtx;
    
private:
    CMFormatDescriptionRef CreateFormatDescriptionFromCodecData(uint32_t format_id, int width, int height, AVCodecContext* codecCtx, uint32_t atom);
    CFDataRef ff_videotoolbox_avcc_extradata_create(AVCodecContext *avctx);
    
private:
    NBMetaData mMetaData;
};

#endif /* NBVIDEOTOOLBOXDECODER_H */
