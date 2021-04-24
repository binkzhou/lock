#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "includes.h"
#include "oled.h"
#include "bmp.h"
#include "rtc.h"
#include "flash.h"
#include <stdio.h>  
#include <string.h> 
#include "key.h"
#include "buzzer.h" 
#include "usart2.h"
#include "as608_fun.h"
#include "as608.h"
#include "MFRC522.h"
#include "lock.h"
OS_Q	usart1_queue;				//串口一消息队列的对象
OS_Q	usart3_queue;				//串口三蓝牙消息队列的对象
OS_Q    rfid_queue;					//RFID消息队列
OS_Q	key_queue;					//key按键消息队列
OS_FLAG_GRP	g_flag_grp;				//RTC事件标志组的对象
OS_MUTEX	g_mutex_oled;				//保护oled互斥量的对象
OS_SEM	g_sem;					//信号量的对象

//外部声明设置时钟结构体
extern  RTC_TimeTypeDef  RTC_TimeStructure;
extern  RTC_DateTypeDef  RTC_DateStructure;
char 	* password=NULL; //存放设置的密码的数据
uint32_t	blue_flag=0;  //蓝牙开锁记录写入扇区标志位  
char 		buf[128]={0}; //存放蓝牙开锁数据	

//MFRC522数据区
u8  mfrc552pidbuf[18];
u8  card_pydebuf[2];  //存放卡片类型
u8  card_numberbuf[5];  //存放卡序列号
u8  card_key0Abuf[6]={0xff,0xff,0xff,0xff,0xff,0xff}; //扇区密码
u8  card_writebuf[6]={1,2,3,4,5,6}; //存放写入的数据
u8  card_readbuf[18]; //存放读取的数据
char i,j,card_size; 
u8 status;
//AS608指纹模块数据
OS_SEM	g_sem;					//信号量的对象
extern SysPara AS608Para;//指纹模块AS608参数
extern u16 ValidN;//模块内有效指纹个数
u8 ensure;  

int times;//自动开锁的时间记录


//任务  启动初始化
OS_TCB startup_init_TCB;
void startup_init(void *parg);
CPU_STK startup_init_stk[128];	

//任务1 蓝牙 控制块
OS_TCB bluetooth_TCB;
void bluetooth(void *parg);
CPU_STK bluetooth_stk[1024];		

//任务2  主页显示控制块
OS_TCB Task2_TCB;
void task2(void *parg);
CPU_STK task2_stk[3600];			

//任务3  运行按键函数控制块
OS_TCB Task3_TCB;
void task3(void *parg);
CPU_STK task3_stk[1024];	
//任务4  4x4按键操作控制块
OS_TCB Task4_TCB;
void task4(void *parg);
CPU_STK task4_stk[2024];			
//任务5  RFID控制块
OS_TCB Task5_TCB;
void task5(void *parg);
CPU_STK task5_stk[1024];		
//任务6  as608（指纹模块）控制块
OS_TCB Task6_TCB;
void task6(void *parg);
CPU_STK task6_stk[1024];	
//任务7
OS_TCB Task7_TCB;
void task7(void *parg);
CPU_STK task7_stk[1024];	
//主函数
int main(void)
{
	OS_ERR err;

	systick_init();  	//时钟初始化
	usart1_init(115200);  	//串口1初始化
	usart3_init(9600);	//串口3波特率为9600bps
	LED_Init();        //LED初始化
	bluetooth_config();		//蓝牙模块配置
	OLED_Init();			//初始化OLED 
	OLED_Clear();  //关闭OLED显示
	key_init();//4x4按键引脚初始化
	MFRC522_Initializtion(); //RFID初始化	
	buzzer_Init();//初始化蜂鸣器
	usart2_init(usart2_baund);//初始化串口2,用于与指纹模块通讯 57600
	PS_StaGPIO_Init();		  //初始化FR读状态引脚(指纹感应引脚，高电平触发)
	lock_init();  //锁头初始化
	bluetooth_MCU();//蓝牙MCU引脚(判断蓝牙是否连接)初始化    连上出现低电平    断开出现高电平
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);	//中断分组
	//RTC初始化
	if (RTC_ReadBackupRegister(RTC_BKP_DR0) != 0x9999)
	{
		rtc_init(); //包括时间日期的初始化
		//建立一个重启标记，告诉当前已经设置过RTC了
		//在下一次复位，就不再需要重置RTC的日期与时间
		//往备份寄存器0写入数据0x8888
		RTC_WriteBackupRegister(RTC_BKP_DR0, 0x9999);		
	}
	else{
		rtc_repeat_init();//不包括时间日期的初始化		
	}
	/*解锁FLASH，允许操作FLASH*/
	FLASH_Unlock();
	/* 清空相应的标志位*/  
	FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | 
	FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR|FLASH_FLAG_PGSERR);
	flash_erase_record(FLASH_Sector_4);//擦除扇区4记录	
	//flash_erase_record(FLASH_Sector_6);//擦除扇区6记录	
 	
	
	//OS初始化，它是第一个运行的函数,初始化各种的全局变量，例如中断嵌套计数器、优先级、存储器
	OSInit(&err);

	//创建任务:启动初始化
	OSTaskCreate(	&startup_init_TCB,									//任务控制块，等同于线程id
					"startup_init_TCB",									//任务的名字，名字可以自定义的
					startup_init,										//任务函数，等同于线程函数
					0,											//传递参数，等同于线程的传递参数
					3,											//任务的优先级6		
					startup_init_stk,									//任务堆栈基地址
					128/10,										//任务堆栈深度限位，用到这个位置，任务不能再继续使用
					128,										//任务堆栈大小			
					0,											//禁止任务消息队列
					0,											//默认是抢占式内核															
					0,											//不需要补充用户存储区
					OS_OPT_TASK_NONE,							//没有任何选项
					&err										//返回的错误码
				);
				
	//创建任务1  蓝牙
	OSTaskCreate(	&bluetooth_TCB,									//任务控制块，等同于线程id
					"bluetooth",									//任务的名字，名字可以自定义的
					bluetooth,										//任务函数，等同于线程函数
					0,											//传递参数，等同于线程的传递参数
					5,											//任务的优先级6		
					bluetooth_stk,									//任务堆栈基地址
					1024/10,										//任务堆栈深度限位，用到这个位置，任务不能再继续使用
					1024,										//任务堆栈大小			
					0,											//禁止任务消息队列
					0,											//默认是抢占式内核															
					0,											//不需要补充用户存储区
					OS_OPT_TASK_NONE,							//没有任何选项
					&err										//返回的错误码
				);


	//创建任务2 主页显示
	OSTaskCreate(	&Task2_TCB,									//任务控制块
					"Task2",									//任务的名字
					task2,										//任务函数
					0,												//传递参数
					4,											 	//任务的优先级7		
					task2_stk,									//任务堆栈基地址
					256/10,									//任务堆栈深度限位，用到这个位置，任务不能再继续使用
					256,										//任务堆栈大小			
					0,											//禁止任务消息队列
					0,												//默认是抢占式内核																
					0,												//不需要补充用户存储区
					OS_OPT_TASK_NONE,								//没有任何选项
					&err													//返回的错误码
				);

	//创建任务3 运行按键函数
	OSTaskCreate(	&Task3_TCB,									//任务控制块
					"Task3",									//任务的名字
					task3,										//任务函数
					0,												//传递参数
					5,											 	//任务的优先级7		
					task3_stk,									//任务堆栈基地址
					128/10,									//任务堆栈深度限位，用到这个位置，任务不能再继续使用
					128,										//任务堆栈大小			
					0,											//禁止任务消息队列
					0,												//默认是抢占式内核																
					0,												//不需要补充用户存储区
					OS_OPT_TASK_NONE,								//没有任何选项
					&err													//返回的错误码
				);
	//创建任务4 4x4按键操作
	OSTaskCreate(	&Task4_TCB,									//任务控制块
					"Task4",									//任务的名字
					task4,										//任务函数
					0,												//传递参数
					5,											 	//任务的优先级7		
					task4_stk,									//任务堆栈基地址
					1024/10,									//任务堆栈深度限位，用到这个位置，任务不能再继续使用
					1024,										//任务堆栈大小			
					0,											//禁止任务消息队列
					0,												//默认是抢占式内核																
					0,												//不需要补充用户存储区
					OS_OPT_TASK_NONE,								//没有任何选项
					&err													//返回的错误码
				);
	//创建任务5  RFID
	OSTaskCreate(	&Task5_TCB,									//任务控制块
					"Task5",									//任务的名字
					task5,										//任务函数
					0,												//传递参数
					6,											 	//任务的优先级7		
					task5_stk,									//任务堆栈基地址
					1024/10,									//任务堆栈深度限位，用到这个位置，任务不能再继续使用
					1024,										//任务堆栈大小			
					0,											//禁止任务消息队列
					0,												//默认是抢占式内核																
					0,												//不需要补充用户存储区
					OS_OPT_TASK_NONE,								//没有任何选项
					&err													//返回的错误码
				);	
	//创建任务6  as608（指纹模块）
	OSTaskCreate(	&Task6_TCB,									//任务控制块
					"Task6",									//任务的名字
					task6,										//任务函数
					0,												//传递参数
					6,											 	//任务的优先级7		
					task6_stk,									//任务堆栈基地址
					1024/10,									//任务堆栈深度限位，用到这个位置，任务不能再继续使用
					1024,										//任务堆栈大小			
					0,											//禁止任务消息队列
					0,												//默认是抢占式内核																
					0,												//不需要补充用户存储区
					OS_OPT_TASK_NONE,								//没有任何选项
					&err													//返回的错误码
				);		
	//创建任务7
	OSTaskCreate(	&Task7_TCB,									//任务控制块
					"Task7",									//任务的名字
					task7,										//任务函数
					0,												//传递参数
					4,											 	//任务的优先级7		
					task7_stk,									//任务堆栈基地址
					1024/10,									//任务堆栈深度限位，用到这个位置，任务不能再继续使用
					1024,										//任务堆栈大小			
					0,											//禁止任务消息队列
					0,												//默认是抢占式内核																
					0,												//不需要补充用户存储区
					OS_OPT_TASK_NONE,								//没有任何选项
					&err													//返回的错误码
				);		

	//创建串口一消息队列
	OSQCreate(&usart1_queue,"usart1_queue",16,&err);
	//创建串口三消息队列
	OSQCreate(&usart3_queue,"usart3_queue",16,&err);
	//创建rfid消息队列
	OSQCreate(&rfid_queue,"rfid_queue",16,&err);	
	//创建按键消息队列
	OSQCreate(&key_queue,"key_queue",16,&err);	
	//创建RTC事件标志组，所有标志位初值为0
	OSFlagCreate(&g_flag_grp,"g_flag_grp",0,&err);
	//创建互斥量
	OSMutexCreate(&g_mutex_oled,"g_mutex_oled",&err);
	
	//创建信号量，初值为1，有一个资源
	OSSemCreate(&g_sem,"g_sem",1,&err);
	
	
	printf("never run.......\r\n");

	//启动OS，进行任务调度
	OSStart(&err);
	while(1);	
	
}

