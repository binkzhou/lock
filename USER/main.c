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
OS_Q	usart1_queue;				//����һ��Ϣ���еĶ���
OS_Q	usart3_queue;				//������������Ϣ���еĶ���
OS_Q    rfid_queue;					//RFID��Ϣ����
OS_Q	key_queue;					//key������Ϣ����
OS_FLAG_GRP	g_flag_grp;				//RTC�¼���־��Ķ���
OS_MUTEX	g_mutex_oled;				//����oled�������Ķ���
OS_SEM	g_sem;					//�ź����Ķ���

//�ⲿ��������ʱ�ӽṹ��
extern  RTC_TimeTypeDef  RTC_TimeStructure;
extern  RTC_DateTypeDef  RTC_DateStructure;
char 	* password=NULL; //������õ����������
uint32_t	blue_flag=0;  //����������¼д��������־λ  
char 		buf[128]={0}; //���������������	

//MFRC522������
u8  mfrc552pidbuf[18];
u8  card_pydebuf[2];  //��ſ�Ƭ����
u8  card_numberbuf[5];  //��ſ����к�
u8  card_key0Abuf[6]={0xff,0xff,0xff,0xff,0xff,0xff}; //��������
u8  card_writebuf[6]={1,2,3,4,5,6}; //���д�������
u8  card_readbuf[18]; //��Ŷ�ȡ������
char i,j,card_size; 
u8 status;
//AS608ָ��ģ������
OS_SEM	g_sem;					//�ź����Ķ���
extern SysPara AS608Para;//ָ��ģ��AS608����
extern u16 ValidN;//ģ������Чָ�Ƹ���
u8 ensure;  

int times;//�Զ�������ʱ���¼


//����  ������ʼ��
OS_TCB startup_init_TCB;
void startup_init(void *parg);
CPU_STK startup_init_stk[128];	

//����1 ���� ���ƿ�
OS_TCB bluetooth_TCB;
void bluetooth(void *parg);
CPU_STK bluetooth_stk[1024];		

//����2  ��ҳ��ʾ���ƿ�
OS_TCB Task2_TCB;
void task2(void *parg);
CPU_STK task2_stk[3600];			

//����3  ���а����������ƿ�
OS_TCB Task3_TCB;
void task3(void *parg);
CPU_STK task3_stk[1024];	
//����4  4x4�����������ƿ�
OS_TCB Task4_TCB;
void task4(void *parg);
CPU_STK task4_stk[2024];			
//����5  RFID���ƿ�
OS_TCB Task5_TCB;
void task5(void *parg);
CPU_STK task5_stk[1024];		
//����6  as608��ָ��ģ�飩���ƿ�
OS_TCB Task6_TCB;
void task6(void *parg);
CPU_STK task6_stk[1024];	
//����7
OS_TCB Task7_TCB;
void task7(void *parg);
CPU_STK task7_stk[1024];	
//������
int main(void)
{
	OS_ERR err;

	systick_init();  	//ʱ�ӳ�ʼ��
	usart1_init(115200);  	//����1��ʼ��
	usart3_init(9600);	//����3������Ϊ9600bps
	LED_Init();        //LED��ʼ��
	bluetooth_config();		//����ģ������
	OLED_Init();			//��ʼ��OLED 
	OLED_Clear();  //�ر�OLED��ʾ
	key_init();//4x4�������ų�ʼ��
	MFRC522_Initializtion(); //RFID��ʼ��	
	buzzer_Init();//��ʼ��������
	usart2_init(usart2_baund);//��ʼ������2,������ָ��ģ��ͨѶ 57600
	PS_StaGPIO_Init();		  //��ʼ��FR��״̬����(ָ�Ƹ�Ӧ���ţ��ߵ�ƽ����)
	lock_init();  //��ͷ��ʼ��
	bluetooth_MCU();//����MCU����(�ж������Ƿ�����)��ʼ��    ���ϳ��ֵ͵�ƽ    �Ͽ����ָߵ�ƽ
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);	//�жϷ���
	//RTC��ʼ��
	if (RTC_ReadBackupRegister(RTC_BKP_DR0) != 0x9999)
	{
		rtc_init(); //����ʱ�����ڵĳ�ʼ��
		//����һ��������ǣ����ߵ�ǰ�Ѿ����ù�RTC��
		//����һ�θ�λ���Ͳ�����Ҫ����RTC��������ʱ��
		//�����ݼĴ���0д������0x8888
		RTC_WriteBackupRegister(RTC_BKP_DR0, 0x9999);		
	}
	else{
		rtc_repeat_init();//������ʱ�����ڵĳ�ʼ��		
	}
	/*����FLASH���������FLASH*/
	FLASH_Unlock();
	/* �����Ӧ�ı�־λ*/  
	FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | 
	FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR|FLASH_FLAG_PGSERR);
	flash_erase_record(FLASH_Sector_4);//��������4��¼	
	//flash_erase_record(FLASH_Sector_6);//��������6��¼	
 	
	
	//OS��ʼ�������ǵ�һ�����еĺ���,��ʼ�����ֵ�ȫ�ֱ����������ж�Ƕ�׼����������ȼ����洢��
	OSInit(&err);

	//��������:������ʼ��
	OSTaskCreate(	&startup_init_TCB,									//������ƿ飬��ͬ���߳�id
					"startup_init_TCB",									//��������֣����ֿ����Զ����
					startup_init,										//����������ͬ���̺߳���
					0,											//���ݲ�������ͬ���̵߳Ĵ��ݲ���
					3,											//��������ȼ�6		
					startup_init_stk,									//�����ջ����ַ
					128/10,										//�����ջ�����λ���õ����λ�ã��������ټ���ʹ��
					128,										//�����ջ��С			
					0,											//��ֹ������Ϣ����
					0,											//Ĭ������ռʽ�ں�															
					0,											//����Ҫ�����û��洢��
					OS_OPT_TASK_NONE,							//û���κ�ѡ��
					&err										//���صĴ�����
				);
				
	//��������1  ����
	OSTaskCreate(	&bluetooth_TCB,									//������ƿ飬��ͬ���߳�id
					"bluetooth",									//��������֣����ֿ����Զ����
					bluetooth,										//����������ͬ���̺߳���
					0,											//���ݲ�������ͬ���̵߳Ĵ��ݲ���
					5,											//��������ȼ�6		
					bluetooth_stk,									//�����ջ����ַ
					1024/10,										//�����ջ�����λ���õ����λ�ã��������ټ���ʹ��
					1024,										//�����ջ��С			
					0,											//��ֹ������Ϣ����
					0,											//Ĭ������ռʽ�ں�															
					0,											//����Ҫ�����û��洢��
					OS_OPT_TASK_NONE,							//û���κ�ѡ��
					&err										//���صĴ�����
				);


	//��������2 ��ҳ��ʾ
	OSTaskCreate(	&Task2_TCB,									//������ƿ�
					"Task2",									//���������
					task2,										//������
					0,												//���ݲ���
					4,											 	//��������ȼ�7		
					task2_stk,									//�����ջ����ַ
					256/10,									//�����ջ�����λ���õ����λ�ã��������ټ���ʹ��
					256,										//�����ջ��С			
					0,											//��ֹ������Ϣ����
					0,												//Ĭ������ռʽ�ں�																
					0,												//����Ҫ�����û��洢��
					OS_OPT_TASK_NONE,								//û���κ�ѡ��
					&err													//���صĴ�����
				);

	//��������3 ���а�������
	OSTaskCreate(	&Task3_TCB,									//������ƿ�
					"Task3",									//���������
					task3,										//������
					0,												//���ݲ���
					5,											 	//��������ȼ�7		
					task3_stk,									//�����ջ����ַ
					128/10,									//�����ջ�����λ���õ����λ�ã��������ټ���ʹ��
					128,										//�����ջ��С			
					0,											//��ֹ������Ϣ����
					0,												//Ĭ������ռʽ�ں�																
					0,												//����Ҫ�����û��洢��
					OS_OPT_TASK_NONE,								//û���κ�ѡ��
					&err													//���صĴ�����
				);
	//��������4 4x4��������
	OSTaskCreate(	&Task4_TCB,									//������ƿ�
					"Task4",									//���������
					task4,										//������
					0,												//���ݲ���
					5,											 	//��������ȼ�7		
					task4_stk,									//�����ջ����ַ
					1024/10,									//�����ջ�����λ���õ����λ�ã��������ټ���ʹ��
					1024,										//�����ջ��С			
					0,											//��ֹ������Ϣ����
					0,												//Ĭ������ռʽ�ں�																
					0,												//����Ҫ�����û��洢��
					OS_OPT_TASK_NONE,								//û���κ�ѡ��
					&err													//���صĴ�����
				);
	//��������5  RFID
	OSTaskCreate(	&Task5_TCB,									//������ƿ�
					"Task5",									//���������
					task5,										//������
					0,												//���ݲ���
					6,											 	//��������ȼ�7		
					task5_stk,									//�����ջ����ַ
					1024/10,									//�����ջ�����λ���õ����λ�ã��������ټ���ʹ��
					1024,										//�����ջ��С			
					0,											//��ֹ������Ϣ����
					0,												//Ĭ������ռʽ�ں�																
					0,												//����Ҫ�����û��洢��
					OS_OPT_TASK_NONE,								//û���κ�ѡ��
					&err													//���صĴ�����
				);	
	//��������6  as608��ָ��ģ�飩
	OSTaskCreate(	&Task6_TCB,									//������ƿ�
					"Task6",									//���������
					task6,										//������
					0,												//���ݲ���
					6,											 	//��������ȼ�7		
					task6_stk,									//�����ջ����ַ
					1024/10,									//�����ջ�����λ���õ����λ�ã��������ټ���ʹ��
					1024,										//�����ջ��С			
					0,											//��ֹ������Ϣ����
					0,												//Ĭ������ռʽ�ں�																
					0,												//����Ҫ�����û��洢��
					OS_OPT_TASK_NONE,								//û���κ�ѡ��
					&err													//���صĴ�����
				);		
	//��������7
	OSTaskCreate(	&Task7_TCB,									//������ƿ�
					"Task7",									//���������
					task7,										//������
					0,												//���ݲ���
					4,											 	//��������ȼ�7		
					task7_stk,									//�����ջ����ַ
					1024/10,									//�����ջ�����λ���õ����λ�ã��������ټ���ʹ��
					1024,										//�����ջ��С			
					0,											//��ֹ������Ϣ����
					0,												//Ĭ������ռʽ�ں�																
					0,												//����Ҫ�����û��洢��
					OS_OPT_TASK_NONE,								//û���κ�ѡ��
					&err													//���صĴ�����
				);		

	//��������һ��Ϣ����
	OSQCreate(&usart1_queue,"usart1_queue",16,&err);
	//������������Ϣ����
	OSQCreate(&usart3_queue,"usart3_queue",16,&err);
	//����rfid��Ϣ����
	OSQCreate(&rfid_queue,"rfid_queue",16,&err);	
	//����������Ϣ����
	OSQCreate(&key_queue,"key_queue",16,&err);	
	//����RTC�¼���־�飬���б�־λ��ֵΪ0
	OSFlagCreate(&g_flag_grp,"g_flag_grp",0,&err);
	//����������
	OSMutexCreate(&g_mutex_oled,"g_mutex_oled",&err);
	
	//�����ź�������ֵΪ1����һ����Դ
	OSSemCreate(&g_sem,"g_sem",1,&err);
	
	
	printf("never run.......\r\n");

	//����OS�������������
	OSStart(&err);
	while(1);	
	
}

