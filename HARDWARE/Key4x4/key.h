#ifndef __KEY_H
#define __KEY_H
#include "stm32f4xx.h" 
void key_init(void);
void get_key(void);
void key_stabilize(GPIO_TypeDef* GPIOx,u16 GPIO_Pin,char *ch);
uint32_t getpassword_key(void);
uint32_t getfingerprint_key(void);
#endif 
