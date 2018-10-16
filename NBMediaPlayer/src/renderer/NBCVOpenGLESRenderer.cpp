//
//  NBCVOpenGLESRenderer.cpp
//  iOSNBMediaPlayer
//
//  Created by liu enbao on 19/09/2018.
//  Copyright © 2018 liu enbao. All rights reserved.
//

#include "NBCVOpenGLESRenderer.h"

#include "NBGLShaderUtils.h"

#include <CoreVideo/CoreVideo.h>

#include <NBLog.h>

#ifdef __cplusplus
extern "C" {
#endif
    
#include <libavformat/avformat.h>
    
#ifdef __cplusplus
}
#endif

static const char* nv12VertexShaderString =
SHADER_STRING
(
 attribute vec4 position;
 attribute vec2 texcoord;
 varying highp vec2 v_texcoord;
 
 void main()
 {
     gl_Position = position;
     v_texcoord = texcoord.xy;
 }
 );

static const char* nv12FragmentShaderString =
SHADER_STRING
(
 uniform highp mat3 colorConversionMatrix;
 varying highp vec2 v_texcoord;
 uniform sampler2D s_texture_y;
 uniform sampler2D s_texture_uv;
 
 void main()
 {
     mediump vec3 yuv;
     yuv.x = (texture2D(s_texture_y, v_texcoord).r - (16.0/255.0));
     yuv.yz = (texture2D(s_texture_uv, v_texcoord).ra - vec2(0.5, 0.5));
     
    lowp vec3 rgb = colorConversionMatrix * yuv;
     
     gl_FragColor = vec4(rgb, 1.0);
 }
 );

// BT.601, which is the standard for SDTV.
static const GLfloat kColorConversion601[] = {
    1.164,  1.164, 1.164,
    0.0, -0.392, 2.017,
    1.596, -0.813,   0.0,
};

// BT.709, which is the standard for HDTV.
static const GLfloat kColorConversion709[] = {
    1.164,  1.164, 1.164,
    0.0, -0.213, 2.112,
    1.793, -0.533,   0.0,
};

//// BT.601 full range (ref: http://www.equasys.de/colorconversion.html)
//const GLfloat kColorConversion601FullRange[] = {
//    1.0,    1.0,    1.0,
//    0.0,    -0.343, 1.765,
//    1.4,    -0.711, 0.0,
//};

#define LOG_TAG "NBCVOpenGLESRenderer"

NBCVOpenGLESRenderer::NBCVOpenGLESRenderer() {
    
}

NBCVOpenGLESRenderer::~NBCVOpenGLESRenderer() {
    
}

nb_status_t NBCVOpenGLESRenderer::start(NBMetaData* metaData) {
    //call base start first
    NBGLVideoRenderer::start(NULL);
    
    mProgramHandle = NBGLShaderUtils::LoadShaders(nv12VertexShaderString, nv12FragmentShaderString);
    OPENGLES2_CHECK();
    
    glUseProgram(mProgramHandle);
    OPENGLES2_CHECK();
    
    mPositionSlot = glGetAttribLocation(mProgramHandle, "position");
    glEnableVertexAttribArray(mPositionSlot);
    OPENGLES2_CHECK();
    
    mTexCoordSlot = glGetAttribLocation(mProgramHandle, "texcoord");
    glEnableVertexAttribArray(mTexCoordSlot);
    OPENGLES2_CHECK();
    
    mSample0TextureSlot = glGetUniformLocation(mProgramHandle, "s_texture_y");
    OPENGLES2_CHECK();
    
    mSample1TextureSlot = glGetUniformLocation(mProgramHandle, "s_texture_uv");
    OPENGLES2_CHECK();
    
    // current use matrix
    mSample2TextureSlot = glGetUniformLocation(mProgramHandle, "colorConversionMatrix");
    OPENGLES2_CHECK();
    glEnableVertexAttribArray(mSample2TextureSlot);
    OPENGLES2_CHECK();
    
    glUniform1i(mSample0TextureSlot, 0);
    OPENGLES2_CHECK();
    
    glUniform1i(mSample1TextureSlot, 1);
    OPENGLES2_CHECK();
    
    _videoTextureCache = NULL;
    _lumaTexture = NULL;
    _chromaTexture = NULL;
    
    return OK;
}

nb_status_t NBCVOpenGLESRenderer::stop() {
    if (_lumaTexture) {
        CFRelease(_lumaTexture);
        _lumaTexture = NULL;
    }
    
    if (_chromaTexture) {
        CFRelease(_chromaTexture);
        _chromaTexture = NULL;
    }
    
    if (_videoTextureCache != NULL) {
        CFRelease(_videoTextureCache);
        _videoTextureCache = NULL;
    }
    
    if (mProgramHandle != 0) {
        glDeleteProgram(mProgramHandle);
        mProgramHandle = 0;
    }
    
    NBGLVideoRenderer::stop();

    return OK;
}

