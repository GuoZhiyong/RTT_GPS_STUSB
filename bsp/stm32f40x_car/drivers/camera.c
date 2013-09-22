/************************************************************
 * Copyright (C), 2008-2012,
 * FileName:		//  camera.c
 * Author:			//  baiyangmin
 * Date:			//  2013-07-08
 * Description:		//  ���չ��ܵ�ʵ�֣�������:���գ��洢���Լ���ؽӿں��������ļ�ʵ�����ļ�jt808_camera.c�����ղ�����������
 * Version:			//  V0.01
 * Function List:	//  ��Ҫ�������书��
 *     1. -------
 * History:			//  ��ʷ�޸ļ�¼
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

#if 1

typedef enum
{
	CAM_NONE = 0, /*����*/
	CAM_IDLE,
	CAM_START,
	CAM_GET_PHOTO,
	CAM_RX_PHOTO,
	CAM_OK,
	CAM_FALSE,
	CAM_END
}CAM_STATE;

/*����cam��Ϣ��ʽ*/
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
	CAM_STATE				State;      ///�������״̬��
	CAM_RX_STATE			Rx_State;   ///����״̬
	u8						Retry;      ///�ظ��������
	Style_Cam_Requset_Para	Para;       ///������ǰ���������Ϣ
} Style_Cam_Para;

#define DF_VOC_START		0x1D0000    ///¼�����ݴ洢��ʼλ��
#define DF_VOC_END			0X298000    ///¼�����ݴ洢����λ��
#define DF_VOC_SECTOR_COUNT 0x400       ///¼�����ݴ洢��С


#define DF_CAM_START		0x108000    ///ͼƬ���ݴ洢��ʼλ��
#define DF_CAM_END			0X1D0000    ///ͼƬ���ݴ洢����λ��
#define DF_CAM_SECTOR_COUNT 0x400       ///ͼƬ���ݴ�С


//static CAM_STATE				cam_state;      ///�������״̬��



extern rt_device_t _console_device;
extern u16 Hex_To_Ascii( const u8* pSrc, u8* pDst, u16 nSrcLength );


const char				CAM_HEAD[] = { "PIC_01" };

static Style_Cam_Para	Current_Cam_Para;
static TypeDF_PICPara	DF_PicParameter; ///FLASH�洢��ͼƬ��Ϣ

/* ��Ϣ���п��ƿ�*/
struct rt_messagequeue	mq_Cam;
/* ��Ϣ�������õ��ķ�����Ϣ���ڴ��*/
static char				msgpool_cam[256];

#define PHOTO_RX_SIZE 1024

static uint8_t		photo_rx[PHOTO_RX_SIZE];
static uint16_t	photo_rx_wr = 0;


extern MsgList			* list_jt808_tx;

