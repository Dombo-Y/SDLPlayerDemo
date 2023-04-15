//
//  PlayerC_interface.h
//  SDLTestDemo
//
//  Created by 尹东博 on 2023/4/15.
//

#ifndef PlayerC_interface_h
#define PlayerC_interface_h

int playerDoSomethingWith (void *hhObjectInstance, void *parameter);

void playerDoDraw(void *hhObjectInstance,void *data, uint32_t w, uint32_t h);// 解码完成绘制视频帧
void stateChanged(void *hhObjectInstance); // 播放器状态改变
void initFinished(void *hhObjectInstance); // 音视频解码器初始化完毕
void timeChanged(void *hhObjectInstance); // 音视频播放音频时间变化
void playFailed(void *hhObjectInstance); // 音视频播放失败

#endif /* PlayerC_interface_h */
