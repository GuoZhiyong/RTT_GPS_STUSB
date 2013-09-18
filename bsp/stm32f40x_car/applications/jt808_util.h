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
#ifndef _H_JT808_UTIL_
#define _H_JT808_UTIL_

#include "stm32f4xx.h"

#define BYTESWAP2( val )    \
    ( ( ( ( val ) & 0xff ) << 8 ) |   \
      ( ( ( val ) & 0xff00 ) >> 8 ) )

#define BYTESWAP4( val )    \
    ( ( ( ( val ) & 0xff ) << 24 ) |   \
      ( ( ( val ) & 0xff00 ) << 8 ) |  \
      ( ( ( val ) & 0xff0000 ) >> 8 ) |  \
      ( ( ( val ) & 0xff000000 ) >> 24 ) )

#define HEX2BCD( x )	( ( ( x ) / 10 ) << 4 | ( ( x ) % 10 ) )
#define BCD2HEX( x )	( ( ( ( x ) >> 4 ) * 10 ) + ( ( x ) & 0x0f ) )

#define PACK_BYTE( buf, byte ) ( *( buf ) = ( byte ) )
#define PACK_WORD( buf, word ) \
    do { \
		*( ( buf ) )		= ( word ) >> 8; \
		*( ( buf ) + 1 )	= ( word ) & 0xff; \
	} \
    while( 0 )

#define PACK_INT( buf, byte4 ) \
    do { \
		*( ( buf ) )		= ( byte4 ) >> 24; \
		*( ( buf ) + 1 )	= ( byte4 ) >> 16; \
		*( ( buf ) + 2 )	= ( byte4 ) >> 8; \
		*( ( buf ) + 3 )	= ( byte4 ) & 0xff; \
	} while( 0 )

typedef uint32_t MYTIME;

#define MYDATETIME( year, month, day, hour, minute, sec ) \
    ( (uint32_t)( ( year ) << 26 ) | \
      (uint32_t)( ( month ) << 22 ) | \
      (uint32_t)( ( day ) << 17 ) | \
      (uint32_t)( ( hour ) << 12 ) | \
      (uint32_t)( ( minute ) << 6 ) | ( sec ) )

#define YEAR( datetime )	( ( datetime >> 26 ) & 0x3F )
#define MONTH( datetime )	( ( datetime >> 22 ) & 0xF )
#define DAY( datetime )		( ( datetime >> 17 ) & 0x1F )
#define HOUR( datetime )	( ( datetime >> 12 ) & 0x1F )
#define MINUTE( datetime )	( ( datetime >> 6 ) & 0x3F )
#define SEC( datetime )		( datetime & 0x3F )

MYTIME mytime_from_hex( uint8_t* buf );


/*转换bcd时间*/
MYTIME mytime_from_bcd( uint8_t* buf );


/*转换为十六进制的时间 例如 2013/07/18 => 0x0d 0x07 0x12*/
void mytime_to_hex( uint8_t* buf, MYTIME time );


/*转换为bcd字符串为自定义时间 例如 0x13 0x07 0x12=>代表 13年7月12日*/
void mytime_to_bcd( uint8_t* buf, MYTIME time );


uint32_t buf_to_data( uint8_t * psrc, uint8_t width );


uint16_t data_to_buf( uint8_t * pdest, uint32_t data, uint8_t width );


unsigned short CalcCRC16( unsigned char *ptr, int pos, unsigned int len, unsigned short crc );


uint8_t bkpsram_init( void );


uint8_t bkpsram_write( uint32_t addr, uint8_t * data, uint16_t len );


uint16_t bkpsram_read( uint32_t addr, uint8_t * data, uint16_t len );


#define LVL_RAW		0
#define LVL_INFO	1
#define LVL_WARNING 2
#define LVL_ERROR	4


#endif

/************************************** The End Of File **************************************/
