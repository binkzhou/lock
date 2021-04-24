#include "key.h"
#include "sys.h"
#include "includes.h"
#include "stdlib.h"
#include "delay.h"

static GPIO_InitTypeDef	GPIO_InitStructure;
//按键初始化
void key_init(void)
{
	//打开端口A的硬件时钟，就是对端口A供电
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	//打开端口A的硬件时钟，就是对端口B供电
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);	
	//打开端口A的硬件时钟，就是对端口C供电
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
	//打开端口A的硬件时钟，就是对端口E供电
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
	//打开端口A的硬件时钟，就是对端口G供电
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG, ENABLE);

					//列配置  (输出)
////////////////////////////////////////////////////////////////////////////////////	
	GPIO_InitStructure.GPIO_Pin		= GPIO_Pin_7;  	//PB7
	GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_OUT;	//输RU模式
	GPIO_InitStructure.GPIO_PuPd	= GPIO_PuPd_DOWN;	//下拉
	GPIO_Init(GPIOB,&GPIO_InitStructure);//初始化IO口PA0为输入
	
	GPIO_InitStructure.GPIO_Pin		= GPIO_Pin_4;  	//PA4
	GPIO_Init(GPIOA,&GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin		= GPIO_Pin_15;  	//PG15
	GPIO_Init(GPIOG,&GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin		= GPIO_Pin_7;  	//PA4
	GPIO_Init(GPIOC,&GPIO_InitStructure);


					//行配置  (输入)
////////////////////////////////////////////////////////////////////////////////////	
	
	//GPIOF9,F10初始化设置 
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_9 ;		//PC9
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN;			    //输入模式，
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;				//推挽输出，驱动LED需要电流驱动
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;		    //100MHz
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;				    //上拉
	GPIO_Init(GPIOC, &GPIO_InitStructure);						//初始化GPIOF，把配置的数据写入寄存器
	
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_6 ;		//PB6
	GPIO_Init(GPIOB, &GPIO_InitStructure);	
	
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_6 ;		//PE6
	GPIO_Init(GPIOE, &GPIO_InitStructure);	
	
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_8 ;		//PA8
	GPIO_Init(GPIOA, &GPIO_InitStructure);	
	
	//列电平全部设为高电平
	PBout(7)=1;
	PAout(4)=1;
	PGout(15)=1;
	PCout(7)=1;

}
extern OS_Q	key_queue;	//外部声明事件标志组的对象
//识别对应的按键
void get_key(void)
{
		PBout(7)=0; //第一列
		PAout(4)=1;
		PGout(15)=1;
		PCout(7)=1;	
		while(PCin(9)==0) //(列:0,行:0)
		{
			 key_stabilize(GPIOC,GPIO_Pin_9,"1");
			break;
		}
		while(PBin(6)==0)//(列:0,行:1)
		{
			key_stabilize(GPIOB,GPIO_Pin_6,"4");		
			break;			
		}		
		while(PEin(6)==0)//(列:0,行:2)
		{
			key_stabilize(GPIOE,GPIO_Pin_6,"7");		
			break;			
		}		
		while(PAin(8)==0)//(列:0,行:3)
		{
			key_stabilize(GPIOA,GPIO_Pin_8,"*");		
			break;			
		}				
		PBout(7)=1; //第二列
		PAout(4)=0;
		PGout(15)=1;
		PCout(7)=1;	
		while(PCin(9)==0) //(列:1,行:0)
		{
			 key_stabilize(GPIOC,GPIO_Pin_9,"2");
			break;
		}
		while(PBin(6)==0)//(列:1,行:1)
		{
			key_stabilize(GPIOB,GPIO_Pin_6,"5");		
			break;			
		}		
		while(PEin(6)==0)//(列:1,行:2)
		{
			key_stabilize(GPIOE,GPIO_Pin_6,"8");		
			break;			
		}		
		while(PAin(8)==0)//(列:1,行:3)
		{
			key_stabilize(GPIOA,GPIO_Pin_8,"0");		
			break;			
		}
		PBout(7)=1; //第三列
		PAout(4)=1;
		PGout(15)=0;
		PCout(7)=1;			
		while(PCin(9)==0) //(列:2,行:0)
		{
			 key_stabilize(GPIOC,GPIO_Pin_9,"3");
			break;
		}
		while(PBin(6)==0)//(列:2,行:1)
		{
			key_stabilize(GPIOB,GPIO_Pin_6,"6");		
			break;			
		}		
		while(PEin(6)==0)//(列:2,行:2)
		{
			key_stabilize(GPIOE,GPIO_Pin_6,"9");		
			break;			
		}		
		while(PAin(8)==0)//(列:2,行:3)
		{
			key_stabilize(GPIOA,GPIO_Pin_8,"#");		
			break;			
		}		
		PBout(7)=1; //第四列
		PAout(4)=1;
		PGout(15)=1;
		PCout(7)=0;	
		while(PCin(9)==0) //(列:3,行:0)
		{
			key_stabilize(GPIOC,GPIO_Pin_9,"A");
			break;
		}
		while(PBin(6)==0)//(列:3,行:1)
		{
			key_stabilize(GPIOB,GPIO_Pin_6,"B");		
			break;			
		}		
		while(PEin(6)==0)//(列:3,行:2)
		{
			key_stabilize(GPIOE,GPIO_Pin_6,"C");		
			break;			
		}		
		while(PAin(8)==0)//(列:3,行:3)
		{
			key_stabilize(GPIOA,GPIO_Pin_8,"D");		
			break;			
		}		
}
//按键消抖及发送消息队列
void key_stabilize(GPIO_TypeDef* GPIOx,u16 GPIO_Pin,char *ch)
{
	OS_ERR err;//错误码


	//延时消抖
	delay_ms(50);
				
	if(GPIO_ReadInputDataBit(GPIOx,GPIO_Pin) ==0)
	{
		 OSQPost(&key_queue,ch,strlen(ch),OS_OPT_POST_FIFO,&err);
		 //等待按键1释放
			while(GPIO_ReadInputDataBit(GPIOx,GPIO_Pin) ==0)
			{
				delay_ms(1);
			 }

	}	
	//清空数据
	memset(ch,0,sizeof ch);
}
//进入密码输入界面后的操作
uint32_t getpassword_key(void)
{
	char *p=NULL;
	OS_ERR  err;
	OS_MSG_SIZE msg_size;	
	while(1)
	{
		p=OSQPend(&key_queue,0,OS_OPT_PEND_BLOCKING,&msg_size,NULL,&err);
		if(p && msg_size)
		{	
			if(strcmp(p,"C")==0)//密码解锁界面删除键
			{	
				//printf("C\r\n");
				return 11;
			}		
			if(strcmp(p,"D")==0) //密码解锁界面退出键
			{	
				return 12;
			}	
			if(strcmp(p,"#")==0)  //密码输完后确认键
			{
				return 13;
			}	
			if(strcmp(p,"1")==0)
			{
				return 1;
			}	
			if(strcmp(p,"4")==0)
			{
				return 4;
			}		
			if(strcmp(p,"7")==0)
			{
				return 7;
			}			
			if(strcmp(p,"2")==0)
			{
				return 2;
			}			
			if(strcmp(p,"5")==0)
			{
				return 5;
			}			
			if(strcmp(p,"8")==0)
			{
				return 8;
			}			
			if(strcmp(p,"0")==0)
			{
				return 0;
			}	
			if(strcmp(p,"3")==0)
			{
				return 3;
			}			
			if(strcmp(p,"6")==0)
			{
				return 6;
			}			
			if(strcmp(p,"9")==0)
			{
				return 9;
			}				
		}
	}

}
//进入指纹管理界面操作
uint32_t getfingerprint_key(void)
{
	char *p=NULL;
	OS_ERR  err;
	OS_MSG_SIZE msg_size;	
	while(1)
	{
		p=OSQPend(&key_queue,0,OS_OPT_PEND_BLOCKING,&msg_size,NULL,&err);
		if(p && msg_size)
		{	
			if(strcmp(p,"B")==0) //录指纹
			{	
					return 1;
			}
			if(strcmp(p,"C")==0) //删除指纹
			{	
					return 2;
			}			
			if(strcmp(p,"D")==0) //退出指纹管理界面->主页
			{	
					return 3;
			}			
		}
	}
}		