//���񣺿���������ʼ��
void startup_init(void *parg)
{
	OS_ERR err;//������	
		OSTaskSuspend(&Task7_TCB,&err); //��������7  �����Ͽ�60���Զ�����
		//�����ȴ�������
		OSMutexPend(&g_mutex_oled,0,OS_OPT_PEND_BLOCKING,NULL,&err);
		password="123456"; //���ó�ʼ����123456
		flash_write_record(password,1,ADDR_FLASH_SECTOR_6,FLASH_Sector_6);//д���ʼ����	
		//�������
		memset(password,0,sizeof password);
	
		//ָ��ģ������
		printf("��AS608ģ������....\r\n");
		while(PS_HandShake(&AS608Addr))//��AS608ģ������
		{
			delay_ms(400);		
			printf("δ��⵽ģ��!!!\r\n");
			delay_ms(800);
			printf("��������ģ��..\r\n");
		}
		printf("ͨѶ�ɹ�");
		ensure=PS_ValidTempleteNum(&ValidN);//����ָ�Ƹ���
		if(ensure!=0x00)
			ShowErrMessage(ensure);//��ʾȷ���������Ϣ
			ensure=PS_ReadSysPara(&AS608Para);  //������ 	
		if(ensure==0x00)
		{	
			printf("������:%d     �Աȵȼ�: %d",AS608Para.PS_max-ValidN,AS608Para.PS_level);		
		}
		else
			ShowErrMessage(ensure);		
		
		//��������
		OLED_Clear();//�ر���ʾ
		OLED_ShowCHinese(32,2,37);//��
		OLED_ShowCHinese(50,2,38);//ӭ
		OLED_ShowCHinese(68,2,39);//ʹ
		OLED_ShowCHinese(86,2,40);//��	
		OLED_ShowCHinese(23,4,6);//��
		OLED_ShowCHinese(41,4,7);//��
		OLED_ShowCHinese(59,4,8);//ָ
		OLED_ShowCHinese(77,4,9);//��
		OLED_ShowCHinese(95,4,5);//��
		OLED_ShowCHinese(10,6,41);//��
		OLED_ShowCHinese(28,6,42);//��
		OLED_ShowString(46,6,":");//:	
		OLED_ShowCHinese(64,6,10);//��
		OLED_ShowCHinese(82,6,11);//��
		delay_ms(4000);
		OLED_Clear();//�ر���ʾ
		OLED_DrawBMP(0,2,128,8,BMP10);//����������ͼ���ʼ
		delay_ms(1000);		
		//�����˽�����
		OLED_DrawBMP(0,2,128,8,BMP5);//����������ͼ��0
		delay_ms(1000);
		OLED_DrawBMP(0,2,128,8,BMP6);//����������ͼ��1	
		delay_ms(800);
		OLED_DrawBMP(0,2,128,8,BMP7);//����������ͼ��2	
		delay_ms(1000);
		OLED_DrawBMP(0,2,128,8,BMP8);//����������ͼ��3
		delay_ms(1500);
		OLED_DrawBMP(0,2,128,8,BMP9);//����������ͼ��4	
		delay_ms(3000);
		OLED_Clear();//�ر���ʾ
		//�����ͷŻ�����
		OSMutexPost(&g_mutex_oled,OS_OPT_POST_NONE,&err);	
	
	OSTaskDel(NULL,&err); //ɾ������ ֹͣ����
}
//����һ������
void bluetooth(void *parg)
{
	
	OS_ERR err;//������	
	OS_MSG_SIZE  msg_size;//���ݴ�С
	char *p=NULL; //������Ϣ��������
	uint32_t	i=0;     
	char 		usart3_send_open[128]={0}; //��ŷ������û��Ľ�����ʾ����

	int a=0;//���ڿ�����֤����
	int b=0;//���ڿ�����֤����
		
	while(1)
	{

		//�ȴ�������������Ϣ����
		p=OSQPend(&usart3_queue,0,OS_OPT_PEND_BLOCKING,&msg_size,NULL,&err);		
		//printf("1111%s\r\n",p);
		//�����ȴ��ź���
		OSSemPend(&g_sem,0,OS_OPT_PEND_BLOCKING,NULL,&err);			
		//�����ȴ�������
		OSMutexPend(&g_mutex_oled,0,OS_OPT_PEND_BLOCKING,NULL,&err);		
		if(p && msg_size)
		{
			OSTaskSuspend(&Task5_TCB,&err);
			//ͨ���ֻ���������
			if(strstr((const char *)p,"lock"))
			{	
				//��˸�Ƽӷ�����
				PFout(8)=1;PFout(9)=0;delay_ms(100);
				PFout(8)=0;PFout(9)=1;delay_ms(100);
				PFout(8)=1;PFout(9)=0;delay_ms(100);
				PFout(8)=0;PFout(9)=1;delay_ms(100);
				PEout(5)=0; //�ϵ磬�ɿ���  ����				
				//�������û���������ʾ�����ɹ�
				usart3_send_str("lock  success\r\n");					
				OLED_Clear();//�ر���ʾ
				OLED_ShowCHinese(32,4,0);//��
				OLED_ShowCHinese(50,4,1);//��
				OLED_ShowCHinese(68,4,2);//��
				OLED_ShowCHinese(86,4,3);//��
				delay_ms(1000);
				OLED_DrawBMP(0,2,128,8,BMP1);  //ͼƬ��ʾ(ͼƬ��ʾ���ã����ɵ��ֱ�ϴ󣬻�ռ�ý϶�ռ䣬FLASH�ռ�8K��������)
				delay_ms(2000);
				OSTaskSuspend(&Task7_TCB,&err); //��������7  �����Ͽ�60���Զ�����
				//�����¼���־��  ����������������
				OSFlagPost(&g_flag_grp,0x02,OS_OPT_POST_FLAG_SET,&err);				
			}
			

			//ͨ���ֻ���������
			if(strstr((const char *)p,"open-"))
			{
				//open-123456
				p=strtok((char *)p,"-");//�ֽ����open
				p = strtok(NULL,"-");//�����ֽ�password����123456#
				if(strcmp((const char *)p,"#")==0) //�ж�����������Ƿ�Ϊ��
				{
					//�������û���������ʾ���벻��Ϊ��
					usart3_send_str("The password cannot be empty\r\n");
				}
				else
				{	
					//���ַ���ת��Ϊ���ͣ�
					b = atoi((const char *)p);
					//��ȡ�洢��������6�������¼
					flash_read_record(password,1,ADDR_FLASH_SECTOR_6);
					//��������3��ȡ������ת��Ϊ���ͣ����ڱȽϣ���
					a = atoi((const char *)password);		
					//�������
					memset(password,0,sizeof password);					
					if( a==b)//��֤����
					{	
						//��˸�Ƽӷ�����
						PFout(8)=1;PFout(9)=0;delay_ms(100);
						PFout(8)=0;PFout(9)=1;delay_ms(100);
						PFout(8)=1;PFout(9)=0;delay_ms(100);
						PFout(8)=0;PFout(9)=1;delay_ms(100);			
						PEout(5)=1; //ͨ�磬������  ����
						
						//��鵱ǰ�����ݼ�¼�Ƿ�С��100��
						if(blue_flag<100)
						{
								/*����FLASH���������FLASH*/
								FLASH_Unlock();
								/* �����Ӧ�ı�־λ*/  
								FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | 
								FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR|FLASH_FLAG_PGSERR);										
								
								//RTC_GetDate����ȡ����
								RTC_GetDate(RTC_Format_BCD, &RTC_DateStructure);
								//printf("DATE-OLED:20%02x/%02x/%02x %0x\r\n",RTC_DateStructure.RTC_Year,RTC_DateStructure.RTC_Month,RTC_DateStructure.RTC_Date,RTC_DateStructure.RTC_WeekDay);	
								//RTC_GetTime����ȡʱ��
								RTC_GetTime(RTC_Format_BCD, &RTC_TimeStructure); 
								//printf("TIME-OLED:%02x:%02x:%02x\r\n",RTC_TimeStructure.RTC_Hours,RTC_TimeStructure.RTC_Minutes,RTC_TimeStructure.RTC_Seconds);		
								//��������������û��Ľ�����ʾ����
								sprintf((char *)usart3_send_open,"20%02x/%02x/%02x  %02x:%02x:%02x Bluetooth open succeed\r\n",\
													RTC_DateStructure.RTC_Year, RTC_DateStructure.RTC_Month, RTC_DateStructure.RTC_Date,\
													RTC_TimeStructure.RTC_Hours, RTC_TimeStructure.RTC_Minutes, RTC_TimeStructure.RTC_Seconds);							
								//����������������
								sprintf((char *)buf,"[%03d]20%02x/%02x/%02x  %02x:%02x:%02x Bluetooth unlock\r\n",\
													blue_flag,\
													RTC_DateStructure.RTC_Year, RTC_DateStructure.RTC_Month, RTC_DateStructure.RTC_Date,\
													RTC_TimeStructure.RTC_Hours, RTC_TimeStructure.RTC_Minutes, RTC_TimeStructure.RTC_Seconds);
								
								//д����������4��¼����ʱ�估��ʽ
								if(0==flash_write_record(buf,blue_flag,ADDR_FLASH_SECTOR_4,FLASH_Sector_4))
								{
									  //��ʾ
									printf(buf);
									  //��ʾ
									//printf(usart3_send);
									//�������û���������ʾ�����ɹ�
									 usart3_send_str((char *)usart3_send_open);
									//��¼�Լ�1
									blue_flag++;
									//�������
									memset(buf,0,sizeof buf);
									memset(usart3_send_open,0,sizeof usart3_send_open);
									OLED_Clear();//�ر���ʾ
									OLED_ShowCHinese(32,4,0);//��
									OLED_ShowCHinese(50,4,1);//��
									OLED_ShowCHinese(68,4,4);//��
									OLED_ShowCHinese(86,4,5);//��
									delay_ms(1000);				
									OLED_DrawBMP(0,2,128,8,BMP2);  //ͼƬ��ʾ(ͼƬ��ʾ���ã����ɵ��ֱ�ϴ󣬻�ռ�ý϶�ռ䣬FLASH�ռ�8K��������)
									delay_ms(1500);
									OSTaskResume(&Task7_TCB,&err);//�ָ�����7����
									times=0;
									//�����¼���־�� //������ҳ���ѽ���
									OSFlagPost(&g_flag_grp,0x04,OS_OPT_POST_FLAG_SET,&err);											
								}	
								else
								{
									blue_flag=0;//��¼��־λ��0
								}
								
					
								
						}
						else //���ݳ���100�����������
						{
							blue_flag=0;//��¼��־λ��0
							flash_erase_record(FLASH_Sector_4);//��������4��¼	
							OLED_Clear();//�ر���ʾ
							OLED_ShowCHinese(28,2,4);//��
							OLED_ShowCHinese(46,2,5);//��
							OLED_ShowCHinese(64,2,15);//��
							OLED_ShowCHinese(82,2,16);//��
							OLED_ShowCHinese(20,4,14);//��
							OLED_ShowCHinese(38,4,17);//��
							OLED_ShowCHinese(56,4,14);//��
							OLED_ShowCHinese(74,4,18);//��
							OLED_ShowCHinese(92,4,19);//��
							delay_ms(5000);				
						}
						
					}
					if(a!=b)
					{
						//�������û���������ʾ�������
						usart3_send_str("password error");	
					 }
				 }
			}	
			//ͨ���ֻ�������ѯ������Ϣ
			if(strstr((const char *)p,"inquire"))
			{
				//���Ի�ȡ100����¼
				for(i=0;i<100;i++)
				{
					//��ȡ�洢��������4�ļ�¼
					flash_read_record(buf,i,ADDR_FLASH_SECTOR_4);
					
					//����¼�Ƿ���ڻ��з��ţ��������򲻴�ӡ���
					if(strstr((char *)buf,"\n")==0)
						break;	
					//����ѯ����������û�����
					 usart3_send_str((char *)buf);
					//��ӡ��¼
					//printf(buf);
					memset(buf,0,sizeof buf);					
					
				}		
				//���i����0������û��һ����¼
				if(i==0)
				{
					printf("There is no record\r\n");
						//��û�м�¼�������û�����
					 usart3_send_str("There is no record");				
				 }				
				
			}
			//ͨ���ֻ������޸�����
			if(strstr((const char *)p,"set-"))
			{
				
				//����"set_password-123456#"
				password = strtok((char *)p,"-");//�ֽ�password����set
				password = strtok(NULL,"-");//�����ֽ�password����123456#
				if(strcmp(password,"#")==0)
				{
					//�������û���������ʾ���벻��Ϊ��
					usart3_send_str("The password cannot be empty");
				}
				delchar(password,"#"); //�����Զ��庯��ɾ����#���õ�����
				if(strlen(password)>6)
				{
					//�������û���������ʾ�������
					usart3_send_str("Set the password too long");					
				}	
				else
				{	
					
					/*����FLASH���������FLASH*/
					FLASH_Unlock();
					/* �����Ӧ�ı�־λ*/  
					FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | 
					FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR|FLASH_FLAG_PGSERR);								
					flash_erase_record(FLASH_Sector_6);//��������6��¼		
					//д����������6��¼����������
					if(0==flash_write_record(password,1,ADDR_FLASH_SECTOR_6,FLASH_Sector_6))
					{	
						//printf("%s\r\n",password);
						//printf("set_password succeed\r\n");
						//�������û���������ʾ��������ɹ�
						usart3_send_str("set_password succeed");
						OLED_Clear();//�ر���ʾ
						OLED_ShowCHinese(10,4,20);//��
						OLED_ShowCHinese(28,4,21);//��
						OLED_ShowCHinese(46,4,22);//��
						OLED_ShowCHinese(64,4,23);//��
						OLED_ShowCHinese(82,4,24);//��
						OLED_ShowCHinese(100,4,25);//��						
						delay_ms(2000);	
						//�������
						memset(password,0,sizeof password);
						
					}	
				}			
			 }	
			//printf("msg[%s][%d]\r\n",p,msg_size);//��ӡ��Ϣ
			memset(p,0,msg_size);//�������p
			//�����ͷŻ�����
			OSMutexPost(&g_mutex_oled,OS_OPT_POST_NONE,&err);
			 OSTaskResume(&Task5_TCB,&err);

		}
			
	}
	
}

