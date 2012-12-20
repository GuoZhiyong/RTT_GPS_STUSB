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
#include <board.h>
#include <rtthread.h>
#include "stm32f4xx.h"
#include "jt808.h"


static struct rt_mailbox	mb_gprsdata;
#define MB_GPRSDATA_POOL_SIZE 32
static uint8_t				mb_gprsdata_pool[MB_GPRSDATA_POOL_SIZE];

static struct rt_mailbox	mb_gpsdata;
#define MB_GPSDATA_POOL_SIZE 32
static uint8_t				mb_gpsdata_pool[MB_GPSDATA_POOL_SIZE];


typedef struct 
{
	uint16_t id;
	uint16_t len;
	uint8_t *pbody;
}TSendMsg;


uint32_t jt808_alarm=0x0;
uint32_t jt808_status=0x0;


TEXT_INFO 	TextInfo;
MSG_TEXT    TEXT_Obj;
MSG_TEXT    TEXT_Obj_8[8],TEXT_Obj_8bak[8];
//------- �¼� ----
EVENT          EventObj;    // �¼�   
EVENT          EventObj_8[8]; // �¼�  
//------ ����  --------
CENTRE_ASK  ASK_Centre;  // ��������

//------  ��Ϣ�㲥  ---
MSG_BRODCAST   MSG_BroadCast_Obj;    // ��Ϣ�㲥         
MSG_BRODCAST   MSG_Obj_8[8];  // ��Ϣ�㲥    

//------  �绰��  -----
PHONE_BOOK    PhoneBook,Rx_PhoneBOOK;   //  �绰��
PHONE_BOOK    PhoneBook_8[8];

MULTIMEDIA   MediaObj;      // ��ý����Ϣ 


//------- ��������״̬ ---------------
uint8_t  CarLoadState_Flag=1;//ѡ�г���״̬�ı�־   1:�ճ�   2:���   3:�س�
uint8_t		Warn_Status[4]		=
{
		0x00, 0x00,0x00,0x00
}; //  ������־λ״̬��Ϣ

//----------- �г���¼�����  -----------------
Avrg_MintSpeed  Avrgspd_Mint; 
uint32_t         PerMinSpdTotal=0; //��¼ÿ�����ٶ�����  
uint8_t          avgspd_Mint_Wr=0;       // ��дÿ����ƽ���ٶȼ�¼�±�
uint8_t          avgspd_Sec_Wr=0;       // ��дÿ����ƽ���ٶȼ�¼�±�
uint8_t          avgWriteOver=0;   // д�����־λ
uint8_t          AspdCounter=0;    // ÿ�����ٶ���Ч���������� 


//-----  ISP    Զ��������� -------
u8       f_ISP_ACK=0;   // Զ������Ӧ��	
u8       ISP_FCS[2];    //  �·���У��
u16      ISP_total_packnum=0;  // ISP  �ܰ���
u16      ISP_current_packnum=0;// ISP  ��ǰ����
u32      ISP_content_fcs=0;    // ISP  ������У��
u8       ISP_ack_resualt=0;    // ISP ��Ӧ
u8       ISP_rxCMD=0;          // ISP �յ�������
u8       f_ISP_88_ACK=0;       // Isp  ����Ӧ��
u8       ISP_running_state=0;  // Isp  ��������״̬
u8       f_ISP_23_ACK=0;    //  Isp  ���� �ļ���ɱ�ʶ
u16      ISP_running_counter=0;// Isp  ����״̬�Ĵ���
u8       ISP_RepeatCounter=0; //   ISP ���������ظ����� ����5��У��ʧ�ܲ�������


u8          APN_String[30]="UNINET"; //"CMNET";   //  �ӱ����ͨ  �ƶ��Ŀ�

u8  Camera_Number;
u8	Duomeiti_sdFlag; 


/*
   ����״̬ά��
   jt808Э�鴦��

 */
ALIGN( RT_ALIGN_SIZE )
static char thread_jt808_stack[512];
struct rt_thread thread_jt808;


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static void rt_thread_entry_jt808( void* parameter )
{
	rt_err_t	ret;
	uint8_t		*pstr;

	while( 1 )
	{
/*����gprs��Ϣ*/
		ret = rt_mb_recv( &mb_gprsdata, (rt_uint32_t*)&pstr, 5 );
		if( ret == RT_EOK )
		{
			jt808_analy(pstr);
			rt_free( pstr );
		}
/*����gps��Ϣ*/		
		ret = rt_mb_recv(&mb_gpsdata,(rt_uint32_t*)&pstr,5);
		if(ret == RT_EOK)
		{
			gps_analy(pstr);
			rt_free(pstr);
		}
/*ά����·*/
		rt_thread_delay( RT_TICK_PER_SECOND / 20 );
	}
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
void jt808_init( void )
{
	rt_mb_init( &mb_gprsdata, "gprsdata", &mb_gprsdata_pool, MB_GPRSDATA_POOL_SIZE / 4, RT_IPC_FLAG_FIFO );
	rt_mb_init( &mb_gpsdata, "gpsdata", &mb_gpsdata_pool, MB_GPSDATA_POOL_SIZE / 4, RT_IPC_FLAG_FIFO );

	rt_thread_init( &thread_jt808,
	                "jt808",
	                rt_thread_entry_jt808,
	                RT_NULL,
	                &thread_jt808_stack[0],
	                sizeof( thread_jt808_stack ), 10, 5 );
	rt_thread_startup( &thread_jt808 );
}


void gps_rx(uint8_t *pinfo,uint16_t length)
{
	uint8_t *pmsg;
	pmsg= rt_malloc(length+2);
	if(pmsg!=RT_NULL)
	{
		pmsg[0] = length >>8;
		pmsg[1] = length &0xff;
		memcpy(pmsg+2,pinfo,length);
		rt_mb_send(&mb_gpsdata,(rt_uint32_t)pmsg);
	}
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
void gprs_rx( uint8_t * pinfo, uint16_t length )
{
	uint8_t *pmsg;
	pmsg = rt_malloc( length + 2 );      /*����������Ϣ*/
	if( pmsg != RT_NULL )
	{
		pmsg[0] = length >> 8;
		pmsg[1] = length & 0xff;
		memcpy( pmsg + 2, pinfo, length );
		rt_mb_send( &mb_gprsdata, (rt_uint32_t)pmsg );
	}
}

/************************************** The End Of File **************************************/
