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

#endif


