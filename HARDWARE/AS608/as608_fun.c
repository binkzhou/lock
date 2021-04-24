#include "as608_fun.h"
SysPara AS608Para;//指纹模块AS608参数
u16 ValidN;//模块内有效指纹个数
extern OS_Q	key_queue;					//key按键消息队列
//显示确认码错误信息
void ShowErrMessage(u8 ensure)
{
	printf("%s\r\n",(u8*)EnsureMessage(ensure));
}

//获取键盘数值
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
			/*******************退格*******************/
			if(strcmp((char *)p,"B")==0)
			{
				OLED_ShowString(38,3,"          ");
				buf[strlen(buf)-1] = '\0';
				printf("buf:%s\r\n",buf);
				OLED_ShowString(38,3,(u8 *)buf);
			}
			/*******************清空*******************/
			else if(strcmp((char *)p,"C")==0)
			{
				memset(buf,0,sizeof buf);
				OLED_ShowString(38,3,"          ");
			}
			/*******************确定*******************/
			else if(strcmp((char *)p,"D")==0)
			{
				printf("确定\r\n");
				break;	
			}
			/********************ID********************/
			else
			{
				//判断是否大于3个字节
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

//录指纹
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
				OLED_ShowCHinese(28,3,49);  //请
				OLED_ShowCHinese(46,3,50); //按
				OLED_ShowCHinese(64,3,51); //指
				OLED_ShowCHinese(82,3,52); //纹
				ensure=PS_GetImage();
				if(ensure==0x00) 
				{
					BEEP=1;
					ensure=PS_GenChar(CharBuffer1);//生成特征
					BEEP=0;
					if(ensure==0x00)
					{
						OLED_Clear();
						OLED_ShowCHinese(28,3,51); //指
						OLED_ShowCHinese(46,3,52); //纹
						OLED_ShowCHinese(64,3,53); //正
						OLED_ShowCHinese(82,3,54); //常
						i=0;
						processnum=1;//跳到第二步						
					}else ShowErrMessage(ensure);				
				}else ShowErrMessage(ensure);						
				break;
			
			case 1:
				i++;
				OLED_Clear();
				OLED_ShowCHinese(0,3,55);   //请
				OLED_ShowCHinese(18,3,56); //再
				OLED_ShowCHinese(36,3,57); //按
				OLED_ShowCHinese(54,3,58); //一
				OLED_ShowCHinese(72,3,59); //次
				OLED_ShowCHinese(90,3,51); //指
				OLED_ShowCHinese(108,3,52);//纹
				ensure=PS_GetImage();
				if(ensure==0x00) 
				{
					BEEP=1;
					ensure=PS_GenChar(CharBuffer2);//生成特征
					BEEP=0;
					if(ensure==0x00)
					{
						OLED_Clear();
						OLED_ShowCHinese(28,3,51); //指
						OLED_ShowCHinese(46,3,52); //纹
						OLED_ShowCHinese(64,3,53); //正
						OLED_ShowCHinese(82,3,54); //常
						i=0;
						processnum=2;//跳到第三步
					}else ShowErrMessage(ensure);	
				}else ShowErrMessage(ensure);		
						break;

			case 2:
				OLED_Clear();
				OLED_ShowCHinese(28,3,60); //正
				OLED_ShowCHinese(46,3,61); //在
				OLED_ShowCHinese(64,3,62); //对
				OLED_ShowCHinese(82,3,63); //比
				ensure=PS_Match();
				if(ensure==0x00) 
				{
					OLED_Clear();
					OLED_ShowCHinese(28,3,62); //对
					OLED_ShowCHinese(46,3,63); //比
					OLED_ShowCHinese(64,3,64); //成
					OLED_ShowCHinese(82,3,65); //功
					processnum=3;//跳到第四步
				}
				else 
				{
					OLED_Clear();
					OLED_ShowCHinese(28,3,62); //对
					OLED_ShowCHinese(46,3,63); //比
					OLED_ShowCHinese(64,3,66); //失
					OLED_ShowCHinese(82,3,67); //败
					ShowErrMessage(ensure);
					i=0;
					processnum=0;//跳回第一步		
				}
				delay_ms(1200);
				break;

			case 3:
				OLED_Clear();
				OLED_ShowCHinese(10,3,68); //正
				OLED_ShowCHinese(28,3,69); //在
				OLED_ShowCHinese(46,3,70); //生
				OLED_ShowCHinese(64,3,71); //成
				OLED_ShowCHinese(82,3,72); //模
				OLED_ShowCHinese(100,3,73);//板
				ensure=PS_RegModel();
				if(ensure==0x00) 
				{
					OLED_Clear();
					OLED_ShowCHinese(10,3,70); //生
					OLED_ShowCHinese(28,3,71); //成
					OLED_ShowCHinese(46,3,72); //模
					OLED_ShowCHinese(64,3,73); //板
					OLED_ShowCHinese(82,3,64); //成
					OLED_ShowCHinese(100,3,65);//功
					processnum=4;//跳到第五步
				}else {processnum=0;ShowErrMessage(ensure);}
				delay_ms(1200);
				break;
				
			case 4:	
				OLED_Clear();
				OLED_ShowCHinese(10,0,74);  //请
				OLED_ShowCHinese(28,0,75); //输
				OLED_ShowCHinese(46,0,76); //入
				OLED_ShowCHinese(64,0,77); //储
				OLED_ShowCHinese(82,0,78); //存
				OLED_ShowString(100,0,"I");//I
				OLED_ShowString(108,0,"D");//D
				OLED_ShowString(0,6,"ID<300"); //ID<300
				OLED_ShowString(81,6,"D"); //D
				OLED_ShowCHinese(90,6,79); //确
				OLED_ShowCHinese(108,6,80);//定
				do{
					ID=GET_NUM();
					printf("ID:%d\r\n",ID);}
				while(!(ID<AS608Para.PS_max));//输入ID必须小于模块容量最大的数值
				ensure=PS_StoreChar(CharBuffer2,ID);//储存模板
				if(ensure==0x00) 
				{			
					OLED_Clear();
					OLED_ShowCHinese(10,3,81); //录
					OLED_ShowCHinese(28,3,82); //入
					OLED_ShowCHinese(46,3,83); //指
					OLED_ShowCHinese(64,3,84); //纹
					OLED_ShowCHinese(82,3,85); //成
					OLED_ShowCHinese(100,3,86);//功
					PS_ValidTempleteNum(&ValidN);//读库指纹个数
					OLED_ShowCHinese(15,6,83); //指
					OLED_ShowCHinese(33,6,84); //纹
					OLED_ShowCHinese(51,6,87); //数
					OLED_ShowCHinese(69,6,88); //量
					OLED_ShowNum(87,6,ValidN,5,16);
					delay_ms(1500);
					OLED_Clear();
					return 1;
				}else {processnum=0;ShowErrMessage(ensure);}					
				break;				
		}
		delay_ms(400);
		if(i==5)//超过5次没有按手指则退出
		{
			OLED_Clear();
			break;	
		}				
	}
	return 0;
}




