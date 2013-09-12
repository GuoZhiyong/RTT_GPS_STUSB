/************************************************************
 * Copyright (C), 2008-2012,
 * FileName:		// 文件名
 * Author:			// 作者
 * Date:			// 日期
 * Description:		// 模块描述
 * Version:			// 版本信息
 * Function List:	// 主要函数及其功能
 *     1. -------
 * History:			// 历史修改记录
 *     <author>  <time>   <version >   <desc>
 *     David    96/10/12     1.0     build this moudle
 ***********************************************************/
#ifndef _JT808_GPS_H_
#define _JT808_GPS_H_

#include <rtthread.h>
#include "stm32f4xx.h"
#include "jt808.h"

#define BIT_STATUS_ACC		0x00000001
#define BIT_STATUS_FIXED	0x00000002
#define BIT_STATUS_NS		0x00000004
#define BIT_STATUS_EW		0x00000008  /*东西经*/
#define BIT_STATUS_SERVICE	0x00000010  /*营运状态 1停运*/
#define BIT_STATUS_ENCRYPT	0x00000020

#define BIT_STATUS_EMPTY	0x00000100
#define BIT_STATUS_FULL		0x00000200

#define BIT_STATUS_OIL		0x00000400  /*油路*/
#define BIT_STATUS_ELEC		0x00000800  /*电路*/
#define BIT_STATUS_DOORLOCK 0x00001000  /*车门锁*/
#define BIT_STATUS_DOOR1	0x00002000
#define BIT_STATUS_DOOR2	0x00004000
#define BIT_STATUS_DOOR3	0x00008000
#define BIT_STATUS_DOOR4	0x00010000
#define BIT_STATUS_DOOR5	0x00020000
#define BIT_STATUS_GPS		0x00040000
#define BIT_STATUS_BD		0x00080000
#define BIT_STATUS_GLONASS	0x00100000
#define BIT_STATUS_GALILEO	0x00200000

#define BIT_ALARM_EMG				0x00000001  /*紧急*/
#define BIT_ALARM_OVERSPEED			0x00000002  /*超速*/
#define BIT_ALARM_OVERTIME			0x00000004  /*超时，疲劳驾驶*/
#define BIT_ALARM_DANGER			0x00000008  /*危险预警*/
#define BIT_ALARM_GPS_ERR			0x00000010  /*GNSS模块故障*/
#define BIT_ALARM_GPS_OPEN			0x00000020  /*天线开路*/
#define BIT_ALARM_GPS_SHORT			0x00000040  /*天线短路*/
#define BIT_ALARM_LOW_PWR			0x00000080  /*终端主电源欠压*/
#define BIT_ALARM_LOST_PWR			0x00000100  /*终端主电源掉电*/
#define BIT_ALARM_FAULT_LCD			0x00000200  /*LCD故障*/
#define BIT_ALARM_FAULT_TTS			0x00000400  /*TTS故障*/
#define BIT_ALARM_FAULT_CAM			0x00000800  /*CAM故障*/
#define BIT_ALARM_FAULT_ICCARD		0x00001000  /*IC卡故障*/
#define BIT_ALARM_PRE_OVERSPEED		0x00002000  /*超速预警 bit13*/
#define BIT_ALARM_PRE_OVERTIME		0x00004000  /*超时预警 bit14*/
#define BIT_ALARM_TODAY_OVERTIME	0x00040000  /*当天累计驾驶超时*/
#define BIT_ALARM_STOP_OVERTIME		0x00080000  /*超时停驶*/
#define BIT_ALARM_VSS				0x01000000  /*VSS故障*/
#define BIT_ALARM_OIL				0x02000000  /*油量异常*/
#define BIT_ALARM_STOLEN			0x04000000  /*被盗*/
#define BIT_ALARM_IGNITION			0x08000000  /*非法点火*/
#define BIT_ALARM_MOVE				0x10000000  /*非法移位*/
#define BIT_ALARM_COLLIDE			0x20000000  /*碰撞*/
#define BIT_ALARM_TILT				0x40000000  /*侧翻*/
#define BIT_ALARM_DOOR_OPEN			0x80000000  /*非法开门*/

/*基本位置信息,因为字节对齐的方式，还是使用数组方便*/
typedef __packed struct _gps_baseinfo
{
	uint32_t	alarm;
	uint32_t	status;
	uint32_t	latitude;                       /*纬度*/
	uint32_t	longitude;                      /*精度*/
	uint16_t	altitude;
	uint16_t	speed_10x;                      /*对地速度 0.1KMH*/
	uint16_t	cog;                            /*对地角度*/
	uint8_t		datetime[6];                    /*BCD格式*/
}GPS_BASEINFO;

enum BDGPS_MODE
{
	MODE_GET=0,                                 /*查询*/
	MODE_BD =1,
	MODE_GPS,
	MODE_BDGPS,
};

typedef  struct  _gps_status
{
	uint32_t		type;           /*型号 0:未定 0x3020x  0x3017x*/
	enum BDGPS_MODE mode;           /* 1: BD   2:  GPS   3: BD+GPS    定位模块的状态*/
	uint8_t			Antenna_Flag;   //显示提示开路
	uint8_t			Raw_Output;     //  原始数据输出
	uint8_t			NoSV;
}GPS_STATUS;

extern GPS_STATUS	gps_status;

extern GPS_BASEINFO gps_baseinfo;

extern uint32_t		gps_lati;               /*内部，小端 纬度 10-E6度*/
extern uint32_t		gps_longi;              /*内部，小端 经度 10-E6度*/
extern uint16_t		gps_speed;              /*速度 kmh*/
extern uint16_t		gps_cog;                /*对地方向角*/
extern uint16_t		gps_alti;               /*高度*/
extern uint8_t		gps_datetime[6];        /*日期时间 hex格式*/

/*告警和状态信息*/
extern uint32_t jt808_alarm;
extern uint32_t jt808_status;

extern uint32_t gps_sec_count;              /*gps秒脉冲输出*/
extern uint32_t utc_now;
extern MYTIME	mytime_now;

extern uint16_t jt808_8202_track_interval;  /*jt808_8202 临时位置跟踪控制*/
extern uint32_t jt808_8202_track_duration;
extern uint16_t jt808_8202_track_counter;

extern uint32_t jt808_8203_manual_ack_seq;  /*人工确认报警的标识位 0,3,20,21,22,27,28*/
extern uint16_t jt808_8203_manual_ack_value;

extern uint32_t gps_notfixed_count;

void gps_rx( uint8_t * pinfo, uint16_t length );


void jt808_gps_init( void );
void jt808_report_get( void );
void jt808_report_init( void );



#endif
/************************************** The End Of File **************************************/
