#include "sys.h"
#include "stdlib.h"
#include "rtc.h"
#include "includes.h"	//ucos 使用	  
static EXTI_InitTypeDef		EXTI_InitStructure;
static NVIC_InitTypeDef		NVIC_InitStructure;
RTC_TimeTypeDef  	RTC_TimeStructure;
RTC_DateTypeDef  	RTC_DateStructure;
static RTC_InitTypeDef  	RTC_InitStructure;


static volatile uint32_t g_rtc_wakeup_event=0;  //时钟标志位

//RTC时钟初始化
void rtc_init(void)
{
	//使能rtc的硬件时钟
	
	/* Enable the PWR clock，使能电源管理时钟 */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
	
	/* Allow access to RTC，允许访问RTC */
	PWR_BackupAccessCmd(ENABLE);

#if 0
	//打开LSE振荡时钟
	RCC_LSEConfig(RCC_LSE_ON);

	//检查LSE振荡时钟是否就绪
	while(RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET);
	
	//为RTC选择LSE作为时钟源
	RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
	
#else
	
	RCC_LSICmd(ENABLE);
	
	/* Wait till LSI is ready */  
	while(RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET)
	{
		
	}
	
	/* Select the RTC Clock Source */
	RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
	
	
	
#endif
	
	/* Enable the RTC Clock，使能RTC的硬件时钟 */
	RCC_RTCCLKCmd(ENABLE);
	
	/* Wait for RTC APB registers synchronisation，等待所有RTC相关寄存器就绪 */
	RTC_WaitForSynchro();
	
	//配置频率1Hz
	/* ck_spre(1Hz) = RTCCLK(LSE) /(uwAsynchPrediv + 1)*(uwSynchPrediv + 1)*/
	//32768Hz/(127+1)/(255+1)=1Hz
	RTC_InitStructure.RTC_AsynchPrediv = 127;
	RTC_InitStructure.RTC_SynchPrediv = 255;
	RTC_InitStructure.RTC_HourFormat = RTC_HourFormat_24;
	RTC_Init(&RTC_InitStructure);
	
	
	//配置日期
	RTC_DateStructure.RTC_Year = 0x20;				//2020年
	RTC_DateStructure.RTC_Month = RTC_Month_April;	//4月份
	RTC_DateStructure.RTC_Date = 0x10;				//第24天/24日/24号
	RTC_DateStructure.RTC_WeekDay = RTC_Weekday_Friday;//星期一
	RTC_SetDate(RTC_Format_BCD, &RTC_DateStructure);


	//配置时间 16:00:00
	RTC_TimeStructure.RTC_H12     = RTC_H12_PM;
	RTC_TimeStructure.RTC_Hours   = 0x00;
	RTC_TimeStructure.RTC_Minutes = 0x32;
	RTC_TimeStructure.RTC_Seconds = 0x00; 
	RTC_SetTime(RTC_Format_BCD, &RTC_TimeStructure);  
	
	
	RTC_WakeUpCmd(DISABLE);
	
	//唤醒时钟源的硬件时钟频率为1Hz
	RTC_WakeUpClockConfig(RTC_WakeUpClock_CK_SPRE_16bits);
	
	//只进行一次计数
	RTC_SetWakeUpCounter(0);
	
	RTC_WakeUpCmd(ENABLE);

	//配置中断的触发方式：唤醒中断
	RTC_ITConfig(RTC_IT_WUT, ENABLE);
	
	
	RTC_ClearFlag(RTC_FLAG_WUTF);
	
	EXTI_ClearITPendingBit(EXTI_Line22);
	EXTI_InitStructure.EXTI_Line = EXTI_Line22;			//
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;//RTC手册规定
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);

	//优先级
	NVIC_InitStructure.NVIC_IRQChannel = RTC_WKUP_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

}
void rtc_repeat_init(void)
{
		//使能rtc的硬件时钟
		
		/* Enable the PWR clock，使能电源管理时钟 */
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
		
		/* Allow access to RTC，允许访问RTC */
		PWR_BackupAccessCmd(ENABLE);

	#if 0
		//打开LSE振荡时钟
		RCC_LSEConfig(RCC_LSE_ON);

		//检查LSE振荡时钟是否就绪
		while(RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET);
		
		//为RTC选择LSE作为时钟源
		RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
		
	#else
		
		RCC_LSICmd(ENABLE);
		
		/* Wait till LSI is ready */  
		while(RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET)
		{
			
		}
		
		/* Select the RTC Clock Source */
		RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
		
		
		
	#endif
		
		/* Enable the RTC Clock，使能RTC的硬件时钟 */
		RCC_RTCCLKCmd(ENABLE);
		
		/* Wait for RTC APB registers synchronisation，等待所有RTC相关寄存器就绪 */
		RTC_WaitForSynchro();
		
		//配置频率1Hz
		/* ck_spre(1Hz) = RTCCLK(LSE) /(uwAsynchPrediv + 1)*(uwSynchPrediv + 1)*/
		//32768Hz/(127+1)/(255+1)=1Hz
		RTC_InitStructure.RTC_AsynchPrediv = 127;
		RTC_InitStructure.RTC_SynchPrediv = 255;
		RTC_InitStructure.RTC_HourFormat = RTC_HourFormat_24;
		RTC_Init(&RTC_InitStructure);
		
		RTC_WakeUpCmd(DISABLE);
		
		//唤醒时钟源的硬件时钟频率为1Hz
		RTC_WakeUpClockConfig(RTC_WakeUpClock_CK_SPRE_16bits);
		
		//只进行一次计数
		RTC_SetWakeUpCounter(0);
		
		RTC_WakeUpCmd(ENABLE);

		//配置中断的触发方式：唤醒中断
		RTC_ITConfig(RTC_IT_WUT, ENABLE);
		
		
		RTC_ClearFlag(RTC_FLAG_WUTF);
		
		EXTI_ClearITPendingBit(EXTI_Line22);
		EXTI_InitStructure.EXTI_Line = EXTI_Line22;			//
		EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
		EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;//RTC手册规定
		EXTI_InitStructure.EXTI_LineCmd = ENABLE;
		EXTI_Init(&EXTI_InitStructure);

		//优先级
		NVIC_InitStructure.NVIC_IRQChannel = RTC_WKUP_IRQn;
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
		NVIC_Init(&NVIC_InitStructure);
		


}
 extern OS_FLAG_GRP	g_flag_grp;//外部声明事件标志组的对象
//RTC中断服务函数
void  RTC_WKUP_IRQHandler(void)
{
//	uint8_t d;
	OS_ERR err;//错误码
	//进入中断
	OSIntEnter(); 
	//检测标志位
	if(RTC_GetITStatus(RTC_IT_WUT) == SET)
	{
		
		//设置事件标志组 bit0为1;
		OSFlagPost(&g_flag_grp,0x01,OS_OPT_POST_FLAG_SET,&err);

		//设置事件标志组 bit3为1;
		OSFlagPost(&g_flag_grp,0x08,OS_OPT_POST_FLAG_SET,&err);
		
		//清空标志位	
		RTC_ClearITPendingBit(RTC_IT_WUT);
		
		EXTI_ClearITPendingBit(EXTI_Line22);
	}
	//退出中断
	OSIntExit(); 
	
}
