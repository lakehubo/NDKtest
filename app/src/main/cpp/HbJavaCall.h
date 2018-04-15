//
// Created by lake on 2018/4/10.
//

#ifndef NDKTEST_HBJAVACALL_H
#define NDKTEST_HBJAVACALL_H


#include <jni.h>
#include <stddef.h>

#define HB_THREAD_MAIN 1
#define HB_THREAD_CHILD 2


class HbJavaCall {

public:
    _JavaVM *javaVM = NULL;
    JNIEnv *jniEnv = NULL;
    jmethodID jmid_timeInfo;
    jmethodID jmid_totalTime;
    jmethodID jmid_channelInfo;
    jobject jobj;

public:
    HbJavaCall(_JavaVM *vm, JNIEnv *env, jobject *obj);

    ~HbJavaCall();

    void onProgressInfo(int type, int64_t currt_secd);

    void onTotalTime(int type, int64_t total_secd);

    void onChannelInfo(int type, int64_t *channel, const char *language[],int size,int cur);

    void release();
};


#endif //NDKTEST_HBJAVACALL_H
