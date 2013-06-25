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
#include <stdio.h>

#include <board.h>
#include <rtthread.h>
#include <finsh.h>

#include "stm32f4xx.h"
#include "jt808.h"
#include "jt808_gps.h"
#include "jt808_param.h"
#include "rtc.h"

#include "jt808_auxio.h"

#include "vdr.h"
#include "math.h"

#define BIT( n ) ( 1 << n )

#define BYTESWAP2( val )    \
    ( ( ( val & 0xff ) << 8 ) |   \
      ( ( val & 0xff00 ) >> 8 ) )

#define BYTESWAP4( val )    \
    ( ( ( val & 0xff ) << 24 ) |   \
      ( ( val & 0xff00 ) << 8 ) |  \
      ( ( val & 0xff0000 ) >> 8 ) |  \
      ( ( val & 0xff000000 ) >> 24 ) )

#define HEX2BCD( x )	( ( x / 10 ) << 4 | ( x % 10 ) )
#define BCD2HEX( x )	( ( ( x >> 4 ) * 10 ) + ( x & 0x0f ) )

typedef struct _GPSPoint
{
	int sign;
	int deg;
	int min;
	int sec;
} GPSPoint;

uint32_t gps_sec_count = 0;                     /*gps秒脉冲输出*/

/*要用union类型保存，位域，访问吗?*/
uint32_t		jt808_alarm			= 0x0;
uint32_t		jt808_alarm_last	= 0x0;      /*上一次的上报状态*/

uint32_t		jt808_status		= 0x0;
uint32_t		jt808_status_last	= 0x0;      /*上一次的状态信息*/

static uint32_t jt808_report_interval	= 0x0;  /*GPS上报时间间隔，为0:停止上报*/
static uint32_t jt808_report_distance	= 0x0;  /*GPS上报距离间隔,为0 停止上报*/

static uint32_t distance		= 0;            /*定距上报当前距离值*/
static uint32_t total_distance	= 0;            /*总的累计里程*/

/*超速判断*/
static uint8_t	overspeed_flag = 0;             /*是否已超速 0:未超速 1:超速预警 2:已超速*/
static uint32_t overspeed_timestamp_start;      /*开始时间戳*/

/*疲劳驾驶计时*/
static uint32_t period_acc_on	= 0;            /*疲劳驾驶时间 acc开*/
static uint32_t period_acc_off	= 0;            /*疲劳驾驶时间acc关*/

uint32_t		gps_second_count = 0;           /*gps秒语句输出*/


/*
   区域的定义,使用list关联起来，如果node过多的话，
   RAM是否够用
   使用dataflash存储，以4k作为cache,缓存访问
   每秒的位置信息都要判断
 */
struct
{
	uint32_t	id;                             /*区域ID*/
	uint16_t	attr;                           /*属性*/
	uint32_t	latitude;                       /*中心纬度*/
	uint32_t	logitude;                       /*中心经度*/
	uint32_t	radius;                         /*半径*/
	uint8_t		datetime_start[6];              /*开始时刻，使用utc是不是更好?*/
	uint8_t		datetime_end[6];
	uint16_t	speed;
	uint8_t		duration;                       /*持续时间*/
} circle;

struct
{
	uint32_t	id;                             /*区域ID*/
	uint16_t	attr;                           /*属性*/
	uint32_t	latitude;                       /*中心纬度*/
	uint32_t	logitude;                       /*中心经度*/
	uint32_t	radius;                         /*半径*/
	uint8_t		datetime_start[6];              /*开始时刻，使用utc是不是更好?*/
	uint8_t		datetime_end[6];
	uint16_t	speed;
	uint8_t		duration;                       /*持续时间*/
}			rectangle;

uint32_t	gps_lati;
uint32_t	gps_longi;
uint16_t	gps_speed;
uint16_t	gps_cog;                            /*course over ground*/
uint16_t	gps_alti;
uint8_t		gps_datetime[6];

/*保存gps基本位置信息*/
GPS_BASEINFO	gps_baseinfo;
/*gps的状态*/
GPS_STATUS		gps_status;


