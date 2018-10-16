//
// Created by liu enbao on 24/09/2018.
//

#ifndef NBMEDIACODECVRENDERER_H
#define NBMEDIACODECVRENDERER_H

#include "NBVideoRenderer.h"

class NBMediaCodecVRenderer : public NBVideoRenderer {
public:
    NBMediaCodecVRenderer();
    ~NBMediaCodecVRenderer();

public:
    virtual nb_status_t displayFrame(NBMediaBuffer* mediaBuffer);

public:
    virtual nb_status_t start(NBMetaData* metaData = NULL);
    virtual nb_status_t stop();
};


#endif //NBMEDIACODECVRENDERER_H
