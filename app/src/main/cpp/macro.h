//
// Created by Acer on 2019/12/31.
//

#ifndef WYWONDERFULPLAYER_MACRO_H
#define WYWONDERFULPLAYER_MACRO_H

#define MIN_QUEUE_SIZE 5                                  //rtmp队列最小值，让队列始终有一定数据，方便控制
#define LOWER_SEEP_TIME 10                                //休眠时间（毫秒）当队列小于最小值时就休眠一段时间再推流
#define PAUSE_SEEP_TIME 100                               //暂停时推送封面保活连接的休眠时间（毫秒）

#define DELETE(object) if(object){delete object;object=0;}//释放内存

#endif //WYWONDERFULPLAYER_MACRO_H