//�������ʾ��ҳ��ʱ�����ڵ�
void task2(void *parg)
{
	
	OS_ERR err;//������
	OS_FLAGS flags=0;
	u8 time[20]; //���ʱ��
	u8 date[20];//�������
	uint32_t flag=0;
	//�������
	while(1)
	{
		//һֱ�����ȴ��¼���־��bit0(RTC�жϱ�־λ)��1  bit1  bit2���ȴ��ɹ��󣬽�bit0��0
		flags=OSFlagPend(&g_flag_grp,0x01|0x02|0x04,0,OS_OPT_PEND_FLAG_SET_ANY + OS_OPT_PEND_FLAG_CONSUME+OS_OPT_PEND_BLOCKING,NULL,&err);				
		//�����ȴ�������
		OSMutexPend(&g_mutex_oled,0,OS_OPT_PEND_BLOCKING,NULL,&err);
	
		if(flags)
		{
			OLED_Clear();
			//RTC_GetTime����ȡʱ��
			RTC_GetTime(RTC_Format_BCD, &RTC_TimeStructure); 			
			//printf("TIME:%02x:%02x:%02x\r\n",RTC_TimeStructure.RTC_Hours,RTC_TimeStructure.RTC_Minutes,RTC_TimeStructure.RTC_Seconds);
			sprintf((char *)time,"%02x:%02x:%02x",RTC_TimeStructure.RTC_Hours,RTC_TimeStructure.RTC_Minutes,RTC_TimeStructure.RTC_Seconds);
			//RTC_GetDate����ȡ����
			RTC_GetDate(RTC_Format_BCD, &RTC_DateStructure);
			//printf("DATE:20%02x/%02x/%02x %0x\r\n",RTC_DateStructure.RTC_Year,RTC_DateStructure.RTC_Month,RTC_DateStructure.RTC_Date,RTC_DateStructure.RTC_WeekDay);
			sprintf((char *)date,"20%02x/%02x/%02x",RTC_DateStructure.RTC_Year,RTC_DateStructure.RTC_Month,RTC_DateStructure.RTC_Date);		
			OLED_ShowCHinese(18,0,6);//��
			OLED_ShowCHinese(36,0,7);//��
			OLED_ShowCHinese(54,0,8);//ָ
			OLED_ShowCHinese(72,0,9);//��
			OLED_ShowCHinese(90,0,5);//��
			OLED_ShowString(0,6,"B");				
			OLED_ShowCHinese(10,6,20);//��
			OLED_ShowCHinese(23,6,21);//��	
			OLED_ShowCHinese(36,6,4);//��
			OLED_ShowCHinese(49,6,5);//��			
			OLED_ShowString(62,6,"C");		
			OLED_ShowCHinese(72,6,83);//ָ
			OLED_ShowCHinese(85,6,84);//��
			OLED_ShowCHinese(98,6,96);//��
			OLED_ShowCHinese(111,6,97);//��
			
			OLED_ShowString(54,4,time);  //��ʾʵʱʱ��
			OLED_ShowString(48,2,date); //��ʾʵʱ����
			OLED_DrawBMP(0,2,30,4,BMP3);//��ʾ��ҳС����
			OLED_ShowCHinese(0,4,5);//��
			OLED_ShowCHinese(15,4,3);//��
			OLED_ShowCHinese(30,4,13);//��		

			if(flags &0x02||flag==1)
			{
				
				flag=1;
				OLED_DrawBMP(0,2,30,4,BMP3);//��ʾ��ҳС����
				OLED_ShowCHinese(0,4,5);//��
				OLED_ShowCHinese(13,4,3);//��
				OLED_ShowCHinese(28,4,13);//��
			}
			if(flags &0x04||flag==2)
			{			
				
				
				flag=2;
				OLED_DrawBMP(0,2,30,4,BMP4);//��ʾ��ҳС����
				OLED_ShowCHinese(0,4,14);//��
				OLED_ShowCHinese(13,4,4);//��
				OLED_ShowCHinese(28,4,5);//��
			}		
	
		}
		flags=0;
			//�ͷ��ź���
			OSSemPost(&g_sem,OS_OPT_POST_1,&err); 			
			//�����ͷŻ�����
			OSMutexPost(&g_mutex_oled,OS_OPT_POST_NONE,&err);
	
	}
	
}
//������ ������������
void task3(void *parg)
{
	while(1)
	{	
			delay_ms(10);
			get_key();

	}		
	 
}
//������ ��������
void task4(void *parg)
{
	OS_ERR err;
	OS_MSG_SIZE msg_size;
	char *key;//�����Ϣ���з����İ�������
	char keybuf[60]={0};//��Ű�������
	uint32_t i=0; //����������OLED��ʾ��λ�ü���ŵ�ַ
	int a=0;//���ڿ�����֤����
	int b=0;//���ڿ�����֤����	
	uint32_t rt; //������������������ֵ�ж�
	while(1)
	{	
	
		
		//�ȴ�������Ϣ�Ķ���
		key=OSQPend(&key_queue,0,OS_OPT_PEND_BLOCKING,&msg_size,NULL,&err);	
		
		if(key && msg_size)
		{
			if(strcmp(key,"A")==0) //��������
			{	
				printf("A\r\n");
			}
			
			if(strcmp(key,"B")==0) //���������������
			{	
				memset(keybuf,0,strlen(keybuf));//�������buf	
				i=0;//���i							
				OSTaskSuspend(&Task5_TCB,&err); //��ͣ����5����
				//printf("B\r\n");
				//�����ȴ��ź���  //ͬ��
				OSSemPend(&g_sem,0,OS_OPT_PEND_BLOCKING,NULL,&err);					
				//�����ȴ�������  //ռ������������ 
				OSMutexPend(&g_mutex_oled,0,OS_OPT_PEND_BLOCKING,NULL,&err);	
				OLED_Clear();//�ر���ʾ				
				OLED_ShowCHinese(18,0,26);//��
				OLED_ShowCHinese(36,0,27);//��
				OLED_ShowCHinese(54,0,28);//��
				OLED_ShowCHinese(72,0,20);//��
				OLED_ShowCHinese(90,0,21);//��
				OLED_ShowString(18,4,"-----------");
				OLED_ShowString(0,6,"#:");//#:
				OLED_ShowCHinese(14,6,29);//ȷ
				OLED_ShowCHinese(28,6,30);//��
				OLED_ShowString(42,6,"C:");//#:			
				OLED_ShowCHinese(56,6,31);//ɾ
				OLED_ShowCHinese(70,6,32);//��
				OLED_ShowString(84,6,"D:");//D:		
				OLED_ShowCHinese(98,6,33);//��
				OLED_ShowCHinese(112,6,34);//��	
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
							
							
							
					if(rt==11)//�����������ɾ����  C
					{				
						
						OLED_Clear();//�ر���ʾ				
						OLED_ShowCHinese(18,0,26);//��
						OLED_ShowCHinese(36,0,27);//��
						OLED_ShowCHinese(54,0,28);//��
						OLED_ShowCHinese(72,0,20);//��
						OLED_ShowCHinese(90,0,21);//��
						OLED_ShowString(18,4,"-----------");
						OLED_ShowString(0,6,"#:");//#:
						OLED_ShowCHinese(14,6,29);//ȷ
						OLED_ShowCHinese(28,6,30);//��
						OLED_ShowString(42,6,"C:");//#:			
						OLED_ShowCHinese(56,6,31);//ɾ
						OLED_ShowCHinese(70,6,32);//��
						OLED_ShowString(84,6,"D:");//D:		
						OLED_ShowCHinese(98,6,33);//��
						OLED_ShowCHinese(112,6,34);//��							
						memset(keybuf,0,strlen(keybuf));//�������buf	
						i=0;//���i	
						
					}
					if(rt==12) //������������˳���  D
					{	
						//printf("D\r\n");
						//�����ͷŻ�����   //�˳�
						OSMutexPost(&g_mutex_oled,OS_OPT_POST_NONE,&err);
						memset(keybuf,0,strlen(keybuf));//�������buf	
						i=0;//���i		
						OSTaskResume(&Task5_TCB,&err); //�ָ�����5����		
						break;

					}		
					if(rt==13)  //���������ȷ�ϼ�
					{	
						//printf("#\r\n");

						//��ȡ�洢��������6�������¼
						flash_read_record(password,1,ADDR_FLASH_SECTOR_6);			
						//printf(password);	
						//��������3��ȡ������ת��Ϊ���ͣ����ڱȽϣ���			
						a = atoi(password);	
						memset(password,0,sizeof password);//�������
						
						delchar(keybuf,"#"); //ɾ��# �õ�����
						//printf("%s\r\n",keybuf);
						b = atoi(keybuf);
						
						//��֤�����Ƿ���ȷ
						if(a==b)
						{
							//��˸�Ƽӷ�����
							PFout(8)=1;PFout(9)=0;delay_ms(100);
							PFout(8)=0;PFout(9)=1;delay_ms(100);
							PFout(8)=1;PFout(9)=0;delay_ms(100);
							PFout(8)=0;PFout(9)=1;delay_ms(100);
							PEout(5)=1; //ͨ�磬������  ����
							//printf("��������ɹ���\r\n");
							//��鵱ǰ�����ݼ�¼�Ƿ�С��100��
								if(blue_flag<100)
								{
										/*����FLASH���������FLASH*/
										FLASH_Unlock();
										/* �����Ӧ�ı�־λ*/  
										FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | 
										FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR|FLASH_FLAG_PGSERR);										
										
										//RTC_GetDate����ȡ����
										RTC_GetDate(RTC_Format_BCD, &RTC_DateStructure);
										//printf("DATE-OLED:20%02x/%02x/%02x %0x\r\n",RTC_DateStructure.RTC_Year,RTC_DateStructure.RTC_Month,RTC_DateStructure.RTC_Date,RTC_DateStructure.RTC_WeekDay);	
										//RTC_GetTime����ȡʱ��
										RTC_GetTime(RTC_Format_BCD, &RTC_TimeStructure); 
										//printf("TIME-OLED:%02x:%02x:%02x\r\n",RTC_TimeStructure.RTC_Hours,RTC_TimeStructure.RTC_Minutes,RTC_TimeStructure.RTC_Seconds);								
										//����������������
										sprintf((char *)buf,"[%03d]20%02x/%02x/%02x  %02x:%02x:%02x password unlock\r\n",\
															blue_flag,\
															RTC_DateStructure.RTC_Year, RTC_DateStructure.RTC_Month, RTC_DateStructure.RTC_Date,\
															RTC_TimeStructure.RTC_Hours, RTC_TimeStructure.RTC_Minutes, RTC_TimeStructure.RTC_Seconds);
										
										//д����������4��¼����ʱ�估��ʽ
										if(0==flash_write_record(buf,blue_flag,ADDR_FLASH_SECTOR_4,FLASH_Sector_4))
										{
											  //��ʾ
											printf(buf);
											//��¼�Լ�1
											blue_flag++;
											//�������
											memset(buf,0,sizeof buf);
											OLED_Clear();//�ر���ʾ
											OLED_ShowCHinese(32,4,20);//��
											OLED_ShowCHinese(50,4,21);//��
											OLED_ShowCHinese(68,4,4);//��
											OLED_ShowCHinese(86,4,5);//��
											delay_ms(1000);				
											OLED_DrawBMP(0,2,128,8,BMP2);  //ͼƬ��ʾ(ͼƬ��ʾ���ã����ɵ��ֱ�ϴ󣬻�ռ�ý϶�ռ䣬FLASH�ռ�8K��������)
											delay_ms(1500);	
											OSTaskResume(&Task7_TCB,&err);//�ָ�����7����
											times=0;
											//�����¼���־�� //������ҳ���ѽ���
											OSFlagPost(&g_flag_grp,0x04,OS_OPT_POST_FLAG_SET,&err);								
											memset(keybuf,0,strlen(keybuf));//�������buf	
											i=0;//���i								
											//�����ͷŻ����� ����#�˳�
											OSMutexPost(&g_mutex_oled,OS_OPT_POST_NONE,&err);	
											OSTaskResume(&Task5_TCB,&err); //�ָ�����5����			
											break;
											
										}	
										else
										{
											blue_flag=0;//��¼��־λ��0
											memset(keybuf,0,strlen(keybuf));//�������buf	
											i=0;//���i								
											//�����ͷŻ����� ����#�˳�
											OSMutexPost(&g_mutex_oled,OS_OPT_POST_NONE,&err);	
											OSTaskResume(&Task5_TCB,&err); //�ָ�����5����
											
											break;											
										}
										
								}
								else //���ݳ���100�����������
								{
									blue_flag=0;//��¼��־λ��0
									flash_erase_record(FLASH_Sector_4);//��������4��¼	
									OLED_Clear();//�ر���ʾ
									OLED_ShowCHinese(28,2,4);//��
									OLED_ShowCHinese(46,2,5);//��
									OLED_ShowCHinese(64,2,15);//��
									OLED_ShowCHinese(82,2,16);//��
									OLED_ShowCHinese(20,4,14);//��
									OLED_ShowCHinese(38,4,17);//��
									OLED_ShowCHinese(56,4,14);//��
									OLED_ShowCHinese(74,4,18);//��
									OLED_ShowCHinese(92,4,19);//��
									delay_ms(5000);	
									memset(keybuf,0,strlen(keybuf));//�������buf	
									i=0;//���i								
									//�����ͷŻ����� ����#�˳�
									OSMutexPost(&g_mutex_oled,OS_OPT_POST_NONE,&err);	
									OSTaskResume(&Task5_TCB,&err); //�ָ�����5����
										
									break;
								}
						}
						else
						{
							OLED_Clear();//�ر���ʾ	
							OLED_ShowCHinese(32,4,20);//��
							OLED_ShowCHinese(50,4,21);//��
							OLED_ShowCHinese(68,4,35);//��
							OLED_ShowCHinese(86,4,36);//��				
							delay_ms(1500);
							OLED_ShowCHinese(18,0,26);//��
							OLED_ShowCHinese(36,0,27);//��
							OLED_ShowCHinese(54,0,28);//��
							OLED_ShowCHinese(72,0,20);//��
							OLED_ShowCHinese(90,0,21);//��
							OLED_ShowString(18,4,"-----------");
							OLED_ShowString(0,6,"#:");//#:
							OLED_ShowCHinese(14,6,29);//ȷ
							OLED_ShowCHinese(28,6,30);//��
							OLED_ShowString(42,6,"C:");//#:			
							OLED_ShowCHinese(56,6,31);//ɾ
							OLED_ShowCHinese(70,6,32);//��
							OLED_ShowString(84,6,"D:");//D:		
							OLED_ShowCHinese(98,6,33);//��
							OLED_ShowCHinese(112,6,34);//��						
							memset(keybuf,0,strlen(keybuf));//�������buf	
							i=0;//���i						
										
						}				
					}	
					if(strlen(keybuf)>6)
					{
						OLED_Clear();//�ر���ʾ	
						OLED_ShowCHinese(32,4,20);//��
						OLED_ShowCHinese(50,4,21);//��
						OLED_ShowCHinese(68,4,47);//��
						OLED_ShowCHinese(86,4,48);//��					
						delay_ms(2000);
						OLED_Clear();//�ر���ʾ				
						OLED_ShowCHinese(18,0,26);//��
						OLED_ShowCHinese(36,0,27);//��
						OLED_ShowCHinese(54,0,28);//��
						OLED_ShowCHinese(72,0,20);//��
						OLED_ShowCHinese(90,0,21);//��
						OLED_ShowString(18,4,"-----------");
						OLED_ShowString(0,6,"#:");//#:
						OLED_ShowCHinese(14,6,29);//ȷ
						OLED_ShowCHinese(28,6,30);//��
						OLED_ShowString(42,6,"C:");//C:			
						OLED_ShowCHinese(56,6,31);//ɾ
						OLED_ShowCHinese(70,6,32);//��
						OLED_ShowString(84,6,"D:");//D:		
						OLED_ShowCHinese(98,6,33);//��
						OLED_ShowCHinese(112,6,34);//��							
						memset(keybuf,0,strlen(keybuf));//�������buf	
						i=0;//���i			
					}
					
				}		
			}	
			if(strcmp(key,"C")==0) //¼ָ��
			{
				OSTaskSuspend(&Task7_TCB,&err); //��������7  �����Ͽ�60���Զ�����
				OSTaskSuspend(&Task5_TCB,&err); //��ͣ����5����
				//printf("B\r\n");
				//�����ȴ��ź���  //ͬ��
				OSSemPend(&g_sem,0,OS_OPT_PEND_BLOCKING,NULL,&err);						
				//�����ȴ�������  //ռ����
				OSMutexPend(&g_mutex_oled,0,OS_OPT_PEND_BLOCKING,NULL,&err);	
				OLED_Clear();//�ر���ʾ	
				OLED_ShowString(20,2,"B:");//B
				OLED_ShowCHinese(38,2,81); //¼
				OLED_ShowCHinese(56,2,82); //��
				OLED_ShowCHinese(74,2,83); //ָ
				OLED_ShowCHinese(92,2,84); //��	
				OLED_ShowString(20,4,"C:");//C
				OLED_ShowCHinese(38,4,92); //ɾ
				OLED_ShowCHinese(56,4,93); //��
				OLED_ShowCHinese(74,4,83); //ָ
				OLED_ShowCHinese(92,4,84); //��		
				OLED_ShowString(20,6,"D:");//D
				OLED_ShowCHinese(38,6,33); //��
				OLED_ShowCHinese(56,6,34); //��				
				while(1) //���� ָ�ƹ�������ȡ��ֵ��������ȡ��ֵ
				{	
					rt=getfingerprint_key();
					if(rt==1) //B//¼ָ��
					{			
						printf("1\r\n");						
						if(Add_FR()==1);//¼ָ��
						{
							OLED_ShowString(20,2,"B:");//B
							OLED_ShowCHinese(38,2,81); //¼
							OLED_ShowCHinese(56,2,82); //��
							OLED_ShowCHinese(74,2,83); //ָ
							OLED_ShowCHinese(92,2,84); //��	
							OLED_ShowString(20,4,"C:");//C
							OLED_ShowCHinese(38,4,92); //ɾ
							OLED_ShowCHinese(56,4,93); //��
							OLED_ShowCHinese(74,4,83); //ָ
							OLED_ShowCHinese(92,4,84); //��		
							OLED_ShowString(20,6,"D:");//D
							OLED_ShowCHinese(38,6,33); //��
							OLED_ShowCHinese(56,6,34); //��							
						}
					}
					if(rt==2)//	Cɾ��ָ��				
					{		
						printf("2\r\n");						
						if(Del_FR()==1)//ɾ��ָ�Ƴɹ�
						{	
							OLED_ShowString(20,2,"B:");//B
							OLED_ShowCHinese(38,2,81); //¼
							OLED_ShowCHinese(56,2,82); //��
							OLED_ShowCHinese(74,2,83); //ָ
							OLED_ShowCHinese(92,2,84); //��	
							OLED_ShowString(20,4,"C:");//C
							OLED_ShowCHinese(38,4,92); //ɾ
							OLED_ShowCHinese(56,4,93); //��
							OLED_ShowCHinese(74,4,83); //ָ
							OLED_ShowCHinese(92,4,84); //��		
							OLED_ShowString(20,6,"D:");//D
							OLED_ShowCHinese(38,6,33); //��
							OLED_ShowCHinese(56,6,34); //��	
						}
						else //ɾ��ָ��ʧ��
						{
							OLED_ShowString(20,2,"B:");//B
							OLED_ShowCHinese(38,2,81); //¼
							OLED_ShowCHinese(56,2,82); //��
							OLED_ShowCHinese(74,2,83); //ָ
							OLED_ShowCHinese(92,2,84); //��	
							OLED_ShowString(20,4,"C:");//C
							OLED_ShowCHinese(38,4,92); //ɾ
							OLED_ShowCHinese(56,4,93); //��
							OLED_ShowCHinese(74,4,83); //ָ
							OLED_ShowCHinese(92,4,84); //��		
							OLED_ShowString(20,6,"D:");//D
							OLED_ShowCHinese(38,6,33); //��
							OLED_ShowCHinese(56,6,34); //��				
						}						
						
					}
					if(rt==3) //D�˳�
					{
						//�����ͷŻ�����   //�˳�
						OSMutexPost(&g_mutex_oled,OS_OPT_POST_NONE,&err);						
						OSTaskResume(&Task5_TCB,&err); //�ָ�����5����
										
						break;
					}	
					
				}		
			}

		}	
	
		memset(key,0,msg_size);//�������p
	

	}		
	
}
//������  RFID����
void task5(void *parg)
{

	OS_ERR err;
	int card_buf[5]={0};//��ſ��ŵ�10����  ������ת��10����
	int card_tset[5]={105,149,113,193,76};//���ڶԱȿ����Ƿ����
	int flag=0; //�Աȱ�־λ
	while(1)
	{
		delay_ms(20);
		MFRC522_Initializtion();//��ʼ��RFID
		status=MFRC522_Request(0x52, card_pydebuf);			//Ѱ��	

		//�����ȴ��ź���  //ͬ��
		OSSemPend(&g_sem,0,OS_OPT_PEND_BLOCKING,NULL,&err);		
		//�����ȴ�������  //ռ������������ 
		OSMutexPend(&g_mutex_oled,0,OS_OPT_PEND_BLOCKING,NULL,&err);			
		if(status==0)		//���������
		{
			status=MFRC522_Anticoll(card_numberbuf);			//��ײ����			
			card_size=MFRC522_SelectTag(card_numberbuf);	//ѡ��  ���ؿ�������
			status=MFRC522_Auth(0x60, 4, card_key0Abuf, card_numberbuf);	//�鿨
			status=MFRC522_Write(4, card_writebuf);				//д����д��ҪС�ģ��ر��Ǹ����Ŀ�3��
			status=MFRC522_Read(4, card_readbuf);					//����
			
			//��������ʾ		
			printf("card_pydebuf:%02X %02X\r\n",card_pydebuf[0],card_pydebuf[1]);
			
			//�����к���ʾ�����һ�ֽ�Ϊ����У����
			printf("card_numberbuf:");
			for(i=0;i<5;i++)
			{
				card_buf[i]=card_numberbuf[i]; //���ַ��Ϳ���ת��int�� �������Ա�
				//printf("h:%d ",card_buf[i]);
			}		
		//	printf("\r\n");
			
			for(i=0;i<5;i++)
			{
					//printf("j:%d ",card_tset[i]);
					if(card_buf[i]==card_tset[i])  //�ж��ǲ����ҹ涨�Ŀ���
					{
						flag++; //�Աȱ�־λ
					}
			}	
		//	printf("\r\n");			
		//	printf("flag:%d\r\n",flag);	
			
			//��������ʾ����λΪKbits
			printf("card_size=%dKbits\r\n",card_size);
			
			//����״̬��ʾ������Ϊ0
			printf("status:%02X\r\n",status);
			
			//��һ�����������ʾ
			printf("card_readbuf:");
			for(i=0;i<8;i++)		//��������ʾ
			{
				printf("%02X ",card_readbuf[i]);
			}			
			printf("\r\n");
			if(strcmp((const char*)card_readbuf,(const char*)card_writebuf)==0&&flag==5) //���д��ĺͶ�������һ���ҿ��ŵ����趨�õĿ��� �Ϳ����ɹ�
			{
					//��˸�Ƽӷ�����
					PFout(8)=1;PFout(9)=0;delay_ms(100);
					PFout(8)=0;PFout(9)=1;delay_ms(100);
					PFout(8)=1;PFout(9)=0;delay_ms(100);
					PFout(8)=0;PFout(9)=1;delay_ms(100);	
					PEout(5)=1; //ͨ�磬������  ����				
					flag=0;	//��նԱȱ�־λ						
				  //��鵱ǰ�����ݼ�¼�Ƿ�С��100��
					if(blue_flag<100)
					{
						/*����FLASH���������FLASH*/
						FLASH_Unlock();
						/* �����Ӧ�ı�־λ*/  
						FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | 
						FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR|FLASH_FLAG_PGSERR);										
									
						//RTC_GetDate����ȡ����
						RTC_GetDate(RTC_Format_BCD, &RTC_DateStructure);
						//printf("DATE-OLED:20%02x/%02x/%02x %0x\r\n",RTC_DateStructure.RTC_Year,RTC_DateStructure.RTC_Month,RTC_DateStructure.RTC_Date,RTC_DateStructure.RTC_WeekDay);	
						//RTC_GetTime����ȡʱ��
						RTC_GetTime(RTC_Format_BCD, &RTC_TimeStructure); 
						//printf("TIME-OLED:%02x:%02x:%02x\r\n",RTC_TimeStructure.RTC_Hours,RTC_TimeStructure.RTC_Minutes,RTC_TimeStructure.RTC_Seconds);								
						//����������������
						sprintf((char *)buf,"[%03d]20%02x/%02x/%02x  %02x:%02x:%02x RFID unlock\r\n",\
														blue_flag,\
														RTC_DateStructure.RTC_Year, RTC_DateStructure.RTC_Month, RTC_DateStructure.RTC_Date,\
														RTC_TimeStructure.RTC_Hours, RTC_TimeStructure.RTC_Minutes, RTC_TimeStructure.RTC_Seconds);
									
						//д����������4��¼����ʱ�估��ʽ
						if(0==flash_write_record(buf,blue_flag,ADDR_FLASH_SECTOR_4,FLASH_Sector_4))
						{
								//��ʾ
								printf(buf);
								//��¼�Լ�1
								blue_flag++;
								//�������
								memset(buf,0,sizeof buf);
								OLED_Clear();//�ر���ʾ
								OLED_ShowString(36,4,"RFID");//RFID
								OLED_ShowCHinese(68,4,4);//��
								OLED_ShowCHinese(86,4,5);//��
								delay_ms(1000);				
								OLED_DrawBMP(0,2,128,8,BMP2);  //ͼƬ��ʾ(ͼƬ��ʾ���ã����ɵ��ֱ�ϴ󣬻�ռ�ý϶�ռ䣬FLASH�ռ�8K��������)
								delay_ms(1500);			
								OSTaskResume(&Task7_TCB,&err);//�ָ�����7����
								times=0;
								//�����¼���־�� //������ҳ���ѽ���
								OSFlagPost(&g_flag_grp,0x04,OS_OPT_POST_FLAG_SET,&err);								
								memset(card_readbuf,0,strlen((char*)card_readbuf));//�������card_readbuf								
						}	
						else
						{
							blue_flag=0;//��¼��־λ��0
						}
									
					}
					else //���ݳ���100�����������
					{
						blue_flag=0;//��¼��־λ��0
						flash_erase_record(FLASH_Sector_4);//��������4��¼	
						OLED_Clear();//�ر���ʾ
						OLED_ShowCHinese(28,2,4);//��
						OLED_ShowCHinese(46,2,5);//��
						OLED_ShowCHinese(64,2,15);//��
						OLED_ShowCHinese(82,2,16);//��
						OLED_ShowCHinese(20,4,14);//��
						OLED_ShowCHinese(38,4,17);//��
						OLED_ShowCHinese(56,4,14);//��
						OLED_ShowCHinese(74,4,18);//��
						OLED_ShowCHinese(92,4,19);//��
						delay_ms(5000);				
					}
							
			}
			else//���д��ĺͶ������Ĳ�һ���Ϳ���ʧ��
			{
				//printf("����ʧ�ܣ�\r\n"); 
				OLED_Clear();//�ر���ʾ	
				OLED_ShowString(18,4,"RFID");//RFID
				OLED_ShowCHinese(68,4,35);//��
				OLED_ShowCHinese(86,4,36);//��				
				delay_ms(1500);	
				OLED_Clear();//�ر���ʾ		
			}
		
			
		}	
		//�����ͷŻ����� ����#�˳�
		OSMutexPost(&g_mutex_oled,OS_OPT_POST_NONE,&err);		
	
	}
}

