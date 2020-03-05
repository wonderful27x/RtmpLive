//
// Created by Acer on 2020/2/13.
//

#include "AudioChannel.h"

AudioChannel::AudioChannel() {

}

AudioChannel::~AudioChannel() {
    DELETE(outBuff);
    if(audioEncoder){
        faacEncClose(audioEncoder);
        audioEncoder = 0;
    }
}

//编码器初始化
//AAC的音频文件格式有 ADIF 和 ADTS。一种是在连续的音频数据的开始处存有解码信息，一种是在每一小段音频数据头部存放7个或者9个字节的头信息用于播放器解码。
//RTMP推流需要的是 AAC 的裸数据。所以如果编码出 ADTS 格式的数据，需要去掉7个或者9个字节的 ADTS 头信息。
//作者：Damon_He
//链接：https://www.jianshu.com/p/f87ac6aa6d63
//来源：简书
//著作权归作者所有。商业转载请联系作者获得授权，非商业转载请注明出处。
void AudioChannel::initAudioEncoder(int sampleRate, int channels) {
    this->channels = channels;
    //打开编码器
    //inputSamples：编码器每次编码是接收的最大样本数
    //maxOutputBytes：编码最大输出数据个数
    audioEncoder = faacEncOpen(sampleRate,channels,&inputSamples,&maxOutputBytes);
    //获取当前配置信息
    faacEncConfigurationPtr config = faacEncGetCurrentConfiguration(audioEncoder);
    config->mpegVersion = MPEG4;             //使用MPEG-4标准
    config->aacObjectType = LOW;             //使用低复杂度标准
    config->inputFormat = FAAC_INPUT_16BIT;  //16为编码
    config->outputFormat = 0;                //需要编成AAC裸流数据ADIF格式，而不是ADTS
    config->useTns = 1;                      //降噪
    config->useLfe = 0;                      //环绕

    int result = faacEncSetConfiguration(audioEncoder,config);
    if (!result){
        //TODO 编码器参数设置失败
        return;
    }

    outBuff = new u_char[maxOutputBytes];

}

//解码并封装rtmp数据包
void AudioChannel::encode(int8_t *data) {
    //编码并返回编码后的数据长度
    int byteLength = faacEncEncode(audioEncoder, reinterpret_cast<int32_t *>(data), inputSamples, outBuff, maxOutputBytes);
    if (byteLength >0){
        //组装trmp包，参考https://www.jianshu.com/p/f87ac6aa6d63
        RTMPPacket *packet = new RTMPPacket;
        //2字节的固定长度 + 数据长度
        int bodySize = 2 + byteLength;
        RTMPPacket_Alloc(packet,bodySize);

        //单声道
        if(channels == 1){
            packet->m_body[0] = 0xAE;
        }
        //双声道
        else if(channels == 2){
            packet->m_body[0] = 0xAF;
        }
        //表示数据
        packet->m_body[1] = 0x01;
        memcpy(&packet->m_body[2],outBuff,byteLength);

        packet->m_packetType = RTMP_PACKET_TYPE_AUDIO;
        packet->m_nBodySize = bodySize;
        packet->m_nTimeStamp = -1;         //时间戳，设置一个-1标志，由外部设置
        packet->m_hasAbsTimestamp = 1;     //有相对时间戳
        packet->m_nChannel = 11;           //通道id，不清楚是否可以随便设置
        packet->m_headerType = RTMP_PACKET_SIZE_LARGE;

        audioCallback(packet);
    } else{
        //TODO 编码失败
    }
}

//回调设置
void AudioChannel::setAudioCallback(AudioChannel::AudioCallback audioCallback) {
    this->audioCallback = audioCallback;
}

int AudioChannel::getInputSamples() {
    return (int)(inputSamples);
}

//获取音频数据包的序列头
//参考https://www.jianshu.com/p/f87ac6aa6d63
RTMPPacket *AudioChannel::getSequenceHead() {
    u_char *ppBuff;
    u_long byteLength;
    faacEncGetDecoderSpecificInfo(audioEncoder,&ppBuff,&byteLength);
    //组装trmp包，参考https://www.jianshu.com/p/f87ac6aa6d63
    RTMPPacket *packet = new RTMPPacket;
    //2字节的固定长度 + 数据长度
    int bodySize = 2 + byteLength;
    RTMPPacket_Alloc(packet,bodySize);

    //单声道
    if(channels == 1){
        packet->m_body[0] = 0xAE;
    }
        //双声道
    else if(channels == 2){
        packet->m_body[0] = 0xAF;
    }
    //表示序列头
    packet->m_body[1] = 0x00;
    memcpy(&packet->m_body[2],ppBuff,byteLength);

    packet->m_packetType = RTMP_PACKET_TYPE_AUDIO;
    packet->m_nBodySize = bodySize;
    packet->m_nTimeStamp = -1;         //时间戳，设置一个-1标志，由外部设置
    packet->m_hasAbsTimestamp = 1;     //有相对时间戳
    packet->m_nChannel = 11;           //通道id，不清楚是否可以随便设置
    packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;

    return packet;
}

