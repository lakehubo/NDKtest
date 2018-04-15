//
// Created by lake on 2018/4/14.
//
#include "HbVideo.h"

HbVideo::HbVideo(HbPlayStatus *playStatus, HbJavaCall *javaCall,
                 AVCodecContext *videoCodeContext,ANativeWindow *nativeView,HbAudio *aduio) {
    hbPlayStatus = playStatus;
    hbJavaCall = javaCall;
    pVideoCodecCtx = videoCodeContext;
    videoPacketQueue = new HbQueue(playStatus);
    nativeWindow = nativeView;
    hbAudio = aduio;
}
HbVideo::~HbVideo() {

}

void *decodeframe(void *data) {
    HbVideo *hbVideo = (HbVideo *) data;
    while (!hbVideo->hbPlayStatus->stop) {
        if (hbVideo->hbPlayStatus->pause) {
            usleep(100000);
            continue;
        }
        if (hbVideo->hbPlayStatus->seek) {
            continue;
        }
        if (hbVideo->videoPacketQueue->getAvFrameSize() > 30) {
            usleep(20000);
            continue;
        }
        AVPacket *packet = av_packet_alloc();
        if (hbVideo->videoPacketQueue->getAvpacket(packet) != 0) {
            av_packet_free(&packet);
            av_free(packet);
            packet = NULL;
            usleep(20000);
            continue;
        }
        LOGI("PTS:%d  DTS:%d",packet->pts,packet->dts);
        int ret = avcodec_send_packet(hbVideo->pVideoCodecCtx, packet);
        if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
            av_packet_free(&packet);
            av_free(packet);
            packet = NULL;
            usleep(20000);
            continue;
        }
        AVFrame *frame = av_frame_alloc();
        ret = avcodec_receive_frame(hbVideo->pVideoCodecCtx, frame);
        if (ret < 0 && ret != AVERROR_EOF) {
            av_frame_free(&frame);
            av_free(frame);
            frame = NULL;
            av_packet_free(&packet);
            av_free(packet);
            packet = NULL;
            usleep(20000);
            continue;
        }
        hbVideo->videoPacketQueue->putAvframe(frame);
        av_packet_free(&packet);
        av_free(packet);
        packet = NULL;
    }
    pthread_exit(&hbVideo->decFrame);
}

/**
 * 同步视频时间
 * @param srcFrame
 * @param pts
 * @return
 */
double HbVideo::synchronize(AVFrame *srcFrame, double pts) {
    double frame_delay;

    if (pts != 0)
        video_clock = pts; // Get pts,then set video clock to it
    else
        pts = video_clock; // Don't get pts,set it to video clock

    frame_delay = av_q2d(time_base);
    frame_delay += srcFrame->repeat_pict * (frame_delay * 0.5);

    video_clock += frame_delay;

    return pts;
}

/**
 * 计算延迟
 * @param diff
 * @return
 */
double HbVideo::getDelayTime(double diff) {

    //LOGI("audio video diff is %f", diff);

    if (diff > 0.003) {
        delayTime = delayTime / 3 * 2;
        if (delayTime < rate / 2) {
            delayTime = rate / 3 * 2;
        } else if (delayTime > rate * 2) {
            delayTime = rate * 2;
        }

    } else if (diff < -0.003) {
        delayTime = delayTime * 3 / 2;
        if (delayTime < rate / 2) {
            delayTime = rate / 3 * 2;
        } else if (delayTime > rate * 2) {
            delayTime = rate * 2;
        }
    } else if (diff == 0) {
        delayTime = rate;
    }
    if (diff > 1.0) {
        delayTime = 0;
    }
    if (diff < -1.0) {
        delayTime = rate * 2;
    }
    if (fabs(diff) > 10) {
        delayTime = rate;
    }
    return delayTime;
}

