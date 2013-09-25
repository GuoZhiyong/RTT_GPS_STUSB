/************************************************************
 * Copyright (C), 2008-2012,
 * FileName:		//  camera.c
 * Author:			//  baiyangmin
 * Date:			//  2013-07-08
 * Description:		//  拍照功能的实现，包括有:拍照，存储，以及相关接口函数，该文件实现了文件jt808_camera.c的拍照部分驱动功能
 * Version:			//  V0.01
 * Function List:	//  主要函数及其功能
 *     1. -------
 * History:			//  历史修改记录
 *     <author>  <time>   <version >   <desc>
 *     David    96/10/12     1.0     build this moudle
 ***********************************************************/
#include <stdio.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <dfs_posix.h>

#include "stm32f4xx.h"
#include "rs485.h"
#include "jt808.h"
#include "jt808_gps.h"
#include "jt808_util.h"
#include "camera.h"
#include "jt808_camera.h"
#include <finsh.h>
#include "sst25.h"

typedef enum
{
	CAM_NONE = 0, /*空闲*/
	CAM_IDLE,
	CAM_START,
	CAM_GET_PHOTO,
	CAM_RX_PHOTO,
	CAM_OK,
	CAM_FALSE,
	CAM_END
}CAM_STATE;

/*接收cam信息格式*/
typedef enum
{
	RX_IDLE = 0,
	RX_SYNC1,
	RX_SYNC2,
	RX_HEAD,
	RX_DATA,
	RX_FCS,
	RX_0D,
	RX_0A,
}CAM_RX_STATE;

typedef  __packed struct
{
	CAM_STATE				State;      ///处理过程状态，
	CAM_RX_STATE			Rx_State;   ///接收状态
	u8						Retry;      ///重复照相次数
	Style_Cam_Requset_Para	Para;       ///触发当前拍照相关信息
} Style_Cam_Para;

#define DF_VOC_START		0x1D0000    ///录音数据存储开始位置
#define DF_VOC_END			0X298000    ///录音数据存储结束位置
#define DF_VOC_SECTOR_COUNT 0x400       ///录音数据存储大小

#define DF_CAM_START		0x108000    ///图片数据存储开始位置
#define DF_CAM_END			0X1D0000    ///图片数据存储结束位置
#define DF_CAM_REC_COUNT	4096        ///图片数据记录单元大小
#define DF_CAM_REC_MASK		0xFFFFF000  //每记录的掩码 ~(count-1)

#define DF_SECTOR_MASK 0xFFFFF000       /*4K的地址掩码*/

//static CAM_STATE				cam_state;      ///处理过程状态，

extern rt_device_t _console_device;
extern u16 Hex_To_Ascii( const u8* pSrc, u8* pDst, u16 nSrcLength );


//const char				CAM_HEAD[] = { "PIC_01" };
#define  CAM_HEAD 0x5049435F            /*图片标识 PIC_ */

static Style_Cam_Para	Current_Cam_Para;
static TypeDF_PICPara	DF_PicParam;    ///FLASH存储的图片信息

/* 消息队列控制块*/
struct rt_messagequeue	mq_Cam;
/* 消息队列中用到的放置消息的内存池*/
static char				msgpool_cam[256];

#define PHOTO_RX_SIZE 1024

static uint8_t	photo_rx[PHOTO_RX_SIZE];
static uint16_t photo_rx_wr = 0;

extern MsgList	* list_jt808_tx;


/*********************************************************************************
  *函数名称:u32 Cam_Flash_AddrCheck(u32 pro_Address)
  *功能描述:检查当前的位置是否合法，不合法就修改为合法数据并返回
  *输	入:none
  *输	出:none
  *返 回 值:正确的地址值
  *作	者:白养民
  *创建日期:2013-06-5
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
static u32 Cam_Flash_AddrCheck( u32 pro_Address )
{
	while( pro_Address >= DF_CAM_END )
	{
		pro_Address = pro_Address + DF_CAM_START - DF_CAM_END;
	}
	if( pro_Address < DF_CAM_START )
	{
		pro_Address = DF_CAM_START;
	}
	return pro_Address;
}

typedef uint32_t  (*PHOTO_FUNC)(void* ctx, void* data);

/*初始化照片数据，找到当前最新图片的位置、和图片数*/
uint32_t photo_init(void *ctx,void *data)
{


}

/*获取图片头*/
uint32_t photo_get_head(void *ctx,void *data)
{


}




