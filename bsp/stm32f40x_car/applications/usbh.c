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

#include "usbh_core.h"
#include "usbh_usr.h"
#include "usbh_msc_core.h"


ALIGN(RT_ALIGN_SIZE)

USB_OTG_CORE_HANDLE      USB_OTG_Core ;
USBH_HOST                USB_Host;




ALIGN(RT_ALIGN_SIZE)
static char thread_usbmsc_stack[1024];
struct rt_thread thread_usbmsc;
static void rt_thread_entry_usbmsc(void* parameter)
{

	while(1)
	{
        rt_thread_delay(RT_TICK_PER_SECOND);
        GPIO_ResetBits(GPIOE, GPIO_Pin_11);
        rt_thread_delay(RT_TICK_PER_SECOND);
        GPIO_SetBits(GPIOE, GPIO_Pin_11);
	}


	dfs_init();
    elm_init();

	USBH_Init(&USB_OTG_Core,
			  USB_OTG_FS_CORE_ID,
			  &USB_Host,
			  &USBH_MSC_cb, 
			  &USR_cb);
	
	rt_kprintf("\r\nUSBH_Init\r\n");

	
    while (1)
    {
    	GPIO_SetBits(GPIOD, GPIO_Pin_15);
		USBH_Process(&USB_OTG_Core,&USB_Host);
		rt_thread_delay(RT_TICK_PER_SECOND/100);
		GPIO_ResetBits(GPIOD, GPIO_Pin_15);
    }

}





void usbh_init(void)
{
    rt_thread_t tid;

    rt_thread_init(&thread_usbmsc,
                   "usbmsc",
                   rt_thread_entry_usbmsc,
                   RT_NULL,
                   &thread_usbmsc_stack[0],
                   sizeof(thread_usbmsc_stack),20,5);
    rt_thread_startup(&thread_usbmsc);
}


/************************************** The End Of File **************************************/
