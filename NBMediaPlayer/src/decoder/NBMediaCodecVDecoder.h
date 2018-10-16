#ifndef NB_MEDIACODEC_VDECODER_H
#define NB_MEDIACODEC_VDECODER_H

#include "NBMediaSource.h"
#include "AndroidMediaformatJava.h"
#include "AndroidMediacodecJava.h"
#include "JNIUtils.h"
#include "AndroidJni.h"

#ifdef __cplusplus
extern "C"
{
#endif

#include "libavcodec/avcodec.h"

#ifdef __cplusplus
}
#endif

struct AVStream;
struct AVFrame;
struct SwsContext;
struct AVCodecContext;
struct AVBitStreamFilterContext;

class NBMediaCodecVDecoder : public NBMediaSource {
public:
    NBMediaCodecVDecoder(NBMediaSource* videoTrack, void* surface);
    virtual ~NBMediaCodecVDecoder();

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
    bool openVideoCodecContext(AVCodecContext* codecContext, NBMetaData* metaData);
    void closeVideoCodecContext();

    bool h264_extradata_to_annexb(AVCodecContext *avctx, const int padding, uint8_t** pOutData, int *outSize);
    int hevc_extradata_to_annexb(AVCodecContext *avctx, uint8_t** out_extradata, int* out_extradata_size);

    NBMediaSource* mVideoTrack;
    AVStream* mVideoStream;
    AVFrame* mSwscaledFrame;
    AVCodecContext* mVideoCodecContext;
    AVBitStreamFilterContext *mBitStreamFilterCtx;
    int64_t mStartTime;

    SwsContext* mSwsContext;

    // std::string mMediaCodecName;

    bool mSeekable;

    int mVideoWidth;
    int mVideoHeight;
    int mColorFormat;

    NBMediaBuffer* mCacheMediaBuffer;

    AMediaFormat *mInputAformat;
    AMediaFormat *mOutputAformat;
    AMediaCodec *mAMediaCodec;
    ssize_t mOutputBufferIndex;

    int64_t mLastFramePTS;
    bool mReachTheEnd;
    bool mQueuedLastBuffer;
    enum AVDiscard* mVideoSkipStream;

    int orig_extradata_size;
    uint8_t *orig_extradata;

    void* mSurface;

private:
    NBMetaData mMetaData;
};

#endif
