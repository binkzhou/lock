#include "sys.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "flash.h"


//��������
void flash_erase_record(uint16_t section)
{
	//���д����������FLASH
	FLASH_Unlock();
	
	/* �����������д���ʧ�ܣ���ȡ�����ݶ�Ϊ0 */
	printf("FLASH_EraseSector %hd start\r\n",section);
	
		//����(32bit)�Ĵ�С��������3
	if (FLASH_EraseSector(section, VoltageRange_3) != FLASH_COMPLETE)
	{ 
			printf("Erase error\r\n");
			return;
	}

	printf("FLASH_EraseSector %hd ends\r\n",section);
}
//д�����ݵ�����
uint32_t flash_write_record(char *pbuf,uint32_t record_count,uint32_t section,uint16_t section_num)
{
	uint32_t addr_start=section+record_count*64; 
	uint32_t addr_end  =addr_start+64;

	uint32_t i=0;
	
	while (addr_start < addr_end)
	{
		//ÿ��д��������4���ֽ�
		if (FLASH_ProgramWord(addr_start, *(uint32_t *)&pbuf[i]) == FLASH_COMPLETE)
		{
			//��ַÿ��ƫ��4���ֽ�
			addr_start +=4;
			
			i+=4;
		}
		else
		{ 
			printf("flash write record fail,now goto erase sector!\r\n");
			
			//���²�������
			flash_erase_record(section_num);

			return 1;
		}
	}
	
	return 0;
}

//��ȡ����������
void flash_read_record(char *pbuf,uint32_t record_count,uint32_t section)
{
	uint32_t addr_start=section+record_count*64;
	uint32_t addr_end  =addr_start+64;

	uint32_t i=0;
	
	while (addr_start < addr_end)
	{
		*(uint32_t *)&pbuf[i] = *(__IO uint32_t*)addr_start;

		addr_start+=4;
		
		i = i + 4;
	}
}
//�Զ��庯����ɾ���ַ�����ĳ��ָ���ִ���
void delchar(char *p1,char *p2) 
{  
		int i,j,k = 0;
		char t[80] = {0};

		for(i=0;i<(strlen(p1));i++)
		{
			for(j=0;j<(strlen(p2));j++)
			{
				if(p1[i]==p2[j])
				{
				break;
				}
			}
			if(j >= strlen(p2))
			{
				t[k++] = p1[i];
			}
		}

		memset(p1,0,80);
		memcpy(p1,t,k);
	
}
