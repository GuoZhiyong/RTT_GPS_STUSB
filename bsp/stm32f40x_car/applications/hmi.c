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

#include "stm32f4xx.h"
#include <board.h>
#include <rtthread.h>
#include <lcd\menu_include.h>
#include "jt808.h"

/* ��Ϣ���п��ƿ� */
static struct rt_messagequeue hmi2app808_mq;   // ��Ϣ����
static u8 hmi2app808_pool[64]; 

static struct rt_messagequeue hmi2gsm_mq;   // ��Ϣ����  
static u8 hmi2gsm_pool[64]; 


 //  1.   MsgQueue  Rx
rt_err_t rt_Rx_hmi2gsm_MsgQue(u8 *buffer,u16 rec_len) 
{       
     if(rt_mq_recv(&hmi2gsm_mq,&buffer[0], rec_len, MsgQ_Timeout) //RT_WAITING_FOREVER
            != RT_EOK )
          return  RT_ERROR; 
    else
	   return RT_EOK;            
}

rt_err_t rt_Rx_hmi2app808_MsgQue(u8 *buffer,u16 rec_len)  
{       
     if(rt_mq_recv(&hmi2app808_mq,&buffer[0], rec_len, MsgQ_Timeout) 
            != RT_EOK )
          return  RT_ERROR; 
    else
	   return RT_EOK;            
}


ALIGN( RT_ALIGN_SIZE )
static char thread_hmi_stack[2048];
struct rt_thread thread_hmi;
/*hmi�߳�*/
static void rt_thread_entry_hmi( void* parameter )
{

	/* GPIOD Periph clock enable */
	Init_lcdkey();
	lcd_init();

	pMenuItem = &Menu_1_bdupgrade;
	pMenuItem->show( );
	while( 1 )
	{
		KeyCheckFun( );
		pMenuItem->timetick( 10 );  // ÿ���Ӳ˵��� ��ʾ�ĸ��� ����  ʱ��Դ�� ����ִ������
		pMenuItem->keypress( 10 );  //ÿ���Ӳ˵��� �������  ʱ��Դ100ms timer
		rt_thread_delay( 5 );
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
void hmi_init( void )
{
	rt_thread_t tid;



	rt_thread_init( &thread_hmi,
	                "hmi",
	                rt_thread_entry_hmi,
	                RT_NULL,
	                &thread_hmi_stack[0],
	                sizeof( thread_hmi_stack ), 17, 5 );
	rt_thread_startup( &thread_hmi );
}

/************************************** The End Of File **************************************/