//������  as608ָ��ģ�����
void task6(void *parg)
{
	OS_ERR err;
	while(1)
	{
		//�����ȴ��ź���  //ͬ��
		OSSemPend(&g_sem,0,OS_OPT_PEND_BLOCKING,NULL,&err);	
					
		if(PS_Sta)	 //���PS_Sta״̬���������ָ����
		{	
			//�����ȴ�������  //ռ����
			OSMutexPend(&g_mutex_oled,0,OS_OPT_PEND_BLOCKING,NULL,&err);				
			if(press_FR()==0)//ˢָ��
			{
					//��˸�Ƽӷ�����
					PFout(8)=1;PFout(9)=0;delay_ms(100);
					PFout(8)=0;PFout(9)=1;delay_ms(100);
					PFout(8)=1;PFout(9)=0;delay_ms(100);
					PFout(8)=0;PFout(9)=1;delay_ms(100);
					PEout(5)=1; //ͨ�磬������  ����
					//printf("��������ɹ���\r\n");
					//��鵱ǰ�����ݼ�¼�Ƿ�С��100��
						if(blue_flag<100)
						{
								/*����FLASH���������FLASH*/
								FLASH_Unlock();
								/* �����Ӧ�ı�־λ*/  
								FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | 
								FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR|FLASH_FLAG_PGSERR);										
								
								//RTC_GetDate����ȡ����
								RTC_GetDate(RTC_Format_BCD, &RTC_DateStructure);
								//printf("DATE-OLED:20%02x/%02x/%02x %0x\r\n",RTC_DateStructure.RTC_Year,RTC_DateStructure.RTC_Month,RTC_DateStructure.RTC_Date,RTC_DateStructure.RTC_WeekDay);	
								//RTC_GetTime����ȡʱ��
								RTC_GetTime(RTC_Format_BCD, &RTC_TimeStructure); 
								//printf("TIME-OLED:%02x:%02x:%02x\r\n",RTC_TimeStructure.RTC_Hours,RTC_TimeStructure.RTC_Minutes,RTC_TimeStructure.RTC_Seconds);								
								//����������������
								sprintf((char *)buf,"[%03d]20%02x/%02x/%02x  %02x:%02x:%02x fingerprint unlock\r\n",\
													blue_flag,\
													RTC_DateStructure.RTC_Year, RTC_DateStructure.RTC_Month, RTC_DateStructure.RTC_Date,\
													RTC_TimeStructure.RTC_Hours, RTC_TimeStructure.RTC_Minutes, RTC_TimeStructure.RTC_Seconds);
								
								//д����������4��¼����ʱ�估��ʽ
								if(0==flash_write_record(buf,blue_flag,ADDR_FLASH_SECTOR_4,FLASH_Sector_4))
								{
									  //��ʾ
									printf(buf);
									//��¼�Լ�1
									blue_flag++;
									//�������
									memset(buf,0,sizeof buf);
									OLED_Clear();//�ر���ʾ
									OLED_ShowCHinese(32,4,83);//ָ
									OLED_ShowCHinese(50,4,84);//��
									OLED_ShowCHinese(68,4,4);//��
									OLED_ShowCHinese(86,4,5);//��
									delay_ms(1000);				
									OLED_DrawBMP(0,2,128,8,BMP2);  //ͼƬ��ʾ(ͼƬ��ʾ���ã����ɵ��ֱ�ϴ󣬻�ռ�ý϶�ռ䣬FLASH�ռ�8K��������)
									delay_ms(1500);	
									OSTaskResume(&Task7_TCB,&err);//�ָ�����7����
									times=0;
									//�����¼���־�� //������ҳ���ѽ���
									OSFlagPost(&g_flag_grp,0x04,OS_OPT_POST_FLAG_SET,&err);																			
								}	
								else
								{
									blue_flag=0;//��¼��־λ��0
								}
								
						}
						else //���ݳ���100�����������
						{
							blue_flag=0;//��¼��־λ��0
							flash_erase_record(FLASH_Sector_4);//��������4��¼	
							OLED_Clear();//�ر���ʾ
							OLED_ShowCHinese(28,2,4);//��
							OLED_ShowCHinese(46,2,5);//��
							OLED_ShowCHinese(64,2,15);//��
							OLED_ShowCHinese(82,2,16);//��
							OLED_ShowCHinese(20,4,14);//��
							OLED_ShowCHinese(38,4,17);//��
							OLED_ShowCHinese(56,4,14);//��
							OLED_ShowCHinese(74,4,18);//��
							OLED_ShowCHinese(92,4,19);//��
							delay_ms(5000);				
						}				
				//�����ͷŻ����� ����#�˳�
				OSMutexPost(&g_mutex_oled,OS_OPT_POST_NONE,&err);					
			}
			else
			{
				OLED_Clear(); 
				OLED_ShowCHinese(28,3,83); //ָ
				OLED_ShowCHinese(46,3,84); //��
				OLED_ShowCHinese(64,3,35); //��
				OLED_ShowCHinese(82,3,36); //��
				ShowErrMessage(ensure);	
				delay_ms(1000);				
				//�����ͷŻ����� ����#�˳�
				OSMutexPost(&g_mutex_oled,OS_OPT_POST_NONE,&err);				
			}
		}	
				
	} 	
	

}
//����7 �Ͽ�����60���Զ�����
void task7(void *parg)
{
		OS_ERR err;	
		OS_FLAGS flags=0;
		while(1)
		{	
			//�����ȴ��ź���  //ͬ��
			OSSemPend(&g_sem,0,OS_OPT_PEND_BLOCKING,NULL,&err);						
			//һֱ�����ȴ��¼���־��bit0(RTC�жϱ�־λ)��1  bit1  bit2���ȴ��ɹ��󣬽�bit0��0
			flags=OSFlagPend(&g_flag_grp,0x08,0,OS_OPT_PEND_FLAG_SET_ANY + OS_OPT_PEND_FLAG_CONSUME+OS_OPT_PEND_BLOCKING,NULL,&err);				
			times++;
			printf("times=%d\r\n",times);
			if(flags &0x08)
			{				
				if(PGin(9)!=1&&times==10) //�����ⳬ��10sû���������� �Զ�����
				{				
							//�����ȴ�������  //ռ������������ 
							OSMutexPend(&g_mutex_oled,0,OS_OPT_PEND_BLOCKING,NULL,&err);						
							//��˸�Ƽӷ�����
							PFout(8)=1;PFout(9)=0;delay_ms(100);
							PFout(8)=0;PFout(9)=1;delay_ms(100);
							PFout(8)=1;PFout(9)=0;delay_ms(100);
							PFout(8)=0;PFout(9)=1;delay_ms(100);
							PEout(5)=0; //�ϵ磬�ɿ���  ����						
							OLED_Clear();//�ر���ʾ
							OLED_ShowCHinese(32,4,98);//��
							OLED_ShowCHinese(50,4,99);//��
							OLED_ShowCHinese(68,4,100);//��
							OLED_ShowCHinese(86,4,101);//��
							delay_ms(1000);
							OLED_DrawBMP(0,2,128,8,BMP1);  //ͼƬ��ʾ(ͼƬ��ʾ���ã����ɵ��ֱ�ϴ󣬻�ռ�ý϶�ռ䣬FLASH�ռ�8K��������)
							delay_ms(2000);
							
							
							//�����¼���־��  ����������������
							OSFlagPost(&g_flag_grp,0x02,OS_OPT_POST_FLAG_SET,&err);					
							//�����ͷŻ����� ����#�˳�
							OSMutexPost(&g_mutex_oled,OS_OPT_POST_NONE,&err);	
							OSTaskSuspend(&Task7_TCB,&err); //��������7  �����Ͽ�60���Զ�����
				}	
				if(PGin(9)==1)
				{
					times=0;
				}	
			}
		}	
}
