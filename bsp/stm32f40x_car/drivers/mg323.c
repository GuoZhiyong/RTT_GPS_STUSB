/************************************************************
 * Copyright (C), 2008-2012,
 * FileName:		mg323
 * Author:			bitter
 * Date:			// 日期
 * Description:		// 模块描述
 * Version:			// 版本信息
 * Function List:	// 主要函数及其功能
 *     1. -------
 * History:			// 历史修改记录
 *     <author>  <time>   <version >   <desc>
 *     David    96/10/12     1.0     build this moudle
 ***********************************************************/

#ifdef MG323
 
#include <stdio.h>
#include <rtthread.h>
#include "stm32f4xx.h"

#include <finsh.h>

static struct rt_device dev_gsm;
static struct rt_timer	tmr_gsm;

struct rt_semaphore gsm_sem;


ALIGN( RT_ALIGN_SIZE )
static char thread_gsm_stack[1024];
struct rt_thread thread_gsm;


/***********************************************************
* Function:       rt_thread_entry_gsm
* Description:    接收处理，状态转换
* Input:          
* Input:          
* Output:         
* Return:         
* Others:         
***********************************************************/
static void rt_thread_entry_gsm( void* parameter )
{

}



static rt_err_t mg323_rx_ind(rt_device_t dev, rt_size_t size)
{
	rt_sem_release(&gsm_sem);
	return RT_EOK;
}



/***********************************************************
* Function:
* Description: 查找并打开gsm对应的串口设备
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static rt_err_t mg323_init( rt_device_t dev )
{
	rt_device_t dev = RT_NULL;
 	dev = rt_device_find("uart2");
	if (dev != RT_NULL && rt_device_open(dev, RT_DEVICE_OFLAG_RDWR) == RT_EOK)
	{
		rt_device_set_rx_indicate(dev, mg323_rx_ind);
	}
	else
	{
		rt_kprintf("finsh: can not find device:%s\n", device_name);
	}
	return RT_EOK;
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
static rt_err_t mg323_open( rt_device_t dev, rt_uint16_t oflag )
{
	return RT_EOK;
}

/***********************************************************
* Function:mg323_read
* Description:数据模式下读取数据
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static rt_size_t mg323_read( rt_device_t dev, rt_off_t pos, void* buff, rt_size_t count )
{
	return RT_EOK;
}


/***********************************************************
* Function:		mg323_write
* Description:	数据模式下发送数据，要对数据进行封装
* Input:		const void* buff	要发送的原始数据
				rt_size_t count		要发送数据的长度
* Output:
* Return:
* Others:
***********************************************************/

static rt_size_t mg323_write( rt_device_t dev, rt_off_t pos, const void* buff, rt_size_t count )
{
	rt_size_t ret = RT_EOK;
	return ret;
}


static rt_err_t mg323_close( rt_device_t dev )
{
	return RT_EOK;
}



/***********************************************************
* Function:		mg323_control
* Description:	控制模块
* Input:		rt_uint8_t cmd	命令类型
				void *arg 		参数,依据cmd的不同，传递的数据格式不同
* Output:
* Return:
* Others:
***********************************************************/
static rt_err_t mg323_control( rt_device_t dev, rt_uint8_t cmd, void *arg )
{
	return RT_EOK;
}


void gsm_init( void )
{
	rt_thread_t id;

	rt_sem_init(&gsm_sem,"gsm_sem",0,0);

	rt_thread_init( &thread_gsm,
	                "gsm",
	                rt_thread_entry_gsm,
	                RT_NULL,
	                &thread_gsm_stack[0],
	                sizeof( thread_gsm_stack ), 7, 5 );
	rt_thread_startup( &thread_gsm );

	rt_timer_init( &tmr_gsm, \
	               "tmr_gsm", \
	               timer_gsm_cb, NULL, \
	               50, \
	               RT_TIMER_FLAG_PERIODIC );

	dev_gsm.type		= RT_Device_Class_Char;
	dev_gsm.init		= mg323_init;
	dev_gsm.open		= mg323_open;
	dev_gsm.close		= mg323_close;
	dev_gsm.read		= mg323_read;
	dev_gsm.write		= mg323_write;
	dev_gsm.control		= mg323_control;
	dev_gsm.user_data	= RT_NULL;

	rt_device_register( &dev_gsm, "gsm", RT_DEVICE_FLAG_RDWR );
	rt_device_init( &dev_gsm );
	rt_timer_start( &tmr_gsm );
}


#endif

/************************************** The End Of File **************************************/
