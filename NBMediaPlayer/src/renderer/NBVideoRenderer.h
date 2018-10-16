//
// Created by liu enbao on 24/09/2018.
//

#ifndef NBVIDEORENDERER_H
#define NBVIDEORENDERER_H

#include <NBMacros.h>
#include <NBRendererInfo.h>
#include "NBMediaBuffer.h"
#include "foundation/NBMetaData.h"

class NBVideoRenderer {
public:
    NBVideoRenderer();
    virtual ~NBVideoRenderer();

public:
    void setVideoOutput(NBRendererTarget* vo) {
        mVideoOutput = vo;
    }

    virtual nb_status_t displayFrame(NBMediaBuffer* mediaBuffer) = 0;

public:
    virtual nb_status_t start(NBMetaData* metaData = NULL) = 0;
    virtual nb_status_t stop() = 0;

protected:
    NBRendererTarget* mVideoOutput;
};


#endif //NBVIDEORENDERER_H
