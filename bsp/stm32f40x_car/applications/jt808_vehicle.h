#ifndef _H_JT808_VEHICLE_
#define _H_JT808_VEHICLE_


#include "stm32f4xx.h"

/*外部IO口*/
typedef struct _auxio_out
{
	GPIO_TypeDef* port;
	uint16_t	pin;
	uint8_t		value;                      /*当前端口值*/
	void ( *onchange )( uint8_t value );    /*端口变化处理函数*/
}AUX_OUT;


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



#define SPEED_LIMIT				5   /*速度门限 大于此值认为启动，小于此值认为停止*/
#define SPEED_LIMIT_DURATION	10  /*速度门限持续时间*/

#define SPEED_STATUS_ACC	0x01    /*acc状态 0:关 1:开*/
#define SPEED_STATUS_BRAKE	0x02    /*刹车状态 0:关 1:开*/

#define SPEED_JUDGE_ACC		0x04    /*是否判断ACC*/
#define SPEED_JUDGE_BRAKE	0x08    /*是否判断BRAKE 刹车信号*/

#define SPEED_USE_PULSE 0x10        /*使用脉冲信号 0:不使用 1:使用*/
#define SPEED_USE_GPS	0x20        /*使用gps信号 0:不使用 1:使用*/

enum _stop_run
{
	STOP=0,
	RUN =1,
};

typedef struct _vehicle_status
{
	enum _stop_run	stop_run;       /*当前车辆状态 0:停止 1:启动*/

	MYTIME	mytime_start;			/*当前状态的开始时刻*/
	uint32_t	utc_start;			/*utc的开始时刻*/
	uint32_t	lati;				/*当前状态开始时刻的位置信息，超时超速驾驶用*/
	uint32_t	longi;
	uint32_t	alti;
	uint32_t	speed;
	
	uint8_t			logic;          /*当前逻辑状态*/
	uint8_t			pulse_speed;    /*速度值*/
	uint32_t		pulse_duration; /*持续时间-秒*/
	uint8_t			gps_speed;      /*速度值，当前速度值*/
	uint32_t		gps_duration;   /*持续时间-秒*/
	
}VEHICLE;


extern VEHICLE car_status;
extern AUX_IN	PIN_IN[10];




void jt808_vehicle_init( void );




#endif

