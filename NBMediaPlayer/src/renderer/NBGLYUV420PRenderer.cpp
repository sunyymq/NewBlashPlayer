//
// Created by liuenbao on 18-9-12.
//

#include "NBGLYUV420PRenderer.h"
#include "NBGLShaderUtils.h"

#include <NBLog.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <libavformat/avformat.h>

#ifdef __cplusplus
}
#endif

#define LOG_TAG "NBGLYUV420PRenderer"

#ifdef BUILD_TARGET_LINUX64
static const char* yuv420pVertexShaderString =
        SHADER_STRING
        (
                attribute vec4 position;
                attribute vec2 texcoord;
                varying vec2 v_texcoord;

                void main()
                {
                    gl_Position = position;
                    v_texcoord = texcoord.xy;
                }
        );

static const char* yuv420pFragmentShaderString =
        SHADER_STRING
        (
                varying vec2 v_texcoord;
                uniform sampler2D s_texture_y;
                uniform sampler2D s_texture_u;
                uniform sampler2D s_texture_v;

                void main()
                {
                    float y = texture2D(s_texture_y, v_texcoord).r;
                    float u = texture2D(s_texture_u, v_texcoord).r - 0.5;
                    float v = texture2D(s_texture_v, v_texcoord).r - 0.5;

                    float r = y +               1.13983 * v;
                    float g = y - 0.39465 * u - 0.58060 * v;
                    float b = y + 2.03211 * u;

                    gl_FragColor = vec4(r,g,b,1.0);
                }
        );
#elif defined(BUILD_TARGET_IOS) || defined(BUILD_TARGET_ANDROID)
static const char* yuv420pVertexShaderString =
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

static const char* yuv420pFragmentShaderString =
SHADER_STRING
(
    varying highp vec2 v_texcoord;
    uniform sampler2D s_texture_y;
    uniform sampler2D s_texture_u;
    uniform sampler2D s_texture_v;
 
    const highp mat3 conv_mat = mat3( 1,       1,         1,
                                      0,       -0.39465,  2.03211,
                                      1.13983, -0.58060,  0);
    void main()
    {
        highp vec3 yuv420p;
        yuv420p.x = texture2D(s_texture_y, v_texcoord).r;
        yuv420p.y = texture2D(s_texture_u, v_texcoord).r - 0.5;
        yuv420p.z = texture2D(s_texture_v, v_texcoord).r - 0.5;
     
        gl_FragColor = vec4(conv_mat*yuv420p, 1.0);
    }
);
#endif

NBGLYUV420PRenderer::NBGLYUV420PRenderer() {

}

NBGLYUV420PRenderer::~NBGLYUV420PRenderer() {

}

nb_status_t NBGLYUV420PRenderer::start(NBMetaData* metaData) {
    //call base start first
    NBGLVideoRenderer::start(NULL);

    mProgramHandle = NBGLShaderUtils::LoadShaders(yuv420pVertexShaderString, yuv420pFragmentShaderString);

    NBLOG_INFO(LOG_TAG, "mProgramHandle is : %d", mProgramHandle);

    glUseProgram(mProgramHandle);

    OPENGLES2_CHECK();

    mPositionSlot = glGetAttribLocation(mProgramHandle, "position");
    OPENGLES2_CHECK();

    glEnableVertexAttribArray(mPositionSlot);
    mTexCoordSlot = glGetAttribLocation(mProgramHandle, "texcoord");
    OPENGLES2_CHECK();

    glEnableVertexAttribArray(mTexCoordSlot);

    mSample0TextureSlot = glGetUniformLocation(mProgramHandle, "s_texture_y");
    OPENGLES2_CHECK();

    mSample1TextureSlot = glGetUniformLocation(mProgramHandle, "s_texture_u");
    OPENGLES2_CHECK();

    mSample2TextureSlot = glGetUniformLocation(mProgramHandle, "s_texture_v");
    OPENGLES2_CHECK();

//        mUniformMatrix = glGetUniformLocation(mProgramHandle, "modelViewProjectionMatrix");

    glActiveTexture(GL_TEXTURE0);

    OPENGLES2_CHECK();

    glGenTextures(1, &mSample0Texture);

    OPENGLES2_CHECK();

    glBindTexture(GL_TEXTURE_2D, mSample0Texture);
    OPENGLES2_CHECK();

    glActiveTexture(GL_TEXTURE1);

    OPENGLES2_CHECK();

    glGenTextures(1, &mSample1Texture);

    OPENGLES2_CHECK();

    glBindTexture(GL_TEXTURE_2D, mSample1Texture);
    OPENGLES2_CHECK();

    glActiveTexture(GL_TEXTURE2);

    OPENGLES2_CHECK();

    glGenTextures(1, &mSample2Texture);

    OPENGLES2_CHECK();

    glBindTexture(GL_TEXTURE_2D, mSample2Texture);

    OPENGLES2_CHECK();

    return OK;
}

nb_status_t NBGLYUV420PRenderer::stop() {
    if (mSample0Texture != 0) {
        glDeleteTextures(1, &mSample0Texture);
    }
    
    if (mSample1Texture != 0) {
        glDeleteTextures(1, &mSample1Texture);
    }
    
    if (mSample2Texture != 0) {
        glDeleteTextures(1, &mSample2Texture);
    }
    
    if (mProgramHandle != 0) {
        glDeleteProgram(mProgramHandle);
    }
    
    NBGLVideoRenderer::stop();
    return OK;
}

