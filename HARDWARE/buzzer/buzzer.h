#ifndef __BUZZER_H
#define __BUZZER_H

#include "stm32f4xx.h" 
//LED�˿ڶ���
#define BEEP PFout(8)	// ����������IO 
//���÷�����
void buzzer_Init(void);


#endif
