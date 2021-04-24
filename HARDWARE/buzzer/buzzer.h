#ifndef __BUZZER_H
#define __BUZZER_H

#include "stm32f4xx.h" 
//LED端口定义
#define BEEP PFout(8)	// 蜂鸣器控制IO 
//配置蜂鸣器
void buzzer_Init(void);


#endif
