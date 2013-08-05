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
   ��buf�л�ȡʱ����Ϣ
   �����0xFF ��ɵ�!!!!!!!����Դ�

 */
MYTIME mytime_from_hex( uint8_t* buf )
{
	MYTIME ret=0;
	uint8_t *p = buf;
	if( *p == 0xFF ) /*������Ч������*/
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

/*ת��bcdʱ��*/
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

/*ת��Ϊʮ�����Ƶ�ʱ�� ���� 2013/07/18 => 0x0d 0x07 0x12*/
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

/*ת��Ϊbcd�ַ���Ϊ�Զ���ʱ�� ���� 0x13 0x07 0x12=>���� 13��7��12��*/
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
  *��������:uint16_t data_to_buf( uint8_t * pdest, uint32_t data, uint8_t width )
  *��������:����ͬ���͵����ݴ���buf�У�������buf��Ϊ���ģʽ
  *��	��:	pdest:  ������ݵ�buffer
   data:	������ݵ�ԭʼ����
   width:	��ŵ�ԭʼ����ռ�õ�buf�ֽ���
  *��	��:
  *�� �� ֵ:������ֽ���
  *��	��:������
  *��������:2013-06-5
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
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
  *��������:uint16_t buf_to_data( uint8_t * psrc, uint8_t width )
  *��������:����ͬ���͵����ݴ�buf��ȡ������������buf��Ϊ���ģʽ
  *��	��:	psrc:   ������ݵ�buffer
   width:	��ŵ�ԭʼ����ռ�õ�buf�ֽ���
  *��	��:	 none
  *�� �� ֵ:uint32_t ���ش洢������
  *��	��:������
  *��������:2013-06-5
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
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






