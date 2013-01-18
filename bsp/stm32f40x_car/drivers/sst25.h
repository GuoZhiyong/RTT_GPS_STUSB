
//#include <includes.h>
#ifndef __SST25V_H
#define __SST25V_H
#include <stm32f4xx.h>

void sst25_init(void);
void sst25_read(uint32_t addr ,uint8_t *p,uint16_t len) ;
void sst25_erase(uint32_t addr,uint32_t len);
void sst25_write(uint32_t addr, uint8_t *p,uint16_t len);     

#endif 

