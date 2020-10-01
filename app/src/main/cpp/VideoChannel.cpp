//
// Created by Acer on 2020/2/13.
//

#include "VideoChannel.h"

/**
 * NALU就是NAL UNIT，nal单元。NAL全称Network Abstract Layer,
 * 即网络抽象层，H.264在网络上传输的结构。一帧图片经过 H.264 编码器之后，
 * 就被编码为一个或多个片（slice），而装载着这些片（slice）的载体，就是 NALU 了 。
 *
 * 而每个NALU之间通过startcode（起始码）进行分割，
 * 起始码分成两种：0x000001（3Byte）或者0x00000001（4Byte）。
 * 如果NALU对应的Slice为一帧的开始就用0x00000001，否则就用0x000001。
 *
 * 每个NALU单元又有SPS、PPS、关键帧与普通帧的区分，
 * SPS：序列参数集，作用于一系列连续的编码图像。
 * PPS：图像参数集，作用于编码视频序列中一个或多个独立的图像。
 * SPS 和 PPS 包含了初始化H.264解码器所需要的信息参数。
 * IDR：一个序列的第一个图像叫做 IDR 图像（立即刷新图像），IDR 图像都是 I 帧图像。
 *
 * 作者：Damon_He
 * 链接：https://www.jianshu.com/p/0c882eca979c
 * 来源：简书
 * 著作权归作者所有。商业转载请联系作者获得授权，非商业转载请注明出处。
 *
 *
 * https://blog.csdn.net/go_str/article/details/80340564?utm_medium=distribute.pc_aggpage_search_result.none-task-blog-2~all~first_rank_v2~rank_v25-6-80340564.nonecase&utm_term=%E4%B8%80%E4%B8%AAnal%E5%8D%95%E5%85%83%E6%9C%89%E5%87%A0%E4%B8%AA%E5%9B%BE%E7%89%87%E5%B8%A7&spm=1000.2123.3001.4430
 */
VideoChannel::VideoChannel() {
    pthread_mutex_init(&encoderMutex, nullptr);
}

VideoChannel::~VideoChannel() {
    pthread_mutex_destroy(&encoderMutex);
}

void VideoChannel::setVideoCallback(VideoChannel::VideoCallback videoCallback) {
    this->videoCallback = videoCallback;
}

//初始化编码器
void VideoChannel::initVideoEncoder(int width, int height, int fps, int bitRate) {
    //处理前先将编码器锁住，因为当surface大小发生改变是这个方法会被调用，而这时很有可能编码器还在编码当中
    pthread_mutex_lock(&encoderMutex);
    this->width = width;
    this->height = height;
    this->fps = fps;
    this->bitRate = bitRate;
    this->y_len = width * height;
    this->uv_len = y_len/4;

    LOGD("width: %d",width);
    LOGD("height: %d",height);
    LOGD("y_len: %d",y_len);
    LOGD("uv_len: %d",uv_len);

    x264_param_t param;
    //ultrafast：超快方式编码 zerolatency：零延迟
    x264_param_default_preset(&param,"ultrafast","zerolatency");
    //编码规格：参考https://wikipedia.tw.wjbk.site/wiki/H.264/MPEG-4_AVC
    //宏块大小一般为 hong = 16*16
    //计算方式：level = width*height*pfs/hong,这里故意设置得更大些了
    param.i_level_idc = 32;
    param.i_width = this->width;
    param.i_height = this->height;
    //无B帧，因为B帧的编码需要参考前后，比较耗时
    param.i_bframe = 0;
    //ABR：平均码率， CPQ：恒定质量， CRF：恒定码率
    param.rc.i_rc_method = X264_RC_ABR;
    //比特率 kb/s
    param.rc.i_bitrate = this->bitRate / 1000;
    //最大瞬时码率
    param.rc.i_vbv_max_bitrate = this->bitRate * 1.2 / 1000;
    //缓冲区
    param.rc.i_vbv_buffer_size = this->bitRate / 1000;
    //码率控制不通时间戳和timebase，而是根据fps
    param.b_vfr_input = 0;
    param.i_fps_num = this->fps; //分子
    param.i_fps_den = 1;         //分母
    param.i_timebase_num = param.i_fps_den;
    param.i_timebase_den = param.i_fps_num;
    //关键帧距离：每两个fps帧一个关键帧，或理解为每2second一个关键帧
    param.i_keyint_max = this->fps*2;
    //赋值sps和pps到每一个关键帧前面
    param.b_repeat_headers = 1;
    //线程数
    param.i_threads = 1;
    x264_param_apply_profile(&param,"baseline");
    //创建输入图像
    picture = new x264_picture_t;
    //输入图像初始化
    x264_picture_alloc(picture,param.i_csp,param.i_width,param.i_height);
    videoEncoder = x264_encoder_open(&param);
    if(!videoEncoder){
        pthread_mutex_unlock(&encoderMutex);
        //TODO 编码器打开失败
        return;
    }
    pthread_mutex_unlock(&encoderMutex);
}