/**
* 绘制视频
*/
int HbVideo::drawVideoFrame() {
    // 用于渲染
    AVFrame *pFrameRGBA = av_frame_alloc();
    ANativeWindow_Buffer windowBuffer;
    if (pFrameRGBA == NULL) {
        LOGE("Could not allocate video frame.");
        return 0;
    }
    // 获取视频宽高
    int videoWidth = pVideoCodecCtx->width;
    int videoHeight = pVideoCodecCtx->height;
    // 设置native window的buffer大小,可自动拉伸
    ANativeWindow_setBuffersGeometry(nativeWindow, videoWidth, videoHeight,
                                     WINDOW_FORMAT_RGBA_8888);
    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA,
                                            pVideoCodecCtx->width,
                                            pVideoCodecCtx->height,
                                            1);
    uint8_t *buffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));
    av_image_fill_arrays(pFrameRGBA->data,
                         pFrameRGBA->linesize,
                         buffer,
                         AV_PIX_FMT_RGBA,
                         pVideoCodecCtx->width,
                         pVideoCodecCtx->height,
                         1);
    // 由于解码出来的帧格式不是RGBA的,在渲染之前需要进行格式转换
    struct SwsContext *sws_ctx = sws_getContext(pVideoCodecCtx->width/*视频宽度*/,
                                                pVideoCodecCtx->height/*视频高度*/,
                                                pVideoCodecCtx->pix_fmt/*像素格式*/,
                                                pVideoCodecCtx->width/*目标宽度*/,
                                                pVideoCodecCtx->height/*目标高度*/,
                                                AV_PIX_FMT_RGBA/*目标格式*/,
                                                SWS_BICUBIC/*图像转换的一些算法*/, NULL, NULL, NULL);
    if (sws_ctx == NULL) {
        LOGE("Cannot initialize the conversion context!\n");
        return 0;
    }

    while (!hbPlayStatus->stop) {
        if (hbPlayStatus->pause)//暂停
        {
            usleep(100000);
            continue;
        }
        if (hbPlayStatus->seek) {
            continue;
        }
        // Allocate video frame
        AVFrame *pFrame = av_frame_alloc();
        if (videoPacketQueue->getAvframe(pFrame) == 0) {
            //LOGE("我在绘图");
            if ((now_videoTime = av_frame_get_best_effort_timestamp(pFrame)) == AV_NOPTS_VALUE) {
                now_videoTime = 0;
            }
            now_videoTime *= av_q2d(time_base);
            //LOGE("now_videoTime=%f",now_videoTime);
            videoClock = synchronize(pFrame, now_videoTime);
            //LOGE("videoClock=%f",videoClock);
            //LOGE("audioClock=%f",audioClock);
            double diff = 0;
            if (hbAudio->audioClock != NULL) {
                diff = hbAudio->audioClock - videoClock;
            }
            delayTime = getDelayTime(diff);
            //LOGE("delayTime=%d", diff);
            if (diff >= 0.8&&!hbPlayStatus->seek) {
                //LOGE("我在丢帧");
                av_frame_free(&pFrame);
                av_free(pFrame);
                pFrame = NULL;
                videoPacketQueue->clearToKeyFrame();
                continue;
            }
            av_usleep(delayTime * 1000);
            // lock native window buffer
            ANativeWindow_lock(nativeWindow, &windowBuffer, 0);
            // 格式转换
            sws_scale(sws_ctx, (uint8_t const *const *) pFrame->data,
                      pFrame->linesize, 0, pVideoCodecCtx->height,
                      pFrameRGBA->data, pFrameRGBA->linesize);
            // 获取stride
            uint8_t *dst = (uint8_t *) windowBuffer.bits;
            int dstStride = windowBuffer.stride * 4;
            uint8_t *src = pFrameRGBA->data[0];
            int srcStride = pFrameRGBA->linesize[0];

            // 由于window的stride和帧的stride不同,因此需要逐行复制
            int h;
            for (h = 0; h < videoHeight; h++) {
                memcpy(dst + h * dstStride, src + h * srcStride, srcStride);
            }
            ANativeWindow_unlockAndPost(nativeWindow);

            av_frame_free(&pFrame);
            av_free(pFrame);
            pFrame = NULL;
            continue;
        }
        av_frame_free(&pFrame);
        av_free(pFrame);
        pFrame = NULL;
        continue;
    }
    //释放ffmpeg资源
    av_free(buffer);
    av_frame_free(&pFrameRGBA);
    av_free(pFrameRGBA);
    pFrameRGBA = NULL;
    pthread_exit(&drawFrame);
}

void *drawframe(void *data){
    HbVideo *hbVideo = (HbVideo *) data;
    hbVideo->drawVideoFrame();
    pthread_exit(&hbVideo->drawFrame);
}

void HbVideo::decodeFrame() {
    pthread_create(&decFrame, NULL, decodeframe, this);
}

void HbVideo::play() {
    pthread_create(&drawFrame, NULL, drawframe, this);
}

void HbVideo::release() {
    if(videoPacketQueue != NULL)
    {
        videoPacketQueue->noticeThread();
    }
    if(videoPacketQueue != NULL)
    {
        videoPacketQueue->release();
        delete(videoPacketQueue);
        videoPacketQueue = NULL;
    }
    if(hbJavaCall != NULL)
    {
        hbJavaCall = NULL;
    }
    if(hbAudio != NULL)
    {
        hbAudio = NULL;
    }
    if(pVideoCodecCtx != NULL)
    {
        avcodec_close(pVideoCodecCtx);
        avcodec_free_context(&pVideoCodecCtx);
        pVideoCodecCtx = NULL;
    }
    if(hbPlayStatus != NULL)
    {
        hbPlayStatus = NULL;
    }
}