/*
 * Author: yewenliang@allwinnertech.com
 * Copyright (C) 2014 Allwinner Technology Co., Ltd.
 */

#ifndef _ECHOCANCEL_H_
#define _ECHOCANCEL_H_
//#include <utils/threads.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define EC_MAX_PACKET       (20)
#define EC_FRAME_SIZE       (64)
#define EC_TAIL             (2048)

typedef enum AecState {
    NO_AEC    = 0,
    NEED_AEC  = 1,
} AecState;

typedef enum AecStreamState {
    STREAM_STOPED      = 0,
    STREAM_STARTED     = 1,
} AecStreamState;

//int aec_recorder_check_state(uint32_t size);

/*
 * 功能：recorder 端执行回音消除功能
 * 输入：inBuffer:待处理的录音音频PCM数据首地址；
 *       outBuffer: 经过回音消除后的PCM数据应存放的首地址
 *       size：PCM数据长度，长度只支持128的整数倍（也可以说是64帧数据的整数被），
 *             如128，256等, 最长支持（1280*2）字节（1280帧）
 * 输出：0：成功；其他值：失败。
 */
int aec_recorder_process(void *inBuffer, void *outBuffer, uint32_t size);

/*
 * 功能：recorder 端初始化回音消除库
 * 输入：sampleRate:采样率，目前只支持8K 与 16K
 *       channels: 目前只支持1ch
 * 输出：0：成功；其他值：失败。
 */
int aec_recorder_start(int sampleRate, int channels);

/*
 * 功能：recorder 端关闭回音消除库
 * 输出：0：成功；其他值：失败。
 */
int aec_recorder_stop();

/*
 * 功能：player 端添加回音参考数据
 * 输入：buffer:参考PCM数据首地址
 *       size: 数据长度
 * 输出：0：成功；其他值：失败。
 */
int aec_player_add_echo_data(const void *buffer, uint32_t size);

/*
 * 功能：player 端设置当前的状态
 * 输入：state: player状态，STREAM_STARTED：player已启动；STREAM_STOPED：player已退出
 */
void aec_player_set_state(AecStreamState state);

#ifdef __cplusplus
}
#endif

#endif //_ECHOCANCEL_H_

