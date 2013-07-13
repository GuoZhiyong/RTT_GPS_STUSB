#ifndef _H_JT808_AUXIO_
#define _H_JT808_AUXIO_





typedef struct _auxio_in
{
	GPIO_TypeDef* port;
	uint16_t	pin;
	uint8_t		value;                              /*当前端口值*/
	uint32_t	dithering_threshold;                /*门限，用作去抖动，50ms检查一次*/
	uint8_t		dithering_count;                    /*抖动计数*/
	uint32_t	duration;                           /*当前状态的持续时间*/
	void ( *onchange )( uint8_t );                  /*端口变化处理函数*/
}AUX_IN;


extern AUX_IN	PIN_IN[10];

void auxio_input_check(void );
void auxio_init( void );




#endif

