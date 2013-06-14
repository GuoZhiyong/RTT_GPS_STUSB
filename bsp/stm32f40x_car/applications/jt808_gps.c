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

#include "jt808_gps.h"

#include "rtc.h"

typedef struct _GPSPoint
{
	int sign;
	int deg;
	int min;
	int sec;
} GPSPoint;

/*要用union类型保存，位域，访问吗?*/
uint32_t	jt808_alarm		= 0x0;
uint32_t	jt808_status	= 0x0;




/*
   区域的定义,使用list关联起来，如果node过多的话，
   RAM是否够用
   使用dataflash存储，以4k作为cache,缓存访问
   每秒的位置信息都要判断
 */
struct
{
	uint32_t	id;                 /*区域ID*/
	uint16_t	attr;               /*属性*/
	uint32_t	latitude;           /*中心纬度*/
	uint32_t	logitude;           /*中心经度*/
	uint32_t	radius;             /*半径*/
	uint8_t		datetime_start[6];  /*开始时刻，使用utc是不是更好?*/
	uint8_t		datetime_end[6];
	uint16_t	speed;
	uint8_t		duration;           /*持续时间*/
} circle;

struct
{
	uint32_t	id;                 /*区域ID*/
	uint16_t	attr;               /*属性*/
	uint32_t	latitude;           /*中心纬度*/
	uint32_t	logitude;           /*中心经度*/
	uint32_t	radius;             /*半径*/
	uint8_t		datetime_start[6];  /*开始时刻，使用utc是不是更好?*/
	uint8_t		datetime_end[6];
	uint16_t	speed;
	uint8_t		duration;           /*持续时间*/
} rectangle;

/*保存gps基本位置信息*/
GPS_BASEINFO	gps_baseinfo;
/*gps的状态*/
GPS_STATUS		gps_status;


/*
Epoch指的是一个特定的时间：1970-01-01 00:00:00 UTC
UNIX时间戳：Unix时间戳（英文为Unix time, POSIX time 或 Unix timestamp）
是从Epoch（1970年1月1日00:00:00 UTC）开始所经过的秒数，不考虑闰秒。

*/
unsigned long	timestamp_last=0;
unsigned long	timestamp_now=0;



/*
Linux源码中的mktime算法解析 
*/
static __inline unsigned long linux_mktime (unsigned int year, unsigned int mon,
    unsigned int day, unsigned int hour,
    unsigned int min, unsigned int sec)
       {
    if (0 >= (int) (mon -= 2)){ /**//* 1..12 -> 11,12,1..10 */
         mon += 12; /**//* Puts Feb last since it has leap day */
         year -= 1;
    }

    return (((
             (unsigned long) (year/4 - year/100 + year/400 + 367*mon/12 + day) +
             year*365 - 719499
          )*24 + hour /**//* now have hours */
       )*60 + min /**//* now have minutes */
    )*60 + sec; /**//* finally seconds */
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
	return 6371.01 * 1000.0 * centralAngle; //Return Distance in meter
}

