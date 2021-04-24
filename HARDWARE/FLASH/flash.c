#include "sys.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "flash.h"


//擦除扇区
void flash_erase_record(uint16_t section)
{
	//解除写保护，解锁FLASH
	FLASH_Unlock();
	
	/* 如果不擦除，写入会失败，读取的数据都为0 */
	printf("FLASH_EraseSector %hd start\r\n",section);
	
		//以字(32bit)的大小擦除扇区3
	if (FLASH_EraseSector(section, VoltageRange_3) != FLASH_COMPLETE)
	{ 
			printf("Erase error\r\n");
			return;
	}

	printf("FLASH_EraseSector %hd ends\r\n",section);
}
//写入数据到扇区
uint32_t flash_write_record(char *pbuf,uint32_t record_count,uint32_t section,uint16_t section_num)
{
	uint32_t addr_start=section+record_count*64; 
	uint32_t addr_end  =addr_start+64;

	uint32_t i=0;
	
	while (addr_start < addr_end)
	{
		//每次写入数据是4个字节
		if (FLASH_ProgramWord(addr_start, *(uint32_t *)&pbuf[i]) == FLASH_COMPLETE)
		{
			//地址每次偏移4个字节
			addr_start +=4;
			
			i+=4;
		}
		else
		{ 
			printf("flash write record fail,now goto erase sector!\r\n");
			
			//重新擦除扇区
			flash_erase_record(section_num);

			return 1;
		}
	}
	
	return 0;
}

//读取扇区的数据
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
//自定义函数（删除字符串中某个指定字串）
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
