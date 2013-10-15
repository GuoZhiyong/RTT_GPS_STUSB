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
#define DF_CAM_REC_COUNT	4096        ///ͼƬ���ݼ�¼��Ԫ��С
#define DF_CAM_REC_MASK		0xFFFFF000  //ÿ��¼������ ~(count-1)

#define DF_SECTOR_MASK 0xFFFFF000       /*4K�ĵ�ַ����*/

extern rt_device_t _console_device;
extern u16 Hex_To_Ascii( const u8* pSrc, u8* pDst, u16 nSrcLength );


#define  CAM_HEAD 0x5049435F            /*ͼƬ��ʶ PIC_ */

static Style_Cam_Para	Current_Cam_Para;
static TypeDF_PICPara	DF_PicParam;    ///FLASH�洢��ͼƬ��Ϣ

/* ��Ϣ���п��ƿ�*/
struct rt_messagequeue	mq_Cam;
/* ��Ϣ�������õ��ķ�����Ϣ���ڴ��*/
static char				msgpool_cam[256];

#define PHOTO_RX_SIZE 1024

static uint8_t	photo_rx[PHOTO_RX_SIZE];
static uint16_t photo_rx_wr = 0;

extern MsgList	* list_jt808_tx;

/*���浥��ͼƬ*/
static uint8_t photo_ram[32 * 1024] __attribute__( ( section( "CCM_RT_STACK" ) ) );


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

