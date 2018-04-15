//
// Created by lake on 2018/4/7.
//

#ifndef NDKTEST_HBQUEUE_H
#define NDKTEST_HBQUEUE_H

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>

}
#include <queue>
#include <pthread.h>
#include "HbPlayStatus.h"

class HbQueue {

public://变量
    std::queue<AVPacket*> queuePacket;
    std::queue<AVFrame*> queueFrame;
    pthread_mutex_t mutexFrame;
    pthread_cond_t condFrame;//条件对象
    pthread_mutex_t mutexPacket;
    pthread_cond_t condPacket;//条件对象
    HbPlayStatus *hbPlayStatus = NULL;

public://方法
    HbQueue(HbPlayStatus *hbPlayStatus);
    ~HbQueue();//析构函数
    int putAvpacket(AVPacket *avPacket);
    int getAvpacket(AVPacket *avPacket);
    int clearAvpacket();
    int clearToKeyFrame();

    int putAvframe(AVFrame *avFrame);
    int getAvframe(AVFrame *avFrame);
    int clearAvFrame();

    void release();
    int getAvPacketSize();
    int getAvFrameSize();

    int noticeThread();//通知线程
};


#endif //NDKTEST_HBQUEUE_H
