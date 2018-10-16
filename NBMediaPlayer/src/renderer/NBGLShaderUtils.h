//
// Created by liuenbao on 18-9-12.
//

#ifndef GLSHADERUTILS_H
#define GLSHADERUTILS_H

#define GL_GLEXT_PROTOTYPES

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

#include <NBLog.h>

#define SHADER_STRING(x) #x

//#define OPENGLES2_NOCHECK

#ifndef OPENGLES2_NOCHECK

#define OPENGLES2_CHECK()                               \
            do {                                        \
                GLenum error = glGetError();            \
                switch (error) {                        \
                case GL_INVALID_ENUM:                   \
                    NBLOG_ERROR("OPENGLES2_NOCHECK", "OpenGLES2 GL_INVALID_ENUM in %s %d\n", __FUNCTION__, __LINE__);  \
                    break;                              \
                case GL_INVALID_VALUE:                  \
                    NBLOG_ERROR("OPENGLES2_NOCHECK", "OpenGLES2 GL_INVALID_VALUE in %s %d\n", __FUNCTION__, __LINE__); \
                    break;                              \
                case GL_INVALID_OPERATION:              \
                    NBLOG_ERROR("OPENGLES2_NOCHECK", "OpenGLES2 GL_INVALID_OPERATION in %s %d\n", __FUNCTION__, __LINE__); \
                    break;                              \
                case GL_OUT_OF_MEMORY:                  \
                    NBLOG_ERROR("OPENGLES2_NOCHECK", "OpenGLES2 GL_OUT_OF_MEMORY in %s %d\n", __FUNCTION__, __LINE__); \
                    break;                              \
                case GL_NO_ERROR:                       \
                default:                                \
                    break;                              \
                }                                       \
            } while(0)                                  \

#else

#define OPENGLES2_CHECK()

#endif

class NBGLShaderUtils {
public:
    static GLuint LoadShaders(const char * vertex_code,const char * fragment_code);
};

#endif //SHADERUTILS_H
