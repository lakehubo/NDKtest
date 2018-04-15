//
// Created by lake on 2018/4/7.
//

#ifndef NDKTEST_HBPLAYSTATUS_H
#define NDKTEST_HBPLAYSTATUS_H


class HbPlayStatus {

public:
    bool stop;
    bool pause;
    bool seek;
    bool load;

public:
    HbPlayStatus();
    ~HbPlayStatus();
};


#endif //NDKTEST_HBPLAYSTATUS_H