/*********************************************************************************
  *��������:u32 Cam_Flash_AddrCheck(u32 pro_Address)
  *��������:��鵱ǰ��λ���Ƿ�Ϸ������Ϸ����޸�Ϊ�Ϸ����ݲ�����
  *��	��:none
  *��	��:none
  *�� �� ֵ:��ȷ�ĵ�ֵַ
  *��	��:������
  *��������:2013-06-5
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
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

/*********************************************************************************
  *��������:u8 Cam_Flash_FirstPicProc(u32 temp_wr_addr)
  *��������:�޸ĵ�1��ͼƬ����λ�ò�������ΪҪ��flash����erase����erase���ǵ�1��ͼƬʱ����Ҫ��DF_PicParameter��
   ��һ��ͼƬ����Ϣ����
  *��	��:��ǰҪ��д��λ��
  *��	��:none
  *�� �� ֵ:1��ʾ�������أ�0��ʾ�д�����
  *��	��:������
  *��������:2013-06-5
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
static u8 Cam_Flash_FirstPicProc( u32 temp_wr_addr )
{
	u8					i;
	u8					ret;
	u32					TempAddress;
	TypeDF_PackageHead	TempPackageHead;

	if( ( Cam_Flash_AddrCheck( temp_wr_addr ) == ( DF_PicParameter.FirstPic.Address & 0xFFFFF000 ) ) && ( DF_PicParameter.Number ) )
	{
		TempAddress = ( DF_PicParameter.FirstPic.Address + DF_PicParameter.FirstPic.Len + DF_CAM_SECTOR_COUNT - 1 ) / DF_CAM_SECTOR_COUNT * DF_CAM_SECTOR_COUNT;
		for( i = 0; i < 8; i++ )
		{
			sst25_read( Cam_Flash_AddrCheck( TempAddress ), (u8*)&TempPackageHead, sizeof( TempPackageHead ) );
			if( strncmp( TempPackageHead.Head, CAM_HEAD, strlen( CAM_HEAD ) ) == 0 )
			{
				if( ( TempAddress & 0xFFFFF000 ) != ( DF_PicParameter.FirstPic.Address & 0xFFFFF000 ) )
				{
					DF_PicParameter.FirstPic.Address	= Cam_Flash_AddrCheck( TempAddress );
					DF_PicParameter.FirstPic.Len		= TempPackageHead.Len;
					DF_PicParameter.FirstPic.Data_ID	= TempPackageHead.Data_ID;
					DF_PicParameter.Number--;
					ret = 1;
					break;
				}
				if( TempPackageHead.Len == 0 )
				{
					ret = 0;
					break;
				}
				TempAddress += ( TempPackageHead.Len + DF_CAM_SECTOR_COUNT - 1 ) / DF_CAM_SECTOR_COUNT * DF_CAM_SECTOR_COUNT;
			}else
			{
				ret = 0;
				break;
			}
		}
		ret = 0;
	}else
	{
		ret = 1;
	}
	sst25_erase_4k( temp_wr_addr );
	return ret;
}

/*********************************************************************************
  *��������:u16 Cam_Flash_InitPara(u8 printf_info)
  *��������:��ʼ��Pic������������ȡFLASH�е�ͼƬ��Ϣ����ȡ��ͼƬ����������ʼλ�ã�����λ�õȣ�
   ��Щ��ȡ�������ݶ��洢�� DF_PicParameter �С�
  *��	��:printf_info	:	0����ʾ����ӡͼƬ��Ϣ��1��ʾ��ӡͼƬ��Ϣ
  *��	��:none
  *�� �� ֵ:��Ч��ͼƬ����
  *��	��:������
  *��������:2013-06-5
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
static u16 Cam_Flash_InitPara( u8 printf_info )
{
	u32					TempAddress;
	TypeDF_PackageHead	TempPackageHead;
	TypeDF_PackageInfo	TempPackageInfo;

	memset( &DF_PicParameter, 0, sizeof( DF_PicParameter ) );
	DF_PicParameter.FirstPic.Address	= DF_CAM_START;
	DF_PicParameter.LastPic.Address		= DF_CAM_START;
	if( printf_info )
	{
		rt_kprintf( "\n PIC_ADDRESS,  %PIC_ID,  %PIC_LEN,  NO_DEL\n" );
	}
	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
	for( TempAddress = DF_CAM_START; TempAddress < DF_CAM_END; )
	{
		sst25_read( TempAddress, (u8*)&TempPackageHead, sizeof( TempPackageHead ) );
		if( strncmp( TempPackageHead.Head, CAM_HEAD, strlen( CAM_HEAD ) ) == 0 )
		{
			DF_PicParameter.Number++;
			TempPackageInfo.Address = TempAddress;
			TempPackageInfo.Data_ID = TempPackageHead.Data_ID;
			TempPackageInfo.Len		= TempPackageHead.Len;
			if( DF_PicParameter.Number == 1 )
			{
				DF_PicParameter.FirstPic	= TempPackageInfo;
				DF_PicParameter.LastPic		= TempPackageInfo;
			}else
			{
				if( TempPackageInfo.Data_ID > DF_PicParameter.LastPic.Data_ID )
				{
					DF_PicParameter.LastPic = TempPackageInfo;
				}else if( TempPackageInfo.Data_ID < DF_PicParameter.FirstPic.Data_ID )
				{
					DF_PicParameter.FirstPic = TempPackageInfo;
				}
			}
			if( printf_info )
			{
				rt_kprintf( "  0x%08x,    %05d,      %04d,     %d\n", TempAddress, TempPackageInfo.Data_ID, TempPackageInfo.Len, TempPackageHead.State & ( BIT( 0 ) ) );
			}
			TempAddress += ( TempPackageInfo.Len + DF_CAM_SECTOR_COUNT - 1 ) / DF_CAM_SECTOR_COUNT * DF_CAM_SECTOR_COUNT;
		}else
		{
			TempAddress += DF_CAM_SECTOR_COUNT;
		}
	}
	rt_sem_release( &sem_dataflash );
	if( printf_info )
	{
		rt_kprintf( "  PIC_NUM =  %04d\n", DF_PicParameter.Number );
	}
	return DF_PicParameter.Number;
}

/*********************************************************************************
  *��������:rt_err_t Cam_Flash_WrPic(u8 *pData,u16 len, TypeDF_PackageHead *pHead)
  *��������:��FLASH��д��ͼƬ���ݣ�������ݵ����ݵ�pHead->lenΪ��0ֵ��ʾΪ���һ������
  *��	��:	pData:д�������ָ�룬ָ������buf��
   len:���ݳ��ȣ�ע�⣬�ó��ȱ���С��4096
   pHead:���ݰ�ͷ��Ϣ���ð�ͷ��Ϣ��Ҫ����ʱ��Timer����ֵÿ�ζ����봫�ݣ�����ͬ���İ���ֵ���ܱ仯��
    ���һ�������轫len����Ϊ���ݳ���len�ĳ��Ȱ�����ͷ���֣���ͷ����Ϊ�̶�64�ֽڣ�������
    lenΪ0.
  *��	��:none
  *�� �� ֵ:re_err_t
  *��	��:������
  *��������:2013-06-5
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
static rt_err_t Cam_Flash_WrPic( u8 *pData, u16 len, TypeDF_PackageHead *pHead )
{
	u16				i;
	u32				temp_wr_addr;
	u32				temp_Len;
	static u8		WriteFuncUserBack	= 0;
	static u32		WriteAddress		= 0;
	static u32		WriteAddressStart	= 0;
	static MYTIME	LastTime			= 0xFFFFFFFF;

	u8				strBuf[256];

	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
	//if( memcmp( &LastTime, &pHead->Time, sizeof( LastTime ) ) != 0 )
	{
		LastTime			= pHead->Time;
		WriteAddressStart	= ( DF_PicParameter.LastPic.Address + DF_PicParameter.LastPic.Len + DF_CAM_SECTOR_COUNT - 1 ) / DF_CAM_SECTOR_COUNT * DF_CAM_SECTOR_COUNT;
		WriteAddressStart	= Cam_Flash_AddrCheck( WriteAddressStart );
		WriteAddress		= WriteAddressStart + 64;
		if( ( WriteAddressStart & 0xFFF ) == 0 )
		{
			Cam_Flash_FirstPicProc( WriteAddressStart );
		}else
		{
			///�жϸ������Ƿ������쳣���쳣��Ҫʹ��falshд����sst25_write_back
			sst25_read( WriteAddressStart, strBuf, sizeof( strBuf ) );
			WriteFuncUserBack = 0;
			for( i = 0; i < sizeof( strBuf ); i++ )
			{
				if( strBuf[i] != 0xFF )
				{
					if( i < 64 ) ///�����ͷ����Ҳ������հ�ͷ��������Ϊ0xFF
					{
						memset( strBuf, 0xFF, sizeof( strBuf ) );
						sst25_write_back( WriteAddressStart, strBuf, sizeof( strBuf ) );
					}

					WriteFuncUserBack = 1;
					break;
				}
			}
		}
	}

	if( WriteFuncUserBack )
	{
		if( ( WriteAddress & 0xFFFFF000 ) != ( ( WriteAddress + len ) & 0xFFFFF000 ) ) ///��Խ���������Ĵ���
		{
			temp_wr_addr	= ( WriteAddress + len ) & 0xFFFFF000;
			temp_Len		= temp_wr_addr - WriteAddress;
			sst25_write_back( Cam_Flash_AddrCheck( WriteAddress ), pData, temp_Len );
			Cam_Flash_FirstPicProc( Cam_Flash_AddrCheck( temp_wr_addr ) );
			sst25_write_through( Cam_Flash_AddrCheck( temp_wr_addr ), pData + temp_Len, len - temp_Len );
			WriteFuncUserBack = 0;
		}else
		{
			sst25_write_back( Cam_Flash_AddrCheck( WriteAddress ), pData, len );
		}
	}else
	{
		if( ( WriteAddress & 0xFFFFF000 ) != ( ( WriteAddress + len ) & 0xFFFFF000 ) ) ///��Խ���������Ĵ���
		{
			temp_wr_addr	= ( WriteAddress + len ) & 0xFFFFF000;
			temp_Len		= temp_wr_addr - WriteAddress;
			sst25_write_through( Cam_Flash_AddrCheck( WriteAddress ), pData, temp_Len );
			Cam_Flash_FirstPicProc( Cam_Flash_AddrCheck( temp_wr_addr ) );
			sst25_write_through( Cam_Flash_AddrCheck( temp_wr_addr ), pData + temp_Len, len - temp_Len );
		}else
		{
			sst25_write_through( Cam_Flash_AddrCheck( WriteAddress ), pData, len );
		}
	}
	WriteAddress += len;
	if( pHead->Len )
	{
		WriteAddress = WriteAddressStart;
		DF_PicParameter.Number++;
		DF_PicParameter.LastPic.Data_ID++;
		DF_PicParameter.LastPic.Address = Cam_Flash_AddrCheck( WriteAddressStart );
		DF_PicParameter.LastPic.Len		= pHead->Len;
		if( DF_PicParameter.Number == 1 )
		{
			DF_PicParameter.FirstPic = DF_PicParameter.LastPic;
		}

		memcpy( pHead->Head, CAM_HEAD, strlen( CAM_HEAD ) );
		pHead->Data_ID		= DF_PicParameter.LastPic.Data_ID;
		pHead->Media_Style	= 0;
		pHead->Media_Format = 0;
		sst25_write_through( Cam_Flash_AddrCheck( WriteAddressStart ), (u8*)pHead, sizeof( TypeDF_PackageHead ) );
	}

	rt_sem_release( &sem_dataflash );
	return RT_EOK;
}

/*********************************************************************************
  *��������:u32 Cam_Flash_FindPicID(u32 id,TypeDF_PackageHead *p_head)
  *��������:��FLASH�в���ָ��ID��ͼƬ������ͼƬ��ͷ���´洢��p_head��
  *��	��:	id:��ý��ID��
   p_head:������������ΪͼƬ��ý�����ݰ�ͷ��Ϣ(����ΪTypeDF_PackageHead)
  *��	��:p_head
  *�� �� ֵ:u32 0xFFFFFFFF��ʾû���ҵ�ͼƬ��������ʾ�ҵ���ͼƬ������ֵΪͼƬ��flash�еĵ�ַ
  *��	��:������
  *��������:2013-06-5
  *---------------------------------------------------------------------------------
  *�� �� ��:bai
  *�޸�����:2013-06-23
  *�޸�����:�޸����������ݴ洢�쳣ʱ�ļ�������
*********************************************************************************/
u32 Cam_Flash_FindPicID( u32 id, TypeDF_PackageHead *p_head )
{
	u32							i;
	u32							TempAddress;
	u32							tempu32data;
	u32							flash_search_area = 0; ///��ʾɨ���˶������򣬵�ɨ���������ͼƬ������ʱ������

	static TypeDF_PackageInfo	lastPackInfo = { 0xFFFFFFFF, 0, 0xFFFFFFFF };
	static TypeDF_PackageHead	TempPackageHead;

	if( DF_PicParameter.Number == 0 )
	{
		return 0xFFFFFFFF;
	}
	if( ( id < DF_PicParameter.FirstPic.Data_ID ) || ( id > DF_PicParameter.LastPic.Data_ID ) )
	{
		return 0xFFFFFFFF;
	}
	if( id != lastPackInfo.Data_ID )
	{
		lastPackInfo.Data_ID	= 0xFFFFFFFF;
		TempAddress				= DF_PicParameter.FirstPic.Address;
		for( i = 0; i < DF_PicParameter.Number; )
		{
			TempAddress = Cam_Flash_AddrCheck( TempAddress );
			sst25_read( TempAddress, (u8*)&TempPackageHead, sizeof( TypeDF_PackageHead ) );
			//rt_kprintf("\n NUM=%d,ADDR=%d,ID=%d, Head=",i,TempAddress,TempPackageHead.Data_ID);
			//printer_data_hex((u8 *)&TempPackageHead,sizeof(TypeDF_PackageHead)-28);
			if( strncmp( TempPackageHead.Head, CAM_HEAD, strlen( CAM_HEAD ) ) == 0 )
			{
				///�鿴��ͼƬ�Ƿ�ɾ��
				if( TempPackageHead.State & BIT( 0 ) )
				{
					if( TempPackageHead.Data_ID == id )
					{
						lastPackInfo.Data_ID	= id;
						lastPackInfo.Address	= TempAddress;
						lastPackInfo.Len		= TempPackageHead.Len;
						memcpy( (void*)p_head, (void*)&TempPackageHead, sizeof( TypeDF_PackageHead ) );
						return lastPackInfo.Address;
					}
				}
				i++;
				//TempAddress+=(TempPackageHead.Len+DF_CAM_SECTOR_COUNT-1)/DF_CAM_SECTOR_COUNT*DF_CAM_SECTOR_COUNT;
				tempu32data			= ( TempPackageHead.Len + DF_CAM_SECTOR_COUNT - 1 ) / DF_CAM_SECTOR_COUNT;
				TempAddress			+= tempu32data * DF_CAM_SECTOR_COUNT;
				flash_search_area	+= tempu32data;
			}else
			{
				TempAddress += DF_CAM_SECTOR_COUNT; ///�޸Ĵ洢�쳣���ڴ����Ӹô��룬֮ǰ����ֱ�ӷ���OXffff
				//return 0xFFFFFFFF;
				flash_search_area++;
			}
			if( flash_search_area > ( DF_CAM_END - DF_CAM_START ) / DF_CAM_SECTOR_COUNT )
			{
				return 0xFFFFFFFF;
			}
		}
	}else
	{
		memcpy( (void*)p_head, (void*)&TempPackageHead, sizeof( TypeDF_PackageHead ) );
		return lastPackInfo.Address;
	}
	return 0xFFFFFFFF;
}

