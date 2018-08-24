#include <jni.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavcodec/jni.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/opt.h>
#include <stdio.h>
#include <time.h>


#ifdef ANDROID

#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>

#define LOGE(format, ...)  __android_log_print(ANDROID_LOG_ERROR, "(>_<)", format, ##__VA_ARGS__)
#define LOGI(format, ...)  __android_log_print(ANDROID_LOG_INFO,  "(^_^)", format, ##__VA_ARGS__)
#else
#define LOGE(format, ...)  printf("(>_<) " format "\n", ##__VA_ARGS__)
#define LOGI(format, ...)  printf("(^_^) " format "\n", ##__VA_ARGS__)
#endif




//#define TEST_RTSP
#define TEST_LOCAL_FILE
#define USE_FILTER



const char *filter_descr = "scale=78:24,transpose=cclock";
const char *filter_descr_null="null"; //将输入视频原样输出
const char *filter_mirror = "crop=iw/2:ih:0:0,split[left][tmp];[tmp]hflip[right];[left]pad=iw*2[a];[a][right]overlay=w"; //镜像输出
const char *filter_watermark = "movie=hebe.jpg[wm];[in][wm]overlay=5:5[out]"; //将一个图片作为水印添加到左上角(5,5)的位置，目前这个不能使用，不知道jpg的图片应该放在哪个文件夹下面。
const char *filter_negate = "negate[out]"; //反相输出
const char *filter_edge = "edgedetect[out]"; //边缘检测
const char *filter_split4 = "scale=iw/2:ih/2[in_tmp];[in_tmp]split=4[in_1][in_2][in_3][in_4];[in_1]pad=iw*2:ih*2[a];[a][in_2]overlay=w[b];[b][in_3]overlay=0:h[d];[d][in_4]overlay=w:h[out]"; //将一路视频分成4路显示，2*2
const char *filter_vintage = "curves=vintage"; //这个不能使用，会引起crash。
const char *filter_mix_2 = "color=c=black@1:s=1920x1080[x0];[in_1]scale=iw/2:ih/2[a];[x0][a]overlay=0:0[x1];[in_2]scale=iw/2:ih/2[b];[x1][b]overlay=w[x2];[x2]null[out]";


AVFormatContext *pFormatCtx1;
AVFormatContext *pFormatCtx2;
AVCodecContext *pCodecCtx1;
AVCodecContext *pCodecCtx2;

AVFilterContext *buffersrc_ctx1;
AVFilterContext *buffersrc_ctx2;
AVFilterContext *buffersink_ctx;
AVFilterGraph *filter_graph;
static int videoStream1 = -1;
static int videoStream2 = -1;

int videoWidth;
int videoHeight;




//Output FFmpeg's av_log()
void custom_log(void *ptr, int level, const char* fmt, va_list vl){
#if 0
    FILE *fp=fopen("/storage/emulated/0/av_log.txt","a+");
    if(fp){
        vfprintf(fp,fmt,vl);
        fflush(fp);
        fclose(fp);
    }
#endif
    switch(level) {
        case AV_LOG_DEBUG:
            __android_log_print(ANDROID_LOG_DEBUG, "(>_<)", fmt, vl);
            break;
        case AV_LOG_VERBOSE:
            __android_log_print(ANDROID_LOG_VERBOSE, "(>_<)", fmt, vl);
            break;
        case AV_LOG_INFO:
            __android_log_print(ANDROID_LOG_INFO, "(>_<)", fmt, vl);
            break;
        case AV_LOG_ERROR:
            __android_log_print(ANDROID_LOG_ERROR, "(>_<)", fmt, vl);
            break;
    }
}


