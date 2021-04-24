#include "key.h"
#include "sys.h"
#include "includes.h"
#include "stdlib.h"
#include "delay.h"

static GPIO_InitTypeDef	GPIO_InitStructure;
//������ʼ��
void key_init(void)
{
	//�򿪶˿�A��Ӳ��ʱ�ӣ����ǶԶ˿�A����
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	//�򿪶˿�A��Ӳ��ʱ�ӣ����ǶԶ˿�B����
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);	
	//�򿪶˿�A��Ӳ��ʱ�ӣ����ǶԶ˿�C����
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
	//�򿪶˿�A��Ӳ��ʱ�ӣ����ǶԶ˿�E����
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
	//�򿪶˿�A��Ӳ��ʱ�ӣ����ǶԶ˿�G����
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG, ENABLE);

					//������  (���)
////////////////////////////////////////////////////////////////////////////////////	
	GPIO_InitStructure.GPIO_Pin		= GPIO_Pin_7;  	//PB7
	GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_OUT;	//��RUģʽ
	GPIO_InitStructure.GPIO_PuPd	= GPIO_PuPd_DOWN;	//����
	GPIO_Init(GPIOB,&GPIO_InitStructure);//��ʼ��IO��PA0Ϊ����
	
	GPIO_InitStructure.GPIO_Pin		= GPIO_Pin_4;  	//PA4
	GPIO_Init(GPIOA,&GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin		= GPIO_Pin_15;  	//PG15
	GPIO_Init(GPIOG,&GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin		= GPIO_Pin_7;  	//PA4
	GPIO_Init(GPIOC,&GPIO_InitStructure);


					//������  (����)
////////////////////////////////////////////////////////////////////////////////////	
	
	//GPIOF9,F10��ʼ������ 
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_9 ;		//PC9
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN;			    //����ģʽ��
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;				//�������������LED��Ҫ��������
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;		    //100MHz
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;				    //����
	GPIO_Init(GPIOC, &GPIO_InitStructure);						//��ʼ��GPIOF�������õ�����д��Ĵ���
	
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_6 ;		//PB6
	GPIO_Init(GPIOB, &GPIO_InitStructure);	
	
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_6 ;		//PE6
	GPIO_Init(GPIOE, &GPIO_InitStructure);	
	
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_8 ;		//PA8
	GPIO_Init(GPIOA, &GPIO_InitStructure);	
	
	//�е�ƽȫ����Ϊ�ߵ�ƽ
	PBout(7)=1;
	PAout(4)=1;
	PGout(15)=1;
	PCout(7)=1;

}
extern OS_Q	key_queue;	//�ⲿ�����¼���־��Ķ���
//ʶ���Ӧ�İ���
void get_key(void)
{
		PBout(7)=0; //��һ��
		PAout(4)=1;
		PGout(15)=1;
		PCout(7)=1;	
		while(PCin(9)==0) //(��:0,��:0)
		{
			 key_stabilize(GPIOC,GPIO_Pin_9,"1");
			break;
		}
		while(PBin(6)==0)//(��:0,��:1)
		{
			key_stabilize(GPIOB,GPIO_Pin_6,"4");		
			break;			
		}		
		while(PEin(6)==0)//(��:0,��:2)
		{
			key_stabilize(GPIOE,GPIO_Pin_6,"7");		
			break;			
		}		
		while(PAin(8)==0)//(��:0,��:3)
		{
			key_stabilize(GPIOA,GPIO_Pin_8,"*");		
			break;			
		}				
		PBout(7)=1; //�ڶ���
		PAout(4)=0;
		PGout(15)=1;
		PCout(7)=1;	
		while(PCin(9)==0) //(��:1,��:0)
		{
			 key_stabilize(GPIOC,GPIO_Pin_9,"2");
			break;
		}
		while(PBin(6)==0)//(��:1,��:1)
		{
			key_stabilize(GPIOB,GPIO_Pin_6,"5");		
			break;			
		}		
		while(PEin(6)==0)//(��:1,��:2)
		{
			key_stabilize(GPIOE,GPIO_Pin_6,"8");		
			break;			
		}		
		while(PAin(8)==0)//(��:1,��:3)
		{
			key_stabilize(GPIOA,GPIO_Pin_8,"0");		
			break;			
		}
		PBout(7)=1; //������
		PAout(4)=1;
		PGout(15)=0;
		PCout(7)=1;			
		while(PCin(9)==0) //(��:2,��:0)
		{
			 key_stabilize(GPIOC,GPIO_Pin_9,"3");
			break;
		}
		while(PBin(6)==0)//(��:2,��:1)
		{
			key_stabilize(GPIOB,GPIO_Pin_6,"6");		
			break;			
		}		
		while(PEin(6)==0)//(��:2,��:2)
		{
			key_stabilize(GPIOE,GPIO_Pin_6,"9");		
			break;			
		}		
		while(PAin(8)==0)//(��:2,��:3)
		{
			key_stabilize(GPIOA,GPIO_Pin_8,"#");		
			break;			
		}		
		PBout(7)=1; //������
		PAout(4)=1;
		PGout(15)=1;
		PCout(7)=0;	
		while(PCin(9)==0) //(��:3,��:0)
		{
			key_stabilize(GPIOC,GPIO_Pin_9,"A");
			break;
		}
		while(PBin(6)==0)//(��:3,��:1)
		{
			key_stabilize(GPIOB,GPIO_Pin_6,"B");		
			break;			
		}		
		while(PEin(6)==0)//(��:3,��:2)
		{
			key_stabilize(GPIOE,GPIO_Pin_6,"C");		
			break;			
		}		
		while(PAin(8)==0)//(��:3,��:3)
		{
			key_stabilize(GPIOA,GPIO_Pin_8,"D");		
			break;			
		}		
}
//����������������Ϣ����
void key_stabilize(GPIO_TypeDef* GPIOx,u16 GPIO_Pin,char *ch)
{
	OS_ERR err;//������


	//��ʱ����
	delay_ms(50);
				
	if(GPIO_ReadInputDataBit(GPIOx,GPIO_Pin) ==0)
	{
		 OSQPost(&key_queue,ch,strlen(ch),OS_OPT_POST_FIFO,&err);
		 //�ȴ�����1�ͷ�
			while(GPIO_ReadInputDataBit(GPIOx,GPIO_Pin) ==0)
			{
				delay_ms(1);
			 }

	}	
	//�������
	memset(ch,0,sizeof ch);
}
//����������������Ĳ���
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
			if(strcmp(p,"C")==0)//�����������ɾ����
			{	
				//printf("C\r\n");
				return 11;
			}		
			if(strcmp(p,"D")==0) //������������˳���
			{	
				return 12;
			}	
			if(strcmp(p,"#")==0)  //���������ȷ�ϼ�
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
//����ָ�ƹ���������
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
			if(strcmp(p,"B")==0) //¼ָ��
			{	
					return 1;
			}
			if(strcmp(p,"C")==0) //ɾ��ָ��
			{	
					return 2;
			}			
			if(strcmp(p,"D")==0) //�˳�ָ�ƹ������->��ҳ
			{	
					return 3;
			}			
		}
	}
}		
