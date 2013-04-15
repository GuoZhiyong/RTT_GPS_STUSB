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
#include "scr.h"


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

static LCD_MSG lcd_msg;


/**/
uint8_t process_rmc( uint8_t * pinfo )
{
	//检查数据完整性,执行数据转换
	uint8_t		year = 0, mon = 0, day = 0, hour = 0, min = 0, sec = 0, fDateModify = 0;
	uint32_t	degrees, minutes;
	uint8_t		count;

	uint8_t 	buf[16];
	uint8_t gps_av,gps_ew,gps_ns;
	
	uint8_t		*psrc = pinfo + 7; //指向开始位置


	//rt_kprintf("\r\ngps>%s",pinfo);

/*时间处理 $GNRMC,023548.00,V,,,,,,,270313,,,N*6F*/
	count = 0;
	while(( *psrc != ',' )&&(count<16))
	{
		buf[count++]	= *psrc;
		buf[count]		= 0;
		psrc++;
	}
	if( count != 0 )
	{
		hour	= ( buf[0] - 0x30 ) * 10 + ( buf[1] - 0x30 ) + 8;
		min		= ( buf[2] - 0x30 ) * 10 + ( buf[3] - 0x30 );
		sec		= ( buf[4] - 0x30 ) * 10 + ( buf[5] - 0x30 );
		if( hour > 23 )
		{
			fDateModify = 1;
			hour		-= 24;
		}
	}
/*A_V处理*/
	psrc++;
	count = 0;
	gps_av='V';
	while(( *psrc != ',' )&&(count<16))
	{
		buf[count++]	= *psrc;
		buf[count]		= 0;
		psrc++;
	}
	if( ( buf[0] == 'A' ) || ( buf[0] == 'V' ) )
	{
		gps_av = buf[0];
	}

/*纬度处理ddmm.mmmmmm*/
	psrc++;
	count = 0;
	degrees=0;
	minutes=0;
	while( ( *psrc != ',' ) && ( count < 16 ) )
	{
		buf [count++]	= *psrc;
		buf [count]	= 0;
		psrc++;
	}
	if( count != 0 )
	{
		degrees = ( ( buf [0] - 0x30 ) * 10 + ( buf [1] - 0x30 ) ) * 60 * 100000;
		minutes = ( buf [2] - 0x30 ) * 1000000 +
		          ( buf [3] - 0x30 ) * 100000 +
		          ( buf [5] - 0x30 ) * 10000 +
		          ( buf [6] - 0x30 ) * 1000 +
		          ( buf [7] - 0x30 ) * 100 +
		          ( buf [8] - 0x30 ) * 10 +
		          ( buf [9] - 0x30 );
	}
/*N_S处理*/
	psrc++;
	count = 0;
	gps_ns='N';
	while(( *psrc != ',' )&&(count<16))
	{
		buf[count++]	= *psrc;
		buf[count]		= 0;
		psrc++;
	}
	if( ( buf[0] == 'N' ) || ( buf[0] == 'S' ) )
	{
		gps_ns = buf[0];
	}

/*经度处理*/
	psrc++;
	count = 0;
	while(( *psrc != ',' )&&(count<16))
	{
		buf[count++] = *psrc;
		buf[count]	= 0;
		psrc++;
	}
	if( count != 0 )
	{
		degrees = ( ( buf [0] - 0x30 ) * 100 + ( buf [1] - 0x30 ) * 10 + ( buf [2] - 0x30 ) ) * 60 * 100000;
		minutes = ( buf [3] - 0x30 ) * 1000000 +
		          ( buf [4] - 0x30 ) * 100000 +
		          ( buf [6] - 0x30 ) * 10000 +
		          ( buf [7] - 0x30 ) * 1000 +
		          ( buf [8] - 0x30 ) * 100 +
		          ( buf [9] - 0x30 ) * 10 +
		          ( buf [10] - 0x30 );
	}
/*N_S处理*/
	psrc++;
	count = 0;
	gps_ns='E';
	while(( *psrc != ',' )&&(count<16))
	{
		buf[count++]	= *psrc;
		buf[count]		= 0;
		psrc++;
	}
	if( ( buf[0] == 'E' ) || ( buf[0] == 'W' ) )
	{
		gps_ns = buf[0];
	}

/*速度处理*/
	psrc++;
	count = 0;
	while( *psrc != ',' )
	{
		buf [count++] = *psrc;
		buf [count]	= 0;
		psrc++;
	}


/*方向处理*/
	psrc++;
	count = 0;
	while( ( *psrc != ',' ) && ( count < 12 ) )
	{
		buf [count++]	= *psrc;
		buf [count]		= 0;
		psrc++;
	}


/*日期处理*/
	psrc++;
	count = 0;
	while( ( *psrc != ',' ) && ( count < 12 ) )
	{
		buf [count++]	= *psrc;
		buf [count]	= 0;
		psrc++;
	}
	if( count == 0 )
	{
		return 8;
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

	lcd_msg.id						= LCD_MSG_ID_GPS;
	lcd_msg.info.gps_rmc.gps_av		= gps_av;
	lcd_msg.info.gps_rmc.year		= year;
	lcd_msg.info.gps_rmc.month		= mon;
	lcd_msg.info.gps_rmc.day		= day;
	lcd_msg.info.gps_rmc.hour		= hour;
	lcd_msg.info.gps_rmc.minitue	= min;
	lcd_msg.info.gps_rmc.sec		= sec;
	pscr->msg( (void*)&lcd_msg );
	return 0;
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
void process_gga( uint8_t * pinfo )
{
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
void gps_analy( uint8_t * pinfo )
{
	uint16_t	len;
	uint8_t		* psrc=pinfo;
	if( ( strncmp( psrc, "$GNRMC,", 7 ) == 0 ) || ( strncmp( psrc, "$BDRMC,", 7 ) == 0 ) || ( strncmp( psrc, "$GPRMC,", 7 ) == 0 ) )
	{
		process_rmc( psrc );
	}
	if( ( strncmp( psrc, "$GNGGA,", 7 ) == 0 ) || ( strncmp( psrc, "$BDGGA,", 7 ) == 0 ) || ( strncmp( psrc, "$GPGGA,", 7 ) == 0 ) )
	{
		process_gga( psrc );
	}
}

/************************************** The End Of File **************************************/
