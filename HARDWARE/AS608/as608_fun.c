#include "as608_fun.h"
SysPara AS608Para;//ָ��ģ��AS608����
u16 ValidN;//ģ������Чָ�Ƹ���
extern OS_Q	key_queue;					//key������Ϣ����
//��ʾȷ���������Ϣ
void ShowErrMessage(u8 ensure)
{
	printf("%s\r\n",(u8*)EnsureMessage(ensure));
}

//��ȡ������ֵ
u16 GET_NUM(void)
{
	char *p=NULL;
	OS_ERR  err;
	OS_MSG_SIZE msg_size;
	char buf[3];
	memset(buf,0,3);
	while(1)
	{
		p=OSQPend(&key_queue,0,OS_OPT_PEND_BLOCKING,&msg_size,NULL,&err);
		if(p && msg_size)
		{
			/*******************�˸�*******************/
			if(strcmp((char *)p,"B")==0)
			{
				OLED_ShowString(38,3,"          ");
				buf[strlen(buf)-1] = '\0';
				printf("buf:%s\r\n",buf);
				OLED_ShowString(38,3,(u8 *)buf);
			}
			/*******************���*******************/
			else if(strcmp((char *)p,"C")==0)
			{
				memset(buf,0,sizeof buf);
				OLED_ShowString(38,3,"          ");
			}
			/*******************ȷ��*******************/
			else if(strcmp((char *)p,"D")==0)
			{
				printf("ȷ��\r\n");
				break;	
			}
			/********************ID********************/
			else
			{
				//�ж��Ƿ����3���ֽ�
				if(strlen(buf)<3)
				{
					strcat(buf,p);
					OLED_ShowString(38,3,(u8 *)buf);
				}
				else
				{
					OLED_ShowString(38,3,(u8 *)buf);
				}
			}
		}
	}
	
	return atoi(buf);
}

