#ifndef _H_M66_SMS_
#define _H_M66_SMS_

#include "stm32f4xx.h"
#include "jt808.h"

uint8_t sms_rx_proc(char *pinfo,uint16_t size);
void sms_tx(char *info);
void sms_proc(void);

#endif
