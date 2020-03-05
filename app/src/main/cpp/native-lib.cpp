#include <jni.h>
#include <string>
#include <rtmp.h>
#include <bitset>
#include <x264.h>
#include <thread>
#include "safeQueue.h"
#include "VideoChannel.h"
#include "wonderfulEnum.h"
#include "macro.h"
#include "AudioChannel.h"


SafeQueue<RTMPPacket *> rtmpPackets;  //rtmp包队列
VideoChannel *videoChannel = nullptr; //视频通道,负责将yuv编码成h264并封装成rtmp
AudioChannel *audioChannel = nullptr; //音频通道,负责将pcm编码成aac并封装成rtmp
char *url = nullptr;   //rtmp地址
uint32_t startTime = 0;//起始时间戳
pthread_t pushThread;  //推流线程id
LiveState liveState = STOP;//直播状态

//释放packet
void releasePacket(RTMPPacket **packet){
    if(*packet){
        RTMPPacket_Free(*packet);
        delete(*packet);
        *packet = nullptr;
    }
}

//VideoChannel、AudioChannel封装好rtmp包回调出来,设置时间戳并入队
void rtmpPacketCallback(RTMPPacket *packet){
    if(packet){
        if(packet->m_nTimeStamp == -1){
            packet->m_nTimeStamp = RTMP_GetTime() - startTime;
        }
        rtmpPackets.push(packet);
    }
}

//推流线程task
void* pushTask(void *path){
    RTMP *rtmp = nullptr;
    int result = -1;
    do{
        //1.初始化rtmp
        rtmp = RTMP_Alloc();
        if(!rtmp){
            //TODO rtmp初始化失败
            break;
        }
        RTMP_Init(rtmp);
        //2.设置流媒体地址
        result = RTMP_SetupURL(rtmp,url);
        if(!result){
            //TODO rtmp地址错误
            break;
        }
        //3.开启输出模式
        RTMP_EnableWrite(rtmp);
        //4.建立连接
        result = RTMP_Connect(rtmp,nullptr);
        if(!result){
            //TODO rtmp连接失败
            break;
        }
        //5.建立流连接
        result = RTMP_ConnectStream(rtmp,0);
        if(!result){
            //TODO rtmp流连接失败
            break;
        }
        //以上通过说明已经和服务器连通了
        //6.记录开始推流的时间
        startTime = RTMP_GetTime();
        rtmpPackets.setWork(1);
        //先获取音频数据序列头
        rtmpPacketCallback(audioChannel->getSequenceHead());
        RTMPPacket *packet = nullptr;
        //当暂停的时候我们就一直推这个包，可以做一些封面
        //如果不推的话服务器5秒后就会断开，这是服务器端设置的
        RTMPPacket *pausePacket = nullptr;
        int ret = 0;
        while(liveState != STOP){
            LOGE("queue size: %d",rtmpPackets.size());
            //在直播的时候让队列始终有一些数据，方便后面的控制
            if(liveState == LIVING && rtmpPackets.size() < MIN_QUEUE_SIZE){
                this_thread::sleep_for(chrono::milliseconds(LOWER_SEEP_TIME));
                continue;
            }
            //如果是暂停状态并且队列只有一个数据包了，我们就只取不出队，循环推送这个封面，相当于暂停功能
            if(liveState == PAUSE && rtmpPackets.size() == 1){
                LOGE("PAUSE!");
                rtmpPackets.take(pausePacket);
                if(pausePacket){
                    RTMP_SendPacket(rtmp,pausePacket,1);
                }
                pausePacket = nullptr;
                this_thread::sleep_for(chrono::milliseconds(PAUSE_SEEP_TIME));
                continue;
            }
            //否则正常取
            else{
                ret = rtmpPackets.pop(packet);
            }
            if(liveState == STOP){
                LOGE("STOP!");
                break;
            }
            if(ret == -1){
//                LOGE("队列未工作!");
                continue;
            } else if(ret == 0){
//                LOGE("队列空!");
                continue;
            } else if(ret == 1){
//                LOGE("取数据成功!");
            } else{
                LOGE("未知错误!");
                continue;
            }
            //7.将rtmp包推流到服务器
            packet->m_nInfoField2 = rtmp->m_stream_id;
            ret = RTMP_SendPacket(rtmp,packet,1);
            if(!ret){
                //TODO 异常，可能断开连接了
                LOGE("异常，可能断开连接了!");
                break;
            }
            //推流后释放，否则有内存泄漏
            releasePacket(&packet);
            packet = nullptr;
//            LOGE("push ...");
        }
        if(packet){
            releasePacket(&packet);
        }
        if(pausePacket){
            releasePacket(&pausePacket);
        }
    }while (0);
    rtmpPackets.setWork(0);
    rtmpPackets.clear();
    if(rtmp){
        RTMP_Close(rtmp);
        RTMP_Free(rtmp);
        rtmp = nullptr;
    }
    if(url){
        delete(url);
        url = nullptr;
    }
    return nullptr;
}

