
#CMake版本信息
cmake_minimum_required(VERSION 3.4.1)

#ffmpeg so文件路径
set(ffmpeg_DIR ${CMAKE_SOURCE_DIR}/libs)

#配置加载native依赖
include_directories(${ffmpeg_DIR}/include)

#CPP文件夹下待编译的c文件
add_library(native-lib SHARED src/main/cpp/native-lib.cpp)

#动态方式加载ffmepg的so文件 第三方库加载方式
add_library(avcodec-57 SHARED IMPORTED)
add_library(avdevice-57 SHARED IMPORTED)
add_library(avfilter-6 SHARED IMPORTED)
add_library(avformat-57 SHARED IMPORTED)
add_library(avutil-55 SHARED IMPORTED)
add_library(postproc-54 SHARED IMPORTED)
add_library(swresample-2 SHARED IMPORTED)
add_library(swscale-4 SHARED IMPORTED)

#引入libavcodec-57.so文件
set_target_properties(avcodec-57  PROPERTIES IMPORTED_LOCATION
${ffmpeg_DIR}/${ANDROID_ABI}/libavcodec-57.so)
#引入libavdevice-57.so文件
set_target_properties(avdevice-57  PROPERTIES IMPORTED_LOCATION
${ffmpeg_DIR}/${ANDROID_ABI}/libavdevice-57.so)
#引入libavfilter-6.so文件
set_target_properties(avfilter-6  PROPERTIES IMPORTED_LOCATION
${ffmpeg_DIR}/${ANDROID_ABI}/libavfilter-6.so)
#引入libavformat-57.so文件
set_target_properties(avformat-57  PROPERTIES IMPORTED_LOCATION
${ffmpeg_DIR}/${ANDROID_ABI}/libavformat-57.so)
#引入libavutil-55.so文件
set_target_properties(avutil-55  PROPERTIES IMPORTED_LOCATION
${ffmpeg_DIR}/${ANDROID_ABI}/libavutil-55.so)
#引入libpostproc-54.so文件
set_target_properties(postproc-54  PROPERTIES IMPORTED_LOCATION
${ffmpeg_DIR}/${ANDROID_ABI}/libpostproc-54.so)
#引入libswresample-2.so文件
set_target_properties(swresample-2  PROPERTIES IMPORTED_LOCATION
${ffmpeg_DIR}/${ANDROID_ABI}/libswresample-2.so)
#引入libswscale-4.so文件
set_target_properties(swscale-4  PROPERTIES IMPORTED_LOCATION
${ffmpeg_DIR}/${ANDROID_ABI}/libswscale-4.so)

#C 日志 ndk官方库
find_library(log-lib log)

#静态库与动态库进行链接 相当于gcc命令行参数 -l。
target_link_libraries(native-lib
                      avcodec-57
                      avfilter-6
                      avformat-57
                      avutil-55
                      -landroid
                      postproc-54
                      swresample-2
                      swscale-4
                      ${log-lib})