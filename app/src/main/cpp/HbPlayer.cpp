#include <jni.h>
#include "HbFFmpeg.h"
extern "C"
{
#include <libavcodec/jni.h>
};
#ifdef ANDROID
#include <android/native_window_jni.h>

#endif

_JavaVM *curVm;
HbJavaCall *hbJavaCall = NULL;
pthread_cond_t cond; //条件对象
const char *file_name = NULL;//文件名 路径
//渲染窗口
ANativeWindow *nativeWindow;
HbFFmpeg *hbFFmpeg =NULL;

extern "C"
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved)//这个类似android的生命周期，加载jni的时候会自己调用
{
    LOGI("ffmpeg JNI_OnLoad");
    JNIEnv *env;
    curVm = vm;
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        return -1;
    }
    av_jni_set_java_vm(vm, reserved);// 支持硬解
    return JNI_VERSION_1_6;
}

extern "C"
JNIEXPORT jint JNICALL Java_com_lake_ndktest_VideoActivity_play
        (JNIEnv *env, jobject obj, jstring input_jstr, jobject surface) {
    LOGI("play");
    if (hbJavaCall == NULL) {
        hbJavaCall = new HbJavaCall(curVm, env, &obj);
    }
    file_name = env->GetStringUTFChars(input_jstr, NULL);
    LOGI("file_name:%s\n", file_name);
    hbFFmpeg = new HbFFmpeg(hbJavaCall,file_name);
    // 获取native window
    nativeWindow = ANativeWindow_fromSurface(env, surface);
    hbFFmpeg->nativeWindow = nativeWindow;
    //初始化
    hbFFmpeg->initFFmpeg();
    hbFFmpeg->startDecode();
    hbFFmpeg->playVideo();
    return 0;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_lake_ndktest_VideoActivity_seek(JNIEnv *env, jobject thiz, jint seekTime) {
    if(hbFFmpeg!=NULL){
        hbFFmpeg->seek((int64_t)seekTime);
    }
    return 0;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_lake_ndktest_VideoActivity_pause(JNIEnv *env, jobject thiz, jboolean pause) {
    if(hbFFmpeg!=NULL){
        pause?hbFFmpeg->pause():hbFFmpeg->resume();
    }
    return 0;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_lake_ndktest_VideoActivity_stop(JNIEnv *env, jobject thiz) {
    if(hbFFmpeg!=NULL){
        hbFFmpeg->release();
        delete(hbFFmpeg);
        hbFFmpeg = NULL;
        if(hbJavaCall != NULL)
        {
            hbJavaCall->release();
            hbJavaCall = NULL;
        }
    }
    return 0;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_lake_ndktest_VideoActivity_switchAudioChannel(JNIEnv *env, jobject thiz, jint channel) {
    if(hbFFmpeg!=NULL){
        hbFFmpeg->switchAudioChannel(channel);
    }
    return 0;
}