nb_status_t NBCVOpenGLESRenderer::displayFrameImpl(NBMediaBuffer* mediaBuffer, int tgtWidth, int tgtHeight) {
    AVFrame* videoFrame = (AVFrame*)mediaBuffer->data();
    // 3 is from ffmpeg source
    CVPixelBufferRef pixelBuffer = (CVPixelBufferRef)videoFrame->data[3];
    if(pixelBuffer == NULL) {
        NBLOG_ERROR(LOG_TAG, "Pixel buffer is null");
        return UNKNOWN_ERROR;
    }
    
    CVReturn err;
    
    glUseProgram(mProgramHandle);
    OPENGLES2_CHECK();
    
    if (_videoTextureCache == NULL) {
        // Create CVOpenGLESTextureCacheRef for optimal CVPixelBufferRef to GLES texture conversion.
        err = CVOpenGLESTextureCacheCreate(kCFAllocatorDefault, NULL, getRendererCtx(mVideoOutput), NULL, (CVOpenGLESTextureCacheRef*)&_videoTextureCache);
        if (err != noErr) {
            NBLOG_ERROR(LOG_TAG, "Error at CVOpenGLESTextureCacheCreate %d", err);
            return UNKNOWN_ERROR;
        }
        OPENGLES2_CHECK();
    }
    
//    size_t planeCount = CVPixelBufferGetPlaneCount(pixelBuffer);
    /*
     Use the color attachment of the pixel buffer to determine the appropriate color conversion matrix.
     */
    CFTypeRef colorAttachments = CVBufferGetAttachment(pixelBuffer, kCVImageBufferYCbCrMatrixKey, NULL);
    
    if (colorAttachments == kCVImageBufferYCbCrMatrix_ITU_R_601_4) {
        _preferredConversion = kColorConversion601;
    }
    else {
        _preferredConversion = kColorConversion709;
    }
    
    if (_lumaTexture) {
        CFRelease(_lumaTexture);
        _lumaTexture = NULL;
    }
    
    if (_chromaTexture) {
        CFRelease(_chromaTexture);
        _chromaTexture = NULL;
    }
    
    CVOpenGLESTextureCacheFlush((CVOpenGLESTextureCacheRef)_videoTextureCache, 0);
    
    size_t frameWidth = CVPixelBufferGetWidth(pixelBuffer);
    size_t frameHeight = CVPixelBufferGetHeight(pixelBuffer);
    
    glActiveTexture(GL_TEXTURE0);
    OPENGLES2_CHECK();
    
    err = CVOpenGLESTextureCacheCreateTextureFromImage(kCFAllocatorDefault,
                                                       (CVOpenGLESTextureCacheRef)_videoTextureCache,
                                                       pixelBuffer,
                                                       NULL,
                                                       GL_TEXTURE_2D,
                                                       GL_LUMINANCE,
                                                       frameWidth,
                                                       frameHeight,
                                                       GL_LUMINANCE,
                                                       GL_UNSIGNED_BYTE,
                                                       0,
                                                       (CVOpenGLESTextureRef*)&_lumaTexture);
    OPENGLES2_CHECK();
    
    if (err) {
        NBLOG_ERROR(LOG_TAG, "Error at CVOpenGLESTextureCacheCreateTextureFromImage for luma %d", err);
    }
    
    glBindTexture(CVOpenGLESTextureGetTarget((CVOpenGLESTextureRef)_lumaTexture), CVOpenGLESTextureGetName((CVOpenGLESTextureRef)_lumaTexture));
    OPENGLES2_CHECK();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    OPENGLES2_CHECK();

    // UV-plane.
    glActiveTexture(GL_TEXTURE1);
    OPENGLES2_CHECK();
    err = CVOpenGLESTextureCacheCreateTextureFromImage(kCFAllocatorDefault,
                                                       (CVOpenGLESTextureCacheRef)_videoTextureCache,
                                                       pixelBuffer,
                                                       NULL,
                                                       GL_TEXTURE_2D,
                                                       GL_LUMINANCE_ALPHA,
                                                       frameWidth / 2,
                                                       frameHeight / 2,
                                                       GL_LUMINANCE_ALPHA,
                                                       GL_UNSIGNED_BYTE,
                                                       1,
                                                       (CVOpenGLESTextureRef*)&_chromaTexture);
    OPENGLES2_CHECK();
    if (err) {
        NBLOG_ERROR(LOG_TAG, "Error at CVOpenGLESTextureCacheCreateTextureFromImage for chroma %d", err);
    }

    glBindTexture(CVOpenGLESTextureGetTarget((CVOpenGLESTextureRef)_chromaTexture), CVOpenGLESTextureGetName((CVOpenGLESTextureRef)_chromaTexture));
    OPENGLES2_CHECK();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    OPENGLES2_CHECK();
    
    static float videoVertexsCoord[8];
    memcpy(videoVertexsCoord, initFrameSize, sizeof(initFrameSize));
    
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    OPENGLES2_CHECK();
    
    glUniformMatrix3fv(mSample2TextureSlot, 1, GL_FALSE, _preferredConversion);
    OPENGLES2_CHECK();

    glVertexAttribPointer(mPositionSlot, 2, GL_FLOAT, 0, 0, videoVertexsCoord);
    glEnableVertexAttribArray(mPositionSlot);
    OPENGLES2_CHECK();
    
    OPENGLES2_CHECK();
    
    glVertexAttribPointer(mTexCoordSlot, 2, GL_FLOAT, 0, 0, videoTextureCoord);
    glEnableVertexAttribArray(mTexCoordSlot);
    OPENGLES2_CHECK();
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    OPENGLES2_CHECK();
    
    return OK;
}
