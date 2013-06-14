#ifndef __RTC_H__
#define __RTC_H__

//#include <time.h>
#include "stm32f4xx_rtc.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_pwr.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_usart.h"



#include <time.h>


rt_err_t rtc_init(void);
void time_set(uint8_t hour,uint8_t min,uint8_t sec);
void date_set(uint8_t year,uint8_t month,uint8_t day);


#endif
