#ifndef _H_JT808_VEHICLE_
#define _H_JT808_VEHICLE_


#include "stm32f4xx.h"

/*�ⲿIO��*/
typedef struct _auxio_out
{
	GPIO_TypeDef* port;
	uint16_t	pin;
	uint8_t		value;                      /*��ǰ�˿�ֵ*/
	void ( *onchange )( uint8_t value );    /*�˿ڱ仯������*/
}AUX_OUT;


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



#define SPEED_LIMIT				5   /*�ٶ����� ���ڴ�ֵ��Ϊ������С�ڴ�ֵ��Ϊֹͣ*/
#define SPEED_LIMIT_DURATION	10  /*�ٶ����޳���ʱ��*/

#define SPEED_STATUS_ACC	0x01    /*acc״̬ 0:�� 1:��*/
#define SPEED_STATUS_BRAKE	0x02    /*ɲ��״̬ 0:�� 1:��*/

#define SPEED_JUDGE_ACC		0x04    /*�Ƿ��ж�ACC*/
#define SPEED_JUDGE_BRAKE	0x08    /*�Ƿ��ж�BRAKE ɲ���ź�*/

#define SPEED_USE_PULSE 0x10        /*ʹ�������ź� 0:��ʹ�� 1:ʹ��*/
#define SPEED_USE_GPS	0x20        /*ʹ��gps�ź� 0:��ʹ�� 1:ʹ��*/

enum _stop_run
{
	STOP=0,
	RUN =1,
};

typedef struct _vehicle_status
{
	enum _stop_run	stop_run;       /*��ǰ����״̬ 0:ֹͣ 1:����*/

	MYTIME	mytime_start;			/*��ǰ״̬�Ŀ�ʼʱ��*/
	uint32_t	utc_start;			/*utc�Ŀ�ʼʱ��*/
	uint32_t	lati;				/*��ǰ״̬��ʼʱ�̵�λ����Ϣ����ʱ���ټ�ʻ��*/
	uint32_t	longi;
	uint32_t	alti;
	uint32_t	speed;
	
	uint8_t			logic;          /*��ǰ�߼�״̬*/
	uint8_t			pulse_speed;    /*�ٶ�ֵ*/
	uint32_t		pulse_duration; /*����ʱ��-��*/
	uint8_t			gps_speed;      /*�ٶ�ֵ����ǰ�ٶ�ֵ*/
	uint32_t		gps_duration;   /*����ʱ��-��*/
	
}VEHICLE;


extern VEHICLE car_status;
extern AUX_IN	PIN_IN[10];




void jt808_vehicle_init( void );




#endif

