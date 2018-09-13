package com.lake.ndktest;

/**
 * Created by lake on 2017/8/30.
 * huboa1234@126.com
 */

public interface YuvDataListener {
    void inputVideoData(byte[] data,int width,int height,int len);
}
