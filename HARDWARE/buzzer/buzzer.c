#include "buzzer.h" 
#include "sys.h"
GPIO_InitTypeDef  GPIO_InitStructure;
//���÷�����
void buzzer_Init(void)
{
//ʹ�ܶ˿�F��Ӳ��ʱ�ӣ��˿�F���ܹ�����˵���˾��ǶԶ˿�F�ϵ�
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOF, ENABLE);	
	

	//����Ӳ��������GPIO���˿�F����8 ������
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_8;			//�� 8 9������
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_OUT;		//���ģʽ
	GPIO_InitStructure.GPIO_Speed=GPIO_High_Speed;		//���Ÿ��ٹ������յ�ָ������������ȱ�㣺���ĸ�
	GPIO_InitStructure.GPIO_OType=GPIO_OType_PP;		//�����������������
	GPIO_InitStructure.GPIO_PuPd=GPIO_PuPd_NOPULL;		//����Ҫ����������
	GPIO_Init(GPIOF,&GPIO_InitStructure);	
	PFout(8)=0;	

}