//刷指纹
uint32_t press_FR(void)
{
	SearchResult seach;
	u8 ensure;
	ensure=PS_GetImage();
	if(ensure==0x00)//获取图像成功 
	{	
		//BEEP=1;//打开蜂鸣器	
		ensure=PS_GenChar(CharBuffer1);
		if(ensure==0x00) //生成特征成功
		{		
			//BEEP=0;//关闭蜂鸣器	
			ensure=PS_HighSpeedSearch(CharBuffer1,0,AS608Para.PS_max,&seach);
			if(ensure==0x00)//搜索成功
			{				
				printf("指纹库匹配成功！,ID:%d  匹配得分:%d\r\n",seach.pageID,seach.mathscore);
			}
			else 
			{
				printf("匹配失败！\r\n");
				return 1;
			}
								
		}
		else
			ShowErrMessage(ensure);
	 //BEEP=0;//关闭蜂鸣器
	 delay_ms(600);
	}	
	return 0;
}

//删除指纹
uint32_t Del_FR(void)
{
	u8  ensure;
	u16 num;
	u16 vnum1; //删除前后指纹个数对比
	u16 vnum2; //删除前后指纹个数对比	
	OLED_Clear();
	OLED_ShowCHinese(10,0,89);  //请
	OLED_ShowCHinese(28,0,90); //输
	OLED_ShowCHinese(46,0,91); //入
	OLED_ShowCHinese(64,0,92); //删
	OLED_ShowCHinese(82,0,93); //除
	OLED_ShowString(100,0,"I");//I
	OLED_ShowString(108,0,"D");//D
	OLED_ShowString(0,6,"ID<300"); //ID<300
	OLED_ShowString(81,6,"D"); //D
	OLED_ShowCHinese(90,6,94); //确
	OLED_ShowCHinese(108,6,95);//定
	delay_ms(50);	
	PS_ValidTempleteNum(&ValidN);//读库指纹个数	
	vnum1=ValidN;
	num=GET_NUM();//获取返回的数值
	if(num==0xFF00)
		ensure=PS_Empty();//清空指纹库
	else 
		ensure=PS_DeletChar(num,1);//删除单个指纹
	if(ensure==0)
	{
			PS_ValidTempleteNum(&ValidN);//读库指纹个数	
			vnum2=ValidN;	
			if(vnum1!=vnum2)
			{	
				OLED_Clear();
				OLED_ShowCHinese(10,3,92);  //删
				OLED_ShowCHinese(28,3,93);  //除
				OLED_ShowCHinese(46,3,83);  //指
				OLED_ShowCHinese(64,3,84);  //纹
				OLED_ShowCHinese(82,3,85);  //成
				OLED_ShowCHinese(100,3,86); //功	
				delay_ms(1000);
				OLED_Clear();
				PS_ValidTempleteNum(&ValidN);//读库指纹个数
				OLED_ShowCHinese(20,4,83); //指
				OLED_ShowCHinese(38,4,84); //纹
				OLED_ShowCHinese(56,4,87); //数
				OLED_ShowCHinese(74,4,88); //量
				OLED_ShowNum(90,4,ValidN,3,16);
				delay_ms(2000);	
				OLED_Clear();		
				return 1;
			}
			else
			{
				OLED_Clear();
				OLED_ShowCHinese(10,3,92);  //删
				OLED_ShowCHinese(28,3,93);  //除
				OLED_ShowCHinese(46,3,83);  //指
				OLED_ShowCHinese(64,3,84);  //纹
				OLED_ShowCHinese(82,3,66);  //成
				OLED_ShowCHinese(100,3,67); //功	
				delay_ms(1000);
				OLED_Clear();
				PS_ValidTempleteNum(&ValidN);//读库指纹个数
				OLED_ShowCHinese(20,4,83); //指
				OLED_ShowCHinese(38,4,84); //纹
				OLED_ShowCHinese(56,4,87); //数
				OLED_ShowCHinese(74,4,88); //量
				OLED_ShowNum(90,4,ValidN,3,16);
				delay_ms(2000);	
				OLED_Clear();		
				return 2;				
			}
	}
	return 0;
}



