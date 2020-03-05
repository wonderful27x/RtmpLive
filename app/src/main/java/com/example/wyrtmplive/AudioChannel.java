package com.example.wyrtmplive;

import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class AudioChannel {

    private WonderfulPush wonderfulPush;
    private int inputSamples;                      //编码器输入样本数（单位：字节）
    private AudioRecord audioRecord;               //音频采集器
    private @LiveState int state = LiveState.STOP; //直播状态
    private ExecutorService executorService;       //单线程池

    public AudioChannel(WonderfulPush wonderfulPush){
        this.wonderfulPush = wonderfulPush;
        int channelConfig;
        executorService = Executors.newSingleThreadExecutor();
        if (AVConfigure.AUDIO_CHANNELS == 1){
            channelConfig = AudioFormat.CHANNEL_IN_MONO;
        }else{
            channelConfig = AudioFormat.CHANNEL_IN_STEREO;
        }
        //先初始化音频编码器，因为只有编码器初始化后才能获取输入样本数
        wonderfulPush.initAudioEncoderNative(AVConfigure.SAMPLE_RATE,AVConfigure.AUDIO_CHANNELS);
        //获取编码器输入样本数，乘2是因为采用16位编码 16bit = 2byte
        //inputSamples = wonderfulPush.getInputSamples() * 2;
        inputSamples = wonderfulPush.getInputSamples() * AVConfigure.AUDIO_FORMAT;
        //根据采样率、通道数和编码格式获取最小缓存区大小,乘2老师说是固定写法，让缓存区大一些（单位：字节）
        int minBuffSize = AudioRecord.getMinBufferSize(AVConfigure.SAMPLE_RATE,channelConfig,AVConfigure.AUDIO_FORMAT) * 2;
        //初始化audioRecord,
        audioRecord = new AudioRecord(
                MediaRecorder.AudioSource.MIC,
                AVConfigure.SAMPLE_RATE,
                channelConfig,
                AVConfigure.AUDIO_FORMAT,
                Math.max(inputSamples,minBuffSize)
                );
    }

    public void startLive() {
        state = LiveState.LIVING;
        executorService.submit(new AudioTask());
    }

    public void stopLive() {
        state = LiveState.STOP;
        if (audioRecord  != null){
            //TODO 线程池如何join???
            audioRecord.release();
            audioRecord = null;
        }
    }

    private class AudioTask implements Runnable{

        @Override
        public void run() {
            audioRecord.startRecording();        //开始录音
            byte[] data = new byte[inputSamples];//输出缓存区,大小要和编码器给出的大小一致
            int result = 0;
            while (state != LiveState.STOP){
                if (state == LiveState.PAUSE)continue;
                result = audioRecord.read(data,0,data.length);
                if (result >0){
                    wonderfulPush.pushAudioNative(data);
                }
            }
            audioRecord.stop();
        }
    }

    public @LiveState int getState() {
        return state;
    }

    public void setState(@LiveState int state) {
        this.state = state;
    }

    //直播状态控制
    public void liveControl(@LiveState int state){
        this.state = state;
    }
}
