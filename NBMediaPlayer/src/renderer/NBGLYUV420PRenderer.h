//
// Created by liuenbao on 18-9-12.
//

#ifndef NBGLYUV420PRENDERER_H
#define NBGLYUV420PRENDERER_H

#include "NBGLVideoRenderer.h"

class NBGLYUV420PRenderer : public NBGLVideoRenderer {
public:
    NBGLYUV420PRenderer();
    virtual ~NBGLYUV420PRenderer();

public:
    virtual nb_status_t start(NBMetaData* metaData = NULL);
    virtual nb_status_t stop();

private:
    virtual nb_status_t displayFrameImpl(NBMediaBuffer* mediaBuffer, int tgtWidth, int tgtHeight);
    
    float videoVertexsCoord[8];
};

#endif //NBGLYUV420PRENDERER_H
