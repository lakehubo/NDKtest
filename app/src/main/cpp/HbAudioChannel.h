//
// Created by lake on 2018/4/13.
//

#ifndef NDKTEST_HBAUDIOCHANNEL_H
#define NDKTEST_HBAUDIOCHANNEL_H

#include <libavutil/rational.h>

class HbAudioChannel {
public:
    int channel;//通道号
    int audioStream;//音频流id
    char *language;//语言
    AVRational time_base;
    int fps;

public:
    HbAudioChannel(int channel,int audioStream,char *language,AVRational time_base);
    HbAudioChannel(int channel,int audioStream,char *language,AVRational time_base,int fps);
    ~HbAudioChannel();

};


#endif //NDKTEST_HBAUDIOCHANNEL_H
