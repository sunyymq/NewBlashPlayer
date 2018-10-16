//
// Created by parallels on 9/9/18.
//

#ifndef NBAUDIOFRAME_H
#define NBAUDIOFRAME_H

#include "NBMediaBuffer.h"

struct AVFrame;

class NBAudioFrame: public NBMediaBuffer {
public:
    NBAudioFrame(AVFrame* frame);
    ~NBAudioFrame();

    virtual void* data();

private:
    AVFrame* mFrame;
};

#endif //NBAUDIOFRAME_H
