#ifndef _H_JT808_AUXIO_
#define _H_JT808_AUXIO_





typedef struct _auxio_in
{
	GPIO_TypeDef* port;
	uint16_t	pin;
	uint8_t		value;                              /*��ǰ�˿�ֵ*/
	uint32_t	dithering_threshold;                /*���ޣ�����ȥ������50ms���һ��*/
	uint8_t		dithering_count;                    /*��������*/
	uint32_t	duration;                           /*��ǰ״̬�ĳ���ʱ��*/
	void ( *onchange )( uint8_t );                  /*�˿ڱ仯������*/
}AUX_IN;


extern AUX_IN	PIN_IN[10];

void auxio_input_check(void );
void auxio_init( void );




#endif