/*
   Epoch指的是一个特定的时间：1970-01-01 00:00:00 UTC
   UNIX时间戳：Unix时间戳（英文为Unix time, POSIX time 或 Unix timestamp）
   是从Epoch（1970年1月1日00:00:00 UTC）开始所经过的秒数，不考虑闰秒。

 */
unsigned long	timestamp_last	= 0;
unsigned long	timestamp_now	= 0;

uint8_t			ACC_status; /*0:ACC关   1:ACC开  */
uint32_t		ACC_ticks;  /*ACC状态发生变化时的tick值，此时GPS可能未定位*/

/*声明一个定时器，用来定时检查AUX*/
struct rt_timer tmr_gps;

struct
{
	uint8_t		mode;       /*上报模式 0:定时 1:定距 2:定时定距*/
	uint8_t		userlogin;  /*是否使用登录*/
	uint32_t	time_unlog;
	uint32_t	time_sleep;
	uint32_t	time_emg;
	uint32_t	time_default;
	uint32_t	distance_unlog;
	uint32_t	distance_sleep;
	uint32_t	distance_emg;
	uint32_t	distance_default;

	uint32_t	last_tick;      /*上一次上报的时刻*/
	uint32_t	last_distance;  /*上一次上报时的里程*/
} jt808_report;

struct
{
	uint32_t perion_acc_on;
} func_tired_drive;

#define DEBUG_GPS

#ifdef DEBUG_GPS
uint8_t		speed_add	= 0;
uint32_t	speed_count = 0;
#endif


/*
   定时器超时函数,当前设置为50ms
 */
static void cb_tmr_gps( void* parameter )
{
	static uint8_t count_to_1_sec = 0;
	auxio_input_check( );
	/*检查疲劳驾驶，GPS未定位时也要检查,使用定时器，长时间定时准不准?*/
	count_to_1_sec++;
	if( count_to_1_sec == 20 )
	{
		count_to_1_sec = 0;
		if( jt808_status & BIT_STATUS_ACC ) /*ACC开*/
		{
			period_acc_off = 0;
			period_acc_on++;
			if( ( period_acc_on % 100 ) == 0 )
			{
				rt_kprintf( "time adjust>%d %d\r\n", rt_tick_get( ), timestamp( ) );
			}
			if( period_acc_on >= jt808_param.id_0x0057 )
			{
			}
		}else
		{
			period_acc_on = 0;
			period_acc_off++;
			if( ( period_acc_off % 100 ) == 0 )
			{
				rt_kprintf( "time adjust>%d %d\r\n", rt_tick_get( ), timestamp( ) );
			}
		}
	}
}

/*
   Linux源码中的mktime算法解析
 */
static __inline unsigned long linux_mktime( unsigned int year, unsigned int mon,
                                            unsigned int day, unsigned int hour,
                                            unsigned int min, unsigned int sec )
{
	if( 0 >= (int)( mon -= 2 ) )    /**//* 1..12 -> 11,12,1..10 */
	{
		mon		+= 12;              /**//* Puts Feb last since it has leap day */
		year	-= 1;
	}

	return ( ( (
	               (unsigned long)( year / 4 - year / 100 + year / 400 + 367 * mon / 12 + day ) +
	               year * 365 - 719499
	               ) * 24 + hour    /**//* now have hours */
	           ) * 60 + min         /**//* now have minutes */
	         ) * 60 + sec;          /**//* finally seconds */
}

/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static double gpsToRad( GPSPoint point )
{
	return point.sign * ( point.deg + ( point.min + point.sec / 60.0 ) / 60.0 ) * 3.141592654 / 180.0;
}

/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static double getDistance( GPSPoint latFrom, GPSPoint lngFrom, GPSPoint latTo, GPSPoint lngTo )
{
	double	latFromRad	= gpsToRad( latFrom );
	double	lngFromRad	= gpsToRad( lngFrom );
	double	latToRad	= gpsToRad( latTo );
	double	lngToRad	= gpsToRad( lngTo );
	double	lngDiff		= lngToRad - lngFromRad;
	double	part1		= pow( cos( latToRad ) * sin( lngDiff ), 2 );
	//double part2 = pow( cos(latFromRad)*sin(latToRad)*cos(lngDiff) , 2);
	double	part2 = pow( cos( latFromRad ) * sin( latToRad ) - sin( latFromRad ) * cos( latToRad ) * cos( lngDiff ), 2 );

	double	part3 = sin( latFromRad ) * sin( latToRad ) + cos( latFromRad ) * cos( latToRad ) * cos( lngDiff );
	//double centralAngle = atan2( sqrt(part1 + part2) / part3 );
	double	centralAngle = atan( sqrt( part1 + part2 ) / part3 );
	return 6371.01 * 1000.0 * centralAngle;                                             //Return Distance in meter
}