/*遍历所有图片记录*/
static void foreach(PHOTO_FUNC visit,void * ctx)
{
	u32					TempAddress;
	TypeDF_PackageHead	TempPackageHead;

	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
	for( TempAddress = DF_CAM_START; TempAddress < DF_CAM_END; )
	{
		sst25_read( TempAddress, (u8*)&TempPackageHead, sizeof( TempPackageHead ) );
		if( TempPackageHead.Head == CAM_HEAD )                      /*有效数据*/
		{
			visit(&TempPackageHead);
			TempAddress += ( TempPackageHead.Len + DF_CAM_REC_COUNT - 1 ) & DF_CAM_REC_MASK;
		}else
		{
			TempAddress += DF_CAM_REC_COUNT;
		}
	}
	rt_sem_release( &sem_dataflash );
}




/*********************************************************************************
  *函数名称:u16 Cam_Flash_InitPara(u8 printf_info)
  *功能描述:初始化Pic参数，包括读取FLASH中的图片信息，获取到图片的数量及开始位置，结束位置等，
   这些读取到得数据都存储在 DF_PicParam 中。
  *输	入:printf_info	:	0、表示不打印图片信息，1表示打印图片信息
  *输	出:none
  *返 回 值:有效的图片数量
  *作	者:白养民
  *创建日期:2013-06-5
  *---------------------------------------------------------------------------------
  *修 改 人:郭智勇
  *修改日期:20130924
  *修改描述:考虑循环存储时id编号并不是与flash一样递增的
*********************************************************************************/
static u16 Cam_Flash_InitPara( u8 printf_info )
{
	u32					TempAddress;
	TypeDF_PackageHead	TempPackageHead;

	DF_PicParam.Number			= 0;
	DF_PicParam.First.Address	= DF_CAM_START; /*没有有效地址*/
	DF_PicParam.First.Data_ID	= 0;
	DF_PicParam.First.Len		= 0;

	DF_PicParam.Last.Address	= DF_CAM_START;
	DF_PicParam.Last.Data_ID	= 0;
	DF_PicParam.Last.Len		= 0;
	if( printf_info )
	{
		rt_kprintf( "\nNO    ADDR        ID     LEN   NO_DEL\n" );
	}
	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
	for( TempAddress = DF_CAM_START; TempAddress < DF_CAM_END; )
	{
		sst25_read( TempAddress, (u8*)&TempPackageHead, sizeof( TempPackageHead ) );
		if( TempPackageHead.Head == CAM_HEAD )                      /*有效数据*/
		{
			DF_PicParam.Number++;
			if( DF_PicParam.First.Data_ID >= TempPackageHead.id )   /*第一个应该是id最小的*/
			{
				DF_PicParam.First.Address	= TempAddress;
				DF_PicParam.First.Len		= TempPackageHead.Len;
				DF_PicParam.First.Data_ID	= TempPackageHead.id;
			}

			if( DF_PicParam.Last.Data_ID <= TempPackageHead.id )    /*最后一个应该是id最大的*/
			{
				DF_PicParam.Last.Address	= TempAddress;
				DF_PicParam.Last.Len		= TempPackageHead.Len;
				DF_PicParam.Last.Data_ID	= TempPackageHead.id;
			}

			if( printf_info )
			{
				rt_kprintf( "%05d 0x%08x  %05d  %04d  %d\n", DF_PicParam.Number, TempAddress, TempPackageHead.id, TempPackageHead.Len, TempPackageHead.State & ( BIT( 0 ) ) );
			}
			TempAddress += ( TempPackageHead.Len + DF_CAM_REC_COUNT - 1 ) & DF_CAM_REC_MASK;
		}else
		{
			TempAddress += DF_CAM_REC_COUNT;
		}
	}
	rt_sem_release( &sem_dataflash );
	return DF_PicParam.Number;
}