/*处理gps信息*/
void process_gps(void)
{
	int i;
/*1.RTC,避免和rtt的set_time,set_date歧义，rtt是一个Driver,此处没有使用*/
	
	if(gps_baseinfo.datetime[5]==0);
	{
		rt_kprintf("datetime>");
		for(i=0;i<6;i++)rt_kprintf("%d ",gps_baseinfo.datetime[i]);
		rt_kprintf("\r\n");
		date_set(gps_baseinfo.datetime[0],gps_baseinfo.datetime[1],gps_baseinfo.datetime[2]);
		date_set(gps_baseinfo.datetime[0],gps_baseinfo.datetime[1],gps_baseinfo.datetime[2]);
		time_set(gps_baseinfo.datetime[3],gps_baseinfo.datetime[4],gps_baseinfo.datetime[5]);
		date_set(gps_baseinfo.datetime[0],gps_baseinfo.datetime[1],gps_baseinfo.datetime[2]);
	}

/*2.数据上报方式*/






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

	uint8_t 	tbl[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

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
				hour	= ( buf[0] - 0x30 ) * 10 + ( buf[1] - 0x30 ) + 8;
				min		= (( buf[2] - 0x30 )<<4) + ( buf[3] - 0x30 );
				sec		= ( buf[4] - 0x30 ) * 10 + ( buf[5] - 0x30 );
				if( hour > 23 )
				{
					fDateModify = 1;
					hour		-= 24;
				}
				rt_kprintf("hour=%d,min=%d,sec=%d\r\n",hour,min,sec);
				break;
			case 2: /*A_V*/
				if( buf[0] == 'A' )
				{
					jt808_status &= ~0x01;
				} else if( buf[0] == 'V' )
				{
					jt808_status |= 0x01;
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
				degrees = ( ( buf [0] - 0x30 ) * 10 + ( buf [1] - 0x30 ) ) * 60 * 100000;
				minutes = ( buf [2] - 0x30 ) * 1000000 +
				          ( buf [3] - 0x30 ) * 100000 +
				          ( buf [5] - 0x30 ) * 10000 +
				          ( buf [6] - 0x30 ) * 1000 +
				          ( buf [7] - 0x30 ) * 100 +
				          ( buf [8] - 0x30 ) * 10 +
				          ( buf [9] - 0x30 );
				lati = degrees + minutes / 60;
				break;
			case 4: /*N_S处理*/
				if( buf[0] == 'N' )
				{
					jt808_status &= ~0x02;
				} else if( buf[0] == 'S' )
				{
					jt808_status |= 0x02;
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
				degrees = ( ( buf [0] - 0x30 ) * 100 + ( buf [1] - 0x30 ) * 10 + ( buf [2] - 0x30 ) ) * 60 * 100000;
				minutes = ( buf [3] - 0x30 ) * 1000000 +
				          ( buf [4] - 0x30 ) * 100000 +
				          ( buf [6] - 0x30 ) * 10000 +
				          ( buf [7] - 0x30 ) * 1000 +
				          ( buf [8] - 0x30 ) * 100 +
				          ( buf [9] - 0x30 ) * 10 +
				          ( buf [10] - 0x30 );
				longi = degrees + minutes / 60;
				break;
			case 6: /*N_S处理*/
				if( buf[0] == 'E' )
				{
					jt808_status &= ~0x04;
				} else if( buf[0] == 'W' )
				{
					jt808_status |= 0x04;
				}else
				{
					return 6;
				}
				break;
			case 7: /*速度处理*/
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
						if( ( year % 4 ) == 0 )
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
				gps_baseinfo.alarm		= jt808_alarm;
				gps_baseinfo.status		= jt808_status;
				gps_baseinfo.latitude	= lati;
				gps_baseinfo.longitude	= longi;
				gps_baseinfo.spd		= speed_10x;
				gps_baseinfo.cog		= cog;

				timestamp_now=linux_mktime(year,mon,day,hour,min,sec);
				gps_baseinfo.datetime[0]=(tbl[year >> 4]<<4)|tbl[year & 0x0f] ;
				gps_baseinfo.datetime[1]=(tbl[mon >> 4]<<4)|tbl[mon & 0x0f] ;
				gps_baseinfo.datetime[2]=(tbl[day >> 4]<<4)|tbl[day & 0x0f] ;
				gps_baseinfo.datetime[3]=(tbl[hour >> 4]<<4)|tbl[hour & 0x0f] ;
				gps_baseinfo.datetime[4]=(tbl[min>> 4]<<4)|tbl[min & 0x0f] ;
				gps_baseinfo.datetime[5]=(sec/10) ;
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
	uint8_t		*psrc		= pinfo + 7;    //指向开始位置
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
			case 1:	/*时间处理 */
			if(count<6) return 1;
			break;

			case 2:/*纬度处理ddmm.mmmmmm*/
			if( count <10 )
			{
				return 2;
			}
			degrees = ( ( buf [0] - 0x30 ) * 10 + ( buf [1] - 0x30 ) ) * 60 * 100000;
			minutes = ( buf [2] - 0x30 ) * 1000000 +
			          ( buf [3] - 0x30 ) * 100000 +
			          ( buf [5] - 0x30 ) * 10000 +
			          ( buf [6] - 0x30 ) * 1000 +
			          ( buf [7] - 0x30 ) * 100 +
			          ( buf [8] - 0x30 ) * 10 +
			          ( buf [9] - 0x30 );
			lati = degrees + minutes / 60;
			break;

			case 3:	/*N_S处理*/
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

			case 4:	/*经度处理*/
			if( count < 11 )
			{
				return 4;
			}
			degrees = ( ( buf [0] - 0x30 ) * 100 + ( buf [1] - 0x30 ) * 10 + ( buf [2] - 0x30 ) ) * 60 * 100000;
			minutes = ( buf [3] - 0x30 ) * 1000000 +
			          ( buf [4] - 0x30 ) * 100000 +
			          ( buf [6] - 0x30 ) * 10000 +
			          ( buf [7] - 0x30 ) * 1000 +
			          ( buf [8] - 0x30 ) * 100 +
			          ( buf [9] - 0x30 ) * 10 +
			          ( buf [10] - 0x30 );
			longi = degrees + minutes / 60;
			break;
			case 5:	/*E_W处理*/
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
			case 6:/*定位类型*/
			break;
			case 7:	/*NoSV,卫星数*/
			NoSV=0;
			for(i=0;i<count;i++)
			{
				NoSV=NoSV*10;
				NoSV+=(buf[i]-0x30);
			}
			gps_status.NoSV = NoSV;
			break;
			case 8:		/*HDOP*/
				return 0;
			break;
			case 9:		/*MSL Altitute*/
				altitute=0;
				for(i=0;i<count;i++)
				{
					if(buf[i]=='.') break;
					altitute=altitute*10;
					altitute+=(buf[i]-'0');
				}
				gps_baseinfo.altitude=altitute;
				return 0;
			break;
			
		}
		count=0;
		buf[0]=0;
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
	
	if( ( strncmp( psrc, "$GNRMC,", 7 ) == 0 ) || ( strncmp( psrc, "$BDRMC,", 7 ) == 0 ) || ( strncmp( psrc, "$GPRMC,", 7 ) == 0 ) )
	{
		process_rmc( psrc );
		process_gps();	/*处理GPS信息*/
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
	gps_status.Raw_Output = mode;
}

FINSH_FUNCTION_EXPORT( gps_dump, dump gps raw info );

/************************************** The End Of File **************************************/
