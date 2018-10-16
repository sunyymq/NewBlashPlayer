//
// Created by parallels on 9/9/18.
//

#include "NBVideoFrame.h"

#ifdef BUILD_TARGET_IOS
#include <VideoToolbox/VideoToolbox.h>
#elif BUILD_TARGET_ANDROID
#include "AndroidMediacodecJava.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "libavcodec/avcodec.h"

#ifdef __cplusplus
}
#endif


NBVideoFrame::NBVideoFrame(AVFrame* frame, int64_t duration, bool release)
        :NBMediaBuffer(duration)
        ,mRelease(release) {
    mFrame = frame;
    mFrame->flags = 0;
}

NBVideoFrame::~NBVideoFrame() {
    if (mRelease && mFrame != NULL) {
#ifdef BUILD_TARGET_IOS
        CVBufferRelease((CVBufferRef)mFrame->data[3]);
#elif BUILD_TARGET_ANDROID
        AMediaCodec_releaseOutputBuffer((AMediaCodec*)mFrame->data[0], mFrame->linesize[0], mFrame->flags == 1 ? true : false);
#endif
        
//        av_frame_unref(mFrame);
        mRelease = false;
        mFrame = NULL;
    }
}

void* NBVideoFrame::data() {
    return mFrame;
}