/*超速、超速预警判断*/
void do_overspeed_check( void )
{
	if( gps_speed >= jt808_param.id_0x0055 )                                            /*超过最高速度*/
	{
		if( overspeed_flag == 2 )                                                       /*已超速*/
		{
			if( timestamp_now - overspeed_timestamp_start >= jt808_param.id_0x0056 )    /*已经持续超速*/
			{
				if( ( jt808_param.id_0x0050 & 0x02 ) == 0 )                             /*报警屏蔽字*/
				{
					jt808_alarm |= 0x02;
				}
			}
		}else /*没有超速或超速预警，记录开始超速，*/
		{
			overspeed_flag				= 2;
			overspeed_timestamp_start	= timestamp_now;
		}
	}else if( gps_speed >= ( jt808_param.id_0x0055 - jt808_param.id_0x005B ) )  /*超速预警*/
	{
		if( ( jt808_param.id_0x0050 & ( 1 << 13 ) ) == 0 )                      /*报警屏蔽字*/
		{
			jt808_alarm |= ( 1 << 13 );
		}
	}else /*没有超速，也没有预警,清除标志位*/
	{
		overspeed_flag	= 0;
		jt808_alarm		&= ~( ( 1 << 1 ) | ( 1 << 13 ) );
	}
}

/*
   处理gps信息,有多种条件组合
 */

void process_gps( void )
{
	uint32_t	i, tmp;
	uint8_t		flag_send = 0;                                                          /*默认不上报*/
	rt_err_t	err;
	uint8_t		*pdata;

/*RTC,避免和rtt的set_time,set_date歧义，rtt是一个Driver,此处没有使用*/
	if( jt808_status & BIT_STATUS_GPS )
	{
		if( ( gps_baseinfo.datetime[4] == 0 ) && ( gps_baseinfo.datetime[5] == 0 ) )    /*整小时校准*/
		{
			date_set( gps_baseinfo.datetime[0], gps_baseinfo.datetime[1], gps_baseinfo.datetime[2] );
			time_set( gps_baseinfo.datetime[3], gps_baseinfo.datetime[4], gps_baseinfo.datetime[5] );
		}
	}
/*数据上报方式,如何组合出各种情况 */
	tmp = jt808_status ^ jt808_status_last;
	if( tmp )                                           /*状态发生变化，要上报,*/
	{
		flag_send = 1;
	}
	/*不理解这个登录状态*/
	if( tmp & BIT_STATUS_ACC )                          /*ACC变化,修改汇报的间隔或距离*/
	{
		jt808_report_distance	= 0;
		jt808_report_interval	= 0;
		if( ( jt808_param.id_0x0020 & 0x01 ) == 0x0 )   /*有定时上报*/
		{
			if( jt808_status & BIT_STATUS_ACC )         /*当前状态为ACC开*/
			{
				jt808_report_interval = jt808_param.id_0x0029;
			}else
			{
				jt808_report_interval = jt808_param.id_0x0027;
			}
			timestamp_last = timestamp_now;             /*重新计时*/
		}
		if( jt808_param.id_0x0020 )                     /*有定距上报*/
		{
			if( jt808_status & BIT_STATUS_ACC )         /*当前状态为ACC开*/
			{
				jt808_report_distance = jt808_param.id_0x002C;
			}else
			{
				jt808_report_distance = jt808_param.id_0x002E;
			}
			distance = 0;                               /*重新计算距离*/
		}
	}

	tmp = ( jt808_alarm ^ jt808_alarm_last );           /*告警位变化*/
	if( tmp )                                           /*告警发生变化，要上报,*/
	{
		flag_send = 1;
	}

	if( tmp & 0x01 )                                    /*紧急告警*/
	{
		if( ( jt808_param.id_0x0020 & 0x01 ) == 0x0 )   /*有定时上报*/
		{
			jt808_report_interval	= jt808_param.id_0x0028;
			timestamp_last			= timestamp_now;
		}
		if( jt808_param.id_0x0020 )                     /*有定距上报*/
		{
			jt808_report_distance	= jt808_param.id_0x002F;
			distance				= 0;
		}
	}

/*计算定时上报*/
	if( ( jt808_param.id_0x0020 & 0x01 ) == 0x0 ) /*有定时上报*/
	{
		if( timestamp_now - timestamp_last >= jt808_report_interval )
		{
			flag_send		= 1;
			timestamp_last	= timestamp_now;
		}
	}
/*计算定距上报*/
	if( jt808_param.id_0x0020 ) /*有定距上报*/
	{
		jt808_report_distance	= jt808_param.id_0x002F;
		distance				= 0;
	}

	jt808_status_last = jt808_status;

	//if(flag_send==0) return;

/*生成要上报的数据*/
#if 0

	if( ( gps_datetime[5] % 60 ) == 0 )
	{
		err = jt808_add_tx_data( 1, TERMINAL_CMD, 0x0200, 28, -1, RT_NULL, RT_NULL, (uint8_t*)&gps_baseinfo );
		rt_kprintf( "%d>add gps report=%d\r\n", rt_tick_get( ), err );
	}
#endif
}

