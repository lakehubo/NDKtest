//
// Created by lake on 2018/4/15.
//

#ifndef NDKTEST_HBFFMPEG_H
#define NDKTEST_HBFFMPEG_H
#define MAX_CHANNEL 8

#define LANGUAGE_CHINESE 0
#define LANGUAGE_ENGLISH 1

#include <cwchar>
#include <pthread.h>
#include "HbPlayStatus.h"
#include "HbAudio.h"
#include "HbVideo.h"
#include "AndroidLog.h"
extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
};
#ifdef ANDROID
#include <android/native_window.h>

#endif

class HbFFmpeg {
public:
    HbJavaCall *hbJavaCall = NULL;
    HbPlayStatus *hbPlayStatus = NULL;
    pthread_t decodVoidThread; //视频解码线程 塞入数据到视频Packet队列和音频Packet队列

    pthread_cond_t cond; //条件对象
    pthread_mutex_t initMutex; //音视频解码线程锁
    pthread_mutex_t seekMutex; //音视频解码seek线程锁

    const char *file_name = NULL;//文件名 路径
    //视频解码器上下文
    AVCodecContext *pVideoCodecCtx = NULL;
    //音频解码器上下文
    AVCodecContext *pAudioCodecCtx = NULL;
    //渲染窗口
    ANativeWindow *nativeWindow;

    AVFormatContext *pFormatCtx;

    int videoStream = -1, audioStream = -1;
    int64_t audioChannel[MAX_CHANNEL] = {-1,-1,-1,-1,-1,-1,-1,-1};
    const char *language[MAX_CHANNEL]={"null","null","null","null","null","null","null","null"};

    int64_t iTotalSeconds = 0;

    HbAudio *hbAudio = NULL;//音频对象类
    HbVideo *hbVideo = NULL;//视频对象类

    std::deque<HbAudioChannel*> audiochannels;

public:
    HbFFmpeg(HbJavaCall *javaCall,const char *path);
    ~HbFFmpeg();
    int initFFmpeg();
    int getCodecContext(AVCodecParameters *pCodecPar, AVCodecContext **pCodecCtx);
    int startDecode();
    int decodeVideo();
    int playVideo();
    int release();
    void pause();
    void resume();
    int seek(int64_t sec);
    int switchAudioChannel(int64_t channel);
};


#endif //NDKTEST_HBFFMPEG_H