/*********************************************************************************
  *函数名称:rt_err_t Cam_Flash_WrPic(u8 *pData,u16 len, TypeDF_PackageHead *pHead)
  *功能描述:向FLASH中写入图片数据，如果传递的数据的pHead->len为非0值表示为最后一包数据
  *输	入:	pData:写入的数据指针，指向数据buf；
   len:数据长度，注意，该长度必须小于4096
   pHead:数据包头信息，该包头信息需要包含时间Timer，该值每次都必须传递，并且同样的包该值不能变化，
    最后一包数据需将len设置为数据长度len的长度包括包头部分，包头部分为固定64字节，其它包
    len为0.
  *输	出:none
  *返 回 值:re_err_t
  *作	者:白养民
  *创建日期:2013-06-5
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
static rt_err_t Cam_Flash_WrPic( u8 *pData, u16 len, TypeDF_PackageHead *pHead )
{
	u32				temp_Len;
	uint32_t		addr;
	static u32		WriteAddress		= 0;
	static u32		WriteAddressStart	= 0;
	static MYTIME	LastTime			= 0xFFFFFFFF;

	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
	if( LastTime != pHead->Time )                                                                                           /*首次写入*/
	{
		LastTime			= pHead->Time;
		WriteAddressStart	= ( DF_PicParam.Last.Address + DF_PicParam.Last.Len + DF_CAM_REC_COUNT - 1 ) & DF_CAM_REC_MASK; /*4096对齐*/
		WriteAddressStart	= Cam_Flash_AddrCheck( WriteAddressStart );
		WriteAddress		= WriteAddressStart + 64;
		sst25_erase_4k( WriteAddressStart );                                                                                /*删除数据*/
	}
	addr = Cam_Flash_AddrCheck( WriteAddress + len );
	if( ( WriteAddress & DF_SECTOR_MASK ) != ( addr & DF_SECTOR_MASK ) )                                                    ///跨越两个扇区的处理
	{
		temp_Len = 0x1000 - ( WriteAddress & 0xFFF );
		sst25_erase_4k( addr );
		sst25_write_through( WriteAddress, pData, temp_Len );
		sst25_write_through( addr & DF_SECTOR_MASK, pData + temp_Len, len - temp_Len );
	}else
	{
		sst25_write_through( WriteAddress, pData, len );
	}

	WriteAddress = addr;    /*下一个要写入的位置*/

	if( pHead->Len )        /*最后一包*/
	{
		DF_PicParam.Number++;
		DF_PicParam.Last.Data_ID++;
		DF_PicParam.Last.Address	= WriteAddressStart;
		DF_PicParam.Last.Len		= pHead->Len;

		pHead->Head			= CAM_HEAD;
		pHead->id			= DF_PicParam.Last.Data_ID;
		pHead->Media_Style	= 0;
		pHead->Media_Format = 0;
		sst25_write_through( WriteAddressStart, (u8*)pHead, sizeof( TypeDF_PackageHead ) );
	}

	rt_sem_release( &sem_dataflash );
	return RT_EOK;
}

/*********************************************************************************
  *函数名称:u32 Cam_Flash_FindPicID(u32 id,TypeDF_PackageHead *p_head)
  *功能描述:从FLASH中查找指定ID的图片，并将图片包头新新存储在p_head中
  *输	入:	id:多媒体ID号
   p_head:输出参数，输出为图片多媒体数据包头信息(类型为TypeDF_PackageHead)
  *输	出:p_head
  *返 回 值:u32 0xFFFFFFFF表示没有找到图片，其它表示找到了图片，返回值为图片在flash中的地址
  *作	者:白养民
  *创建日期:2013-06-5
  *---------------------------------------------------------------------------------
  *修 改 人:bai
  *修改日期:2013-06-23
  *修改描述:修改了拍照数据存储异常时的检索功能
*********************************************************************************/
u32 Cam_Flash_FindPicID( u32 id, TypeDF_PackageHead *p_head )
{
//	u32					i;
	u32					TempAddress;
//	u32					tempu32data;
	TypeDF_PackageHead	TempPackageHead;

	if( DF_PicParam.Number == 0 )
	{
		return 0xFFFFFFFF;
	}
	if( ( id < DF_PicParam.First.Data_ID ) || ( id > DF_PicParam.Last.Data_ID ) )
	{
		return 0xFFFFFFFF;
	}

	for( TempAddress = DF_CAM_START; TempAddress < DF_CAM_END; )
	{
		sst25_read( TempAddress, (u8*)&TempPackageHead, sizeof( TempPackageHead ) );
		if( TempPackageHead.Head == CAM_HEAD )          /*有效数据*/
		{
			if( id == TempPackageHead.id )
			{
				memcpy( p_head, (u8*)&TempPackageHead, sizeof( TempPackageHead ) );
				return TempAddress;
			}
			TempAddress += ( TempPackageHead.Len + DF_CAM_REC_COUNT - 1 ) & DF_CAM_REC_MASK;
		}else
		{
			TempAddress += DF_CAM_REC_COUNT;
		}
	}
	return 0xFFFFFFFF;
}

