#include <jni.h>
#include <string>

extern "C"
JNIEXPORT jstring

JNICALL
Java_com_example_ffmpeg_1yuv_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}

#include "ShaderUtils.h"
#include <EGL/egl.h>
#include <android/native_window_jni.h>
#include <unistd.h>

extern "C"{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavfilter/avfilter.h>
}

#define GET_STR(x) #x
const char *vertexShaderString = GET_STR(
        attribute vec4 aPosition;
        attribute vec2 aTexCoord;
        varying vec2 vTexCoord;
        void main() {
            vTexCoord=vec2(aTexCoord.x,1.0-aTexCoord.y);
            gl_Position = aPosition;
        }
);
const char *fragmentShaderString = GET_STR(
        precision mediump float;
        varying vec2 vTexCoord;
        uniform sampler2D yTexture;
        uniform sampler2D uTexture;
        uniform sampler2D vTexture;
        void main() {
            vec3 yuv;
            vec3 rgb;
            yuv.r = texture2D(yTexture, vTexCoord).r;
            yuv.g = texture2D(uTexture, vTexCoord).r - 0.5;
            yuv.b = texture2D(vTexture, vTexCoord).r - 0.5;
            rgb = mat3(1.0,       1.0,         1.0,
                       0.0,       -0.39465,  2.03211,
                       1.13983, -0.58060,  0.0) * yuv;
            gl_FragColor = vec4(rgb, 1.0);
        }
);

