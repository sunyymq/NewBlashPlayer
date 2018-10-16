//
//  NBCVOpenGLESRenderer.hpp
//  iOSNBMediaPlayer
//
//  Created by liu enbao on 19/09/2018.
//  Copyright © 2018 liu enbao. All rights reserved.
//

#ifndef NBCVOPENGLESRENDERER_H
#define NBCVOPENGLESRENDERER_H

#include "NBGLVideoRenderer.h"

class NBCVOpenGLESRenderer : public NBGLVideoRenderer {
public:
    NBCVOpenGLESRenderer();
    virtual ~NBCVOpenGLESRenderer();
    
public:
    virtual nb_status_t start(NBMetaData* metaData = NULL);
    virtual nb_status_t stop();
    
private:
    virtual nb_status_t displayFrameImpl(NBMediaBuffer* mediaBuffer, int tgtWidth, int tgtHeight);
    
private:
    const GLfloat *_preferredConversion;
    void* _lumaTexture;
    void* _chromaTexture;
    void* _videoTextureCache;
};

#endif /* NBCVOPENGLESRENDERER_H */