//任务：开机启动初始化
void startup_init(void *parg)
{
	OS_ERR err;//错误码	
		OSTaskSuspend(&Task7_TCB,&err); //挂起任务7  蓝牙断开60秒自动锁定
		//阻塞等待互斥量
		OSMutexPend(&g_mutex_oled,0,OS_OPT_PEND_BLOCKING,NULL,&err);
		password="123456"; //设置初始密码123456
		flash_write_record(password,1,ADDR_FLASH_SECTOR_6,FLASH_Sector_6);//写入初始密码	
		//清空数据
		memset(password,0,sizeof password);
	
		//指纹模块连接
		printf("与AS608模块握手....\r\n");
		while(PS_HandShake(&AS608Addr))//与AS608模块握手
		{
			delay_ms(400);		
			printf("未检测到模块!!!\r\n");
			delay_ms(800);
			printf("尝试链接模块..\r\n");
		}
		printf("通讯成功");
		ensure=PS_ValidTempleteNum(&ValidN);//读库指纹个数
		if(ensure!=0x00)
			ShowErrMessage(ensure);//显示确认码错误信息
			ensure=PS_ReadSysPara(&AS608Para);  //读参数 	
		if(ensure==0x00)
		{	
			printf("库容量:%d     对比等级: %d",AS608Para.PS_max-ValidN,AS608Para.PS_level);		
		}
		else
			ShowErrMessage(ensure);		
		
		//开机动画
		OLED_Clear();//关闭显示
		OLED_ShowCHinese(32,2,37);//欢
		OLED_ShowCHinese(50,2,38);//迎
		OLED_ShowCHinese(68,2,39);//使
		OLED_ShowCHinese(86,2,40);//用	
		OLED_ShowCHinese(23,4,6);//智
		OLED_ShowCHinese(41,4,7);//能
		OLED_ShowCHinese(59,4,8);//指
		OLED_ShowCHinese(77,4,9);//纹
		OLED_ShowCHinese(95,4,5);//锁
		OLED_ShowCHinese(10,6,41);//作
		OLED_ShowCHinese(28,6,42);//者
		OLED_ShowString(46,6,":");//:	
		OLED_ShowCHinese(64,6,10);//周
		OLED_ShowCHinese(82,6,11);//斌
		delay_ms(4000);
		OLED_Clear();//关闭显示
		OLED_DrawBMP(0,2,128,8,BMP10);//开机外星人图标初始
		delay_ms(1000);		
		//外星人进度条
		OLED_DrawBMP(0,2,128,8,BMP5);//开机外星人图标0
		delay_ms(1000);
		OLED_DrawBMP(0,2,128,8,BMP6);//开机外星人图标1	
		delay_ms(800);
		OLED_DrawBMP(0,2,128,8,BMP7);//开机外星人图标2	
		delay_ms(1000);
		OLED_DrawBMP(0,2,128,8,BMP8);//开机外星人图标3
		delay_ms(1500);
		OLED_DrawBMP(0,2,128,8,BMP9);//开机外星人图标4	
		delay_ms(3000);
		OLED_Clear();//关闭显示
		//立即释放互斥量
		OSMutexPost(&g_mutex_oled,OS_OPT_POST_NONE,&err);	
	
	OSTaskDel(NULL,&err); //删除任务 停止调度
}
//任务一：蓝牙
void bluetooth(void *parg)
{
	
	OS_ERR err;//错误码	
	OS_MSG_SIZE  msg_size;//数据大小
	char *p=NULL; //接收消息队列数据
	uint32_t	i=0;     
	char 		usart3_send_open[128]={0}; //存放反发给用户的解锁提示数据

	int a=0;//用于开锁验证密码
	int b=0;//用于开锁验证密码
		
	while(1)
	{

		//等待串口三蓝牙消息队列
		p=OSQPend(&usart3_queue,0,OS_OPT_PEND_BLOCKING,&msg_size,NULL,&err);		
		//printf("1111%s\r\n",p);
		//阻塞等待信号量
		OSSemPend(&g_sem,0,OS_OPT_PEND_BLOCKING,NULL,&err);			
		//阻塞等待互斥量
		OSMutexPend(&g_mutex_oled,0,OS_OPT_PEND_BLOCKING,NULL,&err);		
		if(p && msg_size)
		{
			OSTaskSuspend(&Task5_TCB,&err);
			//通过手机蓝牙锁定
			if(strstr((const char *)p,"lock"))
			{	
				//闪烁灯加蜂鸣器
				PFout(8)=1;PFout(9)=0;delay_ms(100);
				PFout(8)=0;PFout(9)=1;delay_ms(100);
				PFout(8)=1;PFout(9)=0;delay_ms(100);
				PFout(8)=0;PFout(9)=1;delay_ms(100);
				PEout(5)=0; //断电，松开锁  关锁				
				//反发给用户蓝牙，提示开锁成功
				usart3_send_str("lock  success\r\n");					
				OLED_Clear();//关闭显示
				OLED_ShowCHinese(32,4,0);//蓝
				OLED_ShowCHinese(50,4,1);//牙
				OLED_ShowCHinese(68,4,2);//锁
				OLED_ShowCHinese(86,4,3);//定
				delay_ms(1000);
				OLED_DrawBMP(0,2,128,8,BMP1);  //图片显示(图片显示慎用，生成的字表较大，会占用较多空间，FLASH空间8K以下慎用)
				delay_ms(2000);
				OSTaskSuspend(&Task7_TCB,&err); //挂起任务7  蓝牙断开60秒自动锁定
				//设置事件标志组  告诉主界面已锁定
				OSFlagPost(&g_flag_grp,0x02,OS_OPT_POST_FLAG_SET,&err);				
			}
			

			//通过手机蓝牙解锁
			if(strstr((const char *)p,"open-"))
			{
				//open-123456
				p=strtok((char *)p,"-");//分解等于open
				p = strtok(NULL,"-");//继续分解password等于123456#
				if(strcmp((const char *)p,"#")==0) //判断语句里密码是否为空
				{
					//反发给用户蓝牙，提示密码不能为空
					usart3_send_str("The password cannot be empty\r\n");
				}
				else
				{	
					//将字符串转换为整型，
					b = atoi((const char *)p);
					//获取存储闪存扇区6的密码记录
					flash_read_record(password,1,ADDR_FLASH_SECTOR_6);
					//将从扇区3获取的密码转换为整型（用于比较），
					a = atoi((const char *)password);		
					//清空数据
					memset(password,0,sizeof password);					
					if( a==b)//验证密码
					{	
						//闪烁灯加蜂鸣器
						PFout(8)=1;PFout(9)=0;delay_ms(100);
						PFout(8)=0;PFout(9)=1;delay_ms(100);
						PFout(8)=1;PFout(9)=0;delay_ms(100);
						PFout(8)=0;PFout(9)=1;delay_ms(100);			
						PEout(5)=1; //通电，触发锁  开锁
						
						//检查当前的数据记录是否小于100条
						if(blue_flag<100)
						{
								/*解锁FLASH，允许操作FLASH*/
								FLASH_Unlock();
								/* 清空相应的标志位*/  
								FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | 
								FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR|FLASH_FLAG_PGSERR);										
								
								//RTC_GetDate，获取日期
								RTC_GetDate(RTC_Format_BCD, &RTC_DateStructure);
								//printf("DATE-OLED:20%02x/%02x/%02x %0x\r\n",RTC_DateStructure.RTC_Year,RTC_DateStructure.RTC_Month,RTC_DateStructure.RTC_Date,RTC_DateStructure.RTC_WeekDay);	
								//RTC_GetTime，获取时间
								RTC_GetTime(RTC_Format_BCD, &RTC_TimeStructure); 
								//printf("TIME-OLED:%02x:%02x:%02x\r\n",RTC_TimeStructure.RTC_Hours,RTC_TimeStructure.RTC_Minutes,RTC_TimeStructure.RTC_Seconds);		
								//打包反发给蓝牙用户的解锁提示数据
								sprintf((char *)usart3_send_open,"20%02x/%02x/%02x  %02x:%02x:%02x Bluetooth open succeed\r\n",\
													RTC_DateStructure.RTC_Year, RTC_DateStructure.RTC_Month, RTC_DateStructure.RTC_Date,\
													RTC_TimeStructure.RTC_Hours, RTC_TimeStructure.RTC_Minutes, RTC_TimeStructure.RTC_Seconds);							
								//打包存入闪存的数据
								sprintf((char *)buf,"[%03d]20%02x/%02x/%02x  %02x:%02x:%02x Bluetooth unlock\r\n",\
													blue_flag,\
													RTC_DateStructure.RTC_Year, RTC_DateStructure.RTC_Month, RTC_DateStructure.RTC_Date,\
													RTC_TimeStructure.RTC_Hours, RTC_TimeStructure.RTC_Minutes, RTC_TimeStructure.RTC_Seconds);
								
								//写入闪存扇区4记录开锁时间及方式
								if(0==flash_write_record(buf,blue_flag,ADDR_FLASH_SECTOR_4,FLASH_Sector_4))
								{
									  //显示
									printf(buf);
									  //显示
									//printf(usart3_send);
									//反发给用户蓝牙，提示开锁成功
									 usart3_send_str((char *)usart3_send_open);
									//记录自加1
									blue_flag++;
									//清空数据
									memset(buf,0,sizeof buf);
									memset(usart3_send_open,0,sizeof usart3_send_open);
									OLED_Clear();//关闭显示
									OLED_ShowCHinese(32,4,0);//蓝
									OLED_ShowCHinese(50,4,1);//牙
									OLED_ShowCHinese(68,4,4);//解
									OLED_ShowCHinese(86,4,5);//锁
									delay_ms(1000);				
									OLED_DrawBMP(0,2,128,8,BMP2);  //图片显示(图片显示慎用，生成的字表较大，会占用较多空间，FLASH空间8K以下慎用)
									delay_ms(1500);
									OSTaskResume(&Task7_TCB,&err);//恢复任务7运行
									times=0;
									//设置事件标志组 //告诉主页面已解锁
									OSFlagPost(&g_flag_grp,0x04,OS_OPT_POST_FLAG_SET,&err);											
								}	
								else
								{
									blue_flag=0;//记录标志位清0
								}
								
					
								
						}
						else //数据超过100已满清空扇区
						{
							blue_flag=0;//记录标志位清0
							flash_erase_record(FLASH_Sector_4);//擦除扇区4记录	
							OLED_Clear();//关闭显示
							OLED_ShowCHinese(28,2,4);//解
							OLED_ShowCHinese(46,2,5);//锁
							OLED_ShowCHinese(64,2,15);//数
							OLED_ShowCHinese(82,2,16);//据
							OLED_ShowCHinese(20,4,14);//已
							OLED_ShowCHinese(38,4,17);//满
							OLED_ShowCHinese(56,4,14);//已
							OLED_ShowCHinese(74,4,18);//清
							OLED_ShowCHinese(92,4,19);//空
							delay_ms(5000);				
						}
						
					}
					if(a!=b)
					{
						//反发给用户蓝牙，提示密码错误
						usart3_send_str("password error");	
					 }
				 }
			}	
			//通过手机蓝牙查询解锁信息
			if(strstr((const char *)p,"inquire"))
			{
				//尝试获取100条记录
				for(i=0;i<100;i++)
				{
					//获取存储闪存扇区4的记录
					flash_read_record(buf,i,ADDR_FLASH_SECTOR_4);
					
					//检查记录是否存在换行符号，不存在则不打印输出
					if(strstr((char *)buf,"\n")==0)
						break;	
					//将查询结果反发给用户蓝牙
					 usart3_send_str((char *)buf);
					//打印记录
					//printf(buf);
					memset(buf,0,sizeof buf);					
					
				}		
				//如果i等于0，代表没有一条记录
				if(i==0)
				{
					printf("There is no record\r\n");
						//将没有记录反发给用户蓝牙
					 usart3_send_str("There is no record");				
				 }				
				
			}
			//通过手机蓝牙修改密码
			if(strstr((const char *)p,"set-"))
			{
				
				//例："set_password-123456#"
				password = strtok((char *)p,"-");//分解password等于set
				password = strtok(NULL,"-");//继续分解password等于123456#
				if(strcmp(password,"#")==0)
				{
					//反发给用户蓝牙，提示密码不能为空
					usart3_send_str("The password cannot be empty");
				}
				delchar(password,"#"); //利用自定义函数删除‘#’得到密码
				if(strlen(password)>6)
				{
					//反发给用户蓝牙，提示密码过长
					usart3_send_str("Set the password too long");					
				}	
				else
				{	
					
					/*解锁FLASH，允许操作FLASH*/
					FLASH_Unlock();
					/* 清空相应的标志位*/  
					FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | 
					FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR|FLASH_FLAG_PGSERR);								
					flash_erase_record(FLASH_Sector_6);//擦除扇区6记录		
					//写入闪存扇区6记录开锁的密码
					if(0==flash_write_record(password,1,ADDR_FLASH_SECTOR_6,FLASH_Sector_6))
					{	
						//printf("%s\r\n",password);
						//printf("set_password succeed\r\n");
						//反发给用户蓝牙，提示设置密码成功
						usart3_send_str("set_password succeed");
						OLED_Clear();//关闭显示
						OLED_ShowCHinese(10,4,20);//密
						OLED_ShowCHinese(28,4,21);//码
						OLED_ShowCHinese(46,4,22);//修
						OLED_ShowCHinese(64,4,23);//改
						OLED_ShowCHinese(82,4,24);//成
						OLED_ShowCHinese(100,4,25);//功						
						delay_ms(2000);	
						//清空数据
						memset(password,0,sizeof password);
						
					}	
				}			
			 }	
			//printf("msg[%s][%d]\r\n",p,msg_size);//打印信息
			memset(p,0,msg_size);//清空数据p
			//立即释放互斥量
			OSMutexPost(&g_mutex_oled,OS_OPT_POST_NONE,&err);
			 OSTaskResume(&Task5_TCB,&err);

		}
			
	}
	
}

