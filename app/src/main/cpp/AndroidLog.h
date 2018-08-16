//
// Created by lake on 2018/4/14.
//

#ifndef NDKTEST_ANDROIDLOG_H
#define NDKTEST_ANDROIDLOG_H
#include <android/log.h>

#define LOG_SHOW true

#define LOGE(format, ...)  __android_log_print(ANDROID_LOG_ERROR, "(>_<)", format, ##__VA_ARGS__)
#define LOGI(format, ...)  __android_log_print(ANDROID_LOG_INFO,  "(^_^)", format, ##__VA_ARGS__)

#endif //NDKTEST_ANDROIDLOG_H
