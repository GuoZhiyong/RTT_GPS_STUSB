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
#include <stdio.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <dfs_posix.h>

#include "stm32f4xx.h"
#include "jt808.h"
#include <finsh.h>
#include "sst25.h"
#include "jt808_util.h"

#define UPDATA_DEBUG

#define UPDATA_USE_CONTINUE

#define DF_UpdataAddress_PARA	0x3C000 ///ͼƬ���ݴ洢��ʼλ��
#define DF_UpdataAddress_Start	0x2000  ///ͼƬ���ݴ洢��ʼλ��
#define DF_UpdataAddress_End	0x3D000 ///ͼƬ���ݴ洢����λ��

#ifndef BIT
#define BIT( i ) ( (unsigned long)( 1 << i ) )
#endif

typedef  __packed struct
{
	u32 file_size;      ///�����ļ���С
	u16 package_total;  ///�ܰ�����
	u16 package_size;   ///ÿ���Ĵ�С
	u16 fram_num_first; ///��һ����֡���
	u16 pack_len_first; ///��һ���ĳ���
	u8	style;          ///�����豸����
	u8	Pack_Mark[128]; ///�����,��������һλΪ0��ʾ�ð�����
}STYLE_UPDATA_STATE;




/*********************************************************************************
  *��������:void updata_flash_read_para(STYLE_UPDATA_STATE *para)
  *��������:��ȡ��������
  *��	��:	para	:���������ṹ��
  *��	��:	none
  *�� �� ֵ:none
  *��	��:������
  *��������:2013-06-23
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
void updata_flash_read_para( STYLE_UPDATA_STATE *para )
{
	sst25_read( DF_UpdataAddress_PARA, (u8*)para, sizeof( STYLE_UPDATA_STATE ) );
}

/*********************************************************************************
  *��������:u8 updata_flash_write_para(STYLE_UPDATA_STATE *para)
  *��������:д����������
  *��	��:	para	:���������ṹ��
  *��	��:	none
  *�� �� ֵ:u8	0:false	,	1:OK
  *��	��:������
  *��������:2013-06-23
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
u8 updata_flash_write_para( STYLE_UPDATA_STATE *para )
{
	u16 i;
	u8	tempbuf[256];
	sst25_write_back( DF_UpdataAddress_PARA, (u8*)para, sizeof( STYLE_UPDATA_STATE ) );
	sst25_read( DF_UpdataAddress_PARA, tempbuf, sizeof( STYLE_UPDATA_STATE ) );

	for( i = 0; i < 5; i++ )
	{
		if( 0 == memcmp( tempbuf, (u8*)para, sizeof( STYLE_UPDATA_STATE ) ) )
		{
			break;
		}else if( i == 5 )
		{
			rt_kprintf( "\n д��������������!" );
			return 0;
		}
	}
	return 1;
}

/*********************************************************************************
  *��������:void updata_flash_write_recv_page(STYLE_UPDATA_STATE *para)
  *��������:д�������������÷���������������������ֻ�ܽ�flash�е�ÿ���ֽڵ�1��Ϊ0�������ܽ�0��Ϊ1
  *��	��:	para	:���������ṹ��
   page	:��ǰ�ɹ����յ��ð����
  *��	��:	none
  *�� �� ֵ:none
  *��	��:������
  *��������:2013-06-23
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
void updata_flash_write_recv_page( STYLE_UPDATA_STATE *para, u16 page )
{
	para->Pack_Mark[page / 8] &= ~( BIT( page % 8 ) );
	sst25_write_through( DF_UpdataAddress_PARA, (u8*)para, sizeof( STYLE_UPDATA_STATE ) );
}

/*********************************************************************************
  *��������:u8 updata_flash_wr_filehead(u8 *pmsg,u16 len)
  *��������:�����������֪ͨ(808����0x0108)
  *��	��:	pmsg		:�ļ�ͷ��Ϣ
   len			:д��ĵ�һ���ĳ���
  *��	��:	none
  *�� �� ֵ:u8	0:false	,	1:OK
  *��	��:������
  *��������:2013-06-24
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
u8 updata_flash_wr_filehead( u8 *pmsg, u16 len )
{
	u8	*msg = pmsg;
	u8	*tempbuf;
	u16 i;
	u32 tempAddress;
	tempbuf = rt_malloc( len );
	if( tempbuf == RT_NULL )
	{
		rt_kprintf( "\n ����ռ�������!" );
		return 0;
	}
	for( i = 0; i < 5; i++ )
	{
		msg[32] = 0; ///����������
		sst25_erase_4k( DF_UpdataAddress_Start );
		sst25_write_through( DF_UpdataAddress_Start, msg, len );
		sst25_read( DF_UpdataAddress_Start, tempbuf, len );
		if( 0 == memcmp( tempbuf, msg, len ) )
		{
			break;
		}else if( i == 5 )
		{
			rt_kprintf( "\n д�������ļ���Ϣͷ����!" );
			rt_free( tempbuf );
			return 0;
		}
	}
	rt_free( tempbuf );
	tempAddress = DF_UpdataAddress_Start + 0x1000;
	while( 1 )
	{
		if( tempAddress < DF_UpdataAddress_End )
		{
			sst25_erase_4k( tempAddress );
		}else
		{
			break;
		}
		tempAddress += 0x1000;
	}
#ifdef UPDATA_DEBUG
	rt_kprintf( "\n д�������ļ���ϢͷOK!" );
#endif
	return 1;
}

/*********************************************************************************
  *��������:u8 updata_flash_wr_file(u32 addr,u8 *pmsg,u16 len)
  *��������:�����������֪ͨ(808����0x0108)
  *��	��:	addr		:д��ĵ�ַ
   pmsg		:�ļ�ͷ��Ϣ
   len			:д��ĵ�һ���ĳ���
  *��	��:	none
  *�� �� ֵ:u8	0:false	,	1:OK
  *��	��:������
  *��������:2013-06-24
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
u8 updata_flash_wr_file( u32 addr, u8 *pmsg, u16 len )
{
	u8	*msg = pmsg;
	u8	*tempbuf;
	u16 i;

	tempbuf = rt_malloc( len );
	if( tempbuf == RT_NULL )
	{
		rt_kprintf( "\n ����ռ����!" );
		return 0;
	}
	for( i = 0; i < 5; i++ )
	{
		msg[32] = 0; ///����������
		sst25_write_through( addr, msg, len );
		sst25_read( addr, tempbuf, len );
		if( 0 == memcmp( tempbuf, msg, len ) )
		{
			break;
		}else if( i == 5 )
		{
			rt_kprintf( "\n д�������ļ���Ϣ����!" );
			rt_free( tempbuf );
			return 0;
		}
	}
	rt_free( tempbuf );
	return 1;
}

/*********************************************************************************
  *��������:u8 updata_comp_file(u8 *file_info,u8 *msg_info)
  *��������:׼��Ҫ�����ĳ����ļ����ն��ڲ��洢���ļ�����Ƚ�
  *��	��:	file_info	:�ն˴洢���ļ���Ϣ
   msg_info	:808��Ϣ���͵ĳ����ļ���Ϣ
  *��	��:	none
  *�� �� ֵ:	u8	0:��ʾ�Ƚ�ʧ�ܣ�����������		1:�������������¼������в���
    2:֮ǰ������һ�룬������������	3:֮ǰ�Ѿ������ɣ����Һ͵�ǰ�汾��ͬ������Ҫ�ظ�����
  *��	��:������
  *��������:2013-06-23
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
u8 updata_comp_file( u8 *file_info, u8 *msg_info )
{
	u16					i;
	STYLE_UPDATA_STATE	updata_state;
	///�Ƚ�TCB�ļ�ͷ��Ϣ�Ƿ�OK
	if( strncmp( (const char*)msg_info, "TCB.GPS.01", 10 ) != 0 )
	{
		return 0;
	}
	///����ָ������ļ���Ϣ��
	msg_info	+= 32;
	file_info	+= 32;

	///����������ǲ���

	///�Ƚϴ�"�����ļ���ʽ"��"�ն˹̼��汾��"֮������в��֣�������ȫƥ�����
	if( strncmp( (const char*)msg_info + 1, (const char*)file_info + 1, 62 - 1 ) != 0 )
	{
		return 0;
	}

	///�Ƚϴ�"��Ʒ��Ӫ��"��"���򳤶�"֮������в��֣�������ȫƥ�����
	if( strncmp( (const char*)msg_info + 62, (const char*)file_info + 62, 86 - 62 ) != 0 )
	{
		return 1;
	}

	updata_flash_read_para( &updata_state );

	for( i = 0; i < updata_state.package_total; i++ )
	{
		if( updata_state.Pack_Mark[i / 8] & ( BIT( i % 8 ) ) )
		{
			break;
		}
	}
	if( i == updata_state.package_total )
	{
		///����֮ǰ�Ѿ������ɹ�������Ҫ��������
		///�����û���������
		return 3;
	}
#ifdef UPDATA_USE_CONTINUE
	return 2; ///Ŀǰ��֧���������ܣ�����ֻ����1
#else
	return 1;
#endif
}

/*********************************************************************************
  *��������:void updata_commit_ack_err(u16 fram_num)
  *��������:ͨ��Ӧ�𣬴���Ӧ��
  *��	��:	fram_num:Ӧ����ˮ��
  *��	��:	none
  *�� �� ֵ:rt_err_t
  *��	��:������
  *��������:2013-06-24
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
void updata_commit_ack_err( u16 fram_num )
{
	u8 pbuf[8];
	data_to_buf( pbuf, fram_num, 2 );
	data_to_buf( pbuf + 2, 0x8108, 2 );
	pbuf[4] = 1;
	jt808_tx_ack( 0x0001, pbuf, 2 );
}

/*********************************************************************************
  *��������:void updata_commit_ack_ok(u16 fram_num)
  *��������:ͨ��Ӧ�𣬵���OKӦ��
  *��	��:	fram_num:Ӧ����ˮ��
  *��	��:	none
  *�� �� ֵ:rt_err_t
  *��	��:������
  *��������:2013-06-24
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
void updata_commit_ack_ok( u16 fram_num )
{
	u8 pbuf[8];
	data_to_buf( pbuf, fram_num, 2 );
	data_to_buf( pbuf + 2, 0x8108, 2 );
	pbuf[4] = 0;
	jt808_tx_ack( 0x0001, pbuf, 2 );
}

/*********************************************************************************
  *��������:void updata_ack_ok(u16 fram_num,u8 style,u8 updata_ok)
  *��������:�����������֪ͨ(808����0x0108)
  *��	��:	fram_num	:Ӧ����ˮ��
   style		:�����豸����
   updata_ok	:�����ɹ�Ϊ0��ʧ��Ϊ1��ȡ��Ϊ2
  *��	��:	none
  *�� �� ֵ:none
  *��	��:������
  *��������:2013-06-24
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
void updata_ack_ok( u16 fram_num, u8 style, u8 updata_ok )
{
	u8 pbuf[8];
	pbuf[0] = style;
	pbuf[1] = updata_ok;
	jt808_tx_ack( 0x0108, pbuf, 2 );
}

/*********************************************************************************
  *��������:u8 updata_ack(STYLE_UPDATA_STATE *para,u8 check)
  *��������:�������
  *��	��:	check	:����Ӧ���飬0��ʾ�����ֱ��Ӧ��1��ʾ�������OK�ͽ���Ӧ��
   para	:��������
  *��	��:	none
  *�� �� ֵ:u8	0:û�������ɹ���	1:�����ɹ�
  *��	��:������
  *��������:2013-06-24
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
u8 updata_ack( STYLE_UPDATA_STATE *para, u8 check )
{
	u16 i;
	u8	ret = 0;
	u8	*pbuf;
	u16 crc;
	u16 tempu16data;
	u32 len = 0;
	u32 tempAddr;
	u32 tempu32data;
	u32 size;

	///
	if( JT808_PACKAGE_MAX < 512 )
	{
		len = 512;
	}else
	{
		len = JT808_PACKAGE_MAX;
	}
	pbuf = rt_malloc( len );
	if( RT_NULL == pbuf )
	{
		return 0;
	}
	memset( pbuf, 0, len );
	len = 0;
	///ԭʼ��Ϣ��ˮ��
	len += data_to_buf( pbuf, para->fram_num_first, 4 );

	///�ش��ܰ���
	len++;

	///�ش��� ID �б�
	for( i = 0; i < para->package_total; i++ )
	{
		if( para->Pack_Mark[i / 8] & ( BIT( i % 8 ) ) )
		{
			pbuf[4]++;
			len += data_to_buf( pbuf + len, i + 1, 2 );
			if( len >= JT808_PACKAGE_MAX - 8 )
			{
				break;
			}
		}
	}
	if( pbuf[4] == 0 ) ///�����ɹ�
	{
		rt_kprintf( "\n �������!" );

		///���ļ�����CRCУ����
		tempu32data = 0;
		size		= para->file_size - 256;
		crc			= 0xFFFF;

		while( 1 )
		{
			if( tempu32data >= size )
			{
				break;
			}
			tempAddr	= DF_UpdataAddress_Start + 256 + tempu32data;
			len			= size - tempu32data;
			if( len > 512 )
			{
				len = 512;
			}

			sst25_read( tempAddr, pbuf, len );
			crc			= CalcCRC16(pbuf,0,len, crc );
			tempu32data += 512;
		}
		sst25_read( DF_UpdataAddress_Start, pbuf, 256 );
		tempu16data = buf_to_data( pbuf + 134, 2 );
		if( crc == tempu16data )
		{
			pbuf[32] = 1;
			sst25_write_back( DF_UpdataAddress_Start, pbuf, 256 );
			ret = 1;
			updata_ack_ok( para->fram_num_first, para->style, 0 );
		}else
		{
			updata_ack_ok( para->fram_num_first, para->style, 1 );
			rt_kprintf( "\n CRC_����!" );
		}
	}
#ifdef UPDATA_USE_CONTINUE
	else if( check == 0 ) ///����û�гɹ�
	{
		jt808_tx_ack( 0x8003, pbuf, len );
	}
#endif
	rt_free( pbuf );
	return ret;
}

/*********************************************************************************
  *��������:rt_err_t updata_jt808_0x8108(uint8_t *pmsg,u16 msg_len)
  *��������:ƽ̨�·������������
  *��	��:	pmsg	:808��Ϣ������
   msg_len	:808��Ϣ�峤��
  *��	��:	none
  *�� �� ֵ:rt_err_t
  *��	��:������
  *��������:2013-06-17
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
rt_err_t updata_jt808_0x8108( uint8_t linkno, uint8_t *pmsg )
{
	u16							i;
	u8							*msg;
	u16							datalen;
	u32							Tempu32data;
	u16							msg_len;
	u16							fram_num;
	static STYLE_UPDATA_STATE	updata_state = { 0, 0, 0, 0, 0 };
	u16							cur_package_total;
	u16							cur_package_num;
	u8							tempbuf[256];
	u8							style;

	msg_len		= buf_to_data( pmsg + 2, 2 ) & 0x3FF;
	fram_num	= buf_to_data( pmsg + 10, 2 );
	if( ( pmsg[2] & 0x20 ) == 0 )
	{
		updata_commit_ack_err( fram_num );
		goto UPDATA_ERR;
	}
	cur_package_total	= buf_to_data( pmsg + 12, 2 );
	cur_package_num		= buf_to_data( pmsg + 14, 2 );
	pmsg				+= 16;
	msg					= pmsg + 16;

#ifdef UPDATA_DEBUG
	rt_kprintf( "\n �յ������,�ܰ���=%4d�������=%4d,LEN=%4d", cur_package_total, cur_package_num, msg_len );
#endif
	///�Ƿ���
	if( ( cur_package_num > cur_package_total ) || ( cur_package_num == 0 ) || ( cur_package_total <= 1 ) )
	{
		updata_commit_ack_err( fram_num );
		goto UPDATA_ERR;
	}

	updata_commit_ack_ok( fram_num );                       ///ͨ��Ӧ��

	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
	if( cur_package_num == 1 )                              ///��һ��
	{
		///��һ������
		style = msg[0];

		if( strncmp( (const char*)msg + 1, "70420", 5 ) )   ///�ж��Ƿ�712�豸����
		{
			updata_ack_ok( fram_num, style, 2 );
			goto UPDATA_ERR;
		}
		if( msg[0] != 0 )                                   ///�ж��Ƿ���ն˽�������
		{
			updata_ack_ok( fram_num, style, 2 );
			goto UPDATA_ERR;
		}

		///��ȡ�����汾��
		memset( tempbuf, 0, sizeof( tempbuf ) );
		memcpy( tempbuf, msg + 7, msg[6] );
#ifdef UPDATA_DEBUG
		rt_kprintf( "\n ���������汾=\"%s\"", tempbuf );
#endif
		datalen = msg_len - 7 - msg[6];
		msg		+= 7 + msg[6];

		///�������ݰ�����,�ļ����ܳ���
		Tempu32data = buf_to_data( msg, 4 );
		msg			+= 4;

		///����������汾�Ƚ�
		sst25_read( DF_UpdataAddress_Start, tempbuf, 256 );
		i = updata_comp_file( tempbuf, msg );
		if( i == 0 ) ///����Ҫ�������ļ���ƥ��
		{
#ifdef UPDATA_DEBUG
			rt_kprintf( "\n ����ƥ�䣬��������!" );
#endif
			updata_ack_ok( fram_num, style, 2 );
			goto UPDATA_ERR;
		}
		if( i == 1 )                                            ///������������
		{
			if( updata_flash_wr_filehead( msg, datalen ) == 0 ) ///д��ؼ�ͷ��Ϣʧ��
			{
				updata_ack_ok( fram_num, style, 1 );
				goto UPDATA_ERR;
			}
			memset( &updata_state, 0xFF, sizeof( updata_state ) );
			updata_state.Pack_Mark[0]	&= ~( BIT( 0 ) );       ///��һ����ȷ����
			updata_state.file_size		= Tempu32data;
			updata_state.package_total	= cur_package_total;
			updata_state.package_size	= msg_len;
			updata_state.fram_num_first = fram_num;
			updata_state.pack_len_first = datalen;
			updata_state.style			= style;
			updata_flash_write_para( &updata_state );

#ifdef UPDATA_DEBUG
			rt_kprintf( "\n ����ʼ����!" );
#endif
		}
#ifdef UPDATA_USE_CONTINUE
		else if( i == 2 ) ///������������
		{
			updata_flash_read_para( &updata_state );
			///�ж���λ���·������ݸ�ʽ�Ƿ��֮ǰ������һ��ĸ�ʽ��ͬ����ͬ�Ϳ��Լ�����������������ֻ����������
			if( ( updata_state.file_size == Tempu32data ) && ( updata_state.package_total == cur_package_total ) && ( updata_state.package_size == msg_len ) && ( updata_state.pack_len_first == datalen ) )
			{
				updata_state.fram_num_first = fram_num;
			}else
			{
				if( updata_flash_wr_filehead( msg, datalen ) == 0 )
				{
					updata_ack_ok( fram_num, style, 1 );
					goto UPDATA_ERR;
				}
				memset( &updata_state, 0xFF, sizeof( updata_state ) );
				updata_state.Pack_Mark[0]	&= ~( BIT( 0 ) ); ///��һ����ȷ����
				updata_state.file_size		= Tempu32data;
				updata_state.package_total	= cur_package_total;
				updata_state.package_size	= msg_len;
				updata_state.fram_num_first = fram_num;
				updata_state.pack_len_first = datalen;
				updata_state.style			= style;
			}
			updata_flash_write_para( &updata_state );
		}
#endif
		else if( i == 3 ) ///���������ɹ�
		{
			///����֮ǰ�Ѿ������ɹ�������Ҫ��������
			///�����û���������
#ifdef UPDATA_DEBUG
			rt_kprintf( "\n ����֮ǰ�Ѿ������ɹ�!" );
#endif
			updata_ack_ok( fram_num, style, 2 );
			goto UPDATA_OK;
		}
	}else  ///������
	{
		if( cur_package_total != updata_state.package_total )
		{
			updata_ack_ok( updata_state.fram_num_first, updata_state.style, 1 ); ///֪ͨȡ������
			goto UPDATA_ERR;
		}
		if( cur_package_num < updata_state.package_total )
		{
			///��ǰ���ǵڶ��������ڶ�����С�Ƿ���ڵ�һ����С���������ȣ�����Ϊ�ڶ�����СΪ�ְ���С
			if( ( cur_package_num == 2 ) && ( updata_state.package_size != msg_len ) )
			{
				updata_state.package_size = msg_len;
				updata_flash_write_para( &updata_state );
			}else ///�ȽϺ�������ݰ��Ƿ�ͷְ���С��ͬ
			{
				if( updata_state.package_size != msg_len )
				{
#ifdef UPDATA_DEBUG
					rt_kprintf( "\n �����޸ķְ���С=%4d", msg_len );
#endif
					updata_ack_ok( updata_state.fram_num_first, updata_state.style, 1 );
					goto UPDATA_ERR;
				}
			}
		}
		///����������
		--cur_package_num;
		Tempu32data = DF_UpdataAddress_Start + ( ( cur_package_num - 1 ) * updata_state.package_size ) + updata_state.pack_len_first;
		if( updata_flash_wr_file( Tempu32data, msg, msg_len ) )
		{
			updata_flash_write_recv_page( &updata_state, cur_package_num ); ///��һ����ȷ����

			if( updata_state.package_total == cur_package_num + 1 )
			{
				if( updata_ack( &updata_state, 0 ) )
				{
					goto UPDATA_SUCCESS;
				}
			}else
			{
				if( updata_ack( &updata_state, 1 ) )
				{
					goto UPDATA_SUCCESS;
				}
			}
		}
	}
UPDATA_OK:
	rt_sem_release( &sem_dataflash );
	return RT_EOK;
UPDATA_ERR:
	rt_sem_release( &sem_dataflash );
	return RT_ERROR;
UPDATA_SUCCESS:
	rt_sem_release( &sem_dataflash );
	rt_kprintf( "\n ����������ɣ�10���λ�豸��" );
	rt_thread_delay( RT_TICK_PER_SECOND * 5 );
	reset( 1 );
	return RT_EOK;
}

/************************************** The End Of File **************************************/
