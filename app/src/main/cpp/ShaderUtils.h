//
// Created by 海米 on 2018/3/27.
//

#ifndef FFMPEG_YUV_SHADERUTILS_H
#define FFMPEG_YUV_SHADERUTILS_H

#include <GLES2/gl2.h>

class ShaderUtils {
public:
    GLuint createProgram(const char *vertexSource, const char *fragmentSource);

    GLuint loadShader(GLenum shaderType, const char *source);
};


#endif //FFMPEG_YUV_SHADERUTILS_H
