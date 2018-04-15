//
// Created by lake on 2018/4/14.
//
#include "HbAudio.h"

/**
 * 构造函数
 * @param playStatus
 * @param javaCall
 */
HbAudio::HbAudio(HbPlayStatus *playStatus, HbJavaCall *javaCall, AVCodecContext *audioContext,
                 HbAudioChannel *audioChannel) {
    hbPlayStatus = playStatus;
    hbJavaCall = javaCall;
    pAudioCodecCtx = audioContext;
    audioPacketQueue = new HbQueue(playStatus);
    out_buffer = (uint8_t *) malloc(pAudioCodecCtx->sample_rate * 2 * 2 * 2 / 3);
    hbAudioChannel = audioChannel;
}

/**
 * 音频初始化
 * @param context
 * @return
 */
void *audioPlayThread(void *context) {
    HbAudio *hbAudio = (HbAudio *) context;
    if (&hbAudio->hbAudioChannel != NULL) {
        hbAudio->initOpenSL(hbAudio->pAudioCodecCtx->sample_rate,
                            hbAudio->pAudioCodecCtx->channels);
    }
    pthread_exit(&hbAudio->audioThread);
}

int HbAudio::getPcmData(void **pcm) {
    while (!hbPlayStatus->stop) {
        if (hbPlayStatus->pause)//暂停
        {
            usleep(100000);
            continue;
        }
        if (hbPlayStatus->seek) {
            continue;
        }
        AVPacket *packet = av_packet_alloc();
        if (audioPacketQueue->getAvpacket(packet) != 0) {
            av_packet_free(&packet);
            av_free(packet);
            packet = NULL;
            continue;
        }

        int ret = avcodec_send_packet(pAudioCodecCtx, packet);
        if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
            av_packet_free(&packet);
            av_free(packet);
            packet = NULL;
            continue;
        }
        AVFrame *frame = av_frame_alloc();
        if (avcodec_receive_frame(pAudioCodecCtx, frame) == 0) {
            SwrContext *swr_ctx;
            //设置格式转换
            swr_ctx = swr_alloc_set_opts(NULL,
                                         pAudioCodecCtx->channel_layout,
                                         AV_SAMPLE_FMT_S16,
                                         pAudioCodecCtx->sample_rate,
                                         pAudioCodecCtx->channel_layout,
                                         pAudioCodecCtx->sample_fmt,
                                         pAudioCodecCtx->sample_rate,
                                         0, NULL);

            if (!swr_ctx || (ret = swr_init(swr_ctx)) < 0) {
                av_frame_free(&frame);
                av_free(frame);
                frame = NULL;
                swr_free(&swr_ctx);
                av_packet_free(&packet);
                av_free(packet);
                packet = NULL;
                continue;
            }
            //处理不同的格式
            if (pAudioCodecCtx->sample_fmt == AV_SAMPLE_FMT_S16P) {
                data_size = av_samples_get_buffer_size(NULL, av_frame_get_channels(frame),
                                                       frame->nb_samples,
                                                       (AVSampleFormat) frame->format,
                                                       1);
            } else {
                av_samples_get_buffer_size(&data_size, av_frame_get_channels(frame),
                                           frame->nb_samples,
                                           (AVSampleFormat) frame->format, 1);
            }
            // 音频格式转换
            swr_convert(swr_ctx, &out_buffer, frame->nb_samples,
                        (uint8_t const **) (frame->extended_data),
                        frame->nb_samples);
            now_audioTime = frame->pts * av_q2d(hbAudioChannel->time_base);
            if (now_audioTime < audioClock) {
                now_audioTime = audioClock;
            }
            audioClock = now_audioTime;

            if ((int) audioClockbk != (int) audioClock) {
                hbJavaCall->onProgressInfo(HB_THREAD_CHILD, (int64_t) audioClock);
            }

            audioClockbk = audioClock;
            *pcm = out_buffer;
            av_frame_free(&frame);
            av_free(frame);
            frame = NULL;
            swr_free(&swr_ctx);
            break;
        } else {
            av_frame_free(&frame);
            av_free(frame);
            frame = NULL;
            av_packet_free(&packet);
            av_free(packet);
            packet = NULL;
            continue;
        }
    }
    return data_size;
}

/*监听是否播放结束*/
void playOverEvent(SLPlayItf caller, void *pContext, SLuint32 playevent) {
    if (playevent == SL_PLAYEVENT_HEADATEND) {
        LOGI("播放结束！！！");//需要判断手动还是自动
        //shutdown();//释放opensl资源
    }
}

/**
 * OPENSL 音频回调函数
 * @param bq
 * @param context
 */
void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context) {
    HbAudio *hbAudio = (HbAudio *) context;
    assert(bq == hbAudio->bqPlayerBufferQueue);
    if (hbAudio != NULL) {
        hbAudio->buffer = NULL;
        hbAudio->pcmsize = hbAudio->getPcmData(&hbAudio->buffer);
        if (NULL != hbAudio->buffer && 0 != hbAudio->pcmsize) {
            hbAudio->audioClock +=
                    hbAudio->pcmsize / ((double) (hbAudio->pAudioCodecCtx->sample_rate * 2 * 2));
            (*hbAudio->bqPlayerBufferQueue)->Enqueue(hbAudio->bqPlayerBufferQueue, hbAudio->buffer,
                                                     hbAudio->pcmsize);
        }
    }
}

/**
 * opensl 初始化
 * @param sampleRate
 * @param channel
 * @return
 */