/*********************************************************************************
  *函数名称:rt_err_t Cam_Flash_RdPic(void *pData,u16 *len, u32 id,u8 offset )
  *功能描述:从FLASH中读取图片数据
  *输	入:	pData:写入的数据指针，指向数据buf；
   len:返回的数据长度指针注意，该长度最大为512
   id:多媒体ID号
   offset:多媒体数据偏移量，从0开始，0表示读取多媒体图片包头信息，包头信息长度固定为64字节，采用
    结构体TypeDF_PackageHead格式存储
  *输	出:none
  *返 回 值:re_err_t
  *作	者:白养民
  *创建日期:2013-06-5
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
rt_err_t Cam_Flash_RdPic( void *pData, u16 *len, u32 id, u8 offset )
{
	u32					TempAddress;
	u32					temp_Len;
	TypeDF_PackageHead	TempPackageHead;
	u8					ret;

	*len = 0;
	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );

	TempAddress = Cam_Flash_FindPicID( id, &TempPackageHead );
	if( TempAddress == 0xFFFFFFFF )
	{
		ret = RT_ERROR;  goto FUNC_RET;
	}
	if( TempPackageHead.id == 0xFFFFFFFF )
	{
		ret = RT_ERROR;  goto FUNC_RET;
	}
	if( offset > ( TempPackageHead.Len - 1 ) / 512 )
	{
		ret = RT_ENOMEM; goto FUNC_RET;
	}
	if( offset == ( TempPackageHead.Len - 1 ) / 512 )
	{
		temp_Len = TempPackageHead.Len - ( offset * 512 );
	}else
	{
		temp_Len = 512;
	}
	sst25_read( TempAddress + offset * 512, (u8*)pData, temp_Len );
	*len = temp_Len;

	ret = RT_EOK;

FUNC_RET:
	rt_sem_release( &sem_dataflash );
	return ret;
}

/*********************************************************************************
  *函数名称:rt_err_t Cam_Flash_DelPic(u32 id)
  *功能描述:从FLASH中删除图片，实际上并没有删除，而是将删除标记清0，这样下次就再也不查询该图片
  *输	入:	id:多媒体ID号
  *输	出:none
  *返 回 值:re_err_t
  *作	者:白养民
  *创建日期:2013-06-13
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
rt_err_t Cam_Flash_DelPic( u32 id )
{
	u32					TempAddress;
	TypeDF_PackageHead	TempPackageHead;
	u8					ret;

	if( DF_PicParam.Number == 0 ) /*没有图片*/
	{
		return RT_EOK;
	}

	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
	TempAddress = Cam_Flash_FindPicID( id, &TempPackageHead );
	if( TempAddress == 0xFFFFFFFF )
	{
		ret = RT_ERROR;  goto FUNC_RET;
	}
	TempPackageHead.State &= ~( BIT( 0 ) );
	sst25_write_through( TempAddress, (u8*)&TempPackageHead, sizeof( TypeDF_PackageHead ) );
	ret = RT_EOK;

FUNC_RET:
	rt_sem_release( &sem_dataflash );
	return ret;
}