//（1）初始化，设置一些参数回调等
extern "C"
JNIEXPORT void JNICALL
Java_com_example_wyrtmplive_WonderfulPush_initNative(JNIEnv *env, jobject thiz) {
    // TODO: implement initNative()
    videoChannel = new VideoChannel;
    videoChannel->setVideoCallback(rtmpPacketCallback);
    audioChannel = new AudioChannel;
    audioChannel->setAudioCallback(rtmpPacketCallback);
    rtmpPackets.setReleaseCallback(releasePacket);
}
//（2）初始化视频编码器
extern "C"
JNIEXPORT void JNICALL
Java_com_example_wyrtmplive_WonderfulPush_initVideoEncoderNative(JNIEnv *env, jobject thiz,
                                                                 jint width, jint height, jint fps,
                                                                 jint bit_rate) {
    // TODO: implement initVideoEncoderNative()
    if(videoChannel){
        videoChannel->initVideoEncoder(width,height,fps,bit_rate);
    }

}
//（2）初始化音频编码器sampleRateHz:采样率（HZ）channels:声道数
extern "C"
JNIEXPORT void JNICALL
Java_com_example_wyrtmplive_WonderfulPush_initAudioEncoderNative(JNIEnv *env, jobject thiz,
                                                                 jint sample_rate_hz,
                                                                 jint channels) {
    // TODO: implement initAudioEncoderNative()
    if(audioChannel){
        audioChannel->initAudioEncoder(sample_rate_hz,channels);
    }
}
//（3）建立服务器连接，开启子线程，循环从队列中读取rtmp包并推流到服务器
extern "C"
JNIEXPORT void JNICALL
Java_com_example_wyrtmplive_WonderfulPush_startLiveNative(JNIEnv *env, jobject thiz,
                                                          jstring rtmp_path) {
    // TODO: implement startLiveNative()
    if(liveState == STOP){
        liveState = LIVING;
        const char *path = env->GetStringUTFChars(rtmp_path,0);
        url = new char[strlen(path) + 1];
        strcpy(url,path);
        pthread_create(&pushThread, nullptr,pushTask, nullptr);
        env->ReleaseStringUTFChars(rtmp_path,path);
    }
}
//（4）将摄像头数据传到底层，进行编码封装成rtmp包并放入队列
extern "C"
JNIEXPORT void JNICALL
Java_com_example_wyrtmplive_WonderfulPush_pushVideoNative(JNIEnv *env, jobject thiz,
                                                          jbyteArray data) {
    // TODO: implement pushVideoNative()
    if(liveState != LIVING || !videoChannel)return;
    jbyte *yuv = env->GetByteArrayElements(data,0);
    videoChannel->encode(yuv);
    env->ReleaseByteArrayElements(data,yuv,0);
}

//（4）将音频数据传到底层，进行编码封装成rtmp包并放入队列
extern "C"
JNIEXPORT void JNICALL
Java_com_example_wyrtmplive_WonderfulPush_pushAudioNative(JNIEnv *env, jobject thiz,
                                                          jbyteArray data) {
    // TODO: implement pushAudioNative()
    if(liveState != LIVING || !audioChannel)return;
    jbyte *pcm = env->GetByteArrayElements(data,0);
    audioChannel->encode(pcm);
    env->ReleaseByteArrayElements(data,pcm,0);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_wyrtmplive_WonderfulPush_stopLiveNative(JNIEnv *env, jobject thiz) {
    // TODO: implement stopLiveNative()
    liveState = STOP;
    rtmpPackets.setWork(0);
    pthread_join(pushThread, nullptr);
    rtmpPackets.clear();
    DELETE(videoChannel);
    DELETE(audioChannel);
}

//获取音频编码输入样本数
extern "C"
JNIEXPORT jint JNICALL
Java_com_example_wyrtmplive_WonderfulPush_getInputSamples(JNIEnv *env, jobject thiz) {
    // TODO: implement getInputSamples()
    if(audioChannel){
        return audioChannel->getInputSamples();
    }
    return 0;
}

//直播状态控制：暂停、继续
extern "C"
JNIEXPORT void JNICALL
Java_com_example_wyrtmplive_WonderfulPush_liveControlNative(JNIEnv *env, jobject thiz, jint state) {
    // TODO: implement liveControlNative()
    switch (state){
        case 1:
            liveState = LIVING;
            break;
        case 2:
            liveState = PAUSE;
            break;
        default:
            break;
    }
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_wyrtmplive_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    std::string version = std::to_string(RTMP_LibVersion());
    x264_picture_t *picture = new x264_picture_t;
    x264_picture_init(picture);
    return env->NewStringUTF(hello.c_str());
}