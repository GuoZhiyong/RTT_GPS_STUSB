
#include <stdio.h>

#include "stm32f4xx.h"
#include <board.h>
#include <rtthread.h>


static struct rt_mailbox	mb_gprsdata;
#define MB_GPRSDATA_POOL_SIZE	32
static uint8_t mb_gprsdata_pool[MB_GPRSDATA_POOL_SIZE];


ALIGN( RT_ALIGN_SIZE )
static char thread_jt808_stack[512];
struct rt_thread thread_jt808;







/*
连接状态维护
jt808协议处理

*/
static void rt_thread_entry_jt808( void* parameter )
{
	rt_err_t ret;
	uint8_t *pstr;
	uint16_t len;
	while( 1 )
	{
		ret = rt_mb_recv( &mb_gprsdata, (rt_uint32_t*)&pstr, 10 );
		if( ret == RT_EOK )
		{
			len=(pstr[0]<<8)|pstr[1];
			rt_kprintf("gsm>rx %d bytes\r\n",len);
			rt_free(pstr);
			
		}
		
		rt_thread_delay(RT_TICK_PER_SECOND/10);
	}
}




void jt808_init(void)
{
	rt_mb_init(&mb_gprsdata,"gprsdata",&mb_gprsdata_pool,MB_GPRSDATA_POOL_SIZE/4,RT_IPC_FLAG_FIFO);

	rt_thread_init( &thread_jt808,
	                "jt808",
	                rt_thread_entry_jt808,
	                RT_NULL,
	                &thread_jt808_stack[0],
	                sizeof( thread_jt808_stack ), 10, 5 );
	rt_thread_startup( &thread_jt808 );
}



void gps_rx(uint8_t *pinfo,uint16_t len)
{


}


void gprs_rx(uint8_t *pinfo,uint16_t len)
{
	uint8_t *pmsg; 

	pmsg=rt_malloc(len+2); /*包含长度信息*/
	if(pmsg!=RT_NULL)
	{
		pmsg[0]=len>>8;
		pmsg[1]=len&0xff;
		memcpy(pmsg+2,pinfo,len);
		rt_mb_send(&mb_gprsdata,(rt_uint32_t)pmsg);
	}
}




