//
// Created by lake on 2018/4/14.
//

#ifndef NDKTEST_HBAUDIO_H
#define NDKTEST_HBAUDIO_H

#include <cwchar>
#include <pthread.h>
#include <assert.h>
#include "HbPlayStatus.h"
#include "HbJavaCall.h"
#include "HbQueue.h"
#include "HbAudioChannel.h"
#include "AndroidLog.h"
#include <unistd.h>

extern "C"
{
#include <libswresample/swresample.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <libavformat/avformat.h>
};

class HbAudio {
public:
    HbJavaCall *hbJavaCall = NULL;//回调函数
    HbPlayStatus *hbPlayStatus = NULL;//播放状态
    HbQueue *audioPacketQueue = NULL; //音频Packet队列
    HbAudioChannel *hbAudioChannel = NULL;//音频通道
    pthread_t audioThread;//初始化开启音频播放线程

    void *buffer = NULL;//pcm缓存buffer
    int data_size = 0;//buffer大小
    uint8_t *out_buffer = NULL;//buffer 内存区域
    int pcmsize = 0;//pam缓存buffer大小

    double now_audioTime = 0;//当前音频时间
    double audioClock = 0;//音频播放时间
    double audioClockbk = 0;//音频播放时间备份
    //音频解码器上下文
    AVCodecContext *pAudioCodecCtx = NULL;

    SLObjectItf engineObject = NULL;
    SLEngineItf engineEngine;
    SLObjectItf outputMixObject = NULL;
    SLEnvironmentalReverbItf outputMixEnvironmentalReverb = NULL;
    SLObjectItf bqPlayerObject = NULL;
    SLPlayItf bqPlayerPlay;
    SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue;
    SLVolumeItf bqPlayerVolume;
    SLmilliHertz bqPlayerSampleRate = 0;
    const SLEnvironmentalReverbSettings reverbSettings =
            SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR;

public:
    HbAudio(HbPlayStatus *hbPlayStatus, HbJavaCall *javaCall, AVCodecContext *audioContext,
            HbAudioChannel *audioChannel);

    ~HbAudio();

    //初始化opensl
    int initOpenSL(int sample_rate, int channel);

    //获取pcm数据
    int getPcmData(void **pcm);

    //暂停
    void pause();

    //重启
    void resume();

    //释放资源
    void release();

    //播放音频
    int play();
};


#endif //NDKTEST_HBAUDIO_H