/*��ʼ�����ղ���*/
static u16 Cam_Flash_InitPara( u8 printf_info )
{
	u32					TempAddress;
	TypeDF_PackageHead	TempPackageHead;

	DF_PicParam.Number			= 0;
	DF_PicParam.First.Address	= DF_CAM_START; /*û����Ч��ַ*/
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
		if( TempPackageHead.Head == CAM_HEAD )                      /*��Ч����*/
		{
			DF_PicParam.Number++;
			if( DF_PicParam.First.Data_ID >= TempPackageHead.id )   /*��һ��Ӧ����id��С��*/
			{
				DF_PicParam.First.Address	= TempAddress;
				DF_PicParam.First.Len		= TempPackageHead.Len;
				DF_PicParam.First.Data_ID	= TempPackageHead.id;
			}

			if( DF_PicParam.Last.Data_ID <= TempPackageHead.id )    /*���һ��Ӧ����id����*/
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

#define my_printf( X ) rt_kprintf( "\n%s=%d", # X, X )


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static rt_err_t Cam_Flash_SavePic( TypeDF_PackageHead * phead )
{
	uint32_t	len;
	uint32_t	addr;
	uint8_t		* p;

	len		= phead->Len;
	addr	= ( DF_PicParam.Last.Address + DF_PicParam.Last.Len + DF_CAM_REC_COUNT - 1 ) & DF_CAM_REC_MASK; /*4096����*/

	phead->id = DF_PicParam.Last.Data_ID++;
	memcpy( photo_ram, (uint8_t*)phead, 64 );
	p = photo_ram;

	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
	while( 1 )
	{
		addr = Cam_Flash_AddrCheck( addr );
		sst25_erase_4k( addr );
		if( len > 4096 )
		{
			sst25_write_through( addr, p, 4096 );
			p		+= 4096;
			addr	+= 4096;
			len		-= 4096;
		}else
		{
			sst25_write_through( addr, p, len );
			break;
		}
	}
	rt_sem_release( &sem_dataflash );
	Cam_Flash_InitPara( 1 );
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
	u32					TempAddress;
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
		if( TempPackageHead.Head == CAM_HEAD ) /*��Ч����*/
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
	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );

	TempAddress = Cam_Flash_FindPicID( id, &TempPackageHead );
	if( TempAddress == 0xFFFFFFFF )
	{
		ret = RT_ERROR;  goto lbl_cam_flash_rdpic_ret;
	}

	if( offset > ( TempPackageHead.Len - 1 ) / 512 )
	{
		ret = RT_ENOMEM; goto lbl_cam_flash_rdpic_ret;
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

lbl_cam_flash_rdpic_ret:
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

	if( DF_PicParam.Number == 0 ) /*û��ͼƬ*/
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

/*����ָ��ʱ�䷶Χ�ڵ�ͼƬͷ��
   ���γɶ�̬����ļ�¼���ܵļ�¼��

   �������Ĵ洢��ý����Ϣ�������ն���ʾͼƬ��Ϣ
   ע���������
   ע�����˳��
   ע���м���ܼ��;
 */
void* Cam_Flash_SearchPicHead( MYTIME start_time, MYTIME end_time, uint8_t channel, uint8_t trigger, uint16_t *findcount )
{
	u16					i;
	TypeDF_PackageHead	TempPackageHead;

	uint16_t			count = 0;
	uint32_t			addr;
	uint32_t			find_addr[100];         /*һ�ٸ�ͼƬ������*/

	uint8_t				*p = RT_NULL;

	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );

	addr = DF_PicParam.First.Address;           /*�����µĿ�ʼ*/
	for( i = 0; i < DF_PicParam.Number; i++ )
	{
		sst25_read( addr, (u8*)&TempPackageHead, sizeof( TempPackageHead ) );
		if( TempPackageHead.Head == CAM_HEAD )  /*��Ч����*/
		{
			if( ( TempPackageHead.Time >= start_time ) && ( TempPackageHead.Time <= end_time ) )
			{
				if( ( channel == 0 ) || ( channel == TempPackageHead.Channel_ID ) )
				{
					if( ( trigger == 0xFF ) || ( trigger == TempPackageHead.TiggerStyle ) )
					{
						find_addr[count] = addr;
						count++;
					}
				}
			}
			addr += ( TempPackageHead.Len + DF_CAM_REC_COUNT - 1 ) & DF_CAM_REC_MASK;
		}else
		{
			rt_kprintf( "\n���ݴ���" );
			addr += DF_CAM_REC_COUNT;
		}
	}

	if( count == 0 ) /*û���ҵ���¼*/
	{
		goto lbl_search_pichead_end;
	}
	p = rt_malloc( count * sizeof( TypeDF_PackageHead ) );
	if( p == RT_NULL )
	{
		goto lbl_search_pichead_end;
	}
/*˳����д�ҵ��ļ�¼*/
	for( i = 0; i < count; i++ )
	{
		sst25_read( find_addr[i], p + i * sizeof( TempPackageHead ), sizeof( TempPackageHead ) );
	}

lbl_search_pichead_end:
	rt_sem_release( &sem_dataflash );
	*findcount = count;
	return p;
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
	Cam_Flash_InitPara( 1 );

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
	u8 Take_photo[10] = { 0x40, 0x40, 0x61, 0x81, 0x02, 0X00, 0X00, 0X02, 0X0D, 0X0A };                             //----  ������������
	Take_photo[4]	= (u8)Cam_ID;
	Take_photo[5]	= (u8)( Cam_ID >> 8 );
	RS485_write( Take_photo, 10 );
	//uart2_rxbuf_rd = uart2_rxbuf_wr;
	rt_kprintf( "\n%d>����=%d", rt_tick_get( ), Cam_ID );
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
	rt_kprintf( "\n%d>����=%d", rt_tick_get( ), Cam_ID );
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
	return DF_PicParam;
}

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
	static uint32_t				photo_tick;
	static TypeDF_PackageHead	pack_head;
	static uint8_t				chn_id = 0;

	switch( Current_Cam_Para.State )
	{
		case CAM_NONE:
			if( RT_EOK == rt_mq_recv( &mq_Cam, (void*)&Current_Cam_Para.Para, sizeof( Style_Cam_Requset_Para ), RT_WAITING_NO ) )
			{
				Current_Cam_Para.State				= CAM_IDLE;
				Current_Cam_Para.Para.start_tick	= rt_tick_get( );
				if( Current_Cam_Para.Para.Channel_ID == 0xFF ) /*��·����*/
				{
					chn_id = 1;
				}
			}else
			{
				return 0;
			}
		case CAM_IDLE:
			if( ( rt_tick_get( ) - Current_Cam_Para.Para.start_tick ) >= Current_Cam_Para.Para.PhoteSpace )
			{
				Current_Cam_Para.Retry	= 0;
				Current_Cam_Para.State	= CAM_START;
			}
			break;
		case CAM_START: /*��ʼ����*/
			cam_page_num	= 0;
			cam_photo_size	= 0;
			photo_tick		= rt_tick_get( );

			Cam_Start_Cmd( chn_id );

			pack_head.Channel_ID	= chn_id;
			pack_head.TiggerStyle	= Current_Cam_Para.Para.TiggerStyle;
			pack_head.Media_Format	= 0;
			pack_head.Media_Style	= 0;
			pack_head.Time			= mytime_now;
			pack_head.Head			= CAM_HEAD;
			pack_head.State			= 0xFF;
			pack_head.id			= DF_PicParam.Last.Data_ID;
			memcpy( (uint8_t*)&( pack_head.position ), (uint8_t*)&gps_baseinfo, 28 );

			Current_Cam_Para.State		= CAM_RX_PHOTO;
			Current_Cam_Para.Rx_State	= RX_IDLE;
			break;
		case CAM_GET_PHOTO:
			photo_tick = rt_tick_get( );
			Cam_Read_Cmd( chn_id );
			Current_Cam_Para.State		= CAM_RX_PHOTO;
			Current_Cam_Para.Rx_State	= RX_IDLE;
			break;
		case CAM_RX_PHOTO:
			if( 1 == Camera_RX_Data( &RxLen ) )
			{
				photo_tick = rt_tick_get( );
				if( photo_rx_wr > 512 ) ///���ݴ���512,�Ƿ�
				{
					rt_kprintf( "\nCAM%d invalided\n", chn_id );
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

				memcpy( photo_ram + cam_photo_size + 64, photo_rx, photo_rx_wr );   /*�������ݵ�sram*/
				cam_page_num++;
				cam_photo_size += photo_rx_wr;

				if( cam_last_page )                                                 ///���һ�����ݣ���Ҫ������д�롣
				{
					pack_head.Len = cam_photo_size + 64;
					Current_Cam_Para.Para.PhotoNum++;		/*����һ��ͼƬ*/
					Cam_Flash_SavePic( &pack_head );
					Current_Cam_Para.State = CAM_OK;
				}else
				{
					Current_Cam_Para.State = CAM_GET_PHOTO;
				}
				photo_rx_wr = 0;
			}else if( rt_tick_get( ) - photo_tick > RT_TICK_PER_SECOND * 5 ) ///�ж��Ƿ�ʱ������������Ҫʱ���Գ�
			{
				Current_Cam_Para.Retry++;
				if( Current_Cam_Para.Retry >= 3 )
				{
					Current_Cam_Para.State = CAM_FALSE;
					rt_kprintf( "\n%d>����chn_%dʧ��", rt_tick_get( ), chn_id );
					Current_Cam_Para.Retry = 0;
				}else
				{
					Current_Cam_Para.State = CAM_START;
				}
			}
			break;
		case CAM_OK:
			rt_kprintf( "\n%d>���ճɹ�!", rt_tick_get( ) );
			getpicpara( );
			Current_Cam_Para.Para.start_tick = rt_tick_get( );

			if( Current_Cam_Para.Para.cb_response_cam_ok != RT_NULL )   ///���õ�����Ƭ���ճɹ��ص�����
			{
				Current_Cam_Para.Para.cb_response_cam_ok( &Current_Cam_Para.Para, pack_head.id );
			}else ///Ĭ�ϵĻص�����
			{
				Cam_response_ok( &Current_Cam_Para.Para, pack_head.id );
			}
		case CAM_FALSE:                                                 /*���ܳɹ�ʧ�ܶ�Ҫ����Ƿ����꣬����ʧ��Ҫ��Ҫ�ص�����ϵ�*/
			if( Current_Cam_Para.Para.Channel_ID == 0xFF )              /*��·����*/
			{
				if( chn_id == 4 )                                       /*��·���*/
				{
					chn_id = 0;
				}
				chn_id++;
			}
			if( Current_Cam_Para.Para.PhotoNum >= Current_Cam_Para.Para.PhotoTotal )
			{
				Current_Cam_Para.State = CAM_END;                       ///�����������
			}else
			{
				Current_Cam_Para.State = CAM_IDLE;                      ///������Ƭ�������
			}
			break;

		case CAM_END:
			rt_kprintf( "\n%d>����%x����!", rt_tick_get( ), Current_Cam_Para.Para.Channel_ID );
			Current_Cam_Para.State = CAM_NONE;
			if( Current_Cam_Para.Para.cb_response_cam_end != RT_NULL )  ///���õ�����Ƭ���ճɹ��ص�����
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
void Cam_takepic( u16 chn_id, u8 save, u8 send, CAM_TRIGGER trige )
{
	Cam_takepic_ex( chn_id, 1, 0, save, send, trige );
}

FINSH_FUNCTION_EXPORT( Cam_takepic, take_pic );


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

/************************************** The End Of File **************************************/
