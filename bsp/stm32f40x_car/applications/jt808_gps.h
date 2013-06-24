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

#define BIT_STATUS_ACC	0x01
#define BIT_STATUS_GPS	0x02
#define BIT_STATUS_NS	0x04
#define BIT_STATUS_EW	0x08

#define BIT_ALARM_EMG 0x01

/*基本位置信息,因为字节对齐的方式，还是使用数组方便*/
typedef __packed struct _gps_baseinfo
{
	uint32_t	alarm;
	uint32_t	status;
	uint32_t	latitude;       /*纬度*/
	uint32_t	longitude;      /*精度*/
	uint16_t	altitude;
	uint16_t	speed_10x;      /*对地速度 0.1KMH*/
	uint16_t	cog;            /*对地角度*/
	uint8_t		datetime[6];    /*BCD格式*/
}GPS_BASEINFO;

enum BDGPS_MODE
{
	MODE_GET=0,                 /*查询*/
	MODE_BD =1,
	MODE_GPS,
	MODE_BDGPS,
};

typedef  struct  _gps_status
{
	enum BDGPS_MODE Position_Moule_Status;  /* 1: BD   2:  GPS   3: BD+GPS    定位模块的状态*/
	uint8_t			Antenna_Flag;           //显示提示开路
	uint8_t			Raw_Output;             //  原始数据输出
	uint8_t			NoSV;
}GPS_STATUS;

extern GPS_STATUS	gps_status;

extern GPS_BASEINFO gps_baseinfo;

extern uint32_t		gps_lati;           /*内部，小端 纬度 10-E6度*/
extern uint32_t		gps_longi;          /*内部，小端 经度 10-E6度*/
extern uint16_t		gps_speed;          /*速度 kmh*/
extern uint16_t		gps_cog;            /*对地方向角*/
extern uint16_t		gps_alti;           /*高度*/
extern uint8_t		gps_datetime[6];    /*日期时间 hex格式*/

/*告警和状态信息*/
extern uint32_t jt808_alarm;
extern uint32_t jt808_status;

extern uint32_t	gps_sec_count;		/*gps秒脉冲输出*/

void gps_rx( uint8_t * pinfo, uint16_t length );


void jt808_gps_init( void );


#endif
/************************************** The End Of File **************************************/