static int init_filters(const char *filter_descr)
{
    int ret = 0;
    char args1[512];
    char args2[512]; //好像args这个参数可以共用，暂时先分开使用。
    char err_buf[128];

    LOGI("++++++++ in init_filters function. \n");

    AVFilter *buffersrc1 = avfilter_get_by_name("buffer"); //if stream is audio, the name will be "abuffer"
    AVFilter *buffersrc2 = avfilter_get_by_name("buffer"); //if stream is audio, the name will be "abuffer"
    AVFilter *buffersink = avfilter_get_by_name("buffersink"); //if stream is audio, the name will be "abuffersink"
    AVFilterInOut *inputs = avfilter_inout_alloc();
    AVFilterInOut **outputs = (AVFilterInOut **)av_malloc(2*sizeof(AVFilterInOut*)); //如果增加多路信号的话，需要修改这里，默认是2路信号
    outputs[0] = avfilter_inout_alloc();
    outputs[1] = avfilter_inout_alloc();
    AVRational time_base1 = pFormatCtx1->streams[videoStream1]->time_base;
    AVRational time_base2 = pFormatCtx2->streams[videoStream2]->time_base;

    filter_graph = avfilter_graph_alloc();
    if(!inputs || !outputs || !filter_graph){
        ret = AVERROR(ENOMEM);
        goto end;
    }
    LOGI("++++++++ in init_filters function.  - 1 - \n");
    /* buffer video source: the decoded frames from the decoder will be inserted here. */
    LOGI("Stream1 video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
         pCodecCtx1->width, pCodecCtx1->height, pCodecCtx1->pix_fmt,
         time_base1.num, time_base1.den,
         pCodecCtx1->sample_aspect_ratio.num, pCodecCtx1->sample_aspect_ratio.den);
    LOGI("Stream2 video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
         pCodecCtx2->width, pCodecCtx2->height, pCodecCtx2->pix_fmt,
         time_base2.num, time_base2.den,
         pCodecCtx2->sample_aspect_ratio.num, pCodecCtx2->sample_aspect_ratio.den);

    snprintf(args1, sizeof(args1),
             "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
             pCodecCtx1->width, pCodecCtx1->height, pCodecCtx1->pix_fmt,
             time_base1.num, time_base1.den,
             pCodecCtx1->sample_aspect_ratio.num, pCodecCtx1->sample_aspect_ratio.den);
    snprintf(args2, sizeof(args2),
             "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
             pCodecCtx2->width, pCodecCtx2->height, pCodecCtx2->pix_fmt,
             time_base2.num, time_base2.den,
             pCodecCtx2->sample_aspect_ratio.num, pCodecCtx2->sample_aspect_ratio.den);
    LOGI("++++++++ in init_filters function.  - 2 - \n");

    ret = avfilter_graph_create_filter(&buffersrc_ctx1, buffersrc1, "in_1", args1, NULL, filter_graph);
    if(ret < 0){
        av_strerror(ret, err_buf, 1024);
        LOGE("Couldn't create buffer source 1, error code: %d (%s). \n", ret, err_buf);
        goto end;
    }
    ret = avfilter_graph_create_filter(&buffersrc_ctx2, buffersrc2, "in_2", args2, NULL, filter_graph);
    if(ret < 0){
        av_strerror(ret, err_buf, 1024);
        LOGE("Couldn't create buffer source 2, error code: %d (%s). \n", ret, err_buf);
        goto end;
    }
    LOGI("++++++++ in init_filters function.  - 3 - \n");

    /* buffer video sink: to terminate the filter chain. */
    ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out", NULL, NULL, filter_graph);
    if(ret < 0){
        LOGI("Cannot create buffer sink. \n");
        goto end;
    }
    LOGI("++++++++ in init_filters function.  - 4 - \n");
    /*  Quite not understand........ */
    /* Endpoints for the filter graph. */
    outputs[0]->name = av_strdup("in_1");
    outputs[0]->filter_ctx = buffersrc_ctx1;
    outputs[0]->pad_idx = 0;
    outputs[0]->next = outputs[1];

    outputs[1]->name = av_strdup("in_2");
    outputs[1]->filter_ctx = buffersrc_ctx2;
    outputs[1]->pad_idx = 0;
    outputs[1]->next = NULL;

    inputs->name = av_strdup("out");
    inputs->filter_ctx = buffersink_ctx;
    inputs->pad_idx = 0;
    inputs->next = NULL;
    LOGI("++++++++ in init_filters function.  - 5 - \n");

    if((ret = avfilter_graph_parse_ptr(filter_graph, filter_descr, &inputs, outputs, NULL)) < 0)
        goto end;
    if((ret = avfilter_graph_config(filter_graph, NULL)) < 0)
        goto end;
    LOGI("++++++++ in init_filters function.  - 6 - \n");

    end:
    avfilter_inout_free(&inputs);
    avfilter_inout_free(outputs);
    av_free(outputs);

    return ret;
}