//¼ָ��
uint32_t Add_FR(void)
{
	u8 i,ensure ,processnum=0;
	u16 ID;
	while(1)
	{
		switch (processnum)
		{
			case 0:
				i++;
				OLED_Clear();
				OLED_ShowCHinese(28,3,49);  //��
				OLED_ShowCHinese(46,3,50); //��
				OLED_ShowCHinese(64,3,51); //ָ
				OLED_ShowCHinese(82,3,52); //��
				ensure=PS_GetImage();
				if(ensure==0x00) 
				{
					BEEP=1;
					ensure=PS_GenChar(CharBuffer1);//��������
					BEEP=0;
					if(ensure==0x00)
					{
						OLED_Clear();
						OLED_ShowCHinese(28,3,51); //ָ
						OLED_ShowCHinese(46,3,52); //��
						OLED_ShowCHinese(64,3,53); //��
						OLED_ShowCHinese(82,3,54); //��
						i=0;
						processnum=1;//�����ڶ���						
					}else ShowErrMessage(ensure);				
				}else ShowErrMessage(ensure);						
				break;
			
			case 1:
				i++;
				OLED_Clear();
				OLED_ShowCHinese(0,3,55);   //��
				OLED_ShowCHinese(18,3,56); //��
				OLED_ShowCHinese(36,3,57); //��
				OLED_ShowCHinese(54,3,58); //һ
				OLED_ShowCHinese(72,3,59); //��
				OLED_ShowCHinese(90,3,51); //ָ
				OLED_ShowCHinese(108,3,52);//��
				ensure=PS_GetImage();
				if(ensure==0x00) 
				{
					BEEP=1;
					ensure=PS_GenChar(CharBuffer2);//��������
					BEEP=0;
					if(ensure==0x00)
					{
						OLED_Clear();
						OLED_ShowCHinese(28,3,51); //ָ
						OLED_ShowCHinese(46,3,52); //��
						OLED_ShowCHinese(64,3,53); //��
						OLED_ShowCHinese(82,3,54); //��
						i=0;
						processnum=2;//����������
					}else ShowErrMessage(ensure);	
				}else ShowErrMessage(ensure);		
						break;

			case 2:
				OLED_Clear();
				OLED_ShowCHinese(28,3,60); //��
				OLED_ShowCHinese(46,3,61); //��
				OLED_ShowCHinese(64,3,62); //��
				OLED_ShowCHinese(82,3,63); //��
				ensure=PS_Match();
				if(ensure==0x00) 
				{
					OLED_Clear();
					OLED_ShowCHinese(28,3,62); //��
					OLED_ShowCHinese(46,3,63); //��
					OLED_ShowCHinese(64,3,64); //��
					OLED_ShowCHinese(82,3,65); //��
					processnum=3;//�������Ĳ�
				}
				else 
				{
					OLED_Clear();
					OLED_ShowCHinese(28,3,62); //��
					OLED_ShowCHinese(46,3,63); //��
					OLED_ShowCHinese(64,3,66); //ʧ
					OLED_ShowCHinese(82,3,67); //��
					ShowErrMessage(ensure);
					i=0;
					processnum=0;//���ص�һ��		
				}
				delay_ms(1200);
				break;

			case 3:
				OLED_Clear();
				OLED_ShowCHinese(10,3,68); //��
				OLED_ShowCHinese(28,3,69); //��
				OLED_ShowCHinese(46,3,70); //��
				OLED_ShowCHinese(64,3,71); //��
				OLED_ShowCHinese(82,3,72); //ģ
				OLED_ShowCHinese(100,3,73);//��
				ensure=PS_RegModel();
				if(ensure==0x00) 
				{
					OLED_Clear();
					OLED_ShowCHinese(10,3,70); //��
					OLED_ShowCHinese(28,3,71); //��
					OLED_ShowCHinese(46,3,72); //ģ
					OLED_ShowCHinese(64,3,73); //��
					OLED_ShowCHinese(82,3,64); //��
					OLED_ShowCHinese(100,3,65);//��
					processnum=4;//�������岽
				}else {processnum=0;ShowErrMessage(ensure);}
				delay_ms(1200);
				break;
				
			case 4:	
				OLED_Clear();
				OLED_ShowCHinese(10,0,74);  //��
				OLED_ShowCHinese(28,0,75); //��
				OLED_ShowCHinese(46,0,76); //��
				OLED_ShowCHinese(64,0,77); //��
				OLED_ShowCHinese(82,0,78); //��
				OLED_ShowString(100,0,"I");//I
				OLED_ShowString(108,0,"D");//D
				OLED_ShowString(0,6,"ID<300"); //ID<300
				OLED_ShowString(81,6,"D"); //D
				OLED_ShowCHinese(90,6,79); //ȷ
				OLED_ShowCHinese(108,6,80);//��
				do{
					ID=GET_NUM();
					printf("ID:%d\r\n",ID);}
				while(!(ID<AS608Para.PS_max));//����ID����С��ģ������������ֵ
				ensure=PS_StoreChar(CharBuffer2,ID);//����ģ��
				if(ensure==0x00) 
				{			
					OLED_Clear();
					OLED_ShowCHinese(10,3,81); //¼
					OLED_ShowCHinese(28,3,82); //��
					OLED_ShowCHinese(46,3,83); //ָ
					OLED_ShowCHinese(64,3,84); //��
					OLED_ShowCHinese(82,3,85); //��
					OLED_ShowCHinese(100,3,86);//��
					PS_ValidTempleteNum(&ValidN);//����ָ�Ƹ���
					OLED_ShowCHinese(15,6,83); //ָ
					OLED_ShowCHinese(33,6,84); //��
					OLED_ShowCHinese(51,6,87); //��
					OLED_ShowCHinese(69,6,88); //��
					OLED_ShowNum(87,6,ValidN,5,16);
					delay_ms(1500);
					OLED_Clear();
					return 1;
				}else {processnum=0;ShowErrMessage(ensure);}					
				break;				
		}
		delay_ms(400);
		if(i==5)//����5��û�а���ָ���˳�
		{
			OLED_Clear();
			break;	
		}				
	}
	return 0;
}




