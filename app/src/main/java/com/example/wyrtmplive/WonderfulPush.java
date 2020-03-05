package com.example.wyrtmplive;

import android.app.Activity;
import android.view.SurfaceHolder;

/**
 * 直播推流控制类，主要用于管理音视频采集以及和底层通信
 */
public class WonderfulPush {

    static {
        System.loadLibrary("native-lib");
    }

    private AudioChannel audioChannel;
    private VideoChannel videoChannel;
    private @LiveState int state = LiveState.STOP; //直播状态

    public WonderfulPush(Activity activity){
        initNative();
        videoChannel = new VideoChannel(this,activity);
        audioChannel = new AudioChannel(this);
    }


    public void setHolder(SurfaceHolder holder){
        videoChannel.setHolder(holder);
    }

    //开始直播
    public void startLive(String rtmpPath){
        if (state == LiveState.STOP){
            state = LiveState.LIVING;
            startLiveNative(rtmpPath);
            audioChannel.startLive();
            videoChannel.startLive();
        }
    }

    //停止直播
    public void stopLive(){
        state = LiveState.STOP;
        audioChannel.stopLive();
        videoChannel.stopLive();
        stopLiveNative();
    }

    //直播状态控制
    public void liveControl(@LiveState int state){
        this.state = state;
        audioChannel.setState(state);
        videoChannel.setState(state);
        liveControlNative(state);
    }

    public @LiveState int getState() {
        return state;
    }

    public void setState(@LiveState int state) {
        this.state = state;
    }

    public void switchCamera() {
        videoChannel.switchCamera();
    }

    //（1）初始化，设置一些参数回调等
    public native  void initNative();
    //（2）初始化视频编码器
    public native void initVideoEncoderNative(int width,int height,int fps,int bitRate);
    //（2）初始化音频编码器sampleRateHz:采样率（HZ）channels:声道数
    public native void initAudioEncoderNative(int sampleRateHz,int channels);
    //（3）建立服务器连接，开启子线程，循环从队列中读取rtmp包并推流到服务器
    public native void startLiveNative(String rtmpPath);
    //（4）将摄像头数据传到底层，进行编码封装成rtmp包并放入队列
    public native void pushVideoNative(byte[] data);
    //将音频数据传到底层，进行编码封装成rtmp包并放入队列
    public native void pushAudioNative(byte[] data);
    //直播状态控制：暂停、继续
    public native void liveControlNative(@LiveState int state);
    //停止直播，断开服务器连接，跳出子线程循环
    public native void stopLiveNative();
    //获取音频编码输入样本数
    public native int getInputSamples();
}