/*
   $GNRMC,074001.00,A,3905.291037,N,11733.138255,E,0.1,,171212,,,A*655220.9*3F0E
   $GNTXT,01,01,01,ANTENNA OK*2B7,N,11733.138255,E,0.1,,171212,,,A*655220.9*3F0E
   $GNGGA,074002.00,3905.291085,N,11733.138264,E,1,11,0.9,8.2,M,-1.6,M,,,1.4*68E
   $GNGLL,3905.291085,N,11733.138264,E,074002.00,A,0*02.9,8.2,M,-1.6,M,,,1.4*68E
   $GPGSA,A,3,18,05,08,02,26,29,15,,,,,,,,,,,,,,,,,,,,,,,,,,1.6,0.9,1.4,0.9*3F8E
   $BDGSA,A,3,04,03,01,07,,,,,,,,,,,,,,,,,,,,,,,,,,,,,1.6,0.9,1.4,0.9*220.9*3F8E
   $GPGSV,2,1,7,18,10,278,29,05,51,063,08,21,052,24,02,24,140,45*4C220.9*3F8E
   $GPGSV,2,2,7,26,72,055,24,29,35,244,37,15,66,224,37*76,24,140,45*4C220.9*3F8E
   $BDGSV,1,1,4,04,27,124,38,03,42,190,34,01,38,146,37,07,34,173,35*55220.9*3F8E

   返回处理的字段数，如果正确的话
 */

