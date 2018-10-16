//
// Created by parallels on 9/9/18.
//

#ifndef NBVIDEOFRAME_H_H
#define NBVIDEOFRAME_H_H

#include "NBMediaBuffer.h"

struct AVFrame;

class NBVideoFrame: public NBMediaBuffer {
public:
    NBVideoFrame(AVFrame* frame, int64_t duration, bool release = true);
    virtual ~NBVideoFrame();

    virtual void* data();

private:
    AVFrame* mFrame;

    //Do not relase in hardware decode mode
    bool mRelease;
};

#endif //NBVIDEOFRAME_H_H
