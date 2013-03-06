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
#include <lcd\menu_include.h>
#include "jt808.h"

/* 消息队列控制块 */
static struct rt_messagequeue hmi2app808_mq;   // 消息队列
static u8 hmi2app808_pool[64]; 

static struct rt_messagequeue hmi2gsm_mq;   // 消息队列  
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

#define KEY_MENU_PORT	GPIOC
#define KEY_MENU_PIN	GPIO_Pin_8

#define KEY_DOWN_PORT		GPIOA
#define KEY_DOWN_PIN		GPIO_Pin_8

#define KEY_OK_PORT		GPIOC
#define KEY_OK_PIN		GPIO_Pin_9

#define KEY_UP_PORT	GPIOD
#define KEY_UP_PIN	GPIO_Pin_3

/*
定义按键状态
bit0
*/
static uint32_t keystatus=0;

static int  keycheck(void)
{
if(!GPIO_ReadInputDataBit(KEY_MENU_PORT,KEY_MENU_PIN)
	{
	KeyCheck_Flag[0]++;
	if(KeyCheck_Flag[0]==2)
		KeyValue=1;
	}
else
	KeyCheck_Flag[0]=0;

if(!GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_9))
	{
	KeyCheck_Flag[1]++;
	if(KeyCheck_Flag[1]==2)
		KeyValue=2;
	}
else
	KeyCheck_Flag[1]=0;

if(!GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_8))
	{
	KeyCheck_Flag[2]++;
	if(KeyCheck_Flag[2]==2)
		KeyValue=3;
	}
else
	KeyCheck_Flag[2]=0;

if(!GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_8))
	{
	KeyCheck_Flag[3]++;
	if(KeyCheck_Flag[3]==2)
		KeyValue=4;
	}
else
	KeyCheck_Flag[3]=0;
}


static void key_lcd_port_init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	 
	 
	 RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	 RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	 RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
	 RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	 RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
	
	 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	 GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	
	 GPIO_InitStructure.GPIO_Pin = KEY_UP_PIN;
	 GPIO_Init(KEY_UP_PORT, &GPIO_InitStructure);
	 
	 GPIO_InitStructure.GPIO_Pin = KEY_DOWN_PIN;
	 GPIO_Init(KEY_DOWN_PORT, &GPIO_InitStructure);
	
	 GPIO_InitStructure.GPIO_Pin = KEY_OK_PIN;
	 GPIO_Init(KEY_OK_PORT, &GPIO_InitStructure);
	
	 GPIO_InitStructure.GPIO_Pin = KEY_MENU_PIN;
	 GPIO_Init(KEY_MENU_PORT, &GPIO_InitStructure);
	
	
	 //OUT	(/MR  SHCP	 DS   STCP	 STCP)	 
	 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12| GPIO_Pin_13| GPIO_Pin_14|GPIO_Pin_15;
	 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	 GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	 GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	 GPIO_Init(GPIOE, &GPIO_InitStructure);  

}



ALIGN( RT_ALIGN_SIZE )
static char thread_hmi_stack[2048];
struct rt_thread thread_hmi;
/*hmi线程*/
static void rt_thread_entry_hmi( void* parameter )
{

	key_lcd_port_init();
	lcd_init();

	pMenuItem = &Menu_1_bdupgrade;
	pMenuItem->show( );
	while( 1 )
	{
		pMenuItem->timetick( 10 );  // 每个子菜单下 显示的更新 操作  时钟源是 任务执行周期
		pMenuItem->keypress( 10 );  //每个子菜单的 按键检测  时钟源100ms timer
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