//ˢָ��
uint32_t press_FR(void)
{
	SearchResult seach;
	u8 ensure;
	ensure=PS_GetImage();
	if(ensure==0x00)//��ȡͼ��ɹ� 
	{	
		//BEEP=1;//�򿪷�����	
		ensure=PS_GenChar(CharBuffer1);
		if(ensure==0x00) //���������ɹ�
		{		
			//BEEP=0;//�رշ�����	
			ensure=PS_HighSpeedSearch(CharBuffer1,0,AS608Para.PS_max,&seach);
			if(ensure==0x00)//�����ɹ�
			{				
				printf("ָ�ƿ�ƥ��ɹ���,ID:%d  ƥ��÷�:%d\r\n",seach.pageID,seach.mathscore);
			}
			else 
			{
				printf("ƥ��ʧ�ܣ�\r\n");
				return 1;
			}
								
		}
		else
			ShowErrMessage(ensure);
	 //BEEP=0;//�رշ�����
	 delay_ms(600);
	}	
	return 0;
}

//ɾ��ָ��
uint32_t Del_FR(void)
{
	u8  ensure;
	u16 num;
	u16 vnum1; //ɾ��ǰ��ָ�Ƹ����Ա�
	u16 vnum2; //ɾ��ǰ��ָ�Ƹ����Ա�	
	OLED_Clear();
	OLED_ShowCHinese(10,0,89);  //��
	OLED_ShowCHinese(28,0,90); //��
	OLED_ShowCHinese(46,0,91); //��
	OLED_ShowCHinese(64,0,92); //ɾ
	OLED_ShowCHinese(82,0,93); //��
	OLED_ShowString(100,0,"I");//I
	OLED_ShowString(108,0,"D");//D
	OLED_ShowString(0,6,"ID<300"); //ID<300
	OLED_ShowString(81,6,"D"); //D
	OLED_ShowCHinese(90,6,94); //ȷ
	OLED_ShowCHinese(108,6,95);//��
	delay_ms(50);	
	PS_ValidTempleteNum(&ValidN);//����ָ�Ƹ���	
	vnum1=ValidN;
	num=GET_NUM();//��ȡ���ص���ֵ
	if(num==0xFF00)
		ensure=PS_Empty();//���ָ�ƿ�
	else 
		ensure=PS_DeletChar(num,1);//ɾ������ָ��
	if(ensure==0)
	{
			PS_ValidTempleteNum(&ValidN);//����ָ�Ƹ���	
			vnum2=ValidN;	
			if(vnum1!=vnum2)
			{	
				OLED_Clear();
				OLED_ShowCHinese(10,3,92);  //ɾ
				OLED_ShowCHinese(28,3,93);  //��
				OLED_ShowCHinese(46,3,83);  //ָ
				OLED_ShowCHinese(64,3,84);  //��
				OLED_ShowCHinese(82,3,85);  //��
				OLED_ShowCHinese(100,3,86); //��	
				delay_ms(1000);
				OLED_Clear();
				PS_ValidTempleteNum(&ValidN);//����ָ�Ƹ���
				OLED_ShowCHinese(20,4,83); //ָ
				OLED_ShowCHinese(38,4,84); //��
				OLED_ShowCHinese(56,4,87); //��
				OLED_ShowCHinese(74,4,88); //��
				OLED_ShowNum(90,4,ValidN,3,16);
				delay_ms(2000);	
				OLED_Clear();		
				return 1;
			}
			else
			{
				OLED_Clear();
				OLED_ShowCHinese(10,3,92);  //ɾ
				OLED_ShowCHinese(28,3,93);  //��
				OLED_ShowCHinese(46,3,83);  //ָ
				OLED_ShowCHinese(64,3,84);  //��
				OLED_ShowCHinese(82,3,66);  //��
				OLED_ShowCHinese(100,3,67); //��	
				delay_ms(1000);
				OLED_Clear();
				PS_ValidTempleteNum(&ValidN);//����ָ�Ƹ���
				OLED_ShowCHinese(20,4,83); //ָ
				OLED_ShowCHinese(38,4,84); //��
				OLED_ShowCHinese(56,4,87); //��
				OLED_ShowCHinese(74,4,88); //��
				OLED_ShowNum(90,4,ValidN,3,16);
				delay_ms(2000);	
				OLED_Clear();		
				return 2;				
			}
	}
	return 0;
}