//任务二显示主页：时间日期等
void task2(void *parg)
{
	
	OS_ERR err;//错误码
	OS_FLAGS flags=0;
	u8 time[20]; //存放时间
	u8 date[20];//存放日期
	uint32_t flag=0;
	//存放日期
	while(1)
	{
		//一直阻塞等待事件标志组bit0(RTC中断标志位)置1  bit1  bit2，等待成功后，将bit0清0
		flags=OSFlagPend(&g_flag_grp,0x01|0x02|0x04,0,OS_OPT_PEND_FLAG_SET_ANY + OS_OPT_PEND_FLAG_CONSUME+OS_OPT_PEND_BLOCKING,NULL,&err);				
		//阻塞等待互斥量
		OSMutexPend(&g_mutex_oled,0,OS_OPT_PEND_BLOCKING,NULL,&err);
	
		if(flags)
		{
			OLED_Clear();
			//RTC_GetTime，获取时间
			RTC_GetTime(RTC_Format_BCD, &RTC_TimeStructure); 			
			//printf("TIME:%02x:%02x:%02x\r\n",RTC_TimeStructure.RTC_Hours,RTC_TimeStructure.RTC_Minutes,RTC_TimeStructure.RTC_Seconds);
			sprintf((char *)time,"%02x:%02x:%02x",RTC_TimeStructure.RTC_Hours,RTC_TimeStructure.RTC_Minutes,RTC_TimeStructure.RTC_Seconds);
			//RTC_GetDate，获取日期
			RTC_GetDate(RTC_Format_BCD, &RTC_DateStructure);
			//printf("DATE:20%02x/%02x/%02x %0x\r\n",RTC_DateStructure.RTC_Year,RTC_DateStructure.RTC_Month,RTC_DateStructure.RTC_Date,RTC_DateStructure.RTC_WeekDay);
			sprintf((char *)date,"20%02x/%02x/%02x",RTC_DateStructure.RTC_Year,RTC_DateStructure.RTC_Month,RTC_DateStructure.RTC_Date);		
			OLED_ShowCHinese(18,0,6);//智
			OLED_ShowCHinese(36,0,7);//能
			OLED_ShowCHinese(54,0,8);//指
			OLED_ShowCHinese(72,0,9);//纹
			OLED_ShowCHinese(90,0,5);//锁
			OLED_ShowString(0,6,"B");				
			OLED_ShowCHinese(10,6,20);//密
			OLED_ShowCHinese(23,6,21);//码	
			OLED_ShowCHinese(36,6,4);//解
			OLED_ShowCHinese(49,6,5);//锁			
			OLED_ShowString(62,6,"C");		
			OLED_ShowCHinese(72,6,83);//指
			OLED_ShowCHinese(85,6,84);//纹
			OLED_ShowCHinese(98,6,96);//管
			OLED_ShowCHinese(111,6,97);//理
			
			OLED_ShowString(54,4,time);  //显示实时时间
			OLED_ShowString(48,2,date); //显示实时日期
			OLED_DrawBMP(0,2,30,4,BMP3);//显示主页小锁定
			OLED_ShowCHinese(0,4,5);//锁
			OLED_ShowCHinese(15,4,3);//定
			OLED_ShowCHinese(30,4,13);//中		

			if(flags &0x02||flag==1)
			{
				
				flag=1;
				OLED_DrawBMP(0,2,30,4,BMP3);//显示主页小锁定
				OLED_ShowCHinese(0,4,5);//锁
				OLED_ShowCHinese(13,4,3);//定
				OLED_ShowCHinese(28,4,13);//中
			}
			if(flags &0x04||flag==2)
			{			
				
				
				flag=2;
				OLED_DrawBMP(0,2,30,4,BMP4);//显示主页小解锁
				OLED_ShowCHinese(0,4,14);//已
				OLED_ShowCHinese(13,4,4);//解
				OLED_ShowCHinese(28,4,5);//锁
			}		
	
		}
		flags=0;
			//释放信号量
			OSSemPost(&g_sem,OS_OPT_POST_1,&err); 			
			//立即释放互斥量
			OSMutexPost(&g_mutex_oled,OS_OPT_POST_NONE,&err);
	
	}
	
}
//任务三 按键函数运行
void task3(void *parg)
{
	while(1)
	{	
			delay_ms(10);
			get_key();

	}		
	 
}
//任务四 按键操作
void task4(void *parg)
{
	OS_ERR err;
	OS_MSG_SIZE msg_size;
	char *key;//存放消息队列发来的按键数据
	char keybuf[60]={0};//存放按键数据
	uint32_t i=0; //控制数字在OLED显示的位置及存放地址
	int a=0;//用于开锁验证密码
	int b=0;//用于开锁验证密码	
	uint32_t rt; //密码界面操作函数返回值判断
	while(1)
	{	
	
		
		//等待按键消息的队列
		key=OSQPend(&key_queue,0,OS_OPT_PEND_BLOCKING,&msg_size,NULL,&err);	
		
		if(key && msg_size)
		{
			if(strcmp(key,"A")==0) //按键坏了
			{	
				printf("A\r\n");
			}
			
			if(strcmp(key,"B")==0) //进入密码解锁界面
			{	
				memset(keybuf,0,strlen(keybuf));//清空数据buf	
				i=0;//清空i							
				OSTaskSuspend(&Task5_TCB,&err); //暂停任务5运行
				//printf("B\r\n");
				//阻塞等待信号量  //同步
				OSSemPend(&g_sem,0,OS_OPT_PEND_BLOCKING,NULL,&err);					
				//阻塞等待互斥量  //占用锁输入密码 
				OSMutexPend(&g_mutex_oled,0,OS_OPT_PEND_BLOCKING,NULL,&err);	
				OLED_Clear();//关闭显示				
				OLED_ShowCHinese(18,0,26);//请
				OLED_ShowCHinese(36,0,27);//输
				OLED_ShowCHinese(54,0,28);//入
				OLED_ShowCHinese(72,0,20);//密
				OLED_ShowCHinese(90,0,21);//码
				OLED_ShowString(18,4,"-----------");
				OLED_ShowString(0,6,"#:");//#:
				OLED_ShowCHinese(14,6,29);//确
				OLED_ShowCHinese(28,6,30);//定
				OLED_ShowString(42,6,"C:");//#:			
				OLED_ShowCHinese(56,6,31);//删
				OLED_ShowCHinese(70,6,32);//除
				OLED_ShowString(84,6,"D:");//D:		
				OLED_ShowCHinese(98,6,33);//退
				OLED_ShowCHinese(112,6,34);//出	
				while(1)
				{
					rt=getpassword_key();
					if(rt==1)
					{	
					//	printf("1\r\n");
						keybuf[i]='1';		
						i++;
						OLED_ShowString(i*18,2,"1");						
					}	
					if(rt==4)
					{	
					//	printf("4\r\n");
						keybuf[i]='4';							
						i++;
						OLED_ShowString(i*18,2,"4");						
					}
					if(rt==7)
					{	
					//	printf("7\r\n");
						keybuf[i]='7';					
						i++;		
						OLED_ShowString(i*18,2,"7");					
					}
					if(rt==2)
					{	
					//	printf("2\r\n");
						keybuf[i]='2';						
						i++;	
						OLED_ShowString(i*18,2,"2");				
					}
					if(rt==5)
					{	
					//	printf("5\r\n");
						keybuf[i]='5';						
						i++;		
						OLED_ShowString(i*18,2,"5");				
					}			
					if(rt==8)
					{	
						//printf("8\r\n");
						keybuf[i]='8';				
						i++;
						OLED_ShowString(i*18,2,"8");						
					}		
					if(rt==0)
					{	
					//	printf("0\r\n");
						keybuf[i]='0';					
						i++;		
						OLED_ShowString(i*18,2,"0");					
					}			
					
					if(rt==3)
					{	
					//	printf("3\r\n");
						keybuf[i]='3';						
						i++;
						OLED_ShowString(i*18,2,"3");				
					}
					if(rt==6)
					{	
						//printf("6\r\n");
						keybuf[i]='6';					
						i++;
						OLED_ShowString(i*18,2,"6");					
					}			
					if(rt==9)
					{	
						//printf("9\r\n");
						keybuf[i]='9';					
						i++;		
						OLED_ShowString(i*18,2,"9");					
					}					
							
							
							
					if(rt==11)//密码解锁界面删除键  C
					{				
						
						OLED_Clear();//关闭显示				
						OLED_ShowCHinese(18,0,26);//请
						OLED_ShowCHinese(36,0,27);//输
						OLED_ShowCHinese(54,0,28);//入
						OLED_ShowCHinese(72,0,20);//密
						OLED_ShowCHinese(90,0,21);//码
						OLED_ShowString(18,4,"-----------");
						OLED_ShowString(0,6,"#:");//#:
						OLED_ShowCHinese(14,6,29);//确
						OLED_ShowCHinese(28,6,30);//定
						OLED_ShowString(42,6,"C:");//#:			
						OLED_ShowCHinese(56,6,31);//删
						OLED_ShowCHinese(70,6,32);//除
						OLED_ShowString(84,6,"D:");//D:		
						OLED_ShowCHinese(98,6,33);//退
						OLED_ShowCHinese(112,6,34);//出							
						memset(keybuf,0,strlen(keybuf));//清空数据buf	
						i=0;//清空i	
						
					}
					if(rt==12) //密码解锁界面退出键  D
					{	
						//printf("D\r\n");
						//立即释放互斥量   //退出
						OSMutexPost(&g_mutex_oled,OS_OPT_POST_NONE,&err);
						memset(keybuf,0,strlen(keybuf));//清空数据buf	
						i=0;//清空i		
						OSTaskResume(&Task5_TCB,&err); //恢复任务5运行		
						break;

					}		
					if(rt==13)  //密码输完后确认键
					{	
						//printf("#\r\n");

						//获取存储闪存扇区6的密码记录
						flash_read_record(password,1,ADDR_FLASH_SECTOR_6);			
						//printf(password);	
						//将从扇区3获取的密码转换为整型（用于比较），			
						a = atoi(password);	
						memset(password,0,sizeof password);//清空数据
						
						delchar(keybuf,"#"); //删除# 得到密码
						//printf("%s\r\n",keybuf);
						b = atoi(keybuf);
						
						//验证密码是否正确
						if(a==b)
						{
							//闪烁灯加蜂鸣器
							PFout(8)=1;PFout(9)=0;delay_ms(100);
							PFout(8)=0;PFout(9)=1;delay_ms(100);
							PFout(8)=1;PFout(9)=0;delay_ms(100);
							PFout(8)=0;PFout(9)=1;delay_ms(100);
							PEout(5)=1; //通电，触发锁  开锁
							//printf("密码解锁成功！\r\n");
							//检查当前的数据记录是否小于100条
								if(blue_flag<100)
								{
										/*解锁FLASH，允许操作FLASH*/
										FLASH_Unlock();
										/* 清空相应的标志位*/  
										FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | 
										FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR|FLASH_FLAG_PGSERR);										
										
										//RTC_GetDate，获取日期
										RTC_GetDate(RTC_Format_BCD, &RTC_DateStructure);
										//printf("DATE-OLED:20%02x/%02x/%02x %0x\r\n",RTC_DateStructure.RTC_Year,RTC_DateStructure.RTC_Month,RTC_DateStructure.RTC_Date,RTC_DateStructure.RTC_WeekDay);	
										//RTC_GetTime，获取时间
										RTC_GetTime(RTC_Format_BCD, &RTC_TimeStructure); 
										//printf("TIME-OLED:%02x:%02x:%02x\r\n",RTC_TimeStructure.RTC_Hours,RTC_TimeStructure.RTC_Minutes,RTC_TimeStructure.RTC_Seconds);								
										//打包存入闪存的数据
										sprintf((char *)buf,"[%03d]20%02x/%02x/%02x  %02x:%02x:%02x password unlock\r\n",\
															blue_flag,\
															RTC_DateStructure.RTC_Year, RTC_DateStructure.RTC_Month, RTC_DateStructure.RTC_Date,\
															RTC_TimeStructure.RTC_Hours, RTC_TimeStructure.RTC_Minutes, RTC_TimeStructure.RTC_Seconds);
										
										//写入闪存扇区4记录开锁时间及方式
										if(0==flash_write_record(buf,blue_flag,ADDR_FLASH_SECTOR_4,FLASH_Sector_4))
										{
											  //显示
											printf(buf);
											//记录自加1
											blue_flag++;
											//清空数据
											memset(buf,0,sizeof buf);
											OLED_Clear();//关闭显示
											OLED_ShowCHinese(32,4,20);//密
											OLED_ShowCHinese(50,4,21);//码
											OLED_ShowCHinese(68,4,4);//解
											OLED_ShowCHinese(86,4,5);//锁
											delay_ms(1000);				
											OLED_DrawBMP(0,2,128,8,BMP2);  //图片显示(图片显示慎用，生成的字表较大，会占用较多空间，FLASH空间8K以下慎用)
											delay_ms(1500);	
											OSTaskResume(&Task7_TCB,&err);//恢复任务7运行
											times=0;
											//设置事件标志组 //告诉主页面已解锁
											OSFlagPost(&g_flag_grp,0x04,OS_OPT_POST_FLAG_SET,&err);								
											memset(keybuf,0,strlen(keybuf));//清空数据buf	
											i=0;//清空i								
											//立即释放互斥量 输入#退出
											OSMutexPost(&g_mutex_oled,OS_OPT_POST_NONE,&err);	
											OSTaskResume(&Task5_TCB,&err); //恢复任务5运行			
											break;
											
										}	
										else
										{
											blue_flag=0;//记录标志位清0
											memset(keybuf,0,strlen(keybuf));//清空数据buf	
											i=0;//清空i								
											//立即释放互斥量 输入#退出
											OSMutexPost(&g_mutex_oled,OS_OPT_POST_NONE,&err);	
											OSTaskResume(&Task5_TCB,&err); //恢复任务5运行
											
											break;											
										}
										
								}
								else //数据超过100已满清空扇区
								{
									blue_flag=0;//记录标志位清0
									flash_erase_record(FLASH_Sector_4);//擦除扇区4记录	
									OLED_Clear();//关闭显示
									OLED_ShowCHinese(28,2,4);//解
									OLED_ShowCHinese(46,2,5);//锁
									OLED_ShowCHinese(64,2,15);//数
									OLED_ShowCHinese(82,2,16);//据
									OLED_ShowCHinese(20,4,14);//已
									OLED_ShowCHinese(38,4,17);//满
									OLED_ShowCHinese(56,4,14);//已
									OLED_ShowCHinese(74,4,18);//清
									OLED_ShowCHinese(92,4,19);//空
									delay_ms(5000);	
									memset(keybuf,0,strlen(keybuf));//清空数据buf	
									i=0;//清空i								
									//立即释放互斥量 输入#退出
									OSMutexPost(&g_mutex_oled,OS_OPT_POST_NONE,&err);	
									OSTaskResume(&Task5_TCB,&err); //恢复任务5运行
										
									break;
								}
						}
						else
						{
							OLED_Clear();//关闭显示	
							OLED_ShowCHinese(32,4,20);//密
							OLED_ShowCHinese(50,4,21);//码
							OLED_ShowCHinese(68,4,35);//错
							OLED_ShowCHinese(86,4,36);//误				
							delay_ms(1500);
							OLED_ShowCHinese(18,0,26);//请
							OLED_ShowCHinese(36,0,27);//输
							OLED_ShowCHinese(54,0,28);//入
							OLED_ShowCHinese(72,0,20);//密
							OLED_ShowCHinese(90,0,21);//码
							OLED_ShowString(18,4,"-----------");
							OLED_ShowString(0,6,"#:");//#:
							OLED_ShowCHinese(14,6,29);//确
							OLED_ShowCHinese(28,6,30);//定
							OLED_ShowString(42,6,"C:");//#:			
							OLED_ShowCHinese(56,6,31);//删
							OLED_ShowCHinese(70,6,32);//除
							OLED_ShowString(84,6,"D:");//D:		
							OLED_ShowCHinese(98,6,33);//退
							OLED_ShowCHinese(112,6,34);//出						
							memset(keybuf,0,strlen(keybuf));//清空数据buf	
							i=0;//清空i						
										
						}				
					}	
					if(strlen(keybuf)>6)
					{
						OLED_Clear();//关闭显示	
						OLED_ShowCHinese(32,4,20);//密
						OLED_ShowCHinese(50,4,21);//码
						OLED_ShowCHinese(68,4,47);//过
						OLED_ShowCHinese(86,4,48);//长					
						delay_ms(2000);
						OLED_Clear();//关闭显示				
						OLED_ShowCHinese(18,0,26);//请
						OLED_ShowCHinese(36,0,27);//输
						OLED_ShowCHinese(54,0,28);//入
						OLED_ShowCHinese(72,0,20);//密
						OLED_ShowCHinese(90,0,21);//码
						OLED_ShowString(18,4,"-----------");
						OLED_ShowString(0,6,"#:");//#:
						OLED_ShowCHinese(14,6,29);//确
						OLED_ShowCHinese(28,6,30);//定
						OLED_ShowString(42,6,"C:");//C:			
						OLED_ShowCHinese(56,6,31);//删
						OLED_ShowCHinese(70,6,32);//除
						OLED_ShowString(84,6,"D:");//D:		
						OLED_ShowCHinese(98,6,33);//退
						OLED_ShowCHinese(112,6,34);//出							
						memset(keybuf,0,strlen(keybuf));//清空数据buf	
						i=0;//清空i			
					}
					
				}		
			}	
			if(strcmp(key,"C")==0) //录指纹
			{
				OSTaskSuspend(&Task7_TCB,&err); //挂起任务7  蓝牙断开60秒自动锁定
				OSTaskSuspend(&Task5_TCB,&err); //暂停任务5运行
				//printf("B\r\n");
				//阻塞等待信号量  //同步
				OSSemPend(&g_sem,0,OS_OPT_PEND_BLOCKING,NULL,&err);						
				//阻塞等待互斥量  //占用锁
				OSMutexPend(&g_mutex_oled,0,OS_OPT_PEND_BLOCKING,NULL,&err);	
				OLED_Clear();//关闭显示	
				OLED_ShowString(20,2,"B:");//B
				OLED_ShowCHinese(38,2,81); //录
				OLED_ShowCHinese(56,2,82); //入
				OLED_ShowCHinese(74,2,83); //指
				OLED_ShowCHinese(92,2,84); //纹	
				OLED_ShowString(20,4,"C:");//C
				OLED_ShowCHinese(38,4,92); //删
				OLED_ShowCHinese(56,4,93); //除
				OLED_ShowCHinese(74,4,83); //指
				OLED_ShowCHinese(92,4,84); //纹		
				OLED_ShowString(20,6,"D:");//D
				OLED_ShowCHinese(38,6,33); //退
				OLED_ShowCHinese(56,6,34); //出				
				while(1) //运行 指纹管理界面获取键值函数来获取键值
				{	
					rt=getfingerprint_key();
					if(rt==1) //B//录指纹
					{			
						printf("1\r\n");						
						if(Add_FR()==1);//录指纹
						{
							OLED_ShowString(20,2,"B:");//B
							OLED_ShowCHinese(38,2,81); //录
							OLED_ShowCHinese(56,2,82); //入
							OLED_ShowCHinese(74,2,83); //指
							OLED_ShowCHinese(92,2,84); //纹	
							OLED_ShowString(20,4,"C:");//C
							OLED_ShowCHinese(38,4,92); //删
							OLED_ShowCHinese(56,4,93); //除
							OLED_ShowCHinese(74,4,83); //指
							OLED_ShowCHinese(92,4,84); //纹		
							OLED_ShowString(20,6,"D:");//D
							OLED_ShowCHinese(38,6,33); //退
							OLED_ShowCHinese(56,6,34); //出							
						}
					}
					if(rt==2)//	C删除指纹				
					{		
						printf("2\r\n");						
						if(Del_FR()==1)//删除指纹成功
						{	
							OLED_ShowString(20,2,"B:");//B
							OLED_ShowCHinese(38,2,81); //录
							OLED_ShowCHinese(56,2,82); //入
							OLED_ShowCHinese(74,2,83); //指
							OLED_ShowCHinese(92,2,84); //纹	
							OLED_ShowString(20,4,"C:");//C
							OLED_ShowCHinese(38,4,92); //删
							OLED_ShowCHinese(56,4,93); //除
							OLED_ShowCHinese(74,4,83); //指
							OLED_ShowCHinese(92,4,84); //纹		
							OLED_ShowString(20,6,"D:");//D
							OLED_ShowCHinese(38,6,33); //退
							OLED_ShowCHinese(56,6,34); //出	
						}
						else //删除指纹失败
						{
							OLED_ShowString(20,2,"B:");//B
							OLED_ShowCHinese(38,2,81); //录
							OLED_ShowCHinese(56,2,82); //入
							OLED_ShowCHinese(74,2,83); //指
							OLED_ShowCHinese(92,2,84); //纹	
							OLED_ShowString(20,4,"C:");//C
							OLED_ShowCHinese(38,4,92); //删
							OLED_ShowCHinese(56,4,93); //除
							OLED_ShowCHinese(74,4,83); //指
							OLED_ShowCHinese(92,4,84); //纹		
							OLED_ShowString(20,6,"D:");//D
							OLED_ShowCHinese(38,6,33); //退
							OLED_ShowCHinese(56,6,34); //出				
						}						
						
					}
					if(rt==3) //D退出
					{
						//立即释放互斥量   //退出
						OSMutexPost(&g_mutex_oled,OS_OPT_POST_NONE,&err);						
						OSTaskResume(&Task5_TCB,&err); //恢复任务5运行
										
						break;
					}	
					
				}		
			}

		}	
	
		memset(key,0,msg_size);//清空数据p
	

	}		
	
}
//任务五  RFID操作
void task5(void *parg)
{

	OS_ERR err;
	int card_buf[5]={0};//存放卡号的10进制  将卡号转成10进制
	int card_tset[5]={105,149,113,193,76};//用于对比卡号是否相等
	int flag=0; //对比标志位
	while(1)
	{
		delay_ms(20);
		MFRC522_Initializtion();//初始化RFID
		status=MFRC522_Request(0x52, card_pydebuf);			//寻卡	

		//阻塞等待信号量  //同步
		OSSemPend(&g_sem,0,OS_OPT_PEND_BLOCKING,NULL,&err);		
		//阻塞等待互斥量  //占用锁输入密码 
		OSMutexPend(&g_mutex_oled,0,OS_OPT_PEND_BLOCKING,NULL,&err);			
		if(status==0)		//如果读到卡
		{
			status=MFRC522_Anticoll(card_numberbuf);			//防撞处理			
			card_size=MFRC522_SelectTag(card_numberbuf);	//选卡  返回卡的容量
			status=MFRC522_Auth(0x60, 4, card_key0Abuf, card_numberbuf);	//验卡
			status=MFRC522_Write(4, card_writebuf);				//写卡（写卡要小心，特别是各区的块3）
			status=MFRC522_Read(4, card_readbuf);					//读卡
			
			//卡类型显示		
			printf("card_pydebuf:%02X %02X\r\n",card_pydebuf[0],card_pydebuf[1]);
			
			//卡序列号显示，最后一字节为卡的校验码
			printf("card_numberbuf:");
			for(i=0;i<5;i++)
			{
				card_buf[i]=card_numberbuf[i]; //将字符型卡号转成int型 方便检验对比
				//printf("h:%d ",card_buf[i]);
			}		
		//	printf("\r\n");
			
			for(i=0;i<5;i++)
			{
					//printf("j:%d ",card_tset[i]);
					if(card_buf[i]==card_tset[i])  //判断是不是我规定的卡号
					{
						flag++; //对比标志位
					}
			}	
		//	printf("\r\n");			
		//	printf("flag:%d\r\n",flag);	
			
			//卡容量显示，单位为Kbits
			printf("card_size=%dKbits\r\n",card_size);
			
			//读卡状态显示，正常为0
			printf("status:%02X\r\n",status);
			
			//读一个块的数据显示
			printf("card_readbuf:");
			for(i=0;i<8;i++)		//分两行显示
			{
				printf("%02X ",card_readbuf[i]);
			}			
			printf("\r\n");
			if(strcmp((const char*)card_readbuf,(const char*)card_writebuf)==0&&flag==5) //如果写入的和读出来的一样且卡号等于设定好的卡号 就开锁成功
			{
					//闪烁灯加蜂鸣器
					PFout(8)=1;PFout(9)=0;delay_ms(100);
					PFout(8)=0;PFout(9)=1;delay_ms(100);
					PFout(8)=1;PFout(9)=0;delay_ms(100);
					PFout(8)=0;PFout(9)=1;delay_ms(100);	
					PEout(5)=1; //通电，触发锁  开锁				
					flag=0;	//清空对比标志位						
				  //检查当前的数据记录是否小于100条
					if(blue_flag<100)
					{
						/*解锁FLASH，允许操作FLASH*/
						FLASH_Unlock();
						/* 清空相应的标志位*/  
						FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | 
						FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR|FLASH_FLAG_PGSERR);										
									
						//RTC_GetDate，获取日期
						RTC_GetDate(RTC_Format_BCD, &RTC_DateStructure);
						//printf("DATE-OLED:20%02x/%02x/%02x %0x\r\n",RTC_DateStructure.RTC_Year,RTC_DateStructure.RTC_Month,RTC_DateStructure.RTC_Date,RTC_DateStructure.RTC_WeekDay);	
						//RTC_GetTime，获取时间
						RTC_GetTime(RTC_Format_BCD, &RTC_TimeStructure); 
						//printf("TIME-OLED:%02x:%02x:%02x\r\n",RTC_TimeStructure.RTC_Hours,RTC_TimeStructure.RTC_Minutes,RTC_TimeStructure.RTC_Seconds);								
						//打包存入闪存的数据
						sprintf((char *)buf,"[%03d]20%02x/%02x/%02x  %02x:%02x:%02x RFID unlock\r\n",\
														blue_flag,\
														RTC_DateStructure.RTC_Year, RTC_DateStructure.RTC_Month, RTC_DateStructure.RTC_Date,\
														RTC_TimeStructure.RTC_Hours, RTC_TimeStructure.RTC_Minutes, RTC_TimeStructure.RTC_Seconds);
									
						//写入闪存扇区4记录开锁时间及方式
						if(0==flash_write_record(buf,blue_flag,ADDR_FLASH_SECTOR_4,FLASH_Sector_4))
						{
								//显示
								printf(buf);
								//记录自加1
								blue_flag++;
								//清空数据
								memset(buf,0,sizeof buf);
								OLED_Clear();//关闭显示
								OLED_ShowString(36,4,"RFID");//RFID
								OLED_ShowCHinese(68,4,4);//解
								OLED_ShowCHinese(86,4,5);//锁
								delay_ms(1000);				
								OLED_DrawBMP(0,2,128,8,BMP2);  //图片显示(图片显示慎用，生成的字表较大，会占用较多空间，FLASH空间8K以下慎用)
								delay_ms(1500);			
								OSTaskResume(&Task7_TCB,&err);//恢复任务7运行
								times=0;
								//设置事件标志组 //告诉主页面已解锁
								OSFlagPost(&g_flag_grp,0x04,OS_OPT_POST_FLAG_SET,&err);								
								memset(card_readbuf,0,strlen((char*)card_readbuf));//清空数据card_readbuf								
						}	
						else
						{
							blue_flag=0;//记录标志位清0
						}
									
					}
					else //数据超过100已满清空扇区
					{
						blue_flag=0;//记录标志位清0
						flash_erase_record(FLASH_Sector_4);//擦除扇区4记录	
						OLED_Clear();//关闭显示
						OLED_ShowCHinese(28,2,4);//解
						OLED_ShowCHinese(46,2,5);//锁
						OLED_ShowCHinese(64,2,15);//数
						OLED_ShowCHinese(82,2,16);//据
						OLED_ShowCHinese(20,4,14);//已
						OLED_ShowCHinese(38,4,17);//满
						OLED_ShowCHinese(56,4,14);//已
						OLED_ShowCHinese(74,4,18);//清
						OLED_ShowCHinese(92,4,19);//空
						delay_ms(5000);				
					}
							
			}
			else//如果写入的和读出来的不一样就开锁失败
			{
				//printf("开锁失败！\r\n"); 
				OLED_Clear();//关闭显示	
				OLED_ShowString(18,4,"RFID");//RFID
				OLED_ShowCHinese(68,4,35);//错
				OLED_ShowCHinese(86,4,36);//误				
				delay_ms(1500);	
				OLED_Clear();//关闭显示		
			}
		
			
		}	
		//立即释放互斥量 输入#退出
		OSMutexPost(&g_mutex_oled,OS_OPT_POST_NONE,&err);		
	
	}
}

