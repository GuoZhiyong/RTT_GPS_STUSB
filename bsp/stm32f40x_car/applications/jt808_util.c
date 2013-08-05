#include "jt808_util.h"


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
__inline MYTIME buf_to_time( uint8_t *p )
{
	uint32_t ret;
	ret = (uint32_t)( ( *p++ ) << 26 );
	ret |= (uint32_t)( ( *p++ ) << 22 );
	ret |= (uint32_t)( ( *p++ ) << 17 );
	ret |= (uint32_t)( ( *p++ ) << 12 );
	ret |= (uint32_t)( ( *p++ ) << 6 );
	ret |= ( *p );
	return ret;
}


/*
   从buf中获取时间信息
   如果是0xFF 组成的!!!!!!!特殊对待

 */
MYTIME mytime_from_hex( uint8_t* buf )
{
	MYTIME ret=0;
	uint8_t *p = buf;
	if( *p == 0xFF ) /*不是有效的数据*/
	{
		return 0xFFFFFFFF;
	}
	ret = (uint32_t)( ( *p++ ) << 26 );
	ret |= (uint32_t)( ( *p++ ) << 22 );
	ret |= (uint32_t)( ( *p++ ) << 17 );
	ret |= (uint32_t)( ( *p++ ) << 12 );
	ret |= (uint32_t)( ( *p++ ) << 6 );
	ret |= ( *p );
	return ret;
}

/*转换bcd时间*/
MYTIME mytime_from_bcd( uint8_t* buf )
{
	uint8_t year, month, day, hour, minute, sec;
	uint8_t *psrc = buf;
	year	= BCD2HEX( *psrc++ );
	month	= BCD2HEX( *psrc++ );
	day		= BCD2HEX( *psrc++ );
	hour	= BCD2HEX( *psrc++ );
	minute	= BCD2HEX( *psrc++ );
	sec		= BCD2HEX( *psrc );
	return MYDATETIME( year, month, day, hour, minute, sec );
}

/*转换为十六进制的时间 例如 2013/07/18 => 0x0d 0x07 0x12*/
void mytime_to_hex( uint8_t* buf, MYTIME time )
{
	uint8_t *psrc = buf;
	*psrc++ = YEAR( time );
	*psrc++ = MONTH( time );
	*psrc++ = DAY( time );
	*psrc++ = HOUR( time );
	*psrc++ = MINUTE( time );
	*psrc	= SEC( time );
}

/*转换为bcd字符串为自定义时间 例如 0x13 0x07 0x12=>代表 13年7月12日*/
void mytime_to_bcd( uint8_t* buf, MYTIME time )
{
	uint8_t *psrc = buf;
	*psrc++ = HEX2BCD( YEAR( time ) );
	*psrc++ = HEX2BCD( MONTH( time ) );
	*psrc++ = HEX2BCD( DAY( time ) );
	*psrc++ = HEX2BCD( HOUR( time ) );
	*psrc++ = HEX2BCD( MINUTE( time ) );
	*psrc	= HEX2BCD( SEC( time ) );
}



/*********************************************************************************
  *函数名称:uint16_t data_to_buf( uint8_t * pdest, uint32_t data, uint8_t width )
  *功能描述:将不同类型的数据存入buf中，数据在buf中为大端模式
  *输	入:	pdest:  存放数据的buffer
   data:	存放数据的原始数据
   width:	存放的原始数据占用的buf字节数
  *输	出:
  *返 回 值:存入的字节数
  *作	者:白养民
  *创建日期:2013-06-5
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
uint16_t data_to_buf( uint8_t * pdest, uint32_t data, uint8_t width )
{
	u8 *buf;
	buf = pdest;

	switch( width )
	{
		case 1:
			*buf++ = data & 0xff;
			break;
		case 2:
			*buf++	= data >> 8;
			*buf++	= data & 0xff;
			break;
		case 4:
			*buf++	= data >> 24;
			*buf++	= data >> 16;
			*buf++	= data >> 8;
			*buf++	= data & 0xff;
			break;
	}
	return width;
}

/*********************************************************************************
  *函数名称:uint16_t buf_to_data( uint8_t * psrc, uint8_t width )
  *功能描述:将不同类型的数据从buf中取出来，数据在buf中为大端模式
  *输	入:	psrc:   存放数据的buffer
   width:	存放的原始数据占用的buf字节数
  *输	出:	 none
  *返 回 值:uint32_t 返回存储的数据
  *作	者:白养民
  *创建日期:2013-06-5
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
uint32_t buf_to_data( uint8_t * psrc, uint8_t width )
{
	u8	i;
	u32 outData = 0;

	for( i = 0; i < width; i++ )
	{
		outData <<= 8;
		outData += *psrc++;
	}
	return outData;
}