/*********************************************************************************
  *��������:rt_err_t Cam_Flash_RdPic(void *pData,u16 *len, u32 id,u8 offset )
  *��������:��FLASH�ж�ȡͼƬ����
  *��	��:	pData:д�������ָ�룬ָ������buf��
   len:���ص����ݳ���ָ��ע�⣬�ó������Ϊ512
   id:��ý��ID��
   offset:��ý������ƫ��������0��ʼ��0��ʾ��ȡ��ý��ͼƬ��ͷ��Ϣ����ͷ��Ϣ���ȹ̶�Ϊ64�ֽڣ�����
    �ṹ��TypeDF_PackageHead��ʽ�洢
  *��	��:none
  *�� �� ֵ:re_err_t
  *��	��:������
  *��������:2013-06-5
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
rt_err_t Cam_Flash_RdPic( void *pData, u16 *len, u32 id, u8 offset )
{
	u32					TempAddress;
	u32					temp_Len;
	TypeDF_PackageHead	TempPackageHead;
	u8					ret;

	*len = 0;
	//rt_kprintf("\n take_flash_sem");
	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );

	TempAddress = Cam_Flash_FindPicID( id, &TempPackageHead );
	if( TempAddress == 0xFFFFFFFF )
	{
		ret = RT_ERROR;  goto FUNC_RET;
	}
	if( TempPackageHead.Data_ID == 0xFFFFFFFF )
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
	//rt_kprintf("\n releas_flash_sem");
	rt_sem_release( &sem_dataflash );
	return ret;
}

/*********************************************************************************
  *��������:rt_err_t Cam_Flash_DelPic(u32 id)
  *��������:��FLASH��ɾ��ͼƬ��ʵ���ϲ�û��ɾ�������ǽ�ɾ�������0�������´ξ���Ҳ����ѯ��ͼƬ
  *��	��:	id:��ý��ID��
  *��	��:none
  *�� �� ֵ:re_err_t
  *��	��:������
  *��������:2013-06-13
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
rt_err_t Cam_Flash_DelPic( u32 id )
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
	TempPackageHead.State &= ~( BIT( 0 ) );
	sst25_write_through( TempAddress, (u8*)&TempPackageHead, sizeof( TypeDF_PackageHead ) );
	ret = RT_EOK;

FUNC_RET:
	rt_sem_release( &sem_dataflash );
	return ret;
}

/*********************************************************************************
  *��������:rt_err_t Cam_Flash_TransOkSet(u32 id)
  *��������:����ID��ͼƬ���Ϊ���ͳɹ�
  *��	��:	id:��ý��ID��
  *��	��:none
  *�� �� ֵ:re_err_t
  *��	��:������
  *��������:2013-06-13
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
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
  *��������:u16 Cam_Flash_SearchPic(T_TIMES *start_time,T_TIMES *end_time,TypeDF_PackageHead *para,u8 *pdest)
  *��������:��FLASH�в���ָ��ʱ��ε�ͼƬ����
  *��	��:	start_time	��ʼʱ�䣬
   end_time		����ʱ�䣬
   para			����ͼƬ������
   pdest			�洢���ҵ���ͼƬ��λ�ã�ÿ����ý��ͼƬռ��4���ֽ�
  *��	��: u16 ���ͣ���ʾ���ҵ���ͼƬ����
  *�� �� ֵ:re_err_t
  *��	��:������
  *��������:2013-06-5
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
u16 Cam_Flash_SearchPic( MYTIME start_time, MYTIME end_time, TypeDF_PackageHead *para, u8 *pdest )
{
	u16					i;
	u32					TempAddress;
	u32					temp_u32;
	TypeDF_PackageHead	TempPackageHead;
	u16					ret_num = 0;

	if( DF_PicParameter.Number == 0 )
	{
		return 0;
	}
	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
	TempAddress = DF_PicParameter.FirstPic.Address;
	for( i = 0; i < DF_PicParameter.Number; i++ )
	{
		TempAddress = Cam_Flash_AddrCheck( TempAddress );
		sst25_read( TempAddress, (u8*)&TempPackageHead, sizeof( TempPackageHead ) );
		if( strncmp( TempPackageHead.Head, CAM_HEAD, strlen( CAM_HEAD ) ) == 0 )
		{
			///�鿴��ͼƬ�Ƿ�ɾ��
			if( TempPackageHead.State & BIT( 0 ) )
			{
				///�Ƚ϶�ý�����ͣ���ý��ͨ������ý�崥��Դ
				if( ( TempPackageHead.Media_Style == para->Media_Style ) && ( ( TempPackageHead.Channel_ID == para->Channel_ID ) || ( para->Channel_ID == 0 ) ) && ( ( TempPackageHead.TiggerStyle == para->TiggerStyle ) || ( para->TiggerStyle == 0xFF ) ) )
				{
					///�Ƚ�ʱ���Ƿ��ڷ�Χ
					if( ( TempPackageHead.Time >  start_time ) && ( TempPackageHead.Time <=  end_time  ) )
					{
						///�ҵ�������
						data_to_buf( pdest, TempAddress, 4 );
						//memcpy(pdest,&TempAddress,4);
						pdest += 4;
						ret_num++;
					}
				}
			}
			TempAddress += ( TempPackageHead.Len + DF_CAM_SECTOR_COUNT - 1 ) / DF_CAM_SECTOR_COUNT * DF_CAM_SECTOR_COUNT;
		}else
		{
			TempAddress += DF_CAM_SECTOR_COUNT; ///�޸Ĵ洢�쳣���ڴ����Ӹô��룬֮ǰ����ֱ�ӷ���OXffff
		}
	}

	rt_sem_release( &sem_dataflash );
	return ret_num;
}



/*********************************************************************************
  *��������:void Cam_Device_init( void )
  *��������:��ʼ��CAMģ����ؽӿ�
  *��	��:none
  *��	��:none
  *�� �� ֵ:none
  *��	��:������
  *��������:2013-06-3
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
void Cam_Device_init( void )
{
	///����
	Power_485CH1_ON;

	///��ʼ����Ϣ����
	rt_mq_init( &mq_Cam, "mq_cam", &msgpool_cam[0], sizeof( Style_Cam_Requset_Para ), sizeof( msgpool_cam ), RT_IPC_FLAG_FIFO );

	///��ʼ��flash����
	Cam_Flash_InitPara( 0 );

	///��ʼ������״̬����
	memset( (u8*)&Current_Cam_Para, 0, sizeof( Current_Cam_Para ) );
	Current_Cam_Para.State = CAM_NONE;
}

/*********************************************************************************
  *��������:static void Cam_Start_Cmd(u16 Cam_ID)
  *��������:��cameraģ�鷢�Ϳ�ʼ����ָ��
  *��	��:Cam_ID	��Ҫ���յ�camera�豸ID
  *��	��:none
  *�� �� ֵ:none
  *��	��:������
  *��������:2013-06-3
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
static void Cam_Start_Cmd( u16 Cam_ID )
{
	u8 Take_photo[10] = { 0x40, 0x40, 0x61, 0x81, 0x02, 0X00, 0X00, 0X02, 0X0D, 0X0A }; //----  ������������
	Take_photo[4]	= (u8)Cam_ID;
	Take_photo[5]	= (u8)( Cam_ID >> 8 );
	RS485_write( Take_photo, 10 );
//	uart2_rxbuf_rd = uart2_rxbuf_wr;
	rt_kprintf( "\n������������" );
}

/*********************************************************************************
  *��������:static void Cam_Read_Cmd(u16 Cam_ID)
  *��������:��cameraģ�鷢�Ͷ�ȡ��Ƭ����ָ��
  *��	��:Cam_ID	��Ҫ���յ�camera�豸ID
  *��	��:none
  *�� �� ֵ:none
  *��	��:������
  *��������:2013-06-3
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
static void Cam_Read_Cmd( u16 Cam_ID )
{
	uint8_t Fectch_photo[10] = { 0x40, 0x40, 0x62, 0x81, 0x02, 0X00, 0XFF, 0XFF, 0X0D, 0X0A }; //----- ����ȡͼ����
	Fectch_photo[4] = (u8)Cam_ID;
	Fectch_photo[5] = (u8)( Cam_ID >> 8 );
	RS485_write( Fectch_photo, 10 );
//	uart2_rxbuf_rd = uart2_rxbuf_wr;
	rt_kprintf( "\n��ȡ��Ƭ����" );
}

/*********************************************************************************
  *��������:TypeDF_PICPara Cam_get_state(void)
  *��������:��ȡϵͳ������ز���
  *��	��:none
  *��	��:none
  *�� �� ֵ:TypeDF_PICPara
  *��	��:������
  *��������:2013-06-3
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
TypeDF_PICPara Cam_get_state( void )
{
	return DF_PicParameter;
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
  *��������:rt_err_t take_pic_request( Style_Cam_Requset_Para *para)
  *��������:��������ָ��
  *��	��:para���ղ���
  *��	��:none
  *�� �� ֵ:rt_err_t
  *��	��:������
  *��������:2013-06-3
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
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
	            DF_PicParameter.FirstPic.Data_ID,
	            DF_PicParameter.FirstPic.Address,
	            DF_PicParameter.FirstPic.Len,
	            DF_PicParameter.LastPic.Data_ID,
	            DF_PicParameter.LastPic.Address,
	            DF_PicParameter.LastPic.Len );
}

FINSH_FUNCTION_EXPORT( getpicpara, getpicpara );


/*********************************************************************************
  *��������:bool Camera_RX_Data(u16 *RxLen)
  *��������:���ս������ݴ���
  *��	��:RxLen:��ʾ���յ������ݳ���ָ��
  *��	��:RxLen:��ʾ���յ������ݳ���ָ��
  *�� �� ֵ:0��ʾ���ս�����,1��ʾ���ճɹ�
  *��	��:������
  *��������:2013-06-3
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
static u8 Camera_RX_Data( u16 *RxLen )
{
	u8			ch;
	static u16	page_size		= 0;
	static u16	cam_rx_head_wr	= 0;

	/*�����Ƿ��յ�����*/
	while( RS485_read_char( &ch ) )
	{
		switch( Current_Cam_Para.Rx_State )
		{
			case RX_DATA: /*������Ϣ��ʽ: λ��(2B) ��С(2B) FLAG_FF ���� 0D 0A*/
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
  *��������:void Cam_response_ok( struct _Style_Cam_Requset_Para *para,uint32_t pic_id )
  *��������:ƽ̨�·�������������Ļص�����_������Ƭ����OK
  *��	��:	para	:���մ���ṹ��
   pic_id	:ͼƬID
  *��	��:	none
  *�� �� ֵ:rt_err_t
  *��	��:������
  *��������:2013-06-17
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
void Cam_response_ok( struct _Style_Cam_Requset_Para *para, uint32_t pic_id )
{
	if( para->SendPhoto )
	{
		Cam_jt808_0x0800( pic_id, !para->SavePhoto );
	}
}

/*********************************************************************************
  *��������:void Cam_response_end( struct _Style_Cam_Requset_Para *para )
  *��������:ƽ̨�·�������������Ļص�����
  *��	��:	para	:���մ���ṹ��
  *��	��:	none
  *�� �� ֵ:rt_err_t
  *��	��:������
  *��������:2013-06-17
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
void Cam_response_end( struct _Style_Cam_Requset_Para *para )
{
	return;
}

#endif


/*********************************************************************************
  *��������:void Camera_Process(void)
  *��������:����������ش���(������:���գ��洢��Ƭ���������ս���ָ���808)
  *��	��:none
  *��	��:none
  *�� �� ֵ:none
  *��	��:������
  *��������:2013-06-3
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
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
				//rt_kprintf( "\n�յ�������Ϣ" );
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
				rt_kprintf( "\n���յ���������" );
				tick = rt_tick_get( );
				///�յ�����,�洢,�ж��Ƿ�ͼƬ����
				if( photo_rx_wr > 512 ) ///���ݴ���512,�Ƿ�
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
				///��һ�����ݣ���Ҫ��д������Ȼ��洢����ȷ
				if( cam_page_num == 1 )
				{
					pack_head.Channel_ID	= Current_Cam_Para.Para.Channel_ID;
					pack_head.TiggerStyle	= Current_Cam_Para.Para.TiggerStyle;
					pack_head.Media_Format	= 0;
					pack_head.Media_Style	= 0;
					//memcpy( &pack_head.Time, gps_datetime, 6 );
					pack_head.Time=mytime_now;
					memcpy( &pack_head.position, &gps_baseinfo, 28 );
					pack_head.State = 0xFF;
					if( Current_Cam_Para.Para.SavePhoto == 0 )
					{
						pack_head.State &= ~( BIT( 2 ) );
					}
				}
				///���һ�����ݣ���Ҫ������д�롣
				if( cam_last_page )
				{
					pack_head.Len			= cam_photo_size + 64;
					Current_Cam_Para.State	= CAM_OK;
				}else
				{
					Current_Cam_Para.State = CAM_GET_PHOTO;
				}
				///��������
				Cam_Flash_WrPic( photo_rx, photo_rx_wr, &pack_head );
				photo_rx_wr = 0;
			}else if( rt_tick_get( ) - tick > RT_TICK_PER_SECOND * 5 ) ///�ж��Ƿ�ʱ������������Ҫʱ���Գ�
			{
				Current_Cam_Para.State = CAM_START;
			}
			break;
		case CAM_OK:
			++Current_Cam_Para.Para.PhotoNum;
			rt_kprintf( "\n���ճɹ�!" );
			getpicpara( );

			if( Current_Cam_Para.Para.cb_response_cam_ok != RT_NULL ) ///���õ�����Ƭ���ճɹ��ص�����
			{
				Current_Cam_Para.Para.cb_response_cam_ok( &Current_Cam_Para.Para, pack_head.Data_ID );
			}else ///Ĭ�ϵĻص�����
			{
				Cam_response_ok( &Current_Cam_Para.Para, pack_head.Data_ID );
			}

			if( Current_Cam_Para.Para.PhotoNum >= Current_Cam_Para.Para.PhotoTotal )
			{
				Current_Cam_Para.State = CAM_END;   ///�����������
			}else
			{
				Current_Cam_Para.State = CAM_IDLE;  ///������Ƭ�������
			}
			break;
		case CAM_FALSE:
			rt_kprintf( "\n����ʧ��!" );
			Current_Cam_Para.State = CAM_END;
		case CAM_END:
			rt_kprintf( "\n���ս���!" );
			Current_Cam_Para.State = CAM_NONE;
			if( Current_Cam_Para.Para.cb_response_cam_end != RT_NULL ) ///���õ�����Ƭ���ճɹ��ص�����
			{
				Current_Cam_Para.Para.cb_response_cam_end( &Current_Cam_Para.Para );
			}else ///Ĭ�ϵĻص�����
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
  *��������:void Cam_takepic_ex(u16 id,u16 num,u16 space,u8 send,Style_Cam_Requset_Para trige)
  *��������:	������������
  *��	��:	id		:�����ID��ΧΪ1-15
   num		:��������
   space	:���ռ������λΪ��
   save	:�Ƿ񱣴�ͼƬ��FLASH��
   send	:������Ƿ��ϴ���1��ʾ�ϴ�
   trige	:���մ���Դ����Ϊ	Style_Cam_Requset_Para
  *��	��:	none
  *�� �� ֵ:none
  *��	��:������
  *��������:2013-06-21
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
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
	rt_kprintf( "\n��������ID=%d", id );
}

FINSH_FUNCTION_EXPORT( Cam_takepic_ex, para_id_num_space_save_send_trige );


/*********************************************************************************
  *��������:void Cam_takepic(u16 id,u8 send,Style_Cam_Requset_Para trige)
  *��������:	������������
  *��	��:	id		:�����ID��ΧΪ1-15
   save	:�Ƿ񱣴�ͼƬ��FLASH��
   send	:������Ƿ��ϴ���1��ʾ�ϴ�
   trige	:���մ���Դ����Ϊ	Style_Cam_Requset_Para
  *��	��:	none
  *�� �� ֵ:none
  *��	��:������
  *��������:2013-06-21
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
void Cam_takepic( u16 id, u8 save, u8 send, CAM_TRIGGER trige )
{
	Cam_takepic_ex( id, 1, 0, save, send, trige );
}

FINSH_FUNCTION_EXPORT( Cam_takepic, para_id_save_send_trige );


/*********************************************************************************
  *��������:void Cam_get_All_pic(void)
  *��������:	��ȡͼƬ���ݣ���ӡ�����Դ���
  *��	��:	none
  *��	��:	none
  *�� �� ֵ:none
  *��	��:������
  *��������:2013-06-21
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
void Cam_get_All_pic( void )
{
	Cam_Flash_InitPara( 1 );
}

FINSH_FUNCTION_EXPORT( Cam_get_All_pic, Cam_get_All_pic );

#else

typedef enum
{
	CAM_NONE = 0,
	CAM_IDLE,
	CAM_START,
	CAM_GET_PHOTO,
	CAM_RX_PHOTO,
	CAM_OK,
	CAM_FALSE,
	CAM_END
}CAM_STATE;

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
	CAM_STATE				State;      ///�������״̬��
	CAM_RX_STATE			Rx_State;   ///����״̬
	u8						Retry;      ///�ظ��������
	Style_Cam_Requset_Para	Para;       ///������ǰ���������Ϣ
} Style_Cam_Para;

#define DF_CAM_START		0x108000    ///ͼƬ���ݴ洢��ʼλ��
#define DF_CAM_END			0X1D0000    ///ͼƬ���ݴ洢����λ��
#define DF_CAM_SECTOR_COUNT 0x400       ///ͼƬ���ݴ洢��С���

extern rt_device_t _console_device;
extern u16 Hex_To_Ascii( const u8* pSrc, u8* pDst, u16 nSrcLength );


const char				CAM_HEAD[] = { "PIC_01" };

static Style_Cam_Para	Current_Cam_Para;
static TypeDF_PICPara	DF_PicParameter; ///FLASH�洢��ͼƬ��Ϣ

/* ��Ϣ���п��ƿ�*/
struct rt_messagequeue	mq_Cam;
/* ��Ϣ�������õ��ķ�����Ϣ���ڴ��*/
static char				msgpool_cam[256];

extern MsgList			* list_jt808_tx;


/*********************************************************************************
  *��������:u8 HEX2BCD(u8 HEX)
  *��������:��1���ֽڴ�С��HEX��ת��ΪBCD�룻����BCD��
  *�� ��:none
  *�� ��:none
  *�� �� ֵ:none
  *�� ��:������
  *��������:2013-06-3
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
u8 HEX2BCD( u8 HEX )
{
	u8 BCD_code = 0;
	BCD_code	= HEX % 10;
	BCD_code	|= ( ( HEX % 100 ) / 10 ) << 4;
	return BCD_code;
}

/*********************************************************************************
  *��������:u8 BCD2HEX(u8 BCD)
  *��������:��1���ֽڴ�С��BCD��ת��ΪHEX�룻����HEX��
  *�� ��:none
  *�� ��:none
  *�� �� ֵ:none
  *�� ��:������
  *��������:2013-06-3
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
u8 BCD2HEX( u8 BCD )
{
	u8 HEX_code = 0;
	HEX_code	= BCD & 0x0F;
	HEX_code	+= ( BCD >> 4 ) * 10;
	return HEX_code;
}

/*********************************************************************************
  *��������:bool leap_year(u16 year)
  *��������:������������year�ǲ�������,�����귵��1,���򷵻�0;
   ����Ϊ366�����Ϊ365��,�����2��Ϊ29��.
  *��	��:year	��
  *��	��:none
  *�� �� ֵ:,�����귵��1,���򷵻�0;
  *��	��:������
  *��������:2013-06-3
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
u8 leap_year( u16 year )
{
	u8 leap;
	if( ( year & 0x0003 ) == 0 )
	{
		if( year % 100 == 0 )
		{
			if( year % 400 == 0 )
			{
				leap = 1;
			} else
			{
				leap = 0;
			}
		}else
		{
			leap = 1;
		}
	}else
	{
		leap = 0;
	}
	return leap;
}

/*********************************************************************************
  *��������:u32 Timer_To_Day(T_TIMES *T)
  *��������:�����ʱ�������������ʼʱ��Ϊ2000��1��1��
  *��	��:none
  *��	��:none
  *�� �� ֵ:ʱ���Ӧ����
  *��	��:������
  *��������:2013-06-3
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
u32 Timer_To_Day( T_TIMES *T )
{
	u32 long_day;
	u16 i, year, month, day;

	year		= BCD2HEX( T->years );
	month		= BCD2HEX( T->months );
	day			= BCD2HEX( T->days );
	year		+= 2000;
	long_day	= 0;
	for( i = 2000; i < year; i++ )
	{
		long_day	+= 365;
		long_day	+= leap_year( i );
	}

	switch( month )
	{
		case 12:
			long_day += 30;
		case 11:
			long_day += 31;
		case 10:
			long_day += 30;
		case 9:
			long_day += 31;
		case 8:
			long_day += 31;
		case 7:
			long_day += 30;
		case 6:
			long_day += 31;
		case 5:
			long_day += 30;
		case 4:
			long_day += 31;
		case 3:
		{
			long_day	+= 28;
			long_day	+= leap_year( year );
		}
		case 2:
			long_day += 31;
		case 1:
			long_day += day - 1;
		default:
			nop;
	}
	return long_day;
}

/*********************************************************************************
  *��������:u8 Get_Month_Day(u8 month,u8 leapyear)
  *��������:��ȡ���µ�����
  *��	��:none
  *��	��:none
  *�� �� ֵ:none
  *��	��:������
  *��������:2013-06-3
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
u8 Get_Month_Day( u8 month, u8 leapyear )
{
	u8 day;
	switch( month )
	{
		case 12:
			day = 31;
			break;
		case 11:
			day = 30;
			break;
		case 10:
			day = 31;
			break;
		case 9:
			day = 30;
			break;
		case 8:
			day = 31;
			break;
		case 7:
			day = 31;
			break;
		case 6:
			day = 30;
			break;
		case 5:
			day = 31;
			break;
		case 4:
			day = 30;
			break;
		case 3:
			day = 31;
			break;
		case 2:
		{
			day = 28;
			day += leapyear;
			break;
		}
		case 1:
			day = 31;
			break;
		default:
			day = 0;
	}
	return day;
}

/*********************************************************************************
  *��������:u32 Times_To_LongInt(T_TIMES *T)
  *��������:��������RTC_TIMES���͵�ʱ��ת��Ϊlong int���͵����ݣ�
   ��λΪ�룬��ʼʱ��Ϊ2000��1��1��00:00:00
  *��	��:none
  *��	��:none
  *�� �� ֵ:none
  *��	��:������
  *��������:2013-06-3
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
u32 Times_To_LongInt( T_TIMES *T )
{
	u32 timer_int, hour;
	u16 minute, second;
	hour		= BCD2HEX( T->hours );
	minute		= BCD2HEX( T->minutes );
	second		= BCD2HEX( T->seconds );
	timer_int	= Timer_To_Day( T );
	hour		*= 3600;
	minute		*= 60;
	timer_int	= timer_int * 86400 + hour + minute + second; ///timer_int*24*3600=timer_int*86400
	return timer_int;
}

/*********************************************************************************
   *��������:void LongInt_To_Times(u32 timer_int, T_TIMES *T)
   *��������:����׼ʱ����2000��1��1��00:00:00�ĳ���������תΪT_TIMES����
   *��	��:none
   *��	��:none
   *�� �� ֵ:none
   *��	��:������
   *��������:2013-06-3
   *---------------------------------------------------------------------------------
   *�� �� ��:
   *�޸�����:
   *�޸�����:
 **********************************************6***********************************/
void LongInt_To_Times( u32 timer_int, T_TIMES *T )
{
	u32 long_day1, long_day2;
	u16 i, day, leapyear = 0;
	long_day2	= 0;
	long_day1	= timer_int / 86400;
	for( i = 2000; i < 2100; i++ )
	{
		day			= 365 + leap_year( i );
		long_day2	+= day;
		//�����ǰ���������С�ڼ���������õ������
		if( long_day2 > long_day1 )
		{
			long_day2	-= day;
			leapyear	= leap_year( i );
			break;
		}
	}
	T->years = HEX2BCD( i - 2000 );
	for( i = 1; i <= 12; i++ )
	{
		day			= Get_Month_Day( i, leapyear );
		long_day2	+= day;
		//�����ǰ�µ�������С�ڼ���������õ����·�
		if( long_day2 > long_day1 )
		{
			long_day2 -= day;
			break;
		}
	}
	T->months	= HEX2BCD( i );
	day			= long_day1 - long_day2 + 1;
	T->days		= HEX2BCD( day );

	long_day1	= timer_int % 86400;
	i			= long_day1 / 3600;
	T->hours	= HEX2BCD( i );

	i			= long_day1 % 3600 / 60;
	T->minutes	= HEX2BCD( i );

	i			= long_day1 % 60;
	T->seconds	= HEX2BCD( i );
}

/*********************************************************************************
  *��������:u32 Cam_Flash_AddrCheck(u32 pro_Address)
  *��������:��鵱ǰ��λ���Ƿ�Ϸ������Ϸ����޸�Ϊ�Ϸ����ݲ�����
  *��	��:none
  *��	��:none
  *�� �� ֵ:��ȷ�ĵ�ֵַ
  *��	��:������
  *��������:2013-06-5
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
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

/*********************************************************************************
  *��������:u8 Cam_Flash_FirstPicProc(u32 temp_wr_addr)
  *��������:�޸ĵ�1��ͼƬ����λ�ò�������ΪҪ��flash����erase����erase���ǵ�1��ͼƬʱ����Ҫ��DF_PicParameter��
   ��һ��ͼƬ����Ϣ����
  *��	��:��ǰҪ��д��λ��
  *��	��:none
  *�� �� ֵ:1��ʾ�������أ�0��ʾ�д�����
  *��	��:������
  *��������:2013-06-5
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
static u8 Cam_Flash_FirstPicProc( u32 temp_wr_addr )
{
	u8					i;
	u8					ret;
	u32					TempAddress;
	TypeDF_PackageHead	TempPackageHead;

	if( ( Cam_Flash_AddrCheck( temp_wr_addr ) == ( DF_PicParameter.FirstPic.Address & 0xFFFFF000 ) ) && ( DF_PicParameter.Number ) )
	{
		TempAddress = ( DF_PicParameter.FirstPic.Address + DF_PicParameter.FirstPic.Len + DF_CAM_SECTOR_COUNT - 1 ) / DF_CAM_SECTOR_COUNT * DF_CAM_SECTOR_COUNT;
		for( i = 0; i < 8; i++ )
		{
			sst25_read( Cam_Flash_AddrCheck( TempAddress ), (u8*)&TempPackageHead, sizeof( TempPackageHead ) );
			if( strncmp( TempPackageHead.Head, CAM_HEAD, strlen( CAM_HEAD ) ) == 0 )
			{
				if( ( TempAddress & 0xFFFFF000 ) != ( DF_PicParameter.FirstPic.Address & 0xFFFFF000 ) )
				{
					DF_PicParameter.FirstPic.Address	= Cam_Flash_AddrCheck( TempAddress );
					DF_PicParameter.FirstPic.Len		= TempPackageHead.Len;
					DF_PicParameter.FirstPic.Data_ID	= TempPackageHead.Data_ID;
					DF_PicParameter.Number--;
					ret = 1;
					break;
				}
				if( TempPackageHead.Len == 0 )
				{
					ret = 0;
					break;
				}
				TempAddress += ( TempPackageHead.Len + DF_CAM_SECTOR_COUNT - 1 ) / DF_CAM_SECTOR_COUNT * DF_CAM_SECTOR_COUNT;
			}else
			{
				ret = 0;
				break;
			}
		}
		ret = 0;
	}else
	{
		ret = 1;
	}
	sst25_erase_4k( temp_wr_addr );
	return ret;
}

/*********************************************************************************
  *��������:u16 Cam_Flash_InitPara(u8 printf_info)
  *��������:��ʼ��Pic������������ȡFLASH�е�ͼƬ��Ϣ����ȡ��ͼƬ����������ʼλ�ã�����λ�õȣ�
   ��Щ��ȡ�������ݶ��洢�� DF_PicParameter �С�
  *��	��:printf_info	:	0����ʾ����ӡͼƬ��Ϣ��1��ʾ��ӡͼƬ��Ϣ
  *��	��:none
  *�� �� ֵ:��Ч��ͼƬ����
  *��	��:������
  *��������:2013-06-5
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
static u16 Cam_Flash_InitPara( u8 printf_info )
{
	u32					TempAddress;
	TypeDF_PackageHead	TempPackageHead;
	TypeDF_PackageInfo	TempPackageInfo;

	memset( &DF_PicParameter, 0, sizeof( DF_PicParameter ) );
	DF_PicParameter.FirstPic.Address	= DF_CAM_START;
	DF_PicParameter.LastPic.Address		= DF_CAM_START;
	if( printf_info )
	{
		rt_kprintf( "\n PIC_ADDRESS,  %PIC_ID,  %PIC_LEN,  NO_DEL\n" );
	}
	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
	for( TempAddress = DF_CAM_START; TempAddress < DF_CAM_END; )
	{
		sst25_read( TempAddress, (u8*)&TempPackageHead, sizeof( TempPackageHead ) );
		if( strncmp( TempPackageHead.Head, CAM_HEAD, strlen( CAM_HEAD ) ) == 0 )
		{
			DF_PicParameter.Number++;
			TempPackageInfo.Address = TempAddress;
			TempPackageInfo.Data_ID = TempPackageHead.Data_ID;
			TempPackageInfo.Len		= TempPackageHead.Len;
			if( DF_PicParameter.Number == 1 )
			{
				DF_PicParameter.FirstPic	= TempPackageInfo;
				DF_PicParameter.LastPic		= TempPackageInfo;
			}else
			{
				if( TempPackageInfo.Data_ID > DF_PicParameter.LastPic.Data_ID )
				{
					DF_PicParameter.LastPic = TempPackageInfo;
				}else if( TempPackageInfo.Data_ID < DF_PicParameter.FirstPic.Data_ID )
				{
					DF_PicParameter.FirstPic = TempPackageInfo;
				}
			}
			if( printf_info )
			{
				rt_kprintf( "  0x%08x,    %05d,      %04d,     %d\n", TempAddress, TempPackageInfo.Data_ID, TempPackageInfo.Len, TempPackageHead.State & ( BIT( 0 ) ) );
			}
			TempAddress += ( TempPackageInfo.Len + DF_CAM_SECTOR_COUNT - 1 ) / DF_CAM_SECTOR_COUNT * DF_CAM_SECTOR_COUNT;
		}else
		{
			TempAddress += DF_CAM_SECTOR_COUNT;
		}
	}
	rt_sem_release( &sem_dataflash );
	if( printf_info )
	{
		rt_kprintf( "  PIC_NUM =  %04d\n", DF_PicParameter.Number );
	}
	return DF_PicParameter.Number;
}

/*********************************************************************************
  *��������:rt_err_t Cam_Flash_WrPic(u8 *pData,u16 len, TypeDF_PackageHead *pHead)
  *��������:��FLASH��д��ͼƬ���ݣ�������ݵ����ݵ�pHead->lenΪ��0ֵ��ʾΪ���һ������
  *��	��:	pData:д�������ָ�룬ָ������buf��
   len:���ݳ��ȣ�ע�⣬�ó��ȱ���С��4096
   pHead:���ݰ�ͷ��Ϣ���ð�ͷ��Ϣ��Ҫ����ʱ��Timer����ֵÿ�ζ����봫�ݣ�����ͬ���İ���ֵ���ܱ仯��
    ���һ�������轫len����Ϊ���ݳ���len�ĳ��Ȱ�����ͷ���֣���ͷ����Ϊ�̶�64�ֽڣ�������
    lenΪ0.
  *��	��:none
  *�� �� ֵ:re_err_t
  *��	��:������
  *��������:2013-06-5
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
static rt_err_t Cam_Flash_WrPic( u8 *pData, u16 len, TypeDF_PackageHead *pHead )
{
	u16				i;
	u32				temp_wr_addr;
	u32				temp_Len;
	static u8		WriteFuncUserBack	= 0;
	static u32		WriteAddress		= 0;
	static u32		WriteAddressStart	= 0;
	static T_TIMES	LastTime			= { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

	u8				strBuf[256];

	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
	if( memcmp( &LastTime, &pHead->Time, sizeof( LastTime ) ) != 0 )
	{
		LastTime			= pHead->Time;
		WriteAddressStart	= ( DF_PicParameter.LastPic.Address + DF_PicParameter.LastPic.Len + DF_CAM_SECTOR_COUNT - 1 ) / DF_CAM_SECTOR_COUNT * DF_CAM_SECTOR_COUNT;
		WriteAddressStart	= Cam_Flash_AddrCheck( WriteAddressStart );
		WriteAddress		= WriteAddressStart + 64;
		if( ( WriteAddressStart & 0xFFF ) == 0 )
		{
			Cam_Flash_FirstPicProc( WriteAddressStart );
		}else
		{
			///�жϸ������Ƿ������쳣���쳣��Ҫʹ��falshд����sst25_write_back
			sst25_read( WriteAddressStart, strBuf, sizeof( strBuf ) );
			WriteFuncUserBack = 0;
			for( i = 0; i < sizeof( strBuf ); i++ )
			{
				if( strBuf[i] != 0xFF )
				{
					if( i < 64 ) ///�����ͷ����Ҳ������հ�ͷ��������Ϊ0xFF
					{
						memset( strBuf, 0xFF, sizeof( strBuf ) );
						sst25_write_back( WriteAddressStart, strBuf, sizeof( strBuf ) );
					}

					WriteFuncUserBack = 1;
					break;
				}
			}
		}
	}

	if( WriteFuncUserBack )
	{
		if( ( WriteAddress & 0xFFFFF000 ) != ( ( WriteAddress + len ) & 0xFFFFF000 ) ) ///��Խ���������Ĵ���
		{
			temp_wr_addr	= ( WriteAddress + len ) & 0xFFFFF000;
			temp_Len		= temp_wr_addr - WriteAddress;
			sst25_write_back( Cam_Flash_AddrCheck( WriteAddress ), pData, temp_Len );
			Cam_Flash_FirstPicProc( Cam_Flash_AddrCheck( temp_wr_addr ) );
			sst25_write_through( Cam_Flash_AddrCheck( temp_wr_addr ), pData + temp_Len, len - temp_Len );
			WriteFuncUserBack = 0;
		}else
		{
			sst25_write_back( Cam_Flash_AddrCheck( WriteAddress ), pData, len );
		}
	}else
	{
		if( ( WriteAddress & 0xFFFFF000 ) != ( ( WriteAddress + len ) & 0xFFFFF000 ) ) ///��Խ���������Ĵ���
		{
			temp_wr_addr	= ( WriteAddress + len ) & 0xFFFFF000;
			temp_Len		= temp_wr_addr - WriteAddress;
			sst25_write_through( Cam_Flash_AddrCheck( WriteAddress ), pData, temp_Len );
			Cam_Flash_FirstPicProc( Cam_Flash_AddrCheck( temp_wr_addr ) );
			sst25_write_through( Cam_Flash_AddrCheck( temp_wr_addr ), pData + temp_Len, len - temp_Len );
		}else
		{
			sst25_write_through( Cam_Flash_AddrCheck( WriteAddress ), pData, len );
		}
	}
	WriteAddress += len;
	if( pHead->Len )
	{
		WriteAddress = WriteAddressStart;
		DF_PicParameter.Number++;
		DF_PicParameter.LastPic.Data_ID++;
		DF_PicParameter.LastPic.Address = Cam_Flash_AddrCheck( WriteAddressStart );
		DF_PicParameter.LastPic.Len		= pHead->Len;
		if( DF_PicParameter.Number == 1 )
		{
			DF_PicParameter.FirstPic = DF_PicParameter.LastPic;
		}

		memcpy( pHead->Head, CAM_HEAD, strlen( CAM_HEAD ) );
		pHead->Data_ID		= DF_PicParameter.LastPic.Data_ID;
		pHead->Media_Style	= 0;
		pHead->Media_Format = 0;
		sst25_write_through( Cam_Flash_AddrCheck( WriteAddressStart ), (u8*)pHead, sizeof( TypeDF_PackageHead ) );
	}

	rt_sem_release( &sem_dataflash );
	return RT_EOK;
}

/*********************************************************************************
  *��������:u32 Cam_Flash_FindPicID(u32 id,TypeDF_PackageHead *p_head)
  *��������:��FLASH�в���ָ��ID��ͼƬ������ͼƬ��ͷ���´洢��p_head��
  *��	��:	id:��ý��ID��
   p_head:������������ΪͼƬ��ý�����ݰ�ͷ��Ϣ(����ΪTypeDF_PackageHead)
  *��	��:p_head
  *�� �� ֵ:u32 0xFFFFFFFF��ʾû���ҵ�ͼƬ��������ʾ�ҵ���ͼƬ������ֵΪͼƬ��flash�еĵ�ַ
  *��	��:������
  *��������:2013-06-5
  *---------------------------------------------------------------------------------
  *�� �� ��:bai
  *�޸�����:2013-06-23
  *�޸�����:�޸����������ݴ洢�쳣ʱ�ļ�������
*********************************************************************************/
u32 Cam_Flash_FindPicID( u32 id, TypeDF_PackageHead *p_head )
{
	u32							i;
	u32							TempAddress;
	u32							tempu32data;
	u32							flash_search_area = 0; ///��ʾɨ���˶������򣬵�ɨ���������ͼƬ������ʱ������

	static TypeDF_PackageInfo	lastPackInfo = { 0xFFFFFFFF, 0, 0xFFFFFFFF };
	static TypeDF_PackageHead	TempPackageHead;

	if( DF_PicParameter.Number == 0 )
	{
		return 0xFFFFFFFF;
	}
	if( ( id < DF_PicParameter.FirstPic.Data_ID ) || ( id > DF_PicParameter.LastPic.Data_ID ) )
	{
		return 0xFFFFFFFF;
	}
	if( id != lastPackInfo.Data_ID )
	{
		lastPackInfo.Data_ID	= 0xFFFFFFFF;
		TempAddress				= DF_PicParameter.FirstPic.Address;
		for( i = 0; i < DF_PicParameter.Number; )
		{
			TempAddress = Cam_Flash_AddrCheck( TempAddress );
			sst25_read( TempAddress, (u8*)&TempPackageHead, sizeof( TypeDF_PackageHead ) );
			//rt_kprintf("\n NUM=%d,ADDR=%d,ID=%d, Head=",i,TempAddress,TempPackageHead.Data_ID);
			//printer_data_hex((u8 *)&TempPackageHead,sizeof(TypeDF_PackageHead)-28);
			if( strncmp( TempPackageHead.Head, CAM_HEAD, strlen( CAM_HEAD ) ) == 0 )
			{
				///�鿴��ͼƬ�Ƿ�ɾ��
				if( TempPackageHead.State & BIT( 0 ) )
				{
					if( TempPackageHead.Data_ID == id )
					{
						lastPackInfo.Data_ID	= id;
						lastPackInfo.Address	= TempAddress;
						lastPackInfo.Len		= TempPackageHead.Len;
						memcpy( (void*)p_head, (void*)&TempPackageHead, sizeof( TypeDF_PackageHead ) );
						return lastPackInfo.Address;
					}
				}
				i++;
				//TempAddress+=(TempPackageHead.Len+DF_CAM_SECTOR_COUNT-1)/DF_CAM_SECTOR_COUNT*DF_CAM_SECTOR_COUNT;
				tempu32data			= ( TempPackageHead.Len + DF_CAM_SECTOR_COUNT - 1 ) / DF_CAM_SECTOR_COUNT;
				TempAddress			+= tempu32data * DF_CAM_SECTOR_COUNT;
				flash_search_area	+= tempu32data;
			}else
			{
				TempAddress += DF_CAM_SECTOR_COUNT; ///�޸Ĵ洢�쳣���ڴ����Ӹô��룬֮ǰ����ֱ�ӷ���OXffff
				//return 0xFFFFFFFF;
				flash_search_area++;
			}
			if( flash_search_area > ( DF_CAM_END - DF_CAM_START ) / DF_CAM_SECTOR_COUNT )
			{
				return 0xFFFFFFFF;
			}
		}
	}else
	{
		memcpy( (void*)p_head, (void*)&TempPackageHead, sizeof( TypeDF_PackageHead ) );
		return lastPackInfo.Address;
	}
	return 0xFFFFFFFF;
}

/*********************************************************************************
  *��������:rt_err_t Cam_Flash_RdPic(void *pData,u16 *len, u32 id,u8 offset )
  *��������:��FLASH�ж�ȡͼƬ����
  *��	��:	pData:д�������ָ�룬ָ������buf��
   len:���ص����ݳ���ָ��ע�⣬�ó������Ϊ512
   id:��ý��ID��
   offset:��ý������ƫ��������0��ʼ��0��ʾ��ȡ��ý��ͼƬ��ͷ��Ϣ����ͷ��Ϣ���ȹ̶�Ϊ64�ֽڣ�����
    �ṹ��TypeDF_PackageHead��ʽ�洢
  *��	��:none
  *�� �� ֵ:re_err_t
  *��	��:������
  *��������:2013-06-5
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
rt_err_t Cam_Flash_RdPic( void *pData, u16 *len, u32 id, u8 offset )
{
	u32					TempAddress;
	u32					temp_Len;
	TypeDF_PackageHead	TempPackageHead;
	u8					ret;

	*len = 0;
	//rt_kprintf("\n take_flash_sem");
	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );

	TempAddress = Cam_Flash_FindPicID( id, &TempPackageHead );
	if( TempAddress == 0xFFFFFFFF )
	{
		ret = RT_ERROR;  goto FUNC_RET;
	}
	if( TempPackageHead.Data_ID == 0xFFFFFFFF )
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
	//rt_kprintf("\n releas_flash_sem");
	rt_sem_release( &sem_dataflash );
	return ret;
}

/*********************************************************************************
  *��������:rt_err_t Cam_Flash_DelPic(u32 id)
  *��������:��FLASH��ɾ��ͼƬ��ʵ���ϲ�û��ɾ�������ǽ�ɾ�������0�������´ξ���Ҳ����ѯ��ͼƬ
  *��	��:	id:��ý��ID��
  *��	��:none
  *�� �� ֵ:re_err_t
  *��	��:������
  *��������:2013-06-13
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
rt_err_t Cam_Flash_DelPic( u32 id )
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
	TempPackageHead.State &= ~( BIT( 0 ) );
	sst25_write_through( TempAddress, (u8*)&TempPackageHead, sizeof( TypeDF_PackageHead ) );
	ret = RT_EOK;

FUNC_RET:
	rt_sem_release( &sem_dataflash );
	return ret;
}

/*********************************************************************************
  *��������:rt_err_t Cam_Flash_TransOkSet(u32 id)
  *��������:����ID��ͼƬ���Ϊ���ͳɹ�
  *��	��:	id:��ý��ID��
  *��	��:none
  *�� �� ֵ:re_err_t
  *��	��:������
  *��������:2013-06-13
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
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
  *��������:u16 Cam_Flash_SearchPic(T_TIMES *start_time,T_TIMES *end_time,TypeDF_PackageHead *para,u8 *pdest)
  *��������:��FLASH�в���ָ��ʱ��ε�ͼƬ����
  *��	��:	start_time	��ʼʱ�䣬
   end_time		����ʱ�䣬
   para			����ͼƬ������
   pdest			�洢���ҵ���ͼƬ��λ�ã�ÿ����ý��ͼƬռ��4���ֽ�
  *��	��: u16 ���ͣ���ʾ���ҵ���ͼƬ����
  *�� �� ֵ:re_err_t
  *��	��:������
  *��������:2013-06-5
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
u16 Cam_Flash_SearchPic( T_TIMES *start_time, T_TIMES *end_time, TypeDF_PackageHead *para, u8 *pdest )
{
	u16					i;
	u32					TempAddress;
	u32					temp_u32;
	TypeDF_PackageHead	TempPackageHead;
	u16					ret_num = 0;

	if( DF_PicParameter.Number == 0 )
	{
		return 0;
	}
	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
	TempAddress = DF_PicParameter.FirstPic.Address;
	for( i = 0; i < DF_PicParameter.Number; i++ )
	{
		TempAddress = Cam_Flash_AddrCheck( TempAddress );
		sst25_read( TempAddress, (u8*)&TempPackageHead, sizeof( TempPackageHead ) );
		if( strncmp( TempPackageHead.Head, CAM_HEAD, strlen( CAM_HEAD ) ) == 0 )
		{
			///�鿴��ͼƬ�Ƿ�ɾ��
			if( TempPackageHead.State & BIT( 0 ) )
			{
				///�Ƚ϶�ý�����ͣ���ý��ͨ������ý�崥��Դ
				if( ( TempPackageHead.Media_Style == para->Media_Style ) && ( ( TempPackageHead.Channel_ID == para->Channel_ID ) || ( para->Channel_ID == 0 ) ) && ( ( TempPackageHead.TiggerStyle == para->TiggerStyle ) || ( para->TiggerStyle == 0xFF ) ) )
				{
					temp_u32 = Times_To_LongInt( &TempPackageHead.Time );
					///�Ƚ�ʱ���Ƿ��ڷ�Χ
					if( ( temp_u32 > Times_To_LongInt( start_time ) ) && ( temp_u32 <= Times_To_LongInt( end_time ) ) )
					{
						///�ҵ�������
						data_to_buf( pdest, TempAddress, 4 );
						//memcpy(pdest,&TempAddress,4);
						pdest += 4;
						ret_num++;
					}
				}
			}
			TempAddress += ( TempPackageHead.Len + DF_CAM_SECTOR_COUNT - 1 ) / DF_CAM_SECTOR_COUNT * DF_CAM_SECTOR_COUNT;
		}else
		{
			TempAddress += DF_CAM_SECTOR_COUNT; ///�޸Ĵ洢�쳣���ڴ����Ӹô��룬֮ǰ����ֱ�ӷ���OXffff
		}
	}

	rt_sem_release( &sem_dataflash );
	return ret_num;
}

#if 0


/*********************************************************************************
  *��������:static void delay_us( const uint32_t usec )
  *��������:��ʱ��������ʱ��λΪus
  *�� ��:	usec	:��ʱʱ�䣬��λΪus
  *�� ��:none
  *�� �� ֵ:none
  *�� ��:������
  *��������:2013-06-3
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
static void delay_us( const uint32_t usec )
{
	__IO uint32_t	count	= 0;
	const uint32_t	utime	= ( 168 * usec / 7 );
	do
	{
		if( ++count > utime )
		{
			return;
		}
	}
	while( 1 );
}

/*********************************************************************************
  *��������:static rt_err_t Cam_Device_open( void )
  *��������:��ģ�鹩��
  *��	��:none
  *��	��:none
  *�� �� ֵ:none
  *��	��:������
  *��������:2013-06-3
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
static rt_err_t Cam_Device_open( void )
{
	Power_485CH1_ON;
	return RT_EOK;
}

/*********************************************************************************
  *��������:static rt_err_t Cam_Devic_close( void )
  *��������:�ر�ģ�鹩��
  *��	��:none
  *��	��:none
  *�� �� ֵ:none
  *��	��:������
  *��������:2013-06-3
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
static rt_err_t Cam_Devic_close( void )
{
	Power_485CH1_OFF;
	return RT_EOK;
}

#endif


/*********************************************************************************
  *��������:void Cam_Device_init( void )
  *��������:��ʼ��CAMģ����ؽӿ�
  *��	��:none
  *��	��:none
  *�� �� ֵ:none
  *��	��:������
  *��������:2013-06-3
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
void Cam_Device_init( void )
{
	///����
	Power_485CH1_ON;

	///��ʼ����Ϣ����
	rt_mq_init( &mq_Cam, "mq_cam", &msgpool_cam[0], sizeof( Style_Cam_Requset_Para ), sizeof( msgpool_cam ), RT_IPC_FLAG_FIFO );

	///��ʼ��flash����
	Cam_Flash_InitPara( 0 );

	///��ʼ������״̬����
	memset( (u8*)&Current_Cam_Para, 0, sizeof( Current_Cam_Para ) );
	Current_Cam_Para.State = CAM_NONE;
}

/*********************************************************************************
  *��������:static void Cam_Start_Cmd(u16 Cam_ID)
  *��������:��cameraģ�鷢�Ϳ�ʼ����ָ��
  *��	��:Cam_ID	��Ҫ���յ�camera�豸ID
  *��	��:none
  *�� �� ֵ:none
  *��	��:������
  *��������:2013-06-3
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
static void Cam_Start_Cmd( u16 Cam_ID )
{
	u8 Take_photo[10] = { 0x40, 0x40, 0x61, 0x81, 0x02, 0X00, 0X00, 0X02, 0X0D, 0X0A }; //----  ������������
	Take_photo[4]	= (u8)Cam_ID;
	Take_photo[5]	= (u8)( Cam_ID >> 8 );
	RS485_write( Take_photo, 10 );
	uart2_rxbuf_rd = uart2_rxbuf_wr;
	rt_kprintf( "\n������������" );
}

/*********************************************************************************
  *��������:static void Cam_Read_Cmd(u16 Cam_ID)
  *��������:��cameraģ�鷢�Ͷ�ȡ��Ƭ����ָ��
  *��	��:Cam_ID	��Ҫ���յ�camera�豸ID
  *��	��:none
  *�� �� ֵ:none
  *��	��:������
  *��������:2013-06-3
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
static void Cam_Read_Cmd( u16 Cam_ID )
{
	uint8_t Fectch_photo[10] = { 0x40, 0x40, 0x62, 0x81, 0x02, 0X00, 0XFF, 0XFF, 0X0D, 0X0A }; //----- ����ȡͼ����
	Fectch_photo[4] = (u8)Cam_ID;
	Fectch_photo[5] = (u8)( Cam_ID >> 8 );
	RS485_write( Fectch_photo, 10 );
	uart2_rxbuf_rd = uart2_rxbuf_wr;
	rt_kprintf( "\n��ȡ��Ƭ����" );
}

/*********************************************************************************
  *��������:TypeDF_PICPara Cam_get_state(void)
  *��������:��ȡϵͳ������ز���
  *��	��:none
  *��	��:none
  *�� �� ֵ:TypeDF_PICPara
  *��	��:������
  *��������:2013-06-3
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
TypeDF_PICPara Cam_get_state( void )
{
	return DF_PicParameter;
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
  *��������:rt_err_t take_pic_request( Style_Cam_Requset_Para *para)
  *��������:��������ָ��
  *��	��:para���ղ���
  *��	��:none
  *�� �� ֵ:rt_err_t
  *��	��:������
  *��������:2013-06-3
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
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
	            DF_PicParameter.FirstPic.Data_ID,
	            DF_PicParameter.FirstPic.Address,
	            DF_PicParameter.FirstPic.Len,
	            DF_PicParameter.LastPic.Data_ID,
	            DF_PicParameter.LastPic.Address,
	            DF_PicParameter.LastPic.Len );
}

FINSH_FUNCTION_EXPORT( getpicpara, getpicpara );


/*********************************************************************************
  *��������:bool Camera_RX_Data(u16 *RxLen)
  *��������:���ս������ݴ���
  *��	��:RxLen:��ʾ���յ������ݳ���ָ��
  *��	��:RxLen:��ʾ���յ������ݳ���ָ��
  *�� �� ֵ:0��ʾ���ս�����,1��ʾ���ճɹ�
  *��	��:������
  *��������:2013-06-3
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
static u8 Camera_RX_Data( u16 *RxLen )
{
	u8			ch;
	static u16	page_size		= 0;
	static u16	cam_rx_head_wr	= 0;

	/*�����Ƿ��յ�����*/
	while( RS485_read_char( &ch ) )
	{
		switch( Current_Cam_Para.Rx_State )
		{
			case RX_DATA: /*������Ϣ��ʽ: λ��(2B) ��С(2B) FLAG_FF ���� 0D 0A*/
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
  *��������:void Cam_response_ok( struct _Style_Cam_Requset_Para *para,uint32_t pic_id )
  *��������:ƽ̨�·�������������Ļص�����_������Ƭ����OK
  *��	��:	para	:���մ���ṹ��
   pic_id	:ͼƬID
  *��	��:	none
  *�� �� ֵ:rt_err_t
  *��	��:������
  *��������:2013-06-17
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
void Cam_response_ok( struct _Style_Cam_Requset_Para *para, uint32_t pic_id )
{
	if( para->SendPhoto )
	{
		Cam_jt808_0x0800( pic_id, !para->SavePhoto );
	}
}

/*********************************************************************************
  *��������:void Cam_response_end( struct _Style_Cam_Requset_Para *para )
  *��������:ƽ̨�·�������������Ļص�����
  *��	��:	para	:���մ���ṹ��
  *��	��:	none
  *�� �� ֵ:rt_err_t
  *��	��:������
  *��������:2013-06-17
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
void Cam_response_end( struct _Style_Cam_Requset_Para *para )
{
	return;
}

#endif


/*********************************************************************************
  *��������:void Camera_Process(void)
  *��������:����������ش���(������:���գ��洢��Ƭ���������ս���ָ���808)
  *��	��:none
  *��	��:none
  *�� �� ֵ:none
  *��	��:������
  *��������:2013-06-3
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
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
				//rt_kprintf( "\n�յ�������Ϣ" );
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
				rt_kprintf( "\n���յ���������" );
				tick = rt_tick_get( );
				///�յ�����,�洢,�ж��Ƿ�ͼƬ����
				if( photo_rx_wr > 512 ) ///���ݴ���512,�Ƿ�
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
				///��һ�����ݣ���Ҫ��д������Ȼ��洢����ȷ
				if( cam_page_num == 1 )
				{
					pack_head.Channel_ID	= Current_Cam_Para.Para.Channel_ID;
					pack_head.TiggerStyle	= Current_Cam_Para.Para.TiggerStyle;
					pack_head.Media_Format	= 0;
					pack_head.Media_Style	= 0;
					memcpy( &pack_head.Time, gps_datetime, 6 );
					memcpy( &pack_head.position, &gps_baseinfo, 28 );
					pack_head.State = 0xFF;
					if( Current_Cam_Para.Para.SavePhoto == 0 )
					{
						pack_head.State &= ~( BIT( 2 ) );
					}
				}
				///���һ�����ݣ���Ҫ������д�롣
				if( cam_last_page )
				{
					pack_head.Len			= cam_photo_size + 64;
					Current_Cam_Para.State	= CAM_OK;
				}else
				{
					Current_Cam_Para.State = CAM_GET_PHOTO;
				}
				///��������
				Cam_Flash_WrPic( photo_rx, photo_rx_wr, &pack_head );
				photo_rx_wr = 0;
			}else if( rt_tick_get( ) - tick > RT_TICK_PER_SECOND * 2 ) ///�ж��Ƿ�ʱ
			{
				Current_Cam_Para.State = CAM_START;
			}
			break;
		case CAM_OK:
			++Current_Cam_Para.Para.PhotoNum;
			rt_kprintf( "\n���ճɹ�!" );
			getpicpara( );
#if 1
			if( Current_Cam_Para.Para.cb_response_cam_ok != RT_NULL ) ///���õ�����Ƭ���ճɹ��ص�����
			{
				Current_Cam_Para.Para.cb_response_cam_ok( &Current_Cam_Para.Para, pack_head.Data_ID );
			}else ///Ĭ�ϵĻص�����
			{
				Cam_response_ok( &Current_Cam_Para.Para, pack_head.Data_ID );
			}
#endif

			if( Current_Cam_Para.Para.PhotoNum >= Current_Cam_Para.Para.PhotoTotal )
			{
				Current_Cam_Para.State = CAM_END;   ///�����������
			}else
			{
				Current_Cam_Para.State = CAM_IDLE;  ///������Ƭ�������
			}
			break;
		case CAM_FALSE:
			rt_kprintf( "\n����ʧ��!" );
			Current_Cam_Para.State = CAM_END;
		case CAM_END:
			rt_kprintf( "\n���ս���!" );
			Current_Cam_Para.State = CAM_NONE;
			if( Current_Cam_Para.Para.cb_response_cam_end != RT_NULL ) ///���õ�����Ƭ���ճɹ��ص�����
			{
				Current_Cam_Para.Para.cb_response_cam_end( &Current_Cam_Para.Para );
			}else ///Ĭ�ϵĻص�����
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
  *��������:void Cam_takepic_ex(u16 id,u16 num,u16 space,u8 send,Style_Cam_Requset_Para trige)
  *��������:	������������
  *��	��:	id		:�����ID��ΧΪ1-15
   num		:��������
   space	:���ռ������λΪ��
   save	:�Ƿ񱣴�ͼƬ��FLASH��
   send	:������Ƿ��ϴ���1��ʾ�ϴ�
   trige	:���մ���Դ����Ϊ	Style_Cam_Requset_Para
  *��	��:	none
  *�� �� ֵ:none
  *��	��:������
  *��������:2013-06-21
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
void Cam_takepic_ex( u16 id, u16 num, u16 space, u8 save, u8 send, Cam_Trigger trige )
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
	rt_kprintf( "\n��������ID=%d", id );
}

FINSH_FUNCTION_EXPORT( Cam_takepic_ex, para_id_num_space_save_send_trige );


/*********************************************************************************
  *��������:void Cam_takepic(u16 id,u8 send,Style_Cam_Requset_Para trige)
  *��������:	������������
  *��	��:	id		:�����ID��ΧΪ1-15
   save	:�Ƿ񱣴�ͼƬ��FLASH��
   send	:������Ƿ��ϴ���1��ʾ�ϴ�
   trige	:���մ���Դ����Ϊ	Style_Cam_Requset_Para
  *��	��:	none
  *�� �� ֵ:none
  *��	��:������
  *��������:2013-06-21
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
void Cam_takepic( u16 id, u8 save, u8 send, Cam_Trigger trige )
{
	Cam_takepic_ex( id, 1, 0, save, send, trige );
}

FINSH_FUNCTION_EXPORT( Cam_takepic, para_id_save_send_trige );


/*********************************************************************************
  *��������:void Cam_get_All_pic(void)
  *��������:	��ȡͼƬ���ݣ���ӡ�����Դ���
  *��	��:	none
  *��	��:	none
  *�� �� ֵ:none
  *��	��:������
  *��������:2013-06-21
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
void Cam_get_All_pic( void )
{
	Cam_Flash_InitPara( 1 );
}

FINSH_FUNCTION_EXPORT( Cam_get_All_pic, Cam_get_All_pic );

#endif

/************************************** The End Of File **************************************/
