//
// Created by lake on 2018/4/15.
//

#include "HbFFmpeg.h"

HbFFmpeg::HbFFmpeg(HbJavaCall *javaCall, const char *path) {
    pthread_mutex_init(&initMutex, NULL);
    pthread_mutex_init(&seekMutex, NULL);
    hbJavaCall = javaCall;
    file_name = path;
    hbPlayStatus = new HbPlayStatus();
}

HbFFmpeg::~HbFFmpeg() {
    pthread_mutex_destroy(&initMutex);
}

int HbFFmpeg::getCodecContext(AVCodecParameters *pCodecPar, AVCodecContext **pCodecCtx) {
    AVCodec *pCodec;
    switch (pCodecPar->codec_id) {
        case AV_CODEC_ID_H264:
            pCodec = avcodec_find_decoder_by_name("h264_mediacodec");//硬解码264
            if (pCodec == NULL) {
                LOGE("Couldn't find Codec.\n");
                return -1;
            }
            break;
        case AV_CODEC_ID_MPEG4:
            pCodec = avcodec_find_decoder_by_name("mpeg4_mediacodec");//硬解码mpeg4
            if (pCodec == NULL) {
                LOGE("Couldn't find Codec.\n");
                return -1;
            }
            break;
        case AV_CODEC_ID_HEVC:
            pCodec = avcodec_find_decoder_by_name("hevc_mediacodec");//硬解码265
            if (pCodec == NULL) {
                LOGE("Couldn't find Codec.\n");
                return -1;
            }
            break;
        case AV_CODEC_ID_VP8:
            pCodec = avcodec_find_decoder_by_name("vp8_mediacodec");//硬解码vp8
            if (pCodec == NULL) {
                LOGE("Couldn't find Codec.\n");
                return -1;
            }
            break;
        case AV_CODEC_ID_VP9:
            pCodec = avcodec_find_decoder_by_name("vp9_mediacodec");//硬解码vp9
            if (pCodec == NULL) {
                LOGE("Couldn't find Codec.\n");
                return -1;
            }
            break;
        default:
            pCodec = avcodec_find_decoder(pCodecPar->codec_id);
            if (pCodec == NULL) {
                LOGE("Couldn't find Codec.\n");
                return -1;
            }
            break;
    }
    *pCodecCtx = avcodec_alloc_context3(pCodec);
    if (avcodec_parameters_to_context(*pCodecCtx, pCodecPar) != 0) {
        LOGE("Couldn't copy codec context");
        return -1; //
    }
    //打开解码器
    if (avcodec_open2(*pCodecCtx, pCodec, NULL) < 0) {
        LOGE("Could not open codec.");
        return -1;
    }
    return 0;
}

int HbFFmpeg::initFFmpeg() {
    pthread_mutex_lock(&initMutex);
    av_register_all();
    avformat_network_init();
    pFormatCtx = avformat_alloc_context();
    if (avformat_open_input(&pFormatCtx, file_name, NULL, NULL) != 0) {
        LOGE("Couldn't open file:%s\n", file_name);
        return -1;
    }
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        LOGE("Couldn't find stream information.");
        return -1;
    }

    iTotalSeconds = (int64_t) pFormatCtx->duration / 1000000;

    hbJavaCall->onTotalTime(HB_THREAD_MAIN, iTotalSeconds);
    LOGE("nb_streams = %d", pFormatCtx->nb_streams);
    int count = 0;
    for (int i = 0; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream = i;
        } else if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {//有可能有多路音轨
            LOGE("audioStream = %d", i);
            AVDictionaryEntry *t = NULL;
            char *lang = "und";
            audioChannel[count] = i;
            t = av_dict_get(pFormatCtx->streams[i]->metadata, "language", NULL,
                            AV_DICT_IGNORE_SUFFIX);//获取语言信息
            if (t != NULL && t->value != NULL) {
                lang = t->value;
                LOGE("%s: %s", t->key, t->value);
            }
            HbAudioChannel *hbAudioChannel = new HbAudioChannel(count, i, lang,
                                                                pFormatCtx->streams[i]->time_base);
            language[count] = lang;
            LOGE("language: %s", language[count]);
            audiochannels.push_front(hbAudioChannel);
            count++;
        }
    }
    hbJavaCall->onChannelInfo(HB_THREAD_MAIN, audioChannel, language, count, 0);

    audioStream = audioChannel[0];

    //获取视频解码器
    getCodecContext(pFormatCtx->streams[videoStream]->codecpar, &pVideoCodecCtx);
    //获取音频解码器
    LOGE("获取视频解码器");
    getCodecContext(pFormatCtx->streams[audioStream]->codecpar, &pAudioCodecCtx);
    LOGE("获取音频解码器");
    //实例化audio类
    hbAudio = new HbAudio(hbPlayStatus, hbJavaCall, pAudioCodecCtx, audiochannels.at(0));

    hbVideo = new HbVideo(hbPlayStatus, hbJavaCall, pVideoCodecCtx, nativeWindow, hbAudio);
    hbVideo->videoStream = videoStream;
    hbVideo->time_base = pFormatCtx->streams[videoStream]->time_base;
    //速率
    hbVideo->rate = pFormatCtx->streams[videoStream]->avg_frame_rate.num /
                    pFormatCtx->streams[videoStream]->avg_frame_rate.den;
    LOGE("fps = %d", hbVideo->rate);

    pthread_mutex_unlock(&initMutex);
    return 0;
}

