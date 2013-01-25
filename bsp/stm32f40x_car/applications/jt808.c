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
#include "gsm.h"
#include "jt808.h"
#include "dlist.h"


#define MULTI_PROCESS


static struct rt_mailbox	mb_gprsdata;
#define MB_GPRSDATA_POOL_SIZE 32
static uint8_t				mb_gprsdata_pool[MB_GPRSDATA_POOL_SIZE];

static struct rt_mailbox	mb_gpsdata;
#define MB_GPSDATA_POOL_SIZE 32
static uint8_t				mb_gpsdata_pool[MB_GPSDATA_POOL_SIZE];



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



static rt_device_t pdev_gsm=RT_NULL;


JT808_PARAM jt808_param;

/*���������б�*/
DList* jt808_msg_list;


/*
����ÿ��Ҫ������Ϣ��״̬
���������д�����?
*/
static DListRet jt808_msg_tx_proc(void* ctx, void* data)
{
	JT808_MSG_NODE * pdata=(JT808_MSG_NODE *)data;
	uint32_t *res=ctx;

	if(pdata->state==IDLE) /*���У�������Ϣ��ʱ��û������*/
	{
		if(pdata->retry==pdata->max_retry)	/*�Ѿ��ﵽ���Դ���*/
		{
			*res=1;	/*��ʾ����ʧ��*/
		}
		else
		{
			pdata->retry++;
			rt_device_write(pdev_gsm,0,pdata->pmsg,pdata->msg_len);
			pdata->tick=rt_tick_get();
			pdata->timeout=pdata->max_retry*pdata->timeout;
			pdata->state=WAIT_ACK;
			rt_kprintf("send retry=%d,timeout=%d\r\n",pdata->retry,pdata->timeout);
			*res=0;	
		}	
		return DLIST_RET_OK;
	}
	
	if(pdata->state==WAIT_ACK)
	{
		if(rt_tick_get()-pdata->tick>pdata->timeout)
		{
			pdata->state=IDLE;
		}
		*res=0;	
	}
	return DLIST_RET_OK;
}

/*
���Ľ��յ���Ϣ�Ĵ�����Ҫ��Ӧ��Ĵ���
*ctx:���ݽ����յ�����Ϣ
*/
static DListRet jt808_msg_rx_proc(void* ctx, void* data)
{
	JT808_MSG_NODE * pdata=(JT808_MSG_NODE *)data;
	

	return DLIST_RET_OK;
}



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
	uint32_t	gsm_status;
	DListNode* iter;

	jt808_msg_list=dlist_create();
	pdev_gsm=rt_device_find("gsm");  /*û�г�����,δ�ҵ���ô��*/
	iter=jt808_msg_list->first;

/*��ȡ������������*/	

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
		rt_device_control(pdev_gsm,CTL_STATUS,&gsm_status);
	

#ifdef MULTI_PROCESS
/*�ദ��*/		
		dlist_foreach(jt808_msg_list,jt808_msg_tx_proc,RT_NULL);
/*��������*/
#else
		jt808_msg_tx_proc(RT_NULL,jt808_msg_list->first->data);
	
#endif		
		rt_thread_delay( RT_TICK_PER_SECOND / 20 );
	}

	dlist_destroy(jt808_msg_list);
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
void gprs_rx( uint8_t linkno,uint8_t * pinfo, uint16_t length )
{
	uint8_t *pmsg;
	pmsg = rt_malloc( length + 3 );      /*����������Ϣ*/
	if( pmsg != RT_NULL )
	{
		pmsg[0] = linkno;
		pmsg[1] = length >> 8;
		pmsg[2] = length & 0xff;
		memcpy( pmsg + 3, pinfo, length );
		rt_mb_send( &mb_gprsdata, (rt_uint32_t)pmsg );
	}
}

/************************************** The End Of File **************************************/
