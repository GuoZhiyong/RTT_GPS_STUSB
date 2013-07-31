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
#include <rtthread.h>
#include <rtdevice.h>
#include <dfs_posix.h>

#include "stm32f4xx.h"
#include "jt808.h"
#include <finsh.h>
#include "sst25.h"

#define UPDATA_DEBUG

#define UPDATA_USE_CONTINUE

#define DF_UpdataAddress_PARA	0x3C000 ///图片数据存储开始位置
#define DF_UpdataAddress_Start	0x2000  ///图片数据存储开始位置
#define DF_UpdataAddress_End	0x3D000 ///图片数据存储结束位置

#ifndef BIT
#define BIT( i ) ( (unsigned long)( 1 << i ) )
#endif

typedef  __packed struct
{
	u32 file_size;      ///程序文件大小
	u16 package_total;  ///总包数量
	u16 package_size;   ///每包的大小
	u16 fram_num_first; ///第一包的帧序号
	u16 pack_len_first; ///第一包的长度
	u8	style;          ///升级设备类型
	u8	Pack_Mark[128]; ///包标记,其中任意一位为0表示该包存在
}STYLE_UPDATA_STATE;

extern uint16_t data_to_buf( uint8_t * pdest, uint32_t data, uint8_t width );


extern uint32_t buf_to_data( uint8_t * psrc, uint8_t width );


extern void reset( uint32_t reason );


const unsigned int crc_ta_1[256] = { /* CRC 余式表 MODBUS 协议余式表*/
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


/*********************************************************************************
  *函数名称:unsigned int CRC16_ModBus(unsigned char *ptr, unsigned int len)
  *功能描述:对数据进行CRC校验，
  *输	入:	ptr	:数据
   len	:数据长度
  *输	出:	none
  *返 回 值:unsigned int CRC校验结果
  *作	者:白养民
  *创建日期:2013-06-23
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
unsigned int CRC16_ModBus( unsigned char *ptr, unsigned int len )
{
	unsigned int	crc;
	unsigned char	da;

	crc = 0xFFFF;
	while( len-- != 0 )
	{
		da	= (unsigned char)( crc / 256 ); /* 以 8 位二进制数的形式暂存CRC 的高8 位 */
		crc <<= 8;                          /* 左移 8 位，相当于CRC 的低8 位乘以28 */
		crc ^= crc_ta_1[da ^ *ptr];         /* 高 8 位和当前字节相加后再查表求CRC ，再加上以前的CRC */
		ptr++;
	}
	return ( crc );
}

/*********************************************************************************
  *函数名称:unsigned int CRC16_ModBusEx(unsigned char *ptr, unsigned int len,unsigned int crc)
  *功能描述:对数据进行CRC校验，
  *输	入:	ptr	:数据
   len	:数据长度
   crc	:初始CRC值，注，第一次时为OxFFFF;
  *输	出:	none
  *返 回 值:unsigned int CRC校验结果
  *作	者:白养民
  *创建日期:2013-06-23
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
unsigned int CRC16_ModBusEx( unsigned char *ptr, unsigned int len, unsigned int crc )
{
	//unsigned int crc;
	unsigned char da;

	//crc=0xFFFF;
	while( len-- != 0 )
	{
		da	= (unsigned char)( crc / 256 ); /* 以 8 位二进制数的形式暂存CRC 的高8 位 */
		crc <<= 8;                          /* 左移 8 位，相当于CRC 的低8 位乘以28 */
		crc ^= crc_ta_1[da ^ *ptr];         /* 高 8 位和当前字节相加后再查表求CRC ，再加上以前的CRC */
		ptr++;
	}
	return ( crc );
}