/**
 * 编码，指针的赋值时跟nv21数据结构有关系的
 * @param data nv21数据
 */
void VideoChannel::encode(int8_t *data) {
    pthread_mutex_lock(&encoderMutex);
    memcpy(picture->img.plane[0],data,y_len);//赋值Y分量
    for (int i = 0; i < uv_len ; ++i) {
        *(picture->img.plane[1] + i) = *(data + y_len + 2 * i + 1);//赋值u分量
        *(picture->img.plane[2] + i) = *(data + y_len + 2 * i);    //赋值v分量
    }

    x264_picture_t outPicture; //输出图像
    x264_nal_t *nals = nullptr;           //NALU单元数组
    int nal_len;                          //数组的长度

    int result = x264_encoder_encode(videoEncoder,&nals,&nal_len,picture,&outPicture);
    if(result <0){
        //TODO 编码失败
        pthread_mutex_unlock(&encoderMutex);
        return;
    }

    int sps_len = 0;
    int pps_len = 0;
    uint8_t sps[100];
    uint8_t pps[100];

    for (int i = 0; i < nal_len; ++i) {
        //NALU单元是SPS数据
        if (nals[i].i_type == NAL_SPS){
            //取出sps数据，需要去掉四个字节的起始码
            //我的理解是在初始化编码器的时候我们设置了param.b_repeat_headers = 1，
            //因此SPS和PPS对应的NALU单元就是一帧数据的开始，并且是关键帧的开始，所以是四字节
            //参考https://www.jianshu.com/p/0c882eca979c

            //当然也可以像sedFrame那样进行判断
            uint8_t *nal = nals[i].p_payload;
            //三字节
            if(nal[2] == 0x01){
                sps_len = nals[i].i_payload - 3;
                memcpy(sps,nals[i].p_payload + 3,sps_len);
            }
            //四字节
            else if(nal[2] == 0x00){
                sps_len = nals[i].i_payload - 4;
                memcpy(sps,nals[i].p_payload + 4,sps_len);
            }

//            sps_len = nals[i].i_payload - 4;
//            memcpy(sps,nals[i].p_payload + 4,sps_len);
        }
        //NALU单元是PPS数据
        else if(nals[i].i_type == NAL_PPS){
            //同上
            //当然也可以像sedFrame那样进行判断
            uint8_t *nal = nals[i].p_payload;
            //三字节
            if(nal[2] == 0x01){
                pps_len = nals[i].i_payload - 3;
                memcpy(pps,nals[i].p_payload + 3,pps_len);
            }
           //四字节
            else if(nal[2] == 0x00){
                pps_len = nals[i].i_payload - 4;
                memcpy(pps,nals[i].p_payload + 4,pps_len);
            }

//            pps_len = nals[i].i_payload - 4;
//            memcpy(pps,nals[i].p_payload + 4,pps_len);

            //当得到SPS和PPS是就组合在一起发送出去
            //因为SPS和PPS是紧密连在一起的，所以认为当拿到PPS时SPS也拿到了
            senSpsPps(sps,pps,sps_len,pps_len);
        }
        //NALU单元是视频数据
        else{
            sedFrame(nals[i].i_type,nals[i].p_payload,nals[i].i_payload);
        }
    }

    pthread_mutex_unlock(&encoderMutex);
}

