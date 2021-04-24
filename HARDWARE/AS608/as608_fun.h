#ifndef __AS608_FUN_H
#define __AS608_FUN_H

#include "delay.h" 	
#include "usart2.h"
#include "as608.h"
#include "oled.h"
#include "timer.h"
#include "usart.h"
#include "sys.h"
#include "buzzer.h"
#include "includes.h"
#include "stm32f4xx.h"
#include <stdio.h>

#define usart2_baund  57600

//录指纹
uint32_t Add_FR(void);

//刷指纹
uint32_t press_FR(void);

//删除指纹
uint32_t Del_FR(void);

//显示确认码错误信息
void ShowErrMessage(u8 ensure);

#endif
