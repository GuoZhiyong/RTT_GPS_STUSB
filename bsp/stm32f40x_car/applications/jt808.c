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

#include "stm32f4xx.h"
#include <board.h>
#include <rtthread.h>

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
	pmsg = rt_malloc( length + 2 );      /*包含长度信息*/
	if( pmsg != RT_NULL )
	{
		pmsg[0] = length >> 8;
		pmsg[1] = length & 0xff;
		memcpy( pmsg + 2, pinfo, length );
		rt_mb_send( &mb_gprsdata, (rt_uint32_t)pmsg );
	}
}

/************************************** The End Of File **************************************/
