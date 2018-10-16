//
// Created by liuenbao on 18-9-12.
//

#include "NBGLShaderUtils.h"
#include <stdio.h>

#include <NBLog.h>

#define LOG_TAG "NBGLShaderUtils"

GLuint NBGLShaderUtils::LoadShaders(const char * vertex_code,const char * fragment_code) {
    // Create the shaders
    GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

    GLint Result = GL_FALSE;
    int InfoLogLength;

    // Compile Vertex Shader
    NBLOG_INFO(LOG_TAG, "Compiling shader code : %s\n", vertex_code);
    char const * VertexSourcePointer = vertex_code;
    glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
    glCompileShader(VertexShaderID);

    // Check Vertex Shader
    glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if ( InfoLogLength > 0 ) {
        char* VertexShaderErrorMessage = new char[InfoLogLength];
        glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, VertexShaderErrorMessage);
        NBLOG_INFO(LOG_TAG, "vertex info : %s\n", VertexShaderErrorMessage);
        delete []VertexShaderErrorMessage;
    }

    // Compile Fragment Shader
    NBLOG_INFO(LOG_TAG, "Compiling shader : %s\n", fragment_code);
    char const * FragmentSourcePointer = fragment_code;
    glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
    glCompileShader(FragmentShaderID);

    // Check Fragment Shader
    glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if ( InfoLogLength > 0 ) {
        char* FragmentShaderErrorMessage = new char[InfoLogLength];
        glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, FragmentShaderErrorMessage);
        NBLOG_INFO(LOG_TAG, "fragment info : %s\n", FragmentShaderErrorMessage);
        delete []FragmentShaderErrorMessage;
    }

    // Link the program
    NBLOG_INFO(LOG_TAG, "Linking program VertexShaderID : %d FragmentShaderID : %d", VertexShaderID, FragmentShaderID);
    GLuint ProgramID = glCreateProgram();
    glAttachShader(ProgramID, VertexShaderID);
    glAttachShader(ProgramID, FragmentShaderID);
    glLinkProgram(ProgramID);

    // Check the program
    glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
    glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if ( InfoLogLength > 0 ) {
        char* ProgramErrorMessage = new char[InfoLogLength];
        glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, ProgramErrorMessage);
        NBLOG_INFO(LOG_TAG, "link info : %s\n", ProgramErrorMessage);
        delete []ProgramErrorMessage;
    }

    glDetachShader(ProgramID, VertexShaderID);
    glDetachShader(ProgramID, FragmentShaderID);

    glDeleteShader(VertexShaderID);
    glDeleteShader(FragmentShaderID);

    NBLOG_INFO(LOG_TAG, "ProgramID is : %d", ProgramID);

    return ProgramID;
}
