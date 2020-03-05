package com.example.wyrtmplive;

import android.media.AudioFormat;

public class AVConfigure {
    //音频声道数
    public static final int AUDIO_CHANNELS = 2;
    //音频采样率，单位：Hz
    public static final int SAMPLE_RATE = 44100;
    //音频编码格式,这个参数不要修改，因为和底层没对应好，只使用16位编码！！！
    public static final int AUDIO_FORMAT = AudioFormat.ENCODING_PCM_16BIT;
}
