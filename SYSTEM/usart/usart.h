#ifndef __USART_H
#define __USART_H
#include "stdio.h"	
#include "stm32f4xx_conf.h"
#include "sys.h" 


extern void usart1_init(uint32_t baud);//初始化串口1
extern void usart3_init(uint32_t baud);//初始化串口3
extern void usart3_send_str(char *str);//发送数据给蓝牙
extern void bluetooth_config(void);	//蓝牙模块配置
void bluetooth_MCU(void);//蓝牙MCU引脚(判断蓝牙是否连接)初始化    连上出现低电平    断开出现高电平
#endif


