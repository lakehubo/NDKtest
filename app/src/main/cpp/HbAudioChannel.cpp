//
// Created by lake on 2018/4/13.
//

#include "HbAudioChannel.h"

HbAudioChannel::HbAudioChannel(int c, int i, char *l, AVRational t) {
    channel = c;
    audioStream = i;
    language = l;
    time_base = t;
}

HbAudioChannel::HbAudioChannel(int c, int i, char *l, AVRational t, int f) {
    channel = c;
    audioStream = i;
    language = l;
    time_base = t;
    fps = f;
}

HbAudioChannel::~HbAudioChannel() {

}