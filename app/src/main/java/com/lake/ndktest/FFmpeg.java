package com.lake.ndktest;

/**
 * Created by lake on 2017/8/23.
 * huboa1234@126.com
 */

public class FFmpeg {
    //JNI
    //public native int decode(String inputurl, String outputurl);

    public static native int play(String inputurl,Object surface);

    static{
        System.loadLibrary("avutil-55");
        System.loadLibrary("swresample-2");
        System.loadLibrary("avcodec-57");
        System.loadLibrary("avdevice-57");
        System.loadLibrary("avformat-57");
        System.loadLibrary("postproc-54");
        System.loadLibrary("swscale-4");
        System.loadLibrary("avfilter-6");
        System.loadLibrary("native-lib");
    }

}