uint8_t process_rmc( uint8_t * pinfo )
{
	//检查数据完整性,执行数据转换
	uint8_t		year = 0, mon = 0, day = 0, hour = 0, min = 0, sec = 0, fDateModify = 0;
	uint32_t	degrees, minutes;
	uint8_t		commacount = 0, count = 0;

	uint32_t	lati, longi;
	uint16_t	speed_10x;
	uint16_t	cog;                /*course over ground*/

	uint8_t		wait_dot_find;      /*确定信息中 dot .的位置*/
	uint8_t		i;
	uint8_t		buf[20];
	uint8_t		*psrc = pinfo + 6;  /*指向开始位置 $GNRMC,074001.00,A,3905.291037,N,11733.138255,E,0.1,,171212,,,A*655220.9*3F0E*/

	uint8_t		tbl[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

/*因为自增了一次，所以从pinfo+6开始*/
	while( *psrc++ )
	{
		if( *psrc != ',' )
		{
			buf[count++]	= *psrc;
			buf[count]		= 0;
			continue;
		}
		commacount++;
		switch( commacount )
		{
			case 1: /*时间*/
				if( count < 6 )
				{
					return 1;
				}

				i = ( buf[0] - 0x30 ) * 10 + ( buf[1] - 0x30 ) + 8;
				if( i > 23 )
				{
					fDateModify = 1;
					i			-= 24;
				}
				/*转成HEX格式*/
				hour	= i;
				min		= ( buf[2] - 0x30 ) * 10 + ( buf[3] - 0x30 );
				sec		= ( buf[4] - 0x30 ) * 10 + ( buf[5] - 0x30 );
				//rt_kprintf( "hour=%02x,min=%02x,sec=%02x\r\n", hour, min, sec );
				break;
			case 2: /*A_V*/
				if( buf[0] == 'A' )
				{
					jt808_status |= BIT_STATUS_GPS;
				} else if( buf[0] == 'V' )
				{
					jt808_status &= ~BIT_STATUS_GPS;
				} else
				{
					return 2;
				}
				break;
			case 3: /*纬度处理ddmm.mmmmmm*/
				if( count < 10 )
				{
					return 3;
				}

				degrees = ( ( buf [0] - 0x30 ) * 10 + ( buf [1] - 0x30 ) ) * 1000000;
				minutes = ( buf [2] - 0x30 ) * 1000000 +
				          ( buf [3] - 0x30 ) * 100000 +
				          ( buf [5] - 0x30 ) * 10000 +
				          ( buf [6] - 0x30 ) * 1000 +
				          ( buf [7] - 0x30 ) * 100 +
				          ( buf [8] - 0x30 ) * 10 +
				          ( buf [9] - 0x30 );   /*多加了一个位，想要保证精度*/
				lati = degrees + minutes / 6;
				break;
			case 4:                             /*N_S处理*/
				if( buf[0] == 'N' )
				{
					jt808_status &= ~BIT_STATUS_NS;
				} else if( buf[0] == 'S' )
				{
					jt808_status |= BIT_STATUS_NS;
				}else
				{
					return 4;
				}
				break;
			case 5: /*经度处理*/
				if( count < 11 )
				{
					return 5;
				}
				degrees = ( ( buf [0] - 0x30 ) * 100 + ( buf [1] - 0x30 ) * 10 + ( buf [2] - 0x30 ) ) * 1000000;
				minutes = ( buf [3] - 0x30 ) * 1000000 +
				          ( buf [4] - 0x30 ) * 100000 +
				          ( buf [6] - 0x30 ) * 10000 +
				          ( buf [7] - 0x30 ) * 1000 +
				          ( buf [8] - 0x30 ) * 100 +
				          ( buf [9] - 0x30 ) * 10 +
				          ( buf [10] - 0x30 );
				longi = degrees + minutes / 6;
				break;
			case 6: /*E_W处理*/
				if( buf[0] == 'E' )
				{
					jt808_status &= ~BIT_STATUS_EW;
				} else if( buf[0] == 'W' )
				{
					jt808_status |= BIT_STATUS_EW;
				}else
				{
					return 6;
				}
				break;
			case 7: /*速度处理 */
				speed_10x = 0;
				for( i = 0; i < count; i++ )
				{
					if( buf[i] == '.' )
					{
						speed_10x += ( buf[i + 1] - 0x30 );
						break;
					}else
					{
						speed_10x	+= ( buf[i] - 0x30 );
						speed_10x	= speed_10x * 10;
					}
				}
				/*当前是0.1knot => 0.1Kmh  1海里=1.852Km  1852=1024+512+256+32+16+8+4*/
#ifdef DEBUG_GPS
				if( speed_count )
				{
					speed_10x += ( speed_add * 10 );
					speed_count--;
				}

#endif
				speed_10x	*= 1.852;
				gps_speed	= speed_10x / 10;
				//i=speed_10x;
				//speed_10x=(i<<10)|(i<<9)|(i<<8)|(i<<5)|(i<<4)|(i<<3)|(i<<2);
				//speed_10x/=1000;
				break;

			case 8: /*方向处理*/
				cog = 0;
				for( i = 0; i < count; i++ )
				{
					if( buf[i] == '.' )
					{
						break;
					}else
					{
						cog = cog * 10;
						cog += ( buf[i] - 0x30 );
					}
				}
				break;

			case 9: /*日期处理*/
				if( count < 6 )
				{
					return 9;
				}
				day		= ( ( buf [0] - 0x30 ) * 10 ) + ( buf [1] - 0x30 );
				mon		= ( ( buf [2] - 0x30 ) * 10 ) + ( buf [3] - 0x30 );
				year	= ( ( buf [4] - 0x30 ) * 10 ) + ( buf [5] - 0x30 );

				if( fDateModify )
				{
					day++;
					if( mon == 2 )
					{
						if( ( year % 4 ) == 0 ) /*没有考虑整百时，要被400整除，NM都2100年*/
						{
							if( day == 30 )
							{
								day = 1; mon++;
							}
						} else
						if( day == 29 )
						{
							day = 1; mon++;
						}
					} else
					if( ( mon == 4 ) || ( mon == 6 ) || ( mon == 9 ) || ( mon == 11 ) )
					{
						if( day == 31 )
						{
							mon++; day = 1;
						}
					} else
					{
						if( day == 32 )
						{
							mon++; day = 1;
						}
						if( mon == 13 )
						{
							mon = 1; year++;
						}
					}
				}

				/*都处理完了更新 gps_baseinfo,没有高程信息*/
				gps_lati		= lati;
				gps_longi		= longi;
				gps_speed		= speed_10x / 10;
				gps_cog			= cog;
				gps_datetime[0] = year;
				gps_datetime[1] = mon;
				gps_datetime[2] = day;
				gps_datetime[3] = hour;
				gps_datetime[4] = min;
				gps_datetime[5] = sec;

				gps_baseinfo.alarm		= BYTESWAP4( jt808_alarm );
				gps_baseinfo.status		= BYTESWAP4( jt808_status );
				gps_baseinfo.latitude	= BYTESWAP4( lati );
				gps_baseinfo.longitude	= BYTESWAP4( longi );
				gps_baseinfo.speed_10x	= BYTESWAP2( speed_10x );
				gps_baseinfo.cog		= BYTESWAP2( cog );

				timestamp_now				= linux_mktime( year, mon, day, hour, min, sec );
				gps_baseinfo.datetime[0]	= HEX2BCD( year );
				gps_baseinfo.datetime[1]	= HEX2BCD( mon );
				gps_baseinfo.datetime[2]	= HEX2BCD( day );
				gps_baseinfo.datetime[3]	= HEX2BCD( hour );
				gps_baseinfo.datetime[4]	= HEX2BCD( min );
				gps_baseinfo.datetime[5]	= HEX2BCD( sec );

				/*首次定位,校时*/
				if( ( jt808_status_last & BIT_STATUS_GPS ) == 0 )
				{
					date_set( year, mon, day );
					time_set( hour, min, sec );
				}

				return 0;
				break;
		}
		count	= 0;
		buf[0]	= 0;
	}
	return 10;
}

/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
uint8_t process_gga( uint8_t * pinfo )
{
	//检查数据完整性,执行数据转换
	uint32_t	degrees, minutes;
	uint32_t	lati, longi;
	uint8_t		NoSV;
	uint8_t		i;
	uint8_t		buf[20];
	uint8_t		commacount	= 0, count = 0;
	uint8_t		*psrc		= pinfo + 7; //指向开始位置
	uint16_t	altitute;

	while( *psrc++ )
	{
		if( *psrc != ',' )
		{
			buf[count++]	= *psrc;
			buf[count]		= 0;
			continue;
		}
		commacount++;
		switch( commacount )
		{
			case 1: /*时间处理 */
				if( count < 6 )
				{
					return 1;
				}
				break;

			case 2: /*纬度处理ddmm.mmmmmm*/
				if( count < 10 )
				{
					return 2;
				}
				break;

			case 3: /*N_S处理*/
				if( buf[0] == 'N' )
				{
					jt808_status &= ~0x02;
				} else if( buf[0] == 'S' )
				{
					jt808_status |= 0x02;
				}else
				{
					return 3;
				}
				break;

			case 4: /*经度处理*/
				if( count < 11 )
				{
					return 4;
				}
				break;
			case 5: /*E_W处理*/
				if( buf[0] == 'E' )
				{
					jt808_status &= ~0x04;
				} else if( buf[0] == 'W' )
				{
					jt808_status |= 0x04;
				}else
				{
					return 5;
				}
				break;
			case 6: /*定位类型*/
				break;
			case 7: /*NoSV,卫星数*/
				NoSV = 0;
				for( i = 0; i < count; i++ )
				{
					NoSV	= NoSV * 10;
					NoSV	+= ( buf[i] - 0x30 );
				}
				gps_status.NoSV = NoSV;
				break;
			case 8: /*HDOP*/
				return 0;
				break;
			case 9: /*MSL Altitute*/
				altitute = 0;
				for( i = 0; i < count; i++ )
				{
					if( buf[i] == '.' )
					{
						break;
					}
					altitute	= altitute * 10;
					altitute	+= ( buf[i] - '0' );
				}
				gps_baseinfo.altitude	= altitute;
				gps_alti				= altitute;
				return 0;
				break;
		}
		count	= 0;
		buf[0]	= 0;
	}
	return 9;
}

/***********************************************************
* Function:
* Description:gps收到信息后的处理，头两个字节为长度
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
void gps_rx( uint8_t * pinfo, uint16_t length )

{
	char * psrc;
	psrc				= pinfo;
	*( psrc + length )	= 0;
	/*是否输出原始信息*/
	if( gps_status.Raw_Output )
	{
		rt_kprintf( "%d gps<%s\r\n", rt_tick_get( ), psrc );
	}

	if( ( strncmp( psrc, "$GNGGA,", 7 ) == 0 ) || ( strncmp( psrc, "$BDGGA,", 7 ) == 0 ) || ( strncmp( psrc, "$GPGGA,", 7 ) == 0 ) )
	{
		process_gga( psrc );
	}

	//if( ( strncmp( psrc, "$GNRMC,", 7 ) == 0 ) || ( strncmp( psrc, "$BDRMC,", 7 ) == 0 ) || ( strncmp( psrc, "$GPRMC,", 7 ) == 0 ) )
	if( strncmp( psrc + 3, "RMC,", 4 ) == 0 )
	{
		gps_sec_count++;
		if( process_rmc( psrc ) == 0 )  /*处理正确的RMC信息,判断格式正确*/
		{
			vdr_rx_gps( );              /*行车记录仪数据处理*/
			process_gps( );             /*处理GPS信息*/
		}
	}

	/*天线开短路检测 gps<$GNTXT,01,01,01,ANTENNA OK*2B*/
	if( strncmp( psrc + 3, "TXT", 3 ) == 0 )
	{
		if( strstr( psrc + 24, "OK" ) != RT_NULL )
		{
			gps_status.Antenna_Flag = 0;
		}
		if( strstr( psrc + 24, "OPEN" ) != RT_NULL )
		{
			gps_status.Antenna_Flag = 1;
			jt808_alarm				|= ( 1 << 5 );  /*bit5 天线开路*/
		}
		if( strstr( psrc + 24, "SHORT" ) != RT_NULL )
		{
			gps_status.Antenna_Flag = 1;
			jt808_alarm				|= ( 1 << 5 );  /*bit5 天线开路*/
		}
	}
}

/*初始化jt808 gps相关的处理*/
void jt808_gps_init( void )
{
	auxio_init( );
	rt_timer_init( &tmr_gps, "tmr_gps",         /* 定时器名字是 tmr_gps */
	               cb_tmr_gps,                  /* 超时时回调的处理函数 */
	               RT_NULL,                     /* 超时函数的入口参数 */
	               RT_TICK_PER_SECOND / 20,     /* 定时长度，以OS Tick为单位 */
	               RT_TIMER_FLAG_PERIODIC );    /* 周期性定时器 */

	rt_timer_start( &tmr_gps );
}

/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
void gps_dump( uint8_t mode )
{
	gps_status.Raw_Output = ~gps_status.Raw_Output;
}

FINSH_FUNCTION_EXPORT( gps_dump, dump gps raw info );

/************************************** The End Of File **************************************/

#ifdef DEBUG_GPS
/**模拟调试gps速度*/
void gps_speed_add( uint8_t sp, uint32_t count )
{
	speed_add	= sp;
	speed_count = count;
}

FINSH_FUNCTION_EXPORT_ALIAS( gps_speed_add, gps_speed, debug gps speed );
#endif

/************************************** The End Of File **************************************/
