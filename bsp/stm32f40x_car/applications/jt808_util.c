/************************************************************
 * Copyright (C), 2008-2012,
 * FileName:		// �ļ���
 * Author:			// ����
 * Date:			// ����
 * Description:		// ģ������
 * Version:			// �汾��Ϣ
 * Function List:	// ��Ҫ�������书��
 *     1. -------
 * History:			// ��ʷ�޸ļ�¼
 *     <author>  <time>   <version >   <desc>
 *     David    96/10/12     1.0     build this moudle
 ***********************************************************/
#include "jt808_util.h"
#include <rtthread.h>
#include <finsh.h>

const unsigned short tbl_crc[256] = { /* CRC ��ʽ�� MODBUS Э����ʽ��*/
	0x0000, 0xC1C0, 0x81C1, 0x4001, 0x01C3, 0xC003, 0x8002, 0x41C2,
	0x01C6, 0xC006, 0x8007, 0x41C7, 0x0005, 0xC1C5, 0x81C4, 0x4004,
	0x01CC, 0xC00C, 0x800D, 0x41CD, 0x000F, 0xC1CF, 0x81CE, 0x400E,
	0x000A, 0xC1CA, 0x81CB, 0x400B, 0x01C9, 0xC009, 0x8008, 0x41C8,
	0x01D8, 0xC018, 0x8019, 0x41D9, 0x001B, 0xC1DB, 0x81DA, 0x401A,
	0x001E, 0xC1DE, 0x81DF, 0x401F, 0x01DD, 0xC01D, 0x801C, 0x41DC,
	0x0014, 0xC1D4, 0x81D5, 0x4015, 0x01D7, 0xC017, 0x8016, 0x41D6,
	0x01D2, 0xC012, 0x8013, 0x41D3, 0x0011, 0xC1D1, 0x81D0, 0x4010,
	0x01F0, 0xC030, 0x8031, 0x41F1, 0x0033, 0xC1F3, 0x81F2, 0x4032,
	0x0036, 0xC1F6, 0x81F7, 0x4037, 0x01F5, 0xC035, 0x8034, 0x41F4,
	0x003C, 0xC1FC, 0x81FD, 0x403D, 0x01FF, 0xC03F, 0x803E, 0x41FE,
	0x01FA, 0xC03A, 0x803B, 0x41FB, 0x0039, 0xC1F9, 0x81F8, 0x4038,
	0x0028, 0xC1E8, 0x81E9, 0x4029, 0x01EB, 0xC02B, 0x802A, 0x41EA,
	0x01EE, 0xC02E, 0x802F, 0x41EF, 0x002D, 0xC1ED, 0x81EC, 0x402C,
	0x01E4, 0xC024, 0x8025, 0x41E5, 0x0027, 0xC1E7, 0x81E6, 0x4026,
	0x0022, 0xC1E2, 0x81E3, 0x4023, 0x01E1, 0xC021, 0x8020, 0x41E0,
	0x01A0, 0xC060, 0x8061, 0x41A1, 0x0063, 0xC1A3, 0x81A2, 0x4062,
	0x0066, 0xC1A6, 0x81A7, 0x4067, 0x01A5, 0xC065, 0x8064, 0x41A4,
	0x006C, 0xC1AC, 0x81AD, 0x406D, 0x01AF, 0xC06F, 0x806E, 0x41AE,
	0x01AA, 0xC06A, 0x806B, 0x41AB, 0x0069, 0xC1A9, 0x81A8, 0x4068,
	0x0078, 0xC1B8, 0x81B9, 0x4079, 0x01BB, 0xC07B, 0x807A, 0x41BA,
	0x01BE, 0xC07E, 0x807F, 0x41BF, 0x007D, 0xC1BD, 0x81BC, 0x407C,
	0x01B4, 0xC074, 0x8075, 0x41B5, 0x0077, 0xC1B7, 0x81B6, 0x4076,
	0x0072, 0xC1B2, 0x81B3, 0x4073, 0x01B1, 0xC071, 0x8070, 0x41B0,
	0x0050, 0xC190, 0x8191, 0x4051, 0x0193, 0xC053, 0x8052, 0x4192,
	0x0196, 0xC056, 0x8057, 0x4197, 0x0055, 0xC195, 0x8194, 0x4054,
	0x019C, 0xC05C, 0x805D, 0x419D, 0x005F, 0xC19F, 0x819E, 0x405E,
	0x005A, 0xC19A, 0x819B, 0x405B, 0x0199, 0xC059, 0x8058, 0x4198,
	0x0188, 0xC048, 0x8049, 0x4189, 0x004B, 0xC18B, 0x818A, 0x404A,
	0x004E, 0xC18E, 0x818F, 0x404F, 0x018D, 0xC04D, 0x804C, 0x418C,
	0x0044, 0xC184, 0x8185, 0x4045, 0x0187, 0xC047, 0x8046, 0x4186,
	0x0182, 0xC042, 0x8043, 0x4183, 0x0041, 0xC181, 0x8180, 0x4040
};


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
__inline MYTIME buf_to_mytime( uint8_t *p )
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
	MYTIME	ret = 0;
	uint8_t *p	= buf;
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
	uint8_t *buf;
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
	uint8_t		i;
	uint32_t	outData = 0;

	for( i = 0; i < width; i++ )
	{
		outData <<= 8;
		outData += *psrc++;
	}
	return outData;
}