/*********************************************************************************
  *函数名称:void updata_flash_read_para(STYLE_UPDATA_STATE *para)
  *功能描述:读取升级参数
  *输	入:	para	:升级参数结构体
  *输	出:	none
  *返 回 值:none
  *作	者:白养民
  *创建日期:2013-06-23
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
void updata_flash_read_para( STYLE_UPDATA_STATE *para )
{
	sst25_read( DF_UpdataAddress_PARA, (u8*)para, sizeof( STYLE_UPDATA_STATE ) );
}

/*********************************************************************************
  *函数名称:u8 updata_flash_write_para(STYLE_UPDATA_STATE *para)
  *功能描述:写入升级参数
  *输	入:	para	:升级参数结构体
  *输	出:	none
  *返 回 值:u8	0:false	,	1:OK
  *作	者:白养民
  *创建日期:2013-06-23
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
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
			rt_kprintf( "\n 写入升级参数错误!" );
			return 0;
		}
	}
	return 1;
}

/*********************************************************************************
  *函数名称:void updata_flash_write_recv_page(STYLE_UPDATA_STATE *para)
  *功能描述:写入升级参数，该方法并不擦除扇区，所以只能将flash中的每个字节的1改为0，并不能讲0改为1
  *输	入:	para	:升级参数结构体
   page	:当前成功接收到得包序号
  *输	出:	none
  *返 回 值:none
  *作	者:白养民
  *创建日期:2013-06-23
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
void updata_flash_write_recv_page( STYLE_UPDATA_STATE *para, u16 page )
{
	para->Pack_Mark[page / 8] &= ~( BIT( page % 8 ) );
	sst25_write_through( DF_UpdataAddress_PARA, (u8*)para, sizeof( STYLE_UPDATA_STATE ) );
}

/*********************************************************************************
  *函数名称:u8 updata_flash_wr_filehead(u8 *pmsg,u16 len)
  *功能描述:程序升级结果通知(808命令0x0108)
  *输	入:	pmsg		:文件头信息
   len			:写入的第一包的长度
  *输	出:	none
  *返 回 值:u8	0:false	,	1:OK
  *作	者:白养民
  *创建日期:2013-06-24
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
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
		rt_kprintf( "\n 分配空间错误错误!" );
		return 0;
	}
	for( i = 0; i < 5; i++ )
	{
		msg[32] = 0; ///清空升级标记
		sst25_erase_4k( DF_UpdataAddress_Start );
		sst25_write_through( DF_UpdataAddress_Start, msg, len );
		sst25_read( DF_UpdataAddress_Start, tempbuf, len );
		if( 0 == memcmp( tempbuf, msg, len ) )
		{
			break;
		}else if( i == 5 )
		{
			rt_kprintf( "\n 写入升级文件信息头错误!" );
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
	rt_kprintf( "\n 写入升级文件信息头OK!" );
#endif
	return 1;
}

/*********************************************************************************
  *函数名称:u8 updata_flash_wr_file(u32 addr,u8 *pmsg,u16 len)
  *功能描述:程序升级结果通知(808命令0x0108)
  *输	入:	addr		:写入的地址
   pmsg		:文件头信息
   len			:写入的第一包的长度
  *输	出:	none
  *返 回 值:u8	0:false	,	1:OK
  *作	者:白养民
  *创建日期:2013-06-24
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
u8 updata_flash_wr_file( u32 addr, u8 *pmsg, u16 len )
{
	u8	*msg = pmsg;
	u8	*tempbuf;
	u16 i;

	tempbuf = rt_malloc( len );
	if( tempbuf == RT_NULL )
	{
		rt_kprintf( "\n 分配空间错误!" );
		return 0;
	}
	for( i = 0; i < 5; i++ )
	{
		msg[32] = 0; ///清空升级标记
		sst25_write_through( addr, msg, len );
		sst25_read( addr, tempbuf, len );
		if( 0 == memcmp( tempbuf, msg, len ) )
		{
			break;
		}else if( i == 5 )
		{
			rt_kprintf( "\n 写入升级文件信息错误!" );
			rt_free( tempbuf );
			return 0;
		}
	}
	rt_free( tempbuf );
	return 1;
}

/*********************************************************************************
  *函数名称:u8 updata_comp_file(u8 *file_info,u8 *msg_info)
  *功能描述:准备要升级的程序文件盒终端内部存储的文件程序比较
  *输	入:	file_info	:终端存储的文件信息
   msg_info	:808消息发送的程序文件信息
  *输	出:	none
  *返 回 值:	u8	0:表示比较失败，不允许升级		1:可以升级，重新加载所有参数
    2:之前升级了一半，可以续传升级	3:之前已经升级成，并且和当前版本相同，不需要重复升级
  *作	者:白养民
  *创建日期:2013-06-23
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
u8 updata_comp_file( u8 *file_info, u8 *msg_info )
{
	u16					i;
	STYLE_UPDATA_STATE	updata_state;
	///比较TCB文件头信息是否OK
	if( strncmp( (const char*)msg_info, "TCB.GPS.01", 10 ) != 0 )
	{
		return 0;
	}
	///数据指向程序文件信息区
	msg_info	+= 32;
	file_info	+= 32;

	///跳过升级标记部分

	///比较从"程序文件格式"到"终端固件版本号"之间的所有部分，必须完全匹配才行
	if( strncmp( (const char*)msg_info + 1, (const char*)file_info + 1, 62 - 1 ) != 0 )
	{
		return 0;
	}

	///比较从"产品运营商"到"程序长度"之间的所有部分，必须完全匹配才行
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
		///程序之前已经升级成功，不需要重新升级
		///增加用户操作代码
		return 3;
	}
#ifdef UPDATA_USE_CONTINUE
	return 2; ///目前不支持续传功能，所以只返回1
#else
	return 1;
#endif
}

/*********************************************************************************
  *函数名称:void updata_commit_ack_err(u16 fram_num)
  *功能描述:通用应答，错误应答
  *输	入:	fram_num:应答流水号
  *输	出:	none
  *返 回 值:rt_err_t
  *作	者:白养民
  *创建日期:2013-06-24
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
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
  *函数名称:void updata_commit_ack_ok(u16 fram_num)
  *功能描述:通用应答，单包OK应答
  *输	入:	fram_num:应答流水号
  *输	出:	none
  *返 回 值:rt_err_t
  *作	者:白养民
  *创建日期:2013-06-24
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
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
  *函数名称:void updata_ack_ok(u16 fram_num,u8 style,u8 updata_ok)
  *功能描述:程序升级结果通知(808命令0x0108)
  *输	入:	fram_num	:应答流水号
   style		:升级设备类型
   updata_ok	:升级成功为0，失败为1，取消为2
  *输	出:	none
  *返 回 值:none
  *作	者:白养民
  *创建日期:2013-06-24
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
void updata_ack_ok( u16 fram_num, u8 style, u8 updata_ok )
{
	u8 pbuf[8];
	pbuf[0] = style;
	pbuf[1] = updata_ok;
	jt808_tx_ack( 0x0108, pbuf, 2 );
}

/*********************************************************************************
  *函数名称:u8 updata_ack(STYLE_UPDATA_STATE *para,u8 check)
  *功能描述:多包接收
  *输	入:	check	:进行应答检查，0表示不检查直接应答，1表示如果升级OK就进行应答
   para	:升级参数
  *输	出:	none
  *返 回 值:u8	0:没有升级成功。	1:升级成功
  *作	者:白养民
  *创建日期:2013-06-24
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
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
	///原始消息流水号
	len += data_to_buf( pbuf, para->fram_num_first, 4 );

	///重传总包数
	len++;

	///重传包 ID 列表
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
	if( pbuf[4] == 0 ) ///升级成功
	{
		rt_kprintf( "\n 升级完成!" );

		///对文件进行CRC校验检测
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
			crc			= CRC16_ModBusEx( pbuf, len, crc );
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
			rt_kprintf( "\n CRC_错误!" );
		}
	}
#ifdef UPDATA_USE_CONTINUE
	else if( check == 0 ) ///升级没有成功
	{
		jt808_tx_ack( 0x8003, pbuf, len );
	}
#endif
	rt_free( pbuf );
	return ret;
}

/*********************************************************************************
  *函数名称:rt_err_t updata_jt808_0x8108(uint8_t *pmsg,u16 msg_len)
  *功能描述:平台下发拍照命令处理函数
  *输	入:	pmsg	:808消息体数据
   msg_len	:808消息体长度
  *输	出:	none
  *返 回 值:rt_err_t
  *作	者:白养民
  *创建日期:2013-06-17
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
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
	rt_kprintf( "\n 收到程序包,总包数=%4d，包序号=%4d,LEN=%4d", cur_package_total, cur_package_num, msg_len );
#endif
	///非法包
	if( ( cur_package_num > cur_package_total ) || ( cur_package_num == 0 ) || ( cur_package_total <= 1 ) )
	{
		updata_commit_ack_err( fram_num );
		goto UPDATA_ERR;
	}

	updata_commit_ack_ok( fram_num );                       ///通用应答

	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
	if( cur_package_num == 1 )                              ///第一包
	{
		///第一包处理
		style = msg[0];

		if( strncmp( (const char*)msg + 1, "70420", 5 ) )   ///判断是否712设备程序
		{
			updata_ack_ok( fram_num, style, 2 );
			goto UPDATA_ERR;
		}
		if( msg[0] != 0 )                                   ///判断是否对终端进行升级
		{
			updata_ack_ok( fram_num, style, 2 );
			goto UPDATA_ERR;
		}

		///获取升级版本号
		memset( tempbuf, 0, sizeof( tempbuf ) );
		memcpy( tempbuf, msg + 7, msg[6] );
#ifdef UPDATA_DEBUG
		rt_kprintf( "\n 程序升级版本=\"%s\"", tempbuf );
#endif
		datalen = msg_len - 7 - msg[6];
		msg		+= 7 + msg[6];

		///升级数据包长度,文件的总长度
		Tempu32data = buf_to_data( msg, 4 );
		msg			+= 4;

		///升级程序包版本比较
		sst25_read( DF_UpdataAddress_Start, tempbuf, 256 );
		i = updata_comp_file( tempbuf, msg );
		if( i == 0 ) ///不需要升级，文件比匹配
		{
#ifdef UPDATA_DEBUG
			rt_kprintf( "\n 程序不匹配，升级错误!" );
#endif
			updata_ack_ok( fram_num, style, 2 );
			goto UPDATA_ERR;
		}
		if( i == 1 )                                            ///重新升级程序
		{
			if( updata_flash_wr_filehead( msg, datalen ) == 0 ) ///写入关键头信息失败
			{
				updata_ack_ok( fram_num, style, 1 );
				goto UPDATA_ERR;
			}
			memset( &updata_state, 0xFF, sizeof( updata_state ) );
			updata_state.Pack_Mark[0]	&= ~( BIT( 0 ) );       ///第一包正确存入
			updata_state.file_size		= Tempu32data;
			updata_state.package_total	= cur_package_total;
			updata_state.package_size	= msg_len;
			updata_state.fram_num_first = fram_num;
			updata_state.pack_len_first = datalen;
			updata_state.style			= style;
			updata_flash_write_para( &updata_state );

#ifdef UPDATA_DEBUG
			rt_kprintf( "\n 程序开始升级!" );
#endif
		}
#ifdef UPDATA_USE_CONTINUE
		else if( i == 2 ) ///续传升级程序
		{
			updata_flash_read_para( &updata_state );
			///判断上位机下发的数据格式是否和之前升级了一半的格式相同，相同就可以继续续传升级，否则只能重新升级
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
				updata_state.Pack_Mark[0]	&= ~( BIT( 0 ) ); ///第一包正确存入
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
		else if( i == 3 ) ///程序升级成功
		{
			///程序之前已经升级成功，不需要重新升级
			///增加用户操作代码
#ifdef UPDATA_DEBUG
			rt_kprintf( "\n 程序之前已经升级成功!" );
#endif
			updata_ack_ok( fram_num, style, 2 );
			goto UPDATA_OK;
		}
	}else  ///其它包
	{
		if( cur_package_total != updata_state.package_total )
		{
			updata_ack_ok( updata_state.fram_num_first, updata_state.style, 1 ); ///通知取消升级
			goto UPDATA_ERR;
		}
		if( cur_package_num < updata_state.package_total )
		{
			///当前包是第二包，检查第二包大小是否等于第一包大小，如果不相等，则认为第二包大小为分包大小
			if( ( cur_package_num == 2 ) && ( updata_state.package_size != msg_len ) )
			{
				updata_state.package_size = msg_len;
				updata_flash_write_para( &updata_state );
			}else ///比较后面的数据包是否和分包大小相同
			{
				if( updata_state.package_size != msg_len )
				{
#ifdef UPDATA_DEBUG
					rt_kprintf( "\n 重新修改分包大小=%4d", msg_len );
#endif
					updata_ack_ok( updata_state.fram_num_first, updata_state.style, 1 );
					goto UPDATA_ERR;
				}
			}
		}
		///其它包处理
		--cur_package_num;
		Tempu32data = DF_UpdataAddress_Start + ( ( cur_package_num - 1 ) * updata_state.package_size ) + updata_state.pack_len_first;
		if( updata_flash_wr_file( Tempu32data, msg, msg_len ) )
		{
			updata_flash_write_recv_page( &updata_state, cur_package_num ); ///第一包正确存入

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
	rt_kprintf( "\n 程序升级完成，10秒后复位设备。" );
	rt_thread_delay( RT_TICK_PER_SECOND * 5 );
	reset( 1 );
	return RT_EOK;
}

/************************************** The End Of File **************************************/
