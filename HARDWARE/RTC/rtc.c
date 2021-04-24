#include "sys.h"
#include "stdlib.h"
#include "rtc.h"
#include "includes.h"	//ucos ʹ��	  
static EXTI_InitTypeDef		EXTI_InitStructure;
static NVIC_InitTypeDef		NVIC_InitStructure;
RTC_TimeTypeDef  	RTC_TimeStructure;
RTC_DateTypeDef  	RTC_DateStructure;
static RTC_InitTypeDef  	RTC_InitStructure;


static volatile uint32_t g_rtc_wakeup_event=0;  //ʱ�ӱ�־λ

//RTCʱ�ӳ�ʼ��
void rtc_init(void)
{
	//ʹ��rtc��Ӳ��ʱ��
	
	/* Enable the PWR clock��ʹ�ܵ�Դ����ʱ�� */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
	
	/* Allow access to RTC���������RTC */
	PWR_BackupAccessCmd(ENABLE);

#if 0
	//��LSE��ʱ��
	RCC_LSEConfig(RCC_LSE_ON);

	//���LSE��ʱ���Ƿ����
	while(RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET);
	
	//ΪRTCѡ��LSE��Ϊʱ��Դ
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
	
	/* Enable the RTC Clock��ʹ��RTC��Ӳ��ʱ�� */
	RCC_RTCCLKCmd(ENABLE);
	
	/* Wait for RTC APB registers synchronisation���ȴ�����RTC��ؼĴ������� */
	RTC_WaitForSynchro();
	
	//����Ƶ��1Hz
	/* ck_spre(1Hz) = RTCCLK(LSE) /(uwAsynchPrediv + 1)*(uwSynchPrediv + 1)*/
	//32768Hz/(127+1)/(255+1)=1Hz
	RTC_InitStructure.RTC_AsynchPrediv = 127;
	RTC_InitStructure.RTC_SynchPrediv = 255;
	RTC_InitStructure.RTC_HourFormat = RTC_HourFormat_24;
	RTC_Init(&RTC_InitStructure);
	
	
	//��������
	RTC_DateStructure.RTC_Year = 0x20;				//2020��
	RTC_DateStructure.RTC_Month = RTC_Month_April;	//4�·�
	RTC_DateStructure.RTC_Date = 0x10;				//��24��/24��/24��
	RTC_DateStructure.RTC_WeekDay = RTC_Weekday_Friday;//����һ
	RTC_SetDate(RTC_Format_BCD, &RTC_DateStructure);


	//����ʱ�� 16:00:00
	RTC_TimeStructure.RTC_H12     = RTC_H12_PM;
	RTC_TimeStructure.RTC_Hours   = 0x00;
	RTC_TimeStructure.RTC_Minutes = 0x32;
	RTC_TimeStructure.RTC_Seconds = 0x00; 
	RTC_SetTime(RTC_Format_BCD, &RTC_TimeStructure);  
	
	
	RTC_WakeUpCmd(DISABLE);
	
	//����ʱ��Դ��Ӳ��ʱ��Ƶ��Ϊ1Hz
	RTC_WakeUpClockConfig(RTC_WakeUpClock_CK_SPRE_16bits);
	
	//ֻ����һ�μ���
	RTC_SetWakeUpCounter(0);
	
	RTC_WakeUpCmd(ENABLE);

	//�����жϵĴ�����ʽ�������ж�
	RTC_ITConfig(RTC_IT_WUT, ENABLE);
	
	
	RTC_ClearFlag(RTC_FLAG_WUTF);
	
	EXTI_ClearITPendingBit(EXTI_Line22);
	EXTI_InitStructure.EXTI_Line = EXTI_Line22;			//
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;//RTC�ֲ�涨
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);

	//���ȼ�
	NVIC_InitStructure.NVIC_IRQChannel = RTC_WKUP_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

}
void rtc_repeat_init(void)
{
		//ʹ��rtc��Ӳ��ʱ��
		
		/* Enable the PWR clock��ʹ�ܵ�Դ����ʱ�� */
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
		
		/* Allow access to RTC���������RTC */
		PWR_BackupAccessCmd(ENABLE);

	#if 0
		//��LSE��ʱ��
		RCC_LSEConfig(RCC_LSE_ON);

		//���LSE��ʱ���Ƿ����
		while(RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET);
		
		//ΪRTCѡ��LSE��Ϊʱ��Դ
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
		
		/* Enable the RTC Clock��ʹ��RTC��Ӳ��ʱ�� */
		RCC_RTCCLKCmd(ENABLE);
		
		/* Wait for RTC APB registers synchronisation���ȴ�����RTC��ؼĴ������� */
		RTC_WaitForSynchro();
		
		//����Ƶ��1Hz
		/* ck_spre(1Hz) = RTCCLK(LSE) /(uwAsynchPrediv + 1)*(uwSynchPrediv + 1)*/
		//32768Hz/(127+1)/(255+1)=1Hz
		RTC_InitStructure.RTC_AsynchPrediv = 127;
		RTC_InitStructure.RTC_SynchPrediv = 255;
		RTC_InitStructure.RTC_HourFormat = RTC_HourFormat_24;
		RTC_Init(&RTC_InitStructure);
		
		RTC_WakeUpCmd(DISABLE);
		
		//����ʱ��Դ��Ӳ��ʱ��Ƶ��Ϊ1Hz
		RTC_WakeUpClockConfig(RTC_WakeUpClock_CK_SPRE_16bits);
		
		//ֻ����һ�μ���
		RTC_SetWakeUpCounter(0);
		
		RTC_WakeUpCmd(ENABLE);

		//�����жϵĴ�����ʽ�������ж�
		RTC_ITConfig(RTC_IT_WUT, ENABLE);
		
		
		RTC_ClearFlag(RTC_FLAG_WUTF);
		
		EXTI_ClearITPendingBit(EXTI_Line22);
		EXTI_InitStructure.EXTI_Line = EXTI_Line22;			//
		EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
		EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;//RTC�ֲ�涨
		EXTI_InitStructure.EXTI_LineCmd = ENABLE;
		EXTI_Init(&EXTI_InitStructure);

		//���ȼ�
		NVIC_InitStructure.NVIC_IRQChannel = RTC_WKUP_IRQn;
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
		NVIC_Init(&NVIC_InitStructure);
		


}
 extern OS_FLAG_GRP	g_flag_grp;//�ⲿ�����¼���־��Ķ���
//RTC�жϷ�����
void  RTC_WKUP_IRQHandler(void)
{
//	uint8_t d;
	OS_ERR err;//������
	//�����ж�
	OSIntEnter(); 
	//����־λ
	if(RTC_GetITStatus(RTC_IT_WUT) == SET)
	{
		
		//�����¼���־�� bit0Ϊ1;
		OSFlagPost(&g_flag_grp,0x01,OS_OPT_POST_FLAG_SET,&err);

		//�����¼���־�� bit3Ϊ1;
		OSFlagPost(&g_flag_grp,0x08,OS_OPT_POST_FLAG_SET,&err);
		
		//��ձ�־λ	
		RTC_ClearITPendingBit(RTC_IT_WUT);
		
		EXTI_ClearITPendingBit(EXTI_Line22);
	}
	//�˳��ж�
	OSIntExit(); 
	
}