/*********************************************************************************
  *函数名称:rt_err_t Cam_Flash_TransOkSet(u32 id)
  *功能描述:将该ID的图片标记为发送成功
  *输	入:	id:多媒体ID号
  *输	出:none
  *返 回 值:re_err_t
  *作	者:白养民
  *创建日期:2013-06-13
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
rt_err_t Cam_Flash_TransOkSet( u32 id )
{
	u32					TempAddress;
	TypeDF_PackageHead	TempPackageHead;
	u8					ret;

	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
	TempAddress = Cam_Flash_FindPicID( id, &TempPackageHead );
	if( TempAddress == 0xFFFFFFFF )
	{
		ret = RT_ERROR;  goto FUNC_RET;
	}
	TempPackageHead.State &= ~( BIT( 1 ) );
	sst25_write_through( TempAddress, (u8*)&TempPackageHead, sizeof( TypeDF_PackageHead ) );
	ret = RT_EOK;

FUNC_RET:
	rt_sem_release( &sem_dataflash );
	return ret;
}

/*********************************************************************************
  *函数名称:u16 Cam_Flash_SearchPic(T_TIMES *start_time,T_TIMES *end_time,TypeDF_PackageHead *para,u8 *pdest)
  *功能描述:从FLASH中查找指定时间段的图片索引
  *输	入:	start_time	开始时间，
   end_time		结束时间，
   para			查找图片的属性
   pdest			存储查找到得图片的位置，每个多媒体图片占用4个字节
  *输	出: u16 类型，表示查找到得图片数量
  *返 回 值:re_err_t
  *作	者:白养民
  *创建日期:2013-06-5
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
u16 Cam_Flash_SearchPic( MYTIME start_time, MYTIME end_time, TypeDF_PackageHead *para, u8 *pdest )
{
	u16					i;
	u32					TempAddress;
//	u32					temp_u32;
	TypeDF_PackageHead	TempPackageHead;
	u16					ret_num = 0;

	if( DF_PicParam.Number == 0 )
	{
		return 0;
	}
	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
	TempAddress = DF_PicParam.First.Address;
	for( i = 0; i < DF_PicParam.Number; i++ )
	{
		TempAddress = Cam_Flash_AddrCheck( TempAddress );
		sst25_read( TempAddress, (u8*)&TempPackageHead, sizeof( TempPackageHead ) );
		if( TempPackageHead.Head == CAM_HEAD )
		{
			///查看该图片是否被删除
			if( TempPackageHead.State & BIT( 0 ) )
			{
				///比较多媒体类型，多媒体通道，多媒体触发源
				if( ( TempPackageHead.Media_Style == para->Media_Style ) && ( ( TempPackageHead.Channel_ID == para->Channel_ID ) || ( para->Channel_ID == 0 ) ) && ( ( TempPackageHead.TiggerStyle == para->TiggerStyle ) || ( para->TiggerStyle == 0xFF ) ) )
				{
					///比较时间是否在范围
					if( ( TempPackageHead.Time > start_time ) && ( TempPackageHead.Time <= end_time ) )
					{
						///找到了数据
						data_to_buf( pdest, TempAddress, 4 );
						//memcpy(pdest,&TempAddress,4);
						pdest += 4;
						ret_num++;
					}
				}
			}
			TempAddress += ( TempPackageHead.Len + DF_CAM_REC_COUNT - 1 ) / DF_CAM_REC_COUNT * DF_CAM_REC_COUNT;
		}else
		{
			TempAddress += DF_CAM_REC_COUNT; ///修改存储异常，在此增加该代码，之前代码直接返回OXffff
		}
	}

	rt_sem_release( &sem_dataflash );
	return ret_num;
}

/*********************************************************************************
  *函数名称:void Cam_Device_init( void )
  *功能描述:初始化CAM模块相关接口
  *输	入:none
  *输	出:none
  *返 回 值:none
  *作	者:白养民
  *创建日期:2013-06-3
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
void Cam_Device_init( void )
{
	///开电
	Power_485CH1_ON;

	///初始化消息队列
	rt_mq_init( &mq_Cam, "mq_cam", &msgpool_cam[0], sizeof( Style_Cam_Requset_Para ), sizeof( msgpool_cam ), RT_IPC_FLAG_FIFO );

	///初始化flash参数
	Cam_Flash_InitPara( 1 );

	///初始化照相状态参数
	memset( (u8*)&Current_Cam_Para, 0, sizeof( Current_Cam_Para ) );
	Current_Cam_Para.State = CAM_NONE;
}

/*********************************************************************************
  *函数名称:static void Cam_Start_Cmd(u16 Cam_ID)
  *功能描述:向camera模块发送开始拍照指令
  *输	入:Cam_ID	需要拍照的camera设备ID
  *输	出:none
  *返 回 值:none
  *作	者:白养民
  *创建日期:2013-06-3
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
static void Cam_Start_Cmd( u16 Cam_ID )
{
	u8 Take_photo[10] = { 0x40, 0x40, 0x61, 0x81, 0x02, 0X00, 0X00, 0X02, 0X0D, 0X0A }; //----  报警拍照命令
	Take_photo[4]	= (u8)Cam_ID;
	Take_photo[5]	= (u8)( Cam_ID >> 8 );
	RS485_write( Take_photo, 10 );
	//uart2_rxbuf_rd = uart2_rxbuf_wr;
	rt_kprintf( "\n发送拍照命令" );
}

/*********************************************************************************
  *函数名称:static void Cam_Read_Cmd(u16 Cam_ID)
  *功能描述:向camera模块发送读取照片数据指令
  *输	入:Cam_ID	需要拍照的camera设备ID
  *输	出:none
  *返 回 值:none
  *作	者:白养民
  *创建日期:2013-06-3
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
static void Cam_Read_Cmd( u16 Cam_ID )
{
	uint8_t Fectch_photo[10] = { 0x40, 0x40, 0x62, 0x81, 0x02, 0X00, 0XFF, 0XFF, 0X0D, 0X0A }; //----- 报警取图命令
	Fectch_photo[4] = (u8)Cam_ID;
	Fectch_photo[5] = (u8)( Cam_ID >> 8 );
	RS485_write( Fectch_photo, 10 );
//	uart2_rxbuf_rd = uart2_rxbuf_wr;
	rt_kprintf( "\n读取照片命令" );
}

/*********************************************************************************
  *函数名称:TypeDF_PICPara Cam_get_state(void)
  *功能描述:获取系统拍照相关参数
  *输	入:none
  *输	出:none
  *返 回 值:TypeDF_PICPara
  *作	者:白养民
  *创建日期:2013-06-3
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
TypeDF_PICPara Cam_get_state( void )
{
	return DF_PicParam;
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
void cam_wr_flash( u32 addr, char *psrc )
{
	char pstr[128];
	memset( pstr, 0, sizeof( pstr ) );
	memcpy( pstr, psrc, strlen( psrc ) );
	sst25_write_back( addr, (u8*)pstr, strlen( pstr ) + 1 );
}

FINSH_FUNCTION_EXPORT( cam_wr_flash, cam_wr_flash );


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
void cam_wr_flash_ex( u32 addr, char *psrc )
{
	char pstr[128];
	memset( pstr, 0, sizeof( pstr ) );
	memcpy( pstr, psrc, strlen( psrc ) );
	sst25_erase_4k( addr );
	sst25_write_through( addr, (u8*)pstr, strlen( pstr ) + 1 );
}

FINSH_FUNCTION_EXPORT( cam_wr_flash_ex, cam_wr_flash_ex );


/*********************************************************************************
  *函数名称:rt_err_t take_pic_request( Style_Cam_Requset_Para *para)
  *功能描述:请求拍照指令
  *输	入:para拍照参数
  *输	出:none
  *返 回 值:rt_err_t
  *作	者:白养民
  *创建日期:2013-06-3
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
rt_err_t take_pic_request( Style_Cam_Requset_Para *para )
{
	return rt_mq_send( &mq_Cam, (void*)para, sizeof( Style_Cam_Requset_Para ) );
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
void readpic( u16 id )
{
	Style_Cam_Requset_Para tempPara;
	memset( &tempPara, 0, sizeof( tempPara ) );
	tempPara.Channel_ID		= id;
	tempPara.PhoteSpace		= 0;
	tempPara.PhotoTotal		= 1;
	tempPara.SavePhoto		= 1;
	tempPara.TiggerStyle	= Cam_TRIGGER_PLANTFORM;
}

FINSH_FUNCTION_EXPORT( readpic, readpic );


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
void getpicpara( void )
{
	rt_kprintf( "\nFirst id=%d(addr=0x%x,len=%d)\nLast id=%d(addr=0x%x,len=%d)",
	            DF_PicParam.First.Data_ID,
	            DF_PicParam.First.Address,
	            DF_PicParam.First.Len,
	            DF_PicParam.Last.Data_ID,
	            DF_PicParam.Last.Address,
	            DF_PicParam.Last.Len );
}

FINSH_FUNCTION_EXPORT( getpicpara, getpicpara );


/*********************************************************************************
  *函数名称:bool Camera_RX_Data(u16 *RxLen)
  *功能描述:拍照接收数据处理
  *输	入:RxLen:表示接收到得数据长度指针
  *输	出:RxLen:表示接收到得数据长度指针
  *返 回 值:0表示接收进行中,1表示接收成功
  *作	者:白养民
  *创建日期:2013-06-3
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
static u8 Camera_RX_Data( u16 *RxLen )
{
	u8			ch;
	static u16	page_size		= 0;
	static u16	cam_rx_head_wr	= 0;

	/*串口是否收到数据*/
	while( RS485_read_char( &ch ) )
	{
		switch( Current_Cam_Para.Rx_State )
		{
			case RX_DATA: /*保存信息格式: 位置(2B) 大小(2B) FLAG_FF 数据 0D 0A*/
				photo_rx[photo_rx_wr++] = ch;
				photo_rx_wr				%= UART2_RX_SIZE;
				if( photo_rx_wr == page_size )
				{
					Current_Cam_Para.Rx_State = RX_FCS;
				}
				break;
			case RX_IDLE:
				if( ch == 0x40 )
				{
					Current_Cam_Para.Rx_State = RX_SYNC1;
				}
				break;
			case RX_SYNC1:
				if( ch == 0x40 )
				{
					Current_Cam_Para.Rx_State = RX_SYNC2;
				} else
				{
					Current_Cam_Para.Rx_State = RX_IDLE;
				}
				break;
			case RX_SYNC2:
				if( ch == 0x63 )
				{
					cam_rx_head_wr				= 0;
					Current_Cam_Para.Rx_State	= RX_HEAD;
				}else
				{
					Current_Cam_Para.Rx_State = RX_IDLE;
				}
				break;
			case RX_HEAD:
				photo_rx[cam_rx_head_wr++] = ch;
				if( cam_rx_head_wr == 5 )
				{
					photo_rx_wr					= 0;
					page_size					= ( photo_rx[3] << 8 ) | photo_rx[2];
					Current_Cam_Para.Rx_State	= RX_DATA;
				}
				break;
			case RX_FCS:
				Current_Cam_Para.Rx_State = RX_0D;
				break;
			case RX_0D:
				if( ch == 0x0d )
				{
					Current_Cam_Para.Rx_State = RX_0A;
				} else
				{
					Current_Cam_Para.Rx_State = RX_IDLE;
				}
				break;
			case RX_0A:
				Current_Cam_Para.Rx_State = RX_IDLE;
				if( ch == 0x0a )
				{
					return 1;
				}
				break;
		}
	}
	return 0;
}