//发送sps + pps，这里并不是真正的将数据推送到服务器，而是将h264封装成rtmp包并丢入队列
void VideoChannel::senSpsPps(uint8_t *sps, uint8_t *pps, int sps_len, int pps_len) {
    //组装(sps+pps)的trmp包，参考https://www.jianshu.com/p/0c882eca979c
    RTMPPacket *packet = new RTMPPacket;
    //5字节的固定长度 + sps和pps 长度
    int bodySize = 5 + 5 + 3 + sps_len + 3 + pps_len;
    RTMPPacket_Alloc(packet,bodySize);

    int i = 0;

    //固定长度，指明数据类型：SPS和PPS
    packet->m_body[i++] = 0x17;
    packet->m_body[i++] = 0x00;
    packet->m_body[i++] = 0x00;
    packet->m_body[i++] = 0x00;
    packet->m_body[i++] = 0x00;

    packet->m_body[i++] = 0x01;   //版本号
    packet->m_body[i++] = sps[1]; //profile
    packet->m_body[i++] = sps[2]; //兼容性
    packet->m_body[i++] = sps[3]; //profile level
    packet->m_body[i++] = 0xff;   //一般为0xff

    packet->m_body[i++] = 0xe1;   //sps个数
    //sps长度，两个字节表示
    packet->m_body[i++] = (sps_len>>8) & 0xff;
    packet->m_body[i++] = sps_len & 0xff;
    //sps内容
    memcpy(&packet->m_body[i],sps,sps_len);

    i += sps_len;

    packet->m_body[i++] = 0x01;   //pps个数
    //pps长度，两字节表示
    packet->m_body[i++] = (pps_len>>8) & 0xff;
    packet->m_body[i++] = pps_len & 0xff;
    //pps内容
    memcpy(&packet->m_body[i],pps,pps_len);

    packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
    packet->m_nBodySize = bodySize;
    packet->m_nTimeStamp = 0;         //时间戳，sps和pps没有时间戳
    packet->m_hasAbsTimestamp = 0;    //没有相对时间戳
    packet->m_nChannel = 10;          //通道id，不清楚是否可以随便设置
    packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;//头类型为中等大小，因为sps和pps数据比较小

    if (videoCallback){
        videoCallback(packet);
    }

}
//发送视频数据，这里并不是真正的将数据推送到服务器，而是将h264封装成rtmp包并丢入队列
void VideoChannel::sedFrame(int type, uint8_t *nal, int len) {
    //每个NALU单元由起始码隔开，组装rtmp包时需要去掉起始码
    //但是起始码有两种：如果NALU对应Slice为一帧开始则为四字节的0x00000001，否则为三字节的0x000001
    //因此根据第三个字节就能判断出事那种类型

    //三字节
    if(nal[2] == 0x01){
        nal += 3;
        len -= 3;
    }
    //四字节
    else if(nal[2] == 0x00){
        nal += 4;
        len -= 4;
    }

    //组装视频trmp包，参考https://www.jianshu.com/p/0c882eca979c
    RTMPPacket *packet = new RTMPPacket;
    //5字节的固定长度 + 4字节数据长度 + 数据长度
    int bodySize = 5 + 4 + len;
    RTMPPacket_Alloc(packet,bodySize);

    int i = 0;

    //关键帧
    if(type == NAL_SLICE_IDR){
        packet->m_body[i++] = 0x17;
    }
    //非关键帧
    else{
        packet->m_body[i++] = 0x27;
    }
    packet->m_body[i++] = 0x01;
    packet->m_body[i++] = 0x00;
    packet->m_body[i++] = 0x00;
    packet->m_body[i++] = 0x00;

    packet->m_body[i++] = (len >> 24) & 0xff;
    packet->m_body[i++] = (len >> 16) & 0xff;
    packet->m_body[i++] = (len >> 8) & 0xff;
    packet->m_body[i++] = len & 0xff;

    memcpy(&packet->m_body[i],nal,len);

    packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
    packet->m_nBodySize = bodySize;
    packet->m_nTimeStamp = -1;         //时间戳，设置一个-1标志，由外部设置
    packet->m_hasAbsTimestamp = 1;     //有相对时间戳
    packet->m_nChannel = 10;           //通道id，不清楚是否可以随便设置
    packet->m_headerType = RTMP_PACKET_SIZE_LARGE;//头类型为LARGE，因为视频数据比较大

    if (videoCallback){
        videoCallback(packet);
    }
}