int display_frame(AVFormatContext *pFormatCtx, AVCodecContext *pCodecCtx, int video_index,
                  AVFilterContext *buffersrc_ctx, AVFilterContext *buffersink_ctx,
                  ANativeWindow *nativeWindow, ANativeWindow_Buffer windowBuffer, struct SwsContext *sws_ctx,
                  AVFrame *pFrameRGBA)
{
    int ret = 0;
    AVPacket packet;
    AVFrame *pFrame = av_frame_alloc();
    AVFrame *filt_frame = av_frame_alloc();
    LOGI("++++++++ in display_frame function.  * 1 * \n");

    while (av_read_frame(pFormatCtx, &packet) >= 0) {
        // Is this a packet from the video stream?
        if ((packet.stream_index == video_index)) {

            // 解码
            ret = avcodec_send_packet(pCodecCtx, &packet);
            if (ret < 0) {
                LOGE("Error to send packet.\n");
                break;
            }
            LOGI("++++++++ in display_frame function.  * 2 * \n");

            while (avcodec_receive_frame(pCodecCtx, pFrame) == 0) {//绘图

#ifdef USE_FILTER
                /* Push the decoded frame into the filtergraph. */
                if(av_buffersrc_add_frame_flags(buffersrc_ctx, pFrame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0){
                    LOGE("Error while feeding the filtergraph. \n");
                    break;
                }
                LOGI("++++++++ in display_frame function.  * 3 * \n");

                /* Pull filtered frame from the filtergraph */
                while (1){
                    ret = av_buffersink_get_frame(buffersink_ctx, filt_frame);
                    if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                        break;
                    if(ret < 0)
                        goto end;
#endif
                    // lock native window buffer
                    ANativeWindow_lock(nativeWindow, &windowBuffer, 0);


#ifdef USE_FILTER
                    // 格式转换
                    sws_scale(sws_ctx, (uint8_t const *const *) filt_frame->data,
                              filt_frame->linesize, 0, pCodecCtx1->height,
                              pFrameRGBA->data, pFrameRGBA->linesize);
#endif


                    /*********************************************************************/
                    /************* If test RTSP or LOCAL_FILE, need to change here. ******/
                    /************* Change filt_frame to pFrame. **************************/
                    /*********************************************************************/
#if 0
                    // 格式转换
                    sws_scale(sws_ctx, (uint8_t const *const *) pFrame->data,
                              pFrame->linesize, 0, pCodecCtx->height,
                              pFrameRGBA->data, pFrameRGBA->linesize);
#endif


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
                    LOGI("++++++++ in display_frame function.  * 4 * \n");
                    ANativeWindow_unlockAndPost(nativeWindow);
#ifdef USE_FILTER
                    av_frame_unref(filt_frame);
                }
#endif
            }
        }
        av_packet_unref(&packet);
    }

end:
    av_free(pFrame);

    return ret;
}


JNIEXPORT jint JNICALL Java_com_lake_ndktest_FFmpeg_play
        (JNIEnv *env, jobject obj, jstring input_jstr, jobject surface) {
    LOGI("play");

    //FFmpeg av_log() callback
    //av_log_set_callback(custom_log);

    av_register_all();
    avformat_network_init();
    avfilter_register_all();

    /**************************** Below is Open input **********************************/

#ifdef TEST_RTSP
    AVDictionary *option = NULL;
    av_dict_set(&option, "buffer_size", "1024000", 0);
    av_dict_set(&option, "max_delay", "300000", 0);
    av_dict_set(&option, "stimeout", "20000000", 0);  //设置超时断开连接时间
    av_dict_set(&option, "rtsp_transport", "tcp", 0);

    pFormatCtx = avformat_alloc_context();


    // Open RTSP
    const char *rtspUrl= env->GetStringUTFChars(input_jstr, JNI_FALSE);
    if (int err = avformat_open_input(&pFormatCtx, rtspUrl, NULL, &option) != 0) {
        LOGE("Cannot open input %s, error code: %d", rtspUrl, err);
        return JNI_ERR;
    }
    env->ReleaseStringUTFChars(input_jstr, rtspUrl);

    av_dict_free(&option);

#endif


#ifdef TEST_LOCAL_FILE
    const char *fileUrl = env->GetStringUTFChars(input_jstr, JNI_FALSE);
    if (int err = avformat_open_input(&pFormatCtx1, "/storage/emulated/0/test1.mp4", NULL, NULL) != 0) {
        LOGE("Cannot open input %s, error code: %d", "/storage/emulated/0/test1.mp4", err);
        return JNI_ERR;
    }

    if (int err = avformat_open_input(&pFormatCtx2, "/storage/emulated/0/test2.mp4", NULL, NULL) != 0) {
        LOGE("Cannot open input %s, error code: %d", "/storage/emulated/0/test2.mp4", err);
        return JNI_ERR;
    }

#endif



    clock_t start, stop;
    double duration;

    start = clock();
    // Retrieve stream information
    if (avformat_find_stream_info(pFormatCtx1, NULL) < 0) {
        LOGE("Couldn't find stream1 information.");
        return -1;
    }

    if (avformat_find_stream_info(pFormatCtx2, NULL) < 0) {
        LOGE("Couldn't find stream2 information.");
        return -1;
    }

    stop = clock();
    duration = ((double)(stop - start))/CLOCKS_PER_SEC;

    LOGI(" +++++ avformat_find_stream_info function runtime is : %lf s. +++++ \n", duration);

    // Find the first video stream
    //找到第一个视频流，因为里面的流还有可能是音频流或者其他的，我们摄像头只关心视频流
    int i;
    for (i = 0; i < pFormatCtx1->nb_streams; i++) {
        if (pFormatCtx1->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO
            && videoStream1 < 0) {
            videoStream1 = i;
            break;
        }
    }
    if (videoStream1 == -1) {
        LOGE("Didn't find a video stream or audio in stream1.");
        return -1; // Didn't find a video stream
    }

    for (i = 0; i < pFormatCtx2->nb_streams; i++) {
        if (pFormatCtx2->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO
            && videoStream2 < 0) {
            videoStream2 = i;
            break;
        }
    }
    if (videoStream2 == -1) {
        LOGE("Didn't find a video stream or audio in stream2.");
        return -1; // Didn't find a video stream
    }



    LOGI("找到视频流");
    AVCodecParameters *pCodecPar1 = pFormatCtx1->streams[videoStream1]->codecpar;
    AVCodecParameters *pCodecPar2 = pFormatCtx2->streams[videoStream2]->codecpar;

    //查找解码器
    //获取一个合适的编码器pCodec find a decoder for the video stream
    //AVCodec *pCodec = avcodec_find_decoder(pCodecPar->codec_id);
    AVCodec *pCodec1;
    switch (pCodecPar1->codec_id){
        case AV_CODEC_ID_H264:
            pCodec1 = avcodec_find_decoder_by_name("h264_mediacodec");//硬解码264
            if (pCodec1 == NULL) {
                LOGE("Couldn't find Codec1.\n");
                return -1;
            }
            break;
        case AV_CODEC_ID_MPEG4:
            pCodec1 = avcodec_find_decoder_by_name("mpeg4_mediacodec");//硬解码mpeg4
            if (pCodec1 == NULL) {
                LOGE("Couldn't find Codec1.\n");
                return -1;
            }
            break;
        case AV_CODEC_ID_HEVC:
            pCodec1 = avcodec_find_decoder_by_name("hevc_mediacodec");//硬解码265
            if (pCodec1 == NULL) {
                LOGE("Couldn't find Codec1.\n");
                return -1;
            }
            break;
        default:
            pCodec1 = avcodec_find_decoder(pCodecPar1->codec_id);
            break;
    }

    LOGI("获取解码器 %d.", pCodecPar1->codec_id);


    AVCodec *pCodec2;
    switch (pCodecPar2->codec_id){
        case AV_CODEC_ID_H264:
            pCodec2 = avcodec_find_decoder_by_name("h264_mediacodec");//硬解码264
            if (pCodec2 == NULL) {
                LOGE("Couldn't find Codec2.\n");
                return -1;
            }
            break;
        case AV_CODEC_ID_MPEG4:
            pCodec2 = avcodec_find_decoder_by_name("mpeg4_mediacodec");//硬解码mpeg4
            if (pCodec2 == NULL) {
                LOGE("Couldn't find Codec2.\n");
                return -1;
            }
            break;
        case AV_CODEC_ID_HEVC:
            pCodec2 = avcodec_find_decoder_by_name("hevc_mediacodec");//硬解码265
            if (pCodec2 == NULL) {
                LOGE("Couldn't find Codec2.\n");
                return -1;
            }
            break;
        default:
            pCodec2 = avcodec_find_decoder(pCodecPar2->codec_id);
            break;
    }

    LOGI("获取解码器 %d.", pCodecPar2->codec_id);




    //打开这个编码器，pCodecCtx表示编码器上下文，里面有流数据的信息
    // Get a pointer to the codec context for the video stream
    pCodecCtx1 = avcodec_alloc_context3(pCodec1);
    pCodecCtx2 = avcodec_alloc_context3(pCodec2);

    // Copy context
    if (avcodec_parameters_to_context(pCodecCtx1, pCodecPar1) != 0) {
        fprintf(stderr, "Couldn't copy codec context1");
        return -1; // Error copying codec context
    }

    // Copy context
    if (avcodec_parameters_to_context(pCodecCtx2, pCodecPar2) != 0) {
        fprintf(stderr, "Couldn't copy codec context2");
        return -1; // Error copying codec context
    }

    LOGI("视频流1帧率：%d fps\n", pFormatCtx1->streams[videoStream1]->r_frame_rate.num /
                           pFormatCtx1->streams[videoStream1]->r_frame_rate.den);
    LOGI("视频流2帧率：%d fps\n", pFormatCtx2->streams[videoStream2]->r_frame_rate.num /
                           pFormatCtx2->streams[videoStream2]->r_frame_rate.den);

#if 0
    int iTotalSeconds = (int) pFormatCtx->duration / 1000000;
    int iHour = iTotalSeconds / 3600;//小时
    int iMinute = iTotalSeconds % 3600 / 60;//分钟
    int iSecond = iTotalSeconds % 60;//秒
    LOGI("持续时间：%02d:%02d:%02d\n", iHour, iMinute, iSecond);

    LOGI("视频时长：%lld微秒\n", pFormatCtx->streams[videoStream]->duration);
    LOGI("持续时间：%lld微秒\n", pFormatCtx->duration);
#endif

    LOGI("获取解码器SUCESS");

    if (avcodec_open2(pCodecCtx1, pCodec1, NULL) < 0) {
        LOGE("Could not open codec1.");
        return -1; // Could not open codec
    }

    if (avcodec_open2(pCodecCtx2, pCodec2, NULL) < 0) {
        LOGE("Could not open codec1.");
        return -1; // Could not open codec
    }



    /**************************** Below is set ANativeWindow **********************************/

    LOGI("获取native window");
    // 获取native window
    ANativeWindow *nativeWindow = ANativeWindow_fromSurface(env, surface);
    LOGI("获取视频宽高");
    // 获取视频宽高
    videoWidth = pCodecCtx1->width;
    videoHeight = pCodecCtx1->height;
    LOGI("设置native window的buffer大小,可自动拉伸");
    // 设置native window的buffer大小,可自动拉伸
    ANativeWindow_setBuffersGeometry(nativeWindow, videoWidth, videoHeight,
                                     WINDOW_FORMAT_RGBA_8888);
    ANativeWindow_Buffer windowBuffer;

    LOGI("Allocate video frame");
    // Allocate video frame
    AVFrame *pFrame1 = av_frame_alloc();
    AVFrame *pFrame2 = av_frame_alloc();
    AVFrame *filt_frame = av_frame_alloc();
    LOGI("用于渲染");
    // 用于渲染
    AVFrame *pFrameRGBA = av_frame_alloc();
    if (pFrameRGBA == NULL || pFrame1 == NULL || pFrame2 == NULL || filt_frame == NULL) {
        LOGE("Could not allocate video frame.");
        return -1;
    }
    LOGI("Determine required buffer size and allocate buffer");
    // Determine required buffer size and allocate buffer
    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA, pCodecCtx1->width, pCodecCtx1->height,
                                            1);
    uint8_t *buffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));
    av_image_fill_arrays(pFrameRGBA->data, pFrameRGBA->linesize, buffer, AV_PIX_FMT_RGBA,
                         pCodecCtx1->width, pCodecCtx1->height, 1);
    LOGI("由于解码出来的帧格式不是RGBA的,在渲染之前需要进行格式转换");
    // 由于解码出来的帧格式不是RGBA的,在渲染之前需要进行格式转换
    struct SwsContext *sws_ctx = sws_getContext(pCodecCtx1->width/*视频宽度*/, pCodecCtx1->height/*视频高度*/,
                                                pCodecCtx1->pix_fmt/*像素格式*/,
                                                pCodecCtx1->width/*目标宽度*/,
                                                pCodecCtx1->height/*目标高度*/, AV_PIX_FMT_RGBA/*目标格式*/,
                                                SWS_BICUBIC/*图像转换的一些算法*/, NULL, NULL, NULL);
    if (sws_ctx == NULL) {
        LOGE("Cannot initialize the conversion context!\n");
        return -1;
    }
    LOGI("格式转换成功");
    LOGE("开始播放");

    /**************************** Below is Set Filter **********************************/

    int ret;

