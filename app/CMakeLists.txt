
#CMake版本信息
cmake_minimum_required(VERSION 3.4.1)

#ffmpeg so文件路径
set(lib_src_DIR ${CMAKE_SOURCE_DIR}/src/main/jniLibs/${ANDROID_ABI})

#配置加载native依赖
include_directories(${CMAKE_SOURCE_DIR}/src/main/cpp/include)

#动态方式加载ffmepg的so文件 第三方库加载方式
add_library(ffmpeg SHARED IMPORTED)

#引入libffmpeg.so文件
set_target_properties(ffmpeg PROPERTIES IMPORTED_LOCATION
${lib_src_DIR}/libffmpeg.so)

#CPP文件夹下待编译的c文件
add_library(HbPlayer SHARED
                       ${CMAKE_SOURCE_DIR}/src/main/cpp/HbPlayer.cpp
                       ${CMAKE_SOURCE_DIR}/src/main/cpp/HbQueue.cpp
                       ${CMAKE_SOURCE_DIR}/src/main/cpp/HbAudio.cpp
                       ${CMAKE_SOURCE_DIR}/src/main/cpp/HbVideo.cpp
                       ${CMAKE_SOURCE_DIR}/src/main/cpp/HbFFmpeg.cpp
                       ${CMAKE_SOURCE_DIR}/src/main/cpp/HbAudioChannel.cpp
                       ${CMAKE_SOURCE_DIR}/src/main/cpp/HbJavaCall.cpp
                       ${CMAKE_SOURCE_DIR}/src/main/cpp/HbPlayStatus.cpp)
#C 日志 ndk官方库
find_library(log-lib log)

#静态库与动态库进行链接 相当于gcc命令行参数 -l。
target_link_libraries(HbPlayer
                      android
                      ffmpeg
                      OpenSLES
                      ${log-lib})