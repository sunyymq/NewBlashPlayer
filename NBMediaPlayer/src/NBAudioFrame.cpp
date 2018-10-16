//
// Created by parallels on 9/9/18.
//

#include "NBAudioFrame.h"

#ifdef __cplusplus
extern "C"
{
#endif
#include "libavformat/avformat.h"
#ifdef __cplusplus
}
#endif

NBAudioFrame::NBAudioFrame(AVFrame* frame) {
    mFrame = frame;
}

NBAudioFrame::~NBAudioFrame() {
    av_frame_unref(mFrame);
}

void* NBAudioFrame::data() {
    return mFrame;
}