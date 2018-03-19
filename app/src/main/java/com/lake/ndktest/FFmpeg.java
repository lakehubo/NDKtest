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
        System.loadLibrary("native-lib");
    }

}