int HbAudio::initOpenSL(int sampleRate, int channel) {
    //创建OpenSLES引擎
    SLresult result;
    //创建引擎
    result = slCreateEngine(&engineObject, 0, 0, 0, 0, 0);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;
    //关联引擎
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;
    //获取引擎接口
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;
    //创建输出混音器
    const SLInterfaceID mids[1] = {SL_IID_ENVIRONMENTALREVERB};
    const SLboolean mreq[1] = {SL_BOOLEAN_FALSE};
    result = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 1, mids, mreq);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;
    //关联输出混音器
    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;
    //获取reverb接口
    result = (*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB,
                                              &outputMixEnvironmentalReverb);
    if (SL_RESULT_SUCCESS == result) {
        result = (*outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(
                outputMixEnvironmentalReverb, &reverbSettings);
        (void) result;
    }

    // create buffer queue audio player
    if (sampleRate >= 0) {
        bqPlayerSampleRate = sampleRate * 1000;
    }
    //配置音频源
    SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
    SLDataFormat_PCM format_pcm = {SL_DATAFORMAT_PCM,
                                   1,
                                   SL_SAMPLINGRATE_8,
                                   SL_PCMSAMPLEFORMAT_FIXED_16,
                                   SL_PCMSAMPLEFORMAT_FIXED_16,
                                   SL_SPEAKER_FRONT_CENTER,
                                   SL_BYTEORDER_LITTLEENDIAN};
    if (bqPlayerSampleRate) {
        format_pcm.samplesPerSec = bqPlayerSampleRate;       //sample rate in mili second
    }
    format_pcm.numChannels = (SLuint32) channel;
    if (channel == 2) {
        format_pcm.channelMask = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
    } else {
        format_pcm.channelMask = SL_SPEAKER_FRONT_CENTER;
    }
    SLDataSource audioSrc = {&loc_bufq, &format_pcm};

    //配置音频池
    SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
    SLDataSink audioSnk = {&loc_outmix, 0};


    const SLInterfaceID ids[3] = {SL_IID_BUFFERQUEUE, SL_IID_VOLUME, SL_IID_EFFECTSEND,
            /*SL_IID_MUTESOLO,*/};
    const SLboolean req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE,
            /*SL_BOOLEAN_TRUE,*/ };
    //创建音频播放器
    result = (*engineEngine)->CreateAudioPlayer(engineEngine, &bqPlayerObject, &audioSrc, &audioSnk,
                                                bqPlayerSampleRate ? 2 : 3, ids, req);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;
    // 关联播放器
    result = (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;
    // 获取播放接口
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY, &bqPlayerPlay);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;
    // 获取缓冲队列接口
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_BUFFERQUEUE,
                                             &bqPlayerBufferQueue);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;
    // 注册缓冲队列回调
    result = (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, bqPlayerCallback, this);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;
    // 获取音量接口
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_VOLUME, &bqPlayerVolume);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;
    //注册回调
    result = (*bqPlayerPlay)->RegisterCallback(bqPlayerPlay, playOverEvent, this);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;
    //设置播放结束回调
    result = (*bqPlayerPlay)->SetCallbackEventsMask(bqPlayerPlay, SL_PLAYEVENT_HEADATEND);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;
    result = (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;
    bqPlayerCallback(bqPlayerBufferQueue, this);
    return 0;
}

/**
 * 播放音频
 * @return
 */
int HbAudio::play() {
    pthread_create(&audioThread, NULL, audioPlayThread, this);
    return 0;
}

/**
 * 暂停播放音频
 */
void HbAudio::pause() {
    SLresult result;
    // 开始播放音乐
    if (bqPlayerPlay != NULL) {
        result = (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PAUSED);
        assert(SL_RESULT_SUCCESS == result);
        (void) result;
    }
}

/**
 * 继续播放音频
 */
void HbAudio::resume() {
    SLresult result;
    // 开始播放音乐
    if (bqPlayerPlay != NULL) {
        result = (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);
        assert(SL_RESULT_SUCCESS == result);
        (void) result;
    }
}

/**
 * 释放音频资源
 */
void HbAudio::release() {
    pause();
    if (audioPacketQueue != NULL) {
        audioPacketQueue->noticeThread();
        audioPacketQueue->release();
        delete (audioPacketQueue);
        audioPacketQueue = NULL;
    }

    // destroy buffer queue audio player object, and invalidate all associated interfaces
    if (bqPlayerObject != NULL) {
        (*bqPlayerObject)->Destroy(bqPlayerObject);
        bqPlayerObject = NULL;
        bqPlayerPlay = NULL;
        bqPlayerBufferQueue = NULL;
        bqPlayerVolume = NULL;
        bqPlayerBufferQueue = NULL;
        buffer = NULL;
        pcmsize = 0;
    }

    // destroy output mix object, and invalidate all associated interfaces
    if (outputMixObject != NULL) {
        (*outputMixObject)->Destroy(outputMixObject);
        outputMixObject = NULL;
        outputMixEnvironmentalReverb = NULL;
    }

    // destroy engine object, and invalidate all associated interfaces
    if (engineObject != NULL) {
        (*engineObject)->Destroy(engineObject);
        engineObject = NULL;
        engineEngine = NULL;
    }

    if (out_buffer != NULL) {
        free(out_buffer);
        out_buffer = NULL;
    }
    if (buffer != NULL) {
        free(buffer);
        buffer = NULL;
    }
    if (pAudioCodecCtx != NULL) {
        avcodec_close(pAudioCodecCtx);
        avcodec_free_context(&pAudioCodecCtx);
        pAudioCodecCtx = NULL;
    }
    if (hbPlayStatus != NULL) {
        hbPlayStatus = NULL;
    }
}

/**
 * 析构函数
 */
HbAudio::~HbAudio() {

}