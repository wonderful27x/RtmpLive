//
// Created by Acer on 2020/2/13.
//

#ifndef WYRTMPLIVE_AUDIOCHANNEL_H
#define WYRTMPLIVE_AUDIOCHANNEL_H

#include <rtmp.h>
#include <faac.h>
#include <rtmp_sys.h>
#include "macro.h"

class AudioChannel {

typedef void (*AudioCallback)(RTMPPacket *packet);

public:
    AudioChannel();
    ~AudioChannel();

    void setAudioCallback(AudioCallback audioCallback);
    void initAudioEncoder(int sampleRate,int channels);
    void encode(int8_t *data);
    RTMPPacket* getSequenceHead();
    int getInputSamples();

private:
    AudioCallback audioCallback;
    faacEncHandle audioEncoder;   //编码器
    unsigned long inputSamples;   //编码器每次编码是接收的最大样本数
    unsigned long maxOutputBytes; //编码最大输出数据个数
    unsigned char *outBuff;       //输出缓冲区
    int channels;                 //声道数


};


#endif //WYRTMPLIVE_AUDIOCHANNEL_H