//任务六  as608指纹模块操作
void task6(void *parg)
{
	OS_ERR err;
	while(1)
	{
		//阻塞等待信号量  //同步
		OSSemPend(&g_sem,0,OS_OPT_PEND_BLOCKING,NULL,&err);	
					
		if(PS_Sta)	 //检测PS_Sta状态，如果有手指按下
		{	
			//阻塞等待互斥量  //占用锁
			OSMutexPend(&g_mutex_oled,0,OS_OPT_PEND_BLOCKING,NULL,&err);				
			if(press_FR()==0)//刷指纹
			{
					//闪烁灯加蜂鸣器
					PFout(8)=1;PFout(9)=0;delay_ms(100);
					PFout(8)=0;PFout(9)=1;delay_ms(100);
					PFout(8)=1;PFout(9)=0;delay_ms(100);
					PFout(8)=0;PFout(9)=1;delay_ms(100);
					PEout(5)=1; //通电，触发锁  开锁
					//printf("密码解锁成功！\r\n");
					//检查当前的数据记录是否小于100条
						if(blue_flag<100)
						{
								/*解锁FLASH，允许操作FLASH*/
								FLASH_Unlock();
								/* 清空相应的标志位*/  
								FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | 
								FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR|FLASH_FLAG_PGSERR);										
								
								//RTC_GetDate，获取日期
								RTC_GetDate(RTC_Format_BCD, &RTC_DateStructure);
								//printf("DATE-OLED:20%02x/%02x/%02x %0x\r\n",RTC_DateStructure.RTC_Year,RTC_DateStructure.RTC_Month,RTC_DateStructure.RTC_Date,RTC_DateStructure.RTC_WeekDay);	
								//RTC_GetTime，获取时间
								RTC_GetTime(RTC_Format_BCD, &RTC_TimeStructure); 
								//printf("TIME-OLED:%02x:%02x:%02x\r\n",RTC_TimeStructure.RTC_Hours,RTC_TimeStructure.RTC_Minutes,RTC_TimeStructure.RTC_Seconds);								
								//打包存入闪存的数据
								sprintf((char *)buf,"[%03d]20%02x/%02x/%02x  %02x:%02x:%02x fingerprint unlock\r\n",\
													blue_flag,\
													RTC_DateStructure.RTC_Year, RTC_DateStructure.RTC_Month, RTC_DateStructure.RTC_Date,\
													RTC_TimeStructure.RTC_Hours, RTC_TimeStructure.RTC_Minutes, RTC_TimeStructure.RTC_Seconds);
								
								//写入闪存扇区4记录开锁时间及方式
								if(0==flash_write_record(buf,blue_flag,ADDR_FLASH_SECTOR_4,FLASH_Sector_4))
								{
									  //显示
									printf(buf);
									//记录自加1
									blue_flag++;
									//清空数据
									memset(buf,0,sizeof buf);
									OLED_Clear();//关闭显示
									OLED_ShowCHinese(32,4,83);//指
									OLED_ShowCHinese(50,4,84);//纹
									OLED_ShowCHinese(68,4,4);//解
									OLED_ShowCHinese(86,4,5);//锁
									delay_ms(1000);				
									OLED_DrawBMP(0,2,128,8,BMP2);  //图片显示(图片显示慎用，生成的字表较大，会占用较多空间，FLASH空间8K以下慎用)
									delay_ms(1500);	
									OSTaskResume(&Task7_TCB,&err);//恢复任务7运行
									times=0;
									//设置事件标志组 //告诉主页面已解锁
									OSFlagPost(&g_flag_grp,0x04,OS_OPT_POST_FLAG_SET,&err);																			
								}	
								else
								{
									blue_flag=0;//记录标志位清0
								}
								
						}
						else //数据超过100已满清空扇区
						{
							blue_flag=0;//记录标志位清0
							flash_erase_record(FLASH_Sector_4);//擦除扇区4记录	
							OLED_Clear();//关闭显示
							OLED_ShowCHinese(28,2,4);//解
							OLED_ShowCHinese(46,2,5);//锁
							OLED_ShowCHinese(64,2,15);//数
							OLED_ShowCHinese(82,2,16);//据
							OLED_ShowCHinese(20,4,14);//已
							OLED_ShowCHinese(38,4,17);//满
							OLED_ShowCHinese(56,4,14);//已
							OLED_ShowCHinese(74,4,18);//清
							OLED_ShowCHinese(92,4,19);//空
							delay_ms(5000);				
						}				
				//立即释放互斥量 输入#退出
				OSMutexPost(&g_mutex_oled,OS_OPT_POST_NONE,&err);					
			}
			else
			{
				OLED_Clear(); 
				OLED_ShowCHinese(28,3,83); //指
				OLED_ShowCHinese(46,3,84); //纹
				OLED_ShowCHinese(64,3,35); //错
				OLED_ShowCHinese(82,3,36); //误
				ShowErrMessage(ensure);	
				delay_ms(1000);				
				//立即释放互斥量 输入#退出
				OSMutexPost(&g_mutex_oled,OS_OPT_POST_NONE,&err);				
			}
		}	
				
	} 	
	

}
//任务7 断开蓝牙60秒自动锁定
void task7(void *parg)
{
		OS_ERR err;	
		OS_FLAGS flags=0;
		while(1)
		{	
			//阻塞等待信号量  //同步
			OSSemPend(&g_sem,0,OS_OPT_PEND_BLOCKING,NULL,&err);						
			//一直阻塞等待事件标志组bit0(RTC中断标志位)置1  bit1  bit2，等待成功后，将bit0清0
			flags=OSFlagPend(&g_flag_grp,0x08,0,OS_OPT_PEND_FLAG_SET_ANY + OS_OPT_PEND_FLAG_CONSUME+OS_OPT_PEND_BLOCKING,NULL,&err);				
			times++;
			printf("times=%d\r\n",times);
			if(flags &0x08)
			{				
				if(PGin(9)!=1&&times==10) //如果检测超过10s没有蓝牙连接 自动锁定
				{				
							//阻塞等待互斥量  //占用锁输入密码 
							OSMutexPend(&g_mutex_oled,0,OS_OPT_PEND_BLOCKING,NULL,&err);						
							//闪烁灯加蜂鸣器
							PFout(8)=1;PFout(9)=0;delay_ms(100);
							PFout(8)=0;PFout(9)=1;delay_ms(100);
							PFout(8)=1;PFout(9)=0;delay_ms(100);
							PFout(8)=0;PFout(9)=1;delay_ms(100);
							PEout(5)=0; //断电，松开锁  关锁						
							OLED_Clear();//关闭显示
							OLED_ShowCHinese(32,4,98);//自
							OLED_ShowCHinese(50,4,99);//动
							OLED_ShowCHinese(68,4,100);//锁
							OLED_ShowCHinese(86,4,101);//定
							delay_ms(1000);
							OLED_DrawBMP(0,2,128,8,BMP1);  //图片显示(图片显示慎用，生成的字表较大，会占用较多空间，FLASH空间8K以下慎用)
							delay_ms(2000);
							
							
							//设置事件标志组  告诉主界面已锁定
							OSFlagPost(&g_flag_grp,0x02,OS_OPT_POST_FLAG_SET,&err);					
							//立即释放互斥量 输入#退出
							OSMutexPost(&g_mutex_oled,OS_OPT_POST_NONE,&err);	
							OSTaskSuspend(&Task7_TCB,&err); //挂起任务7  蓝牙断开60秒自动锁定
				}	
				if(PGin(9)==1)
				{
					times=0;
				}	
			}
		}	
}
