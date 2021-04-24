#include "sys.h"
#include "usart.h"	
#include "delay.h"
#include "includes.h"					//ucos 使用	  


static GPIO_InitTypeDef GPIO_InitStructure;
static USART_InitTypeDef USART_InitStructure;
static NVIC_InitTypeDef NVIC_InitStructure;

//加入以下代码,支持printf函数,而不需要选择use MicroLIB	  

#pragma import(__use_no_semihosting)             
//标准库需要的支持函数                 
struct __FILE 
{ 
	int handle; 
}; 

FILE __stdout;       
//定义_sys_exit()以避免使用半主机模式    
_sys_exit(int x) 
{ 
	x = x; 
} 
//重定义fputc函数 
int fputc(int ch, FILE *f)
{ 	
	while((USART1->SR&0X40)==0);//循环发送,直到发送完毕   
	USART1->DR = (u8) ch;      
	return ch;
}

 
//初始化IO 串口1 
//baud:波特率
void usart1_init(u32 baud)
{

	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,ENABLE); //使能GPIOA时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1,ENABLE);//使能USART1时钟
 
	//串口1对应引脚复用映射
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource9,GPIO_AF_USART1); //GPIOA9复用为USART1
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource10,GPIO_AF_USART1); //GPIOA10复用为USART1
	
	//USART1端口配置
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10; //GPIOA9与GPIOA10
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//复用功能
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//速度50MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //推挽复用输出
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //上拉
	GPIO_Init(GPIOA,&GPIO_InitStructure); //初始化PA9，PA10

   //USART1 初始化设置
	USART_InitStructure.USART_BaudRate = baud;//波特率设置
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式
	USART_Init(USART1, &USART_InitStructure); //初始化串口1
	
	USART_Cmd(USART1, ENABLE);  //使能串口1 
	
	USART_ClearFlag(USART1, USART_FLAG_TC);
	

	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);//开启相关中断

	//Usart1 NVIC 配置
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;//串口1中断通道
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3;//抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =3;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器、

	
}
//初始化串口3
void usart3_init(uint32_t baud)
{
	

	//使能串口3硬件时钟
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE); 
	
	//使能端口B的硬件时钟
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	
	//配置PB10 PB11为复用功能模式
	
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_10|GPIO_Pin_11;	//第10 11个引脚
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_AF;		//复用功能模式
	GPIO_InitStructure.GPIO_Speed=GPIO_High_Speed;	//引脚高速工作，收到指令立即工作；缺点：功耗高
	GPIO_InitStructure.GPIO_OType=GPIO_OType_PP;	//增加输出电流的能力
	GPIO_InitStructure.GPIO_PuPd=GPIO_PuPd_NOPULL;	//不需要上下拉电阻
	GPIO_Init(GPIOB,&GPIO_InitStructure);
	
	//将PB10和PB11连接到串口3硬件
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource10, GPIO_AF_USART3);	
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource11, GPIO_AF_USART3);		

	//配置串口3：波特率，校验位、数据位、停止位
	USART_InitStructure.USART_BaudRate = baud;						//波特率
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;		//数据位
	USART_InitStructure.USART_StopBits = USART_StopBits_1;			//1个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;				//无校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//取消流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;//能够收发数据
	USART_Init(USART3,&USART_InitStructure);
	
	
	//配置串口3的中断触发方式：接收完字节触发中断
	USART_ITConfig(USART3,USART_IT_RXNE , ENABLE);
	
	
	//使能串口3工作	
	USART_Cmd(USART3, ENABLE);
	
	
	//配置NVIC管理串口3
	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;		//串口1的中断号
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x02;//抢占优先级 0x2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x02;//响应优先级 0x2
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;		//NVIC使能串口1中断请求通道
	NVIC_Init(&NVIC_InitStructure);	
}
//发送数据给蓝牙
void usart3_send_str(char *str)
{
	char *p = str;
	
	while(p && (*p!='\0'))
	{
	
		USART_SendData(USART3,*p);
		while(USART_GetFlagStatus(USART3,USART_FLAG_TXE)==RESET);	
		p++;
	}
}
	//蓝牙模块配置
