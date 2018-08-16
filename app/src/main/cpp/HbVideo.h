//
// Created by lake on 2018/4/14.
//

#ifndef NDKTEST_HBVIDEO_H
#define NDKTEST_HBVIDEO_H

#include <cwchar>
#include <android/native_window.h>
#include "HbPlayStatus.h"
#include "HbQueue.h"
#include "HbJavaCall.h"
#include "HbAudio.h"
#include "AndroidLog.h"
#include <unistd.h>

extern "C"
{
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavutil/time.h>
};

class HbVideo {
public:
    HbAudio *hbAudio = NULL;
    HbPlayStatus *hbPlayStatus = NULL;//播放状态
    HbQueue *videoPacketQueue = NULL; //视频队列
    pthread_t decFrame;//解packet压缩线程
    pthread_t drawFrame;//绘图线程
    HbJavaCall *hbJavaCall = NULL;
    //视频解码器上下文
    AVCodecContext *pVideoCodecCtx = NULL;
    //渲染窗口
    ANativeWindow *nativeWindow;

    int videoStream;
    AVRational time_base;
    int rate = 0;
    double delayTime = 0;//延迟
    double videoClock = 0;//视频播放时间
    double video_clock = 0;
    double now_videoTime = 0;//当前视频时间

    bool seekbyUser = false;


public:
    HbVideo(HbPlayStatus *hbPlayStatus, HbJavaCall *hbJavaCall, AVCodecContext *videoCodeContext,
            ANativeWindow *nativeWindow, HbAudio *audio);

    ~HbVideo();

    double synchronize(AVFrame *srcFrame, double pts);

    double getDelayTime(double diff);

    int drawVideoFrame();

    void decodeFrame();

    void play();

    int pause();

    int resume();

    void release();

};


#endif //NDKTEST_HBVIDEO_H