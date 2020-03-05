//
// Created by Acer on 2020/2/13.
//

#ifndef WYRTMPLIVE_VIDEOCHANNEL_H
#define WYRTMPLIVE_VIDEOCHANNEL_H

#include <rtmp.h>
#include <sys/types.h>
#include <zconf.h>
#include <pthread.h>
#include <x264.h>
#include <cstring>
#include "androidlog.h"

/**
 * 视频通道,负责将yuv编码成h264并封装成rtmp
 */
class VideoChannel {

typedef void (*VideoCallback)(RTMPPacket *packet);

public:
    VideoChannel();
    ~VideoChannel();

    void setVideoCallback(VideoCallback videoCallback);
    void initVideoEncoder(int width,int height,int fps,int bitRate);
    void encode(int8_t *data);
    void senSpsPps(uint8_t *sps,uint8_t *pps,int sps_len,int pps_len);
    void sedFrame(int type,uint8_t *nal,int len);

private:
    VideoCallback videoCallback;       //将封装好的rtmp包回调出去
    pthread_mutex_t encoderMutex;      //编码器互斥锁
    int width;                         //宽
    int height;                        //高
    int fps;                           //帧率
    int bitRate;                       //比特率
    int y_len;                         //yuv y分量size大小
    int uv_len;                        //yuv u、v分量大小
    x264_t *videoEncoder = nullptr;    //编码器
    x264_picture_t *picture = nullptr; //输入图像
};


#endif //WYRTMPLIVE_VIDEOCHANNEL_H
