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
//------- 事件 ----
EVENT          EventObj;    // 事件   
EVENT          EventObj_8[8]; // 事件  
//------ 提问  --------
CENTRE_ASK  ASK_Centre;  // 中心提问

//------  信息点播  ---
MSG_BRODCAST   MSG_BroadCast_Obj;    // 信息点播         
MSG_BRODCAST   MSG_Obj_8[8];  // 信息点播    

//------  电话本  -----
PHONE_BOOK    PhoneBook,Rx_PhoneBOOK;   //  电话本
PHONE_BOOK    PhoneBook_8[8];

MULTIMEDIA   MediaObj;      // 多媒体信息 


//------- 车辆负载状态 ---------------
uint8_t  CarLoadState_Flag=1;//选中车辆状态的标志   1:空车   2:半空   3:重车
uint8_t		Warn_Status[4]		=
{
		0x00, 0x00,0x00,0x00
}; //  报警标志位状态信息

//----------- 行车记录仪相关  -----------------
Avrg_MintSpeed  Avrgspd_Mint; 
uint32_t         PerMinSpdTotal=0; //记录每分钟速度总数  
uint8_t          avgspd_Mint_Wr=0;       // 填写每分钟平均速度记录下标
uint8_t          avgspd_Sec_Wr=0;       // 填写每秒钟平均速度记录下标
uint8_t          avgWriteOver=0;   // 写溢出标志位
uint8_t          AspdCounter=0;    // 每分钟速度有效报数计数器 


//-----  ISP    远程下载相关 -------
u8       f_ISP_ACK=0;   // 远程升级应答	
u8       ISP_FCS[2];    //  下发的校验
u16      ISP_total_packnum=0;  // ISP  总包数
u16      ISP_current_packnum=0;// ISP  当前包数
u32      ISP_content_fcs=0;    // ISP  的内容校验
u8       ISP_ack_resualt=0;    // ISP 响应
u8       ISP_rxCMD=0;          // ISP 收到的命令
u8       f_ISP_88_ACK=0;       // Isp  内容应答
u8       ISP_running_state=0;  // Isp  程序运行状态
u8       f_ISP_23_ACK=0;    //  Isp  返回 文件完成标识
u16      ISP_running_counter=0;// Isp  运行状态寄存器
u8       ISP_RepeatCounter=0; //   ISP 单包发送重复次数 超过5次校验失败擦除区域


u8          APN_String[30]="UNINET"; //"CMNET";   //  河北天地通  移动的卡

u8  Camera_Number;
u8	Duomeiti_sdFlag; 



static rt_device_t pdev_gsm=RT_NULL;


JT808_PARAM jt808_param;

/*发送命令列表*/
DList* jt808_msg_list;


/*
处理每个要发送信息的状态
现在允许并行处理吗?
*/
static DListRet jt808_msg_tx_proc(void* ctx, void* data)
{
	JT808_MSG_NODE * pdata=(JT808_MSG_NODE *)data;
	uint32_t *res=ctx;

	if(pdata->state==IDLE) /*空闲，发送信息或超时后没有数据*/
	{
		if(pdata->retry==pdata->max_retry)	/*已经达到重试次数*/
		{
			*res=1;	/*表示发送失败*/
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
中心接收到消息的处理，主要是应答的处理
*ctx:传递进来收到的信息
*/
static DListRet jt808_msg_rx_proc(void* ctx, void* data)
{
	JT808_MSG_NODE * pdata=(JT808_MSG_NODE *)data;
	

	return DLIST_RET_OK;
}



/*
   连接状态维护
   jt808协议处理

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
	pdev_gsm=rt_device_find("gsm");  /*没有出错处理,未找到怎么办*/
	iter=jt808_msg_list->first;

/*读取参数，并配置*/	

	while( 1 )
	{
/*接收gprs信息*/
		ret = rt_mb_recv( &mb_gprsdata, (rt_uint32_t*)&pstr, 5 );
		if( ret == RT_EOK )
		{
			jt808_analy(pstr);
			rt_free( pstr );
		}
/*接收gps信息*/		
		ret = rt_mb_recv(&mb_gpsdata,(rt_uint32_t*)&pstr,5);
		if(ret == RT_EOK)
		{
			gps_analy(pstr);
			rt_free(pstr);
		}
/*维护链路*/
		rt_device_control(pdev_gsm,CTL_STATUS,&gsm_status);
	

#ifdef MULTI_PROCESS
/*多处理*/		
		dlist_foreach(jt808_msg_list,jt808_msg_tx_proc,RT_NULL);
/*逐条处理*/
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
	pmsg = rt_malloc( length + 3 );      /*包含长度信息*/
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