/*********************************************************************************
  *��������:unsigned int CRC16_ModBusEx(unsigned char *ptr, unsigned int len,unsigned int crc)
  *��������:�����ݽ���CRCУ�飬
  *��	��:	ptr	:����
   len	:���ݳ���
   crc	:��ʼCRCֵ��ע����һ��ʱΪOxFFFF;
  *��	��:	none
  *�� �� ֵ:unsigned int CRCУ����
  *��	��:������
  *��������:2013-06-23
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
unsigned short CalcCRC16( unsigned char *ptr, int pos, unsigned int len, unsigned short crc )
{
	unsigned char	da;
	unsigned char	*p = ptr + pos;
	while( len-- != 0 )
	{
		da	= (unsigned char)( crc / 256 ); /* �� 8 λ������������ʽ�ݴ�CRC �ĸ�8 λ */
		crc <<= 8;                          /* ���� 8 λ���൱��CRC �ĵ�8 λ����28 */
		crc ^= tbl_crc[da ^ *p];            /* �� 8 λ�͵�ǰ�ֽ���Ӻ��ٲ����CRC ���ټ�����ǰ��CRC */
		p++;
	}
	return ( crc );
}

/*********************************************************************************
  *��������:uint8_t BkpSram_write(uint32_t addr,uint8_t *data, uint16_t len)
  *��������:backup sram ����д��
  *��	��:	addr	:д��ĵ�ַ
   data	:д�������ָ��
   len		:д��ĳ���
  *��	��:	none
  *�� �� ֵ:uint8_t	:	0:��ʾ����ʧ�ܣ�	1:��ʾ�����ɹ�
  *��	��:������
  *��������:2013-06-18
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
uint8_t bkpsram_write( uint32_t addr, uint8_t * data, uint16_t len )
{
	uint32_t i;
	//addr &= 0xFFFC;
	//*(__IO uint32_t *) (BKPSRAM_BASE + addr) = data;
	for( i = 0; i < len; i++ )
	{
		if( addr < 0x1000 )
		{
			*(__IO uint8_t*)( BKPSRAM_BASE + addr ) = *data++;
		}else
		{
			return 0;
		}
		++addr;
	}
	return 1;
}

/*********************************************************************************
  *��������:uint16_t bkpSram_read(uint32_t addr,uint8_t *data, uint16_t len)
  *��������:backup sram ���ݶ�ȡ
  *��	��:	addr	:��ȡ�ĵ�ַ
   data	:��ȡ������ָ��
   len		:��ȡ�ĳ���
  *��	��:	none
  *�� �� ֵ:uint16_t	:��ʾʵ�ʶ�ȡ�ĳ���
  *��	��:������
  *��������:2013-06-18
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
uint16_t bkpsram_read( uint32_t addr, uint8_t * data, uint16_t len )
{
	uint32_t i;
	//addr &= 0xFFFC;
	//data = *(__IO uint32_t *) (BKPSRAM_BASE + addr);
	for( i = 0; i < len; i++ )
	{
		if( addr < 0x1000 )
		{
			*data++ = *(__IO uint8_t*)( BKPSRAM_BASE + addr );
		}else
		{
			break;
		}
		++addr;
	}
	return i;
}

/*��ʼ��bkp sram �ɹ�����0��ʧ�ܻ�1*/
uint8_t bkpsram_init( void )
{
	__IO uint32_t	tmp;
	uint8_t			buf[8];

	RCC_APB1PeriphClockCmd( RCC_APB1Periph_PWR, ENABLE );
	PWR_BackupAccessCmd( ENABLE );
	RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_BKPSRAM, ENABLE );
	PWR_BackupRegulatorCmd( ENABLE );
	while( PWR_GetFlagStatus( PWR_FLAG_BRR ) == RESET )
	{
		tmp++;
		if( tmp > 0xFFFFFF )
		{
			rt_kprintf( "\nBKPSRAM����" );
			return 1;		
		}
	}
	/*����־,�Ƿ���ĩ�����*/
	bkpsram_read( 0x0000, buf, 4 );
	if( memcmp( buf, "WXDH", 4 ) == 0 )
	{
		return 0;
	} else
	{
		bkpsram_write( 0x0000, "WXDH", 4 );
		rt_kprintf("\nbkp������ʧ");
		return 2;	/*������ʧ*/
	}
}

#define BKPSRAM_TEST
#ifdef BKPSRAM_TEST
/**/
void bkpsram_wr( uint32_t addr, char *psrc )
{
	char pstr[128];
	memset( pstr, 0, sizeof( pstr ) );
	memcpy( pstr, psrc, strlen( psrc ) );
	bkpsram_write( addr, (uint8_t*)pstr, strlen( pstr ) + 1 );
}

FINSH_FUNCTION_EXPORT( bkpsram_wr, write from backup sram );

/**/
void bkpsram_rd( uint32_t addr,uint16_t count )
{
	uint16_t i;
	for( i = 0; i < count; i++ )
	{
		if(i%16==0) rt_kprintf("\n");
		rt_kprintf("%02x ",*(__IO uint8_t*)( BKPSRAM_BASE + addr+i ));
	}
	rt_kprintf("\n");


}

FINSH_FUNCTION_EXPORT( bkpsram_rd, read from backup sram );
#endif

/************************************** The End Of File **************************************/
