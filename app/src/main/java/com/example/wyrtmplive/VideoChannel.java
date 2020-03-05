package com.example.wyrtmplive;

import android.app.Activity;
import android.hardware.Camera;
import android.view.SurfaceHolder;

public class VideoChannel implements CameraHelper.PreviewCallback,CameraHelper.SizeChangedListener{

    private static final int cameraId = Camera.CameraInfo.CAMERA_FACING_FRONT;
    private static final int width = 800;
    private static final int height = 480;
    private static final int fps = 25;
    private static final int bitRate = 800*1000;   //比特率
    private @LiveState int state = LiveState.STOP; //直播状态

    private WonderfulPush wonderfulPush;
    private CameraHelper cameraHelper;

    public VideoChannel(WonderfulPush wonderfulPush,Activity activity){
        this.wonderfulPush = wonderfulPush;
        cameraHelper = new CameraHelper(activity,cameraId,width,height);
        cameraHelper.setPreviewCallback(this);
        cameraHelper.setSizeChangedListener(this);
    }

    public void startLive(){
        state = LiveState.LIVING;
    }

    public void stopLive(){
        state = LiveState.STOP;
        cameraHelper.stopPreview();
    }


    public void switchCamera(){
        cameraHelper.switchCamera();
    }

    public void setHolder(SurfaceHolder holder){
        cameraHelper.setHolder(holder);
    }


    @Override
    public void onChanged(int width, int height) {
        //初始化视频编码器
        wonderfulPush.initVideoEncoderNative(width,height,fps,bitRate);
    }

    @Override
    public void onPreviewFrame(byte[] data) {
        //开始直播推流
        if (state == LiveState.LIVING){
            wonderfulPush.pushVideoNative(data);
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