int HbFFmpeg::decodeVideo() {
    pthread_mutex_lock(&initMutex);
    int ret = -1;
    while (!hbPlayStatus->stop) {
        if (hbPlayStatus->pause)//暂停
        {
            usleep(100000);
            continue;
        }
        if (hbVideo->videoPacketQueue->getAvPacketSize() > 100) {
            usleep(20000);
            continue;
        }
        if (hbAudio->audioPacketQueue->getAvPacketSize() > 100) {
            usleep(20000);
            continue;
        }
        AVPacket *packet = av_packet_alloc();
        pthread_mutex_lock(&seekMutex);
        ret = av_read_frame(pFormatCtx, packet);
        pthread_mutex_unlock(&seekMutex);
        if (hbPlayStatus->seek) {
            av_packet_free(&packet);
            av_free(packet);
            continue;
        }
        if (ret == 0) {
            if (packet->stream_index == videoStream) {//视频
                hbVideo->videoPacketQueue->putAvpacket(packet);
            } else if (packet->stream_index == audioStream) {//音频
                hbAudio->audioPacketQueue->putAvpacket(packet);
            } else {
                av_packet_free(&packet);
                av_free(packet);
                packet = NULL;
            }

        } else {
            av_packet_free(&packet);
            av_free(packet);
            packet = NULL;
            if ((hbVideo->videoPacketQueue->getAvPacketSize() == 0) &&
                (hbAudio->audioPacketQueue->getAvPacketSize() == 0)) {
                hbPlayStatus->stop = true;
                break;
            }
        }
    }
    pthread_mutex_unlock(&initMutex);
    return 0;
}

/**
 * 解码线程
 * @param data
 * @return
 */
void *decodeThread(void *data) {
    HbFFmpeg *hbFFmpeg = (HbFFmpeg *) data;
    hbFFmpeg->decodeVideo();
    pthread_exit(&hbFFmpeg->decodVoidThread);
}

/**
 * 开始解码
 * @return
 */
int HbFFmpeg::startDecode() {
    pthread_create(&decodVoidThread, NULL, decodeThread, this);
    return 0;
}

int HbFFmpeg::playVideo() {
    if (hbVideo != NULL) {
        hbVideo->decodeFrame();
        hbVideo->play();
    }
    if (hbAudio != NULL) {
        hbAudio->play();
    }
    startDecode();//开始解码
    return 0;
}

int HbFFmpeg::release() {
    hbPlayStatus->stop = true;
    pthread_mutex_lock(&initMutex);
    if (hbAudio != NULL) {
        if (LOG_SHOW) {
            LOGE("释放audio....................................2");
        }

        hbAudio->release();
        delete (hbAudio);
        hbAudio = NULL;
    }
    if (LOG_SHOW) {
        LOGE("释放video....................................");
    }

    if (hbVideo != NULL) {
        if (LOG_SHOW) {
            LOGE("释放video....................................2");
        }

        hbVideo->release();
        delete (hbVideo);
        hbVideo = NULL;
    }
    if (LOG_SHOW) {
        LOGE("释放format...................................");
    }

    if (pFormatCtx != NULL) {
        avformat_close_input(&pFormatCtx);
        avformat_free_context(pFormatCtx);
        pFormatCtx = NULL;
    }
    if (LOG_SHOW) {
        LOGE("释放javacall.................................");
    }

    if (hbJavaCall != NULL) {
        hbJavaCall = NULL;
    }
    pthread_mutex_unlock(&initMutex);
    return 0;
}

void HbFFmpeg::pause() {
    if (hbPlayStatus != NULL) {
        hbPlayStatus->pause = true;
        if (hbAudio != NULL) {
            hbAudio->pause();
        }
    }
}

void HbFFmpeg::resume() {
    if (hbPlayStatus != NULL) {
        hbPlayStatus->pause = false;
        if (hbAudio != NULL) {
            hbAudio->resume();
        }
    }
}

int HbFFmpeg::seek(int64_t sec) {
    hbPlayStatus->seek = true;
    pthread_mutex_lock(&seekMutex);
    int64_t rel = ((int64_t) sec + 10) * AV_TIME_BASE;
    int ret = av_seek_frame(pFormatCtx, -1, rel, AVSEEK_FLAG_BACKWARD);
    if (hbVideo->videoPacketQueue != NULL) {
        hbVideo->videoPacketQueue->clearAvpacket();
        hbVideo->videoPacketQueue->clearAvFrame();
        hbVideo->now_videoTime = 0;
        hbVideo->videoClock = 0;
    }
    if (hbAudio->audioPacketQueue != NULL) {
        hbAudio->audioPacketQueue->clearAvpacket();
        hbAudio->now_audioTime = 0;
        hbAudio->audioClock = 0;
    }
    pthread_mutex_unlock(&seekMutex);
    hbPlayStatus->seek = false;
    return 0;
}

int HbFFmpeg::switchAudioChannel(int64_t channel) {
    //LOGE("CHANNEL==%d",channel);
    for (int i = 0; i < audiochannels.size(); i++) {
        //LOGE("audiochannels.size()=%d",audiochannels.size());
        if (audiochannels.at(i)->channel == channel) {
            if (hbAudio != NULL) {
                hbAudio->hbAudioChannel = (HbAudioChannel *) audiochannels.at(i);
            }
            audioStream = audiochannels.at(i)->audioStream;
        }
    }
    return 0;

}