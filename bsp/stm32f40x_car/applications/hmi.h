#ifndef _H_HMI_
#define _H_HMI

void beep(uint8_t high_ticks,uint8_t low_ticks,uint16_t count);

void hmi_init( void );
void pop_msg(char *msg,uint32_t interval);

#endif