nb_status_t NBGLYUV420PRenderer::displayFrameImpl(NBMediaBuffer* mediaBuffer, int tgtWidth, int tgtHeight) {
    glUseProgram(mProgramHandle);
    
    AVFrame* videoFrame = (AVFrame*)mediaBuffer->data();
    if(!mInited) {
        int yLinesize = videoFrame->linesize[0];
        
        NBLOG_INFO(LOG_TAG, "the video frame format is : %d AV_PIX_FMT_GRAY8 is : %d", videoFrame->format, AV_PIX_FMT_GRAY8);
        
        if ((yLinesize & 4) == 0) {
            m0UnpackAlignBytes = 8;
        }else if ((yLinesize & 3) == 0) {
            m0UnpackAlignBytes = 4;
        }else if((yLinesize & 1) == 0) {
            m0UnpackAlignBytes = 2;
        }else {
            m0UnpackAlignBytes = 1;
        }

        int uLinesize = videoFrame->linesize[1];
        if ((uLinesize & 7) == 0) {
            m1UnpackAlignBytes = 8;
        }else if ((uLinesize & 3) == 0) {
            m1UnpackAlignBytes = 4;
        }else if((uLinesize & 1) == 0) {
            m1UnpackAlignBytes = 2;
        }else {
            m1UnpackAlignBytes = 1;
        }

        int vLinesize = videoFrame->linesize[2];
        if ((vLinesize & 7) == 0) {
            m2UnpackAlignBytes = 8;
        }else if ((vLinesize & 3) == 0) {
            m2UnpackAlignBytes = 4;
        }else if((vLinesize & 1) == 0) {
            m2UnpackAlignBytes = 2;
        }else {
            m2UnpackAlignBytes = 1;
        }

        memcpy(videoVertexsCoord, initFrameSize, sizeof(initFrameSize));

        float rightAlign = (videoFrame->linesize[0] - videoFrame->width) * 1.0f / videoFrame->width;
        videoVertexsCoord[2] += rightAlign * 2.1f;   //adjust the right position
        videoVertexsCoord[6] += rightAlign * 2.1f;

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mSample0Texture);
        glPixelStorei(GL_UNPACK_ALIGNMENT, m0UnpackAlignBytes);
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_LUMINANCE,
                     videoFrame->linesize[0],
                     videoFrame->height,
                     0,
                     GL_LUMINANCE,
                     GL_UNSIGNED_BYTE,
                     videoFrame->data[0]);
        OPENGLES2_CHECK();

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glUniform1i(mSample0TextureSlot, 0);
        OPENGLES2_CHECK();

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, mSample1Texture);
        glPixelStorei(GL_UNPACK_ALIGNMENT, m1UnpackAlignBytes);
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_LUMINANCE,
                     videoFrame->linesize[1],
                     videoFrame->height / 2,
                     0,
                     GL_LUMINANCE,
                     GL_UNSIGNED_BYTE,
                     videoFrame->data[1]);
        OPENGLES2_CHECK();

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glUniform1i(mSample1TextureSlot, 1);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, mSample2Texture);
        glPixelStorei(GL_UNPACK_ALIGNMENT, m2UnpackAlignBytes);
        OPENGLES2_CHECK();

        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_LUMINANCE,
                     videoFrame->linesize[2],
                     videoFrame->height / 2,
                     0,
                     GL_LUMINANCE,
                     GL_UNSIGNED_BYTE,
                     videoFrame->data[2]);
        OPENGLES2_CHECK();

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        OPENGLES2_CHECK();

        glUniform1i(mSample2TextureSlot, 2);
        OPENGLES2_CHECK();
        
//            glUniformMatrix4fv(mUniformMatrix, 1, GL_FALSE, mModelviewProj);

        glVertexAttribPointer(mPositionSlot, 2, GL_FLOAT, 0, 0, videoVertexsCoord);
        glEnableVertexAttribArray(mPositionSlot);
        OPENGLES2_CHECK();

        glVertexAttribPointer(mTexCoordSlot, 2, GL_FLOAT, 0, 0, videoTextureCoord);
        glEnableVertexAttribArray(mTexCoordSlot);
        OPENGLES2_CHECK();

        mInited = true;
    }else {
        glBindTexture(GL_TEXTURE_2D, mSample0Texture);
        glPixelStorei(GL_UNPACK_ALIGNMENT, m0UnpackAlignBytes);
        glTexSubImage2D(GL_TEXTURE_2D,
                        0,
                        0,
                        0,
                        videoFrame->linesize[0],
                        videoFrame->height,
                        GL_LUMINANCE,
                        GL_UNSIGNED_BYTE,
                        videoFrame->data[0]);
        OPENGLES2_CHECK();

        glBindTexture(GL_TEXTURE_2D, mSample1Texture);
        glPixelStorei(GL_UNPACK_ALIGNMENT, m1UnpackAlignBytes);
        glTexSubImage2D(GL_TEXTURE_2D,
                        0,
                        0,
                        0,
                        videoFrame->linesize[1],
                        videoFrame->height / 2,
                        GL_LUMINANCE,
                        GL_UNSIGNED_BYTE,
                        videoFrame->data[1]);
        OPENGLES2_CHECK();

        glBindTexture(GL_TEXTURE_2D, mSample2Texture);
        OPENGLES2_CHECK();

        glPixelStorei(GL_UNPACK_ALIGNMENT, m2UnpackAlignBytes);
        OPENGLES2_CHECK();

        glTexSubImage2D(GL_TEXTURE_2D,
                        0,
                        0,
                        0,
                        videoFrame->linesize[2],
                        videoFrame->height / 2,
                        GL_LUMINANCE,
                        GL_UNSIGNED_BYTE,
                        videoFrame->data[2]);
        OPENGLES2_CHECK();
    }
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    OPENGLES2_CHECK();
    return OK;
}
