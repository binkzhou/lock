#include "buzzer.h" 
#include "sys.h"
GPIO_InitTypeDef  GPIO_InitStructure;
//配置蜂鸣器
void buzzer_Init(void)
{
//使能端口F的硬件时钟，端口F才能工作，说白了就是对端口F上电
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOF, ENABLE);	
	

	//配置硬件，配置GPIO，端口F，第8 个引脚
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_8;			//第 8 9个引脚
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_OUT;		//输出模式
	GPIO_InitStructure.GPIO_Speed=GPIO_High_Speed;		//引脚高速工作，收到指令立即工作；缺点：功耗高
	GPIO_InitStructure.GPIO_OType=GPIO_OType_PP;		//增加输出电流的能力
	GPIO_InitStructure.GPIO_PuPd=GPIO_PuPd_NOPULL;		//不需要上下拉电阻
	GPIO_Init(GPIOF,&GPIO_InitStructure);	
	PFout(8)=0;	

}
