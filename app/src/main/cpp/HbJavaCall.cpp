//
// Created by lake on 2018/4/10.
//

#include <stdint.h>
#include "HbJavaCall.h"

HbJavaCall::HbJavaCall(JavaVM *vm, JNIEnv *env, jobject *obj) {
    javaVM = vm;
    jniEnv = env;
    jobj = *obj;
    jobj = env->NewGlobalRef(jobj);
    jclass jclazz = jniEnv->GetObjectClass(jobj);
    if (!jclazz) {
        return;
    }
    jmid_timeInfo = jniEnv->GetMethodID(jclazz, "setProgressInfo", "(I)V");
    jmid_totalTime = jniEnv->GetMethodID(jclazz, "setTotalTime", "(I)V");
    jmid_channelInfo = jniEnv->GetMethodID(jclazz, "initAudioChannel", "([I[Ljava/lang/String;I)V");

}

HbJavaCall::~HbJavaCall() {

}

void HbJavaCall::release() {
    if (javaVM != NULL) {
        javaVM = NULL;
    }
    if (jniEnv != NULL) {
        jniEnv = NULL;
    }
}

void HbJavaCall::onProgressInfo(int type, int64_t currt_secd) {
    if (type == HB_THREAD_CHILD) {
        JNIEnv *jniEnv;
        if (javaVM->AttachCurrentThread(&jniEnv, 0) != JNI_OK) {
            return;
        }
        jniEnv->CallVoidMethod(jobj, jmid_timeInfo, currt_secd);
        javaVM->DetachCurrentThread();
    } else {
        jniEnv->CallVoidMethod(jobj, jmid_timeInfo, currt_secd);
    }
}

void HbJavaCall::onTotalTime(int type, int64_t total_secd) {
    if (type == HB_THREAD_CHILD) {
        JNIEnv *jniEnv;
        if (javaVM->AttachCurrentThread(&jniEnv, 0) != JNI_OK) {
            return;
        }
        jniEnv->CallVoidMethod(jobj, jmid_totalTime, total_secd);
        javaVM->DetachCurrentThread();
    } else {
        jniEnv->CallVoidMethod(jobj, jmid_totalTime, total_secd);
    }
}

void
HbJavaCall::onChannelInfo(int type, int64_t *channel, const char *language[], int size, int cur) {
    if (type == HB_THREAD_CHILD) {
        JNIEnv *jniEnv;
        jstring str;
        jint index;
        if (javaVM->AttachCurrentThread(&jniEnv, 0) != JNI_OK) {
            return;
        }
        jintArray channelArray = jniEnv->NewIntArray(size);
        jniEnv->SetIntArrayRegion(channelArray, 0, size, (jint *) channel);
        jobjectArray languageArray = jniEnv->NewObjectArray(size,
                                                            jniEnv->FindClass("java/lang/String"),
                                                            0);
        for (int i = 0; i < size; i++) {
            str = jniEnv->NewStringUTF(language[size - i - 1]);
            jniEnv->SetObjectArrayElement(languageArray, i, str);
        }
        jniEnv->CallVoidMethod(jobj, jmid_channelInfo, channelArray, languageArray, cur);
        jniEnv->DeleteLocalRef(channelArray);
        jniEnv->DeleteLocalRef(languageArray);
        jniEnv->DeleteLocalRef(str);
        javaVM->DetachCurrentThread();
    } else {
        jstring str;
        jintArray channelArray = jniEnv->NewIntArray(size);
        jniEnv->SetIntArrayRegion(channelArray, 0, size, (jint *) channel);
        jobjectArray languageArray = jniEnv->NewObjectArray(size,
                                                            jniEnv->FindClass("java/lang/String"),
                                                            0);
        for (int i = 0; i < size; i++) {
            str = jniEnv->NewStringUTF(language[size - i - 1]);
            jniEnv->SetObjectArrayElement(languageArray, i, str);
        }
        jniEnv->CallVoidMethod(jobj, jmid_channelInfo, channelArray, languageArray, cur);
        jniEnv->DeleteLocalRef(channelArray);
        jniEnv->DeleteLocalRef(languageArray);
        jniEnv->DeleteLocalRef(str);
    }
}