#ifdef USE_FILTER
    ret = init_filters(filter_mix_2);
    if(ret < 0){
        goto end;
    }
#endif


    /**************************** Below is Display **********************************/
    display_frame(pFormatCtx2, pCodecCtx2, videoStream2, buffersrc_ctx2, buffersink_ctx,
                  nativeWindow, windowBuffer, sws_ctx, pFrameRGBA);

    display_frame(pFormatCtx1, pCodecCtx1, videoStream1, buffersrc_ctx1, buffersink_ctx,
            nativeWindow, windowBuffer, sws_ctx, pFrameRGBA);


    LOGE("播放完成");


#ifdef USE_FILTER
end:
    avfilter_graph_free(&filter_graph);
#endif

    av_free(buffer);
    av_free(pFrameRGBA);

    // Close the codecs
    avcodec_free_context(&pCodecCtx1);
    avcodec_free_context(&pCodecCtx2);

    // Close the video file
    avformat_close_input(&pFormatCtx1);
    avformat_close_input(&pFormatCtx2);

    return 0;
}
}

jint JNI_OnLoad(JavaVM* vm, void* reserved)//这个类似android的生命周期，加载jni的时候会自己调用
{
    LOGI("ffmpeg JNI_OnLoad");
    av_jni_set_java_vm(vm, reserved);
    return JNI_VERSION_1_6;
}