extern "C"
JNIEXPORT void JNICALL
Java_com_example_ffmpeg_1yuv_MainActivity_videoPlay(JNIEnv *env, jobject instance, jstring path_,
                                                    jobject surface) {
    const char *path = env->GetStringUTFChars(path_, 0);

    // TODO
/***
     * ffmpeg 初始化
     * **/
    av_register_all();
    AVFormatContext *fmt_ctx = avformat_alloc_context();
    if (avformat_open_input(&fmt_ctx, path, NULL, NULL) < 0) {
        return;
    }
    if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
        return;
    }
    AVStream *avStream = NULL;
    int video_stream_index = -1;
    for (int i = 0; i < fmt_ctx->nb_streams; i++) {
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            avStream =fmt_ctx->streams[i];
            video_stream_index = i;
            break;
        }
    }
    if (video_stream_index == -1) {
        return;
    }
    AVCodecContext *codec_ctx = avcodec_alloc_context3(NULL);
    avcodec_parameters_to_context(codec_ctx, avStream->codecpar);
    AVCodec *avCodec = avcodec_find_decoder(codec_ctx->codec_id);
    if (avcodec_open2(codec_ctx, avCodec, NULL) < 0) {
        return;
    }
    int y_size = codec_ctx->width * codec_ctx->height;
    AVPacket *pkt = (AVPacket *) malloc(sizeof(AVPacket));
    av_new_packet(pkt, y_size);

    /**
    *初始化egl
    **/
    EGLConfig eglConf;
    EGLSurface eglWindow;
    EGLContext eglCtx;
    int windowWidth;
    int windowHeight;
    ANativeWindow *nativeWindow = ANativeWindow_fromSurface(env, surface);

    EGLint configSpec[] = { EGL_RED_SIZE, 8,
                            EGL_GREEN_SIZE, 8,
                            EGL_BLUE_SIZE, 8,
                            EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_NONE };

    EGLDisplay eglDisp = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    EGLint eglMajVers, eglMinVers;
    EGLint numConfigs;
    eglInitialize(eglDisp, &eglMajVers, &eglMinVers);
    eglChooseConfig(eglDisp, configSpec, &eglConf, 1, &numConfigs);

    eglWindow = eglCreateWindowSurface(eglDisp, eglConf,nativeWindow, NULL);

    eglQuerySurface(eglDisp,eglWindow,EGL_WIDTH,&windowWidth);
    eglQuerySurface(eglDisp,eglWindow,EGL_HEIGHT,&windowHeight);
    const EGLint ctxAttr[] = {
            EGL_CONTEXT_CLIENT_VERSION, 2,
            EGL_NONE
    };
    eglCtx = eglCreateContext(eglDisp, eglConf,EGL_NO_CONTEXT, ctxAttr);

    eglMakeCurrent(eglDisp, eglWindow, eglWindow, eglCtx);


    /**
     * 设置opengl 要在egl初始化后进行
     * **/
    float *vertexData = new float[12]{
            1.0f, -1.0f, 0.0f,
            -1.0f, -1.0f, 0.0f,
            1.0f, 1.0f, 0.0f,
            -1.0f, 1.0f, 0.0f
    };

    float *textureVertexData = new float[8]{
            1.0f, 0.0f,//右下
            0.0f, 0.0f,//左下
            1.0f, 1.0f,//右上
            0.0f, 1.0f//左上
    };
    ShaderUtils *shaderUtils = new ShaderUtils();

    GLuint programId = shaderUtils->createProgram(vertexShaderString,fragmentShaderString );
    delete shaderUtils;
    GLuint aPositionHandle = (GLuint) glGetAttribLocation(programId, "aPosition");
    GLuint aTextureCoordHandle = (GLuint) glGetAttribLocation(programId, "aTexCoord");

    GLuint textureSamplerHandleY = (GLuint) glGetUniformLocation(programId, "yTexture");
    GLuint textureSamplerHandleU = (GLuint) glGetUniformLocation(programId, "uTexture");
    GLuint textureSamplerHandleV = (GLuint) glGetUniformLocation(programId, "vTexture");



    //因为没有用矩阵所以就手动自适应
    int videoWidth = codec_ctx->width;
    int videoHeight = codec_ctx->height;

    int left,top,viewWidth,viewHeight;
    if(windowHeight > windowWidth){
        left = 0;
        viewWidth = windowWidth;
        viewHeight = (int)(videoHeight*1.0f/videoWidth*viewWidth);
        top = (windowHeight - viewHeight)/2;
    }else{
        top = 0;
        viewHeight = windowHeight;
        viewWidth = (int)(videoWidth*1.0f/videoHeight*viewHeight);
        left = (windowWidth - viewWidth)/2;
    }
    glViewport(left, top, viewWidth, viewHeight);

    glUseProgram(programId);
    glEnableVertexAttribArray(aPositionHandle);
    glVertexAttribPointer(aPositionHandle, 3, GL_FLOAT, GL_FALSE,12, vertexData);

    glEnableVertexAttribArray(aTextureCoordHandle);
    glVertexAttribPointer(aTextureCoordHandle,2,GL_FLOAT,GL_FALSE,8,textureVertexData);
    /***
     * 初始化空的yuv纹理
     * **/
    GLuint yTextureId;
    GLuint uTextureId;
    GLuint vTextureId;
    glGenTextures(1,&yTextureId);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,yTextureId);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glUniform1i(textureSamplerHandleY,0);

    glGenTextures(1,&uTextureId);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D,uTextureId);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glUniform1i(textureSamplerHandleU,1);

    glGenTextures(1,&vTextureId);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D,vTextureId);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glUniform1i(textureSamplerHandleV,2);


    /***
     * 开始解码
     * **/
    int ret;
    while (1) {
        usleep(30000);
        if (av_read_frame(fmt_ctx, pkt) < 0) {
            //播放结束
            break;
        }
        if (pkt->stream_index == video_stream_index) {
            ret = avcodec_send_packet(codec_ctx, pkt);
            if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
                av_packet_unref(pkt);
                continue;
            }
            AVFrame *yuvFrame = av_frame_alloc();
            ret = avcodec_receive_frame(codec_ctx, yuvFrame);
            if (ret < 0 && ret != AVERROR_EOF) {
                av_frame_free(&yuvFrame);
                av_packet_unref(pkt);
                continue;
            }
            /***
              * 解码后的数据更新到yuv纹理中
            * **/


            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, yTextureId);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, yuvFrame->linesize[0], yuvFrame->height,0, GL_LUMINANCE, GL_UNSIGNED_BYTE, yuvFrame->data[0]);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, uTextureId);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE,  yuvFrame->linesize[1], yuvFrame->height/2,0, GL_LUMINANCE, GL_UNSIGNED_BYTE, yuvFrame->data[1]);

            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, vTextureId);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE,  yuvFrame->linesize[2], yuvFrame->height/2,0, GL_LUMINANCE, GL_UNSIGNED_BYTE, yuvFrame->data[2]);


            /***
            * 纹理更新完成后开始绘制
            ***/
            glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            eglSwapBuffers(eglDisp, eglWindow);

            av_frame_free(&yuvFrame);
        }
        av_packet_unref(pkt);
    }
    /***
     * 释放资源
     * **/
    delete vertexData;
    delete textureVertexData;

    eglMakeCurrent(eglDisp, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(eglDisp, eglCtx);
    eglDestroySurface(eglDisp, eglWindow);
    eglTerminate(eglDisp);
    eglDisp = EGL_NO_DISPLAY;
    eglWindow = EGL_NO_SURFACE;
    eglCtx = EGL_NO_CONTEXT;

    avcodec_close(codec_ctx);
    avformat_close_input(&fmt_ctx);

    env->ReleaseStringUTFChars(path_, path);
}