#if 1


/*********************************************************************************
  *函数名称:void Cam_response_ok( struct _Style_Cam_Requset_Para *para,uint32_t pic_id )
  *功能描述:平台下发拍照命令处理函数的回调函数_单张照片拍照OK
  *输	入:	para	:拍照处理结构体
   pic_id	:图片ID
  *输	出:	none
  *返 回 值:rt_err_t
  *作	者:白养民
  *创建日期:2013-06-17
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
void Cam_response_ok( struct _Style_Cam_Requset_Para *para, uint32_t pic_id )
{
	if( para->SendPhoto )
	{
		Cam_jt808_0x0800( pic_id, !para->SavePhoto );
	}
}

/*********************************************************************************
  *函数名称:void Cam_response_end( struct _Style_Cam_Requset_Para *para )
  *功能描述:平台下发拍照命令处理函数的回调函数
  *输	入:	para	:拍照处理结构体
  *输	出:	none
  *返 回 值:rt_err_t
  *作	者:白养民
  *创建日期:2013-06-17
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
void Cam_response_end( struct _Style_Cam_Requset_Para *para )
{
	return;
}

#endif


/*********************************************************************************
  *函数名称:void Camera_Process(void)
  *功能描述:进行照相相关处理(包括有:拍照，存储照片，发送拍照结束指令给808)
  *输	入:none
  *输	出:none
  *返 回 值:none
  *作	者:白养民
  *创建日期:2013-06-3
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
u8 Camera_Process( void )
{
	u16							RxLen;
	static u16					cam_photo_size;
	static u8					cam_page_num	= 0;
	u8							cam_last_page	= 0;
	static uint32_t				tick;
	static TypeDF_PackageHead	pack_head;

	switch( Current_Cam_Para.State )
	{
		case CAM_NONE:
			if( RT_EOK == rt_mq_recv( &mq_Cam, (void*)&Current_Cam_Para.Para, sizeof( Style_Cam_Requset_Para ), RT_WAITING_NO ) )
			{
				Current_Cam_Para.State				= CAM_IDLE;
				Current_Cam_Para.Para.start_tick	= rt_tick_get( );
			}else
			{
				return 0;
			}
		case CAM_IDLE:
			if( ( rt_tick_get( ) - Current_Cam_Para.Para.start_tick ) >= ( Current_Cam_Para.Para.PhoteSpace * (u32)Current_Cam_Para.Para.PhotoNum ) )
			{
				Current_Cam_Para.Retry	= 0;
				Current_Cam_Para.State	= CAM_START;
			}
			break;
		case CAM_START:
			if( Current_Cam_Para.Retry >= 3 )
			{
				Current_Cam_Para.State = CAM_FALSE;
				break;
			}
			Current_Cam_Para.Retry++;
			memset( &pack_head, 0, sizeof( pack_head ) );
			cam_page_num	= 0;
			cam_photo_size	= 0;
			tick			= rt_tick_get( );
			Cam_Start_Cmd( Current_Cam_Para.Para.Channel_ID );
			Current_Cam_Para.State		= CAM_RX_PHOTO;
			Current_Cam_Para.Rx_State	= RX_IDLE;
			break;
		case CAM_GET_PHOTO:
			tick = rt_tick_get( );
			Cam_Read_Cmd( Current_Cam_Para.Para.Channel_ID );
			Current_Cam_Para.State		= CAM_RX_PHOTO;
			Current_Cam_Para.Rx_State	= RX_IDLE;
			break;
		case CAM_RX_PHOTO:
			if( 1 == Camera_RX_Data( &RxLen ) )
			{
				rt_kprintf( "\n接收到拍照数据" );
				tick = rt_tick_get( );
				///收到数据,存储,判断是否图片结束
				if( photo_rx_wr > 512 ) ///数据大于512,非法
				{
					rt_kprintf( "\nCAM%d invalided\n", Current_Cam_Para.Para.Channel_ID );
					Current_Cam_Para.State = CAM_START;
					break;
				}
				if( photo_rx_wr == 512 )
				{
					if( ( photo_rx[510] == 0xff ) && ( photo_rx[511] == 0xD9 ) )
					{
						cam_last_page = 1;
					}
				}else
				{
					cam_last_page = 1;
				}

				cam_page_num++;
				cam_photo_size += photo_rx_wr;

				if( cam_page_num == 1 ) ///第一包数据，需要填写参数，然后存储才正确
				{
					pack_head.Channel_ID	= Current_Cam_Para.Para.Channel_ID;
					pack_head.TiggerStyle	= Current_Cam_Para.Para.TiggerStyle;
					pack_head.Media_Format	= 0;
					pack_head.Media_Style	= 0;
					pack_head.Time			= mytime_now;
					memcpy( &pack_head.position, &gps_baseinfo, 28 );
					pack_head.State = 0xFF;
					if( Current_Cam_Para.Para.SavePhoto == 0 )
					{
						pack_head.State &= ~( BIT( 2 ) );
					}
				}
				///最后一包数据，需要将长度写入。
				if( cam_last_page )
				{
					pack_head.Len			= cam_photo_size + 64;
					Current_Cam_Para.State	= CAM_OK;
				}else
				{
					Current_Cam_Para.State = CAM_GET_PHOTO;
				}
				///保存数据
				Cam_Flash_WrPic( photo_rx, photo_rx_wr, &pack_head );
				photo_rx_wr = 0;
			}else if( rt_tick_get( ) - tick > RT_TICK_PER_SECOND * 5 ) ///判读是否超时，启动拍照需要时间稍长
			{
				Current_Cam_Para.State = CAM_START;
			}
			break;
		case CAM_OK:
			++Current_Cam_Para.Para.PhotoNum;
			rt_kprintf( "\n拍照成功!" );
			getpicpara( );

			if( Current_Cam_Para.Para.cb_response_cam_ok != RT_NULL ) ///调用单张照片拍照成功回调函数
			{
				Current_Cam_Para.Para.cb_response_cam_ok( &Current_Cam_Para.Para, pack_head.id );
			}else ///默认的回调函数
			{
				Cam_response_ok( &Current_Cam_Para.Para, pack_head.id );
			}

			if( Current_Cam_Para.Para.PhotoNum >= Current_Cam_Para.Para.PhotoTotal )
			{
				Current_Cam_Para.State = CAM_END;   ///拍照任务完成
			}else
			{
				Current_Cam_Para.State = CAM_IDLE;  ///单张照片拍照完成
			}
			break;
		case CAM_FALSE:
			rt_kprintf( "\n拍照失败!" );
			Current_Cam_Para.State = CAM_END;
		case CAM_END:
			rt_kprintf( "\n拍照结束!" );
			Current_Cam_Para.State = CAM_NONE;
			if( Current_Cam_Para.Para.cb_response_cam_end != RT_NULL ) ///调用单张照片拍照成功回调函数
			{
				Current_Cam_Para.Para.cb_response_cam_end( &Current_Cam_Para.Para );
			}else ///默认的回调函数
			{
				Cam_response_end( &Current_Cam_Para.Para );
			}
			break;
		default:
			Current_Cam_Para.State = CAM_NONE;
	}
	return 1;
}

/*********************************************************************************
  *函数名称:void Cam_takepic_ex(u16 id,u16 num,u16 space,u8 send,Style_Cam_Requset_Para trige)
  *功能描述:	拍照请求函数，
  *输	入:	id		:照相机ID范围为1-15
   num		:拍照数量
   space	:拍照间隔，单位为秒
   save	:是否保存图片到FLASH中
   send	:拍完后是否上传，1表示上传
   trige	:拍照触发源类型为	Style_Cam_Requset_Para
  *输	出:	none
  *返 回 值:none
  *作	者:白养民
  *创建日期:2013-06-21
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
void Cam_takepic_ex( u16 id, u16 num, u16 space, u8 save, u8 send, CAM_TRIGGER trige )
{
	Style_Cam_Requset_Para tempPara;
	if( trige > Cam_TRIGGER_OTHER )
	{
		trige = Cam_TRIGGER_OTHER;
	}
	memset( &tempPara, 0, sizeof( tempPara ) );
	tempPara.Channel_ID		= id;
	tempPara.PhoteSpace		= space * RT_TICK_PER_SECOND;
	tempPara.PhotoTotal		= num;
	tempPara.SavePhoto		= save;
	tempPara.SendPhoto		= send;
	tempPara.TiggerStyle	= trige;
	take_pic_request( &tempPara );
	rt_kprintf( "\n请求拍照ID=%d", id );
}

FINSH_FUNCTION_EXPORT( Cam_takepic_ex, para_id_num_space_save_send_trige );


/*********************************************************************************
  *函数名称:void Cam_takepic(u16 id,u8 send,Style_Cam_Requset_Para trige)
  *功能描述:	拍照请求函数，
  *输	入:	id		:照相机ID范围为1-15
   save	:是否保存图片到FLASH中
   send	:拍完后是否上传，1表示上传
   trige	:拍照触发源类型为	Style_Cam_Requset_Para
  *输	出:	none
  *返 回 值:none
  *作	者:白养民
  *创建日期:2013-06-21
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
void Cam_takepic( u16 id, u8 save, u8 send, CAM_TRIGGER trige )
{
	Cam_takepic_ex( id, 1, 0, save, send, trige );
}

FINSH_FUNCTION_EXPORT( Cam_takepic, para_id_save_send_trige );


/*********************************************************************************
  *函数名称:void Cam_get_All_pic(void)
  *功能描述:	获取图片数据，打印到调试串口
  *输	入:	none
  *输	出:	none
  *返 回 值:none
  *作	者:白养民
  *创建日期:2013-06-21
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
void Cam_get_All_pic( void )
{
	Cam_Flash_InitPara( 1 );
}

FINSH_FUNCTION_EXPORT( Cam_get_All_pic, Cam_get_All_pic );

/************************************** The End Of File **************************************/
