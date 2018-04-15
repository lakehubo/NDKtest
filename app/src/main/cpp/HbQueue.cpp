//
// Created by lake on 2018/4/7.
//

#include "HbQueue.h"

HbQueue::HbQueue(HbPlayStatus *playStatus) {

    hbPlayStatus = playStatus;
    pthread_mutex_init(&mutexPacket, NULL);//初始化锁对象
    pthread_cond_init(&condPacket, NULL);//初始化条件变量
    pthread_mutex_init(&mutexFrame, NULL);//初始化锁对象
    pthread_cond_init(&condFrame, NULL);//初始化条件变量

}

HbQueue::~HbQueue() {//自动回收

    hbPlayStatus = NULL;
    pthread_mutex_destroy(&mutexPacket);
    pthread_cond_destroy(&condPacket);
    pthread_mutex_destroy(&mutexFrame);
    pthread_cond_destroy(&condFrame);

}

/**
 * 释放队列类
 */
void HbQueue::release() {

    noticeThread();//通知线程
    clearAvpacket();//清空packet队列
    clearAvFrame();//清空frame队列

}

/**
 * 填充packet队列
 * @param avPacket
 * @return
 */
int HbQueue::putAvpacket(AVPacket *avPacket) {

    pthread_mutex_lock(&mutexPacket);
    queuePacket.push(avPacket);
    pthread_cond_signal(&condPacket);
    pthread_mutex_unlock(&mutexPacket);

    return 0;
}

/**
 * 获取packet数据
 * @param avPacket
 * @return
 */
int HbQueue::getAvpacket(AVPacket *avPacket) {

    pthread_mutex_lock(&mutexPacket);

    while (hbPlayStatus != NULL && !hbPlayStatus->stop) {
        if (queuePacket.size() > 0) {
            AVPacket *pkt = queuePacket.front();
            if (av_packet_ref(avPacket, pkt) == 0)//产生一个AVPacket的reference（引用）
            {
                queuePacket.pop();
            }
            av_packet_free(&pkt);
            av_free(pkt);
            pkt = NULL;
            break;
        } else {
            if (!hbPlayStatus->stop) {
                pthread_cond_wait(&condPacket, &mutexPacket);
            }
        }
    }
    pthread_mutex_unlock(&mutexPacket);
    return 0;
}

/**
 * 清空Avpacket队列
 * @return
 */
int HbQueue::clearAvpacket() {

    pthread_cond_signal(&condPacket);
    pthread_mutex_lock(&mutexPacket);
    while (!queuePacket.empty()) {
        AVPacket *pkt = queuePacket.front();
        queuePacket.pop();
        av_packet_free(&pkt);
        av_free(pkt);
        pkt = NULL;
    }
    pthread_mutex_unlock(&mutexPacket);
    return 0;
}

/**
 * 获取队列长度
 * @return
 */
int HbQueue::getAvPacketSize() {
    int size = 0;
    pthread_mutex_lock(&mutexPacket);
    size = queuePacket.size();
    pthread_mutex_unlock(&mutexPacket);
    return size;
}

/**
 * 填充frame队列
 * @param avFrame
 * @return
 */
int HbQueue::putAvframe(AVFrame *avFrame) {
    pthread_mutex_lock(&mutexFrame);
    queueFrame.push(avFrame);
    pthread_cond_signal(&condFrame);
    pthread_mutex_unlock(&mutexFrame);
    return 0;
}

/**
 * 获取frame数据
 * @param avFrame
 * @return
 */
int HbQueue::getAvframe(AVFrame *avFrame) {
    pthread_mutex_lock(&mutexFrame);

    while (hbPlayStatus != NULL && !hbPlayStatus->stop) {
        if (queueFrame.size() > 0) {
            AVFrame *frame = queueFrame.front();
            if (av_frame_ref(avFrame, frame) == 0) {
                queueFrame.pop();
            }
            av_frame_free(&frame);
            av_free(frame);
            frame = NULL;
            break;
        } else {
            if (!hbPlayStatus->stop) {
                pthread_cond_wait(&condFrame, &mutexFrame);
            }
        }
    }
    pthread_mutex_unlock(&mutexFrame);
    return 0;
}

/**
 * 清空frame队列
 * @return
 */
int HbQueue::clearAvFrame() {
    pthread_cond_signal(&condFrame);
    pthread_mutex_lock(&mutexFrame);
    while (!queueFrame.empty()) {
        AVFrame *frame = queueFrame.front();
        queueFrame.pop();
        av_frame_free(&frame);
        av_free(frame);
        frame = NULL;
    }
    pthread_mutex_unlock(&mutexFrame);
    return 0;
}

/**
 * 获取frame队列长度
 * @return
 */
int HbQueue::getAvFrameSize() {
    int size = 0;
    pthread_mutex_lock(&mutexFrame);
    size = queueFrame.size();
    pthread_mutex_unlock(&mutexFrame);
    return size;
}

/**
 * 通知线程
 * @return
 */
int HbQueue::noticeThread() {
    pthread_cond_signal(&condFrame);
    pthread_cond_signal(&condPacket);
    return 0;
}

/**
 * 清空frame队列到关键frame位置 这里有问题，因为dts和pts是不一致的
 * @return
 */
int HbQueue::clearToKeyFrame() {
    pthread_cond_signal(&condPacket);
    pthread_mutex_lock(&mutexPacket);
    while (!queuePacket.empty()) {
        AVPacket *pkt = queuePacket.front();
        if (pkt->flags != AV_PKT_FLAG_KEY) {
            queuePacket.pop();
            av_free(pkt->data);
            av_free(pkt->buf);
            av_free(pkt->side_data);
            pkt = NULL;
        } else {
            break;
        }
    }
    pthread_mutex_unlock(&mutexPacket);
    return 0;
}