void bluetooth_config(void)
{
	usart3_send_str("AT\r\n");
	delay_ms(500);	
	usart3_send_str("AT+NAMEHJH\r\n");
	delay_ms(500);	

}
	//蓝牙MCU引脚(判断蓝牙是否连接)初始化    连上出现低电平    断开出现高电平
void bluetooth_MCU(void)
{

	//打开端口A的硬件时钟，就是对端口G供电
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG, ENABLE);	
	
	//GPIOF9,F10初始化设置 
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_9 ;		//PC9
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN;			    //输入模式，
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;				//推挽输出，驱动LED需要电流驱动
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;		    //100MHz
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;				    //上拉
	GPIO_Init(GPIOG, &GPIO_InitStructure);						//初始化GPIOF，把配置的数据写入寄存器
	
}
extern OS_Q	usart1_queue;//外部申明串口一消息队列对象
 static volatile uint8_t  	g_usart1_recv_buf[128]={0}; //存放获取的蓝牙数据
 static volatile uint32_t 	g_usart1_recv_cnt = 0;  //数据数量标志位
 static volatile uint32_t 	g_usart1_event=0; //检测标志位
 
//串口一中断
void USART1_IRQHandler(void)                	//串口1中断服务程序
{
	uint8_t d=0;
	OS_ERR err;
	//进入中断
	OSIntEnter();    

	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)  //接收中断(接收到的数据必须是0x0d 0x0a结尾)
	{
		//接收串口数据
		d=USART_ReceiveData(USART1);	
		g_usart1_recv_buf[g_usart1_recv_cnt]=d;//存入数组
		g_usart1_recv_cnt++;//记录有多少个数据
		
		//检测到'#'符或接收的数据满的时候则发送数据
		if(d=='#' || g_usart1_recv_cnt>=(sizeof g_usart1_recv_buf))
		{
			g_usart1_event = 1;
		
		}			
		//清空串口接收中断标志位
		USART_ClearITPendingBit(USART1, USART_IT_RXNE);
	} 
	
	//退出中断
	OSIntExit(); 
	//发送消息  (消息队列)
	if(g_usart1_event)
	{
		//发送消息函数
		OSQPost(&usart1_queue,(void *)g_usart1_recv_buf,g_usart1_recv_cnt,OS_OPT_POST_FIFO,&err);
		
		//清空标志位
		g_usart1_recv_cnt=0;
		g_usart1_event=0;
	}
   	
} 


extern OS_Q	usart3_queue;//外部申明串口三消息队列对象
static volatile uint8_t  	g_usart3_recv_buf[128]={0}; //存放获取的蓝牙数据
static volatile uint32_t 	g_usart3_recv_cnt = 0;  //数据数量标志位
static volatile uint32_t 	g_usart3_event=0; //检测标志位
//串口3中断
void  USART3_IRQHandler(void)
{
	uint8_t d;

	OS_ERR err;
	//进入中断
	OSIntEnter(); 
	//检测标志位
	if(USART_GetITStatus(USART3,USART_IT_RXNE) == SET)
	{
		
		//接收数据
		d = USART_ReceiveData(USART3);
		
		g_usart3_recv_buf[g_usart3_recv_cnt]=d;//存入数组
		g_usart3_recv_cnt++;//记录有多少个数据
		
		
		//检测到'#'符或接收的数据满的时候则发送数据
		if(d=='#' || g_usart3_recv_cnt>=(sizeof g_usart3_recv_buf))
		{
			g_usart3_event = 1;
		
		}	
		
		//清空标志位	
		USART_ClearITPendingBit(USART3,USART_IT_RXNE);
	}
	//退出中断
	OSIntExit();   

	
	
	//发送消息  (消息队列)
	if(g_usart3_event)
	{

		//发送消息函数
		OSQPost(&usart3_queue,(void *)g_usart3_recv_buf,g_usart3_recv_cnt,OS_OPT_POST_FIFO,&err); 	
		//清空标志位
		g_usart3_recv_cnt=0;
		g_usart3_event=0;
		if(err != OS_ERR_NONE)
		{
			printf("OSQPost error code %d\r\n",err);
		}
	}
}



