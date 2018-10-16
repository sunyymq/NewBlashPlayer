//
// Created by parallels on 9/10/18.
//

#ifndef NBGLVIDEORENDERER_H
#define NBGLVIDEORENDERER_H

#include "NBVideoRenderer.h"

#ifdef BUILD_TARGET_LINUX64
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#elif BUILD_TARGET_IOS
#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>
#elif BUILD_TARGET_ANDROID
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#endif

class NBGLVideoRenderer : public NBVideoRenderer {
public:
    NBGLVideoRenderer();
    virtual ~NBGLVideoRenderer();

    void setVideoOutput(NBRendererTarget* vo);
    nb_status_t displayFrame(NBMediaBuffer* mediaBuffer);

public:
    virtual nb_status_t start(NBMetaData* metaData = NULL);
    virtual nb_status_t stop();

protected:
    virtual nb_status_t displayFrameImpl(NBMediaBuffer* mediaBuffer, int tgtWidth, int tgtHeight) = 0;

    void mat4fLoadPerspective(GLfloat eyex, GLfloat eyey, GLfloat eyez,
                              GLfloat centerx, GLfloat centery, GLfloat centerz,
                              GLfloat upx, GLfloat upy, GLfloat upz);

    //The Sample Texture Id
    GLuint mSample0Texture;
    GLuint mSample1Texture;
    GLuint mSample2Texture;

    //The Shader Texture Slot
    GLint mSample0TextureSlot;
    GLint mSample1TextureSlot;
    GLint mSample2TextureSlot;

    //The Shader Align Bytes
    int32_t m0UnpackAlignBytes;
    int32_t m1UnpackAlignBytes;
    int32_t m2UnpackAlignBytes;

    //The Position Slot
    GLint mPositionSlot;

    //The Texture Slot
    GLint mTexCoordSlot;

    //The OpenGLES2 Program handle
    GLuint mProgramHandle;

    //The View Matrix
//        GLint mUniformMatrix;
    GLfloat mModelviewProj[16];

    //Whether the object is rendered
    bool mInited;

    static float videoTextureCoord[8];
    static float initFrameSize[8];
};

#endif //NBGLVIDEORENDERER_H
