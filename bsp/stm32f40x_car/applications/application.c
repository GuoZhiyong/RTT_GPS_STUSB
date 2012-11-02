/*
 * File      : application.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2009-01-05     Bernard      the first version
 */

/**
 * @addtogroup STM32
 */
/*@{*/

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



void rt_init_thread_entry(void* parameter)
{

  //  rt_usb_host_init();
  //  rt_hw_susb_init();
  //  rt_thread_delay(50);
  //  rt_device_init_all();

  while(1)
  {
    rt_thread_delay(RT_TICK_PER_SECOND/2);
  }
}


ALIGN(RT_ALIGN_SIZE)
static char thread_led1_stack[1024];
struct rt_thread thread_led1;
static void rt_thread_entry_led1(void* parameter)
{
    GPIO_InitTypeDef  GPIO_InitStructure;

    /* GPIOD Periph clock enable */

    int i=0;
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

    /* Configure PD12, PD13, PD14 and PD15 in output pushpull mode */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13| GPIO_Pin_14| GPIO_Pin_15;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOD, &GPIO_InitStructure);

    while (1)
    {
        rt_thread_delay(RT_TICK_PER_SECOND/2);
        GPIO_ResetBits(GPIOD, GPIO_Pin_12|GPIO_Pin_13|GPIO_Pin_14);
        rt_thread_delay(RT_TICK_PER_SECOND/2);
        GPIO_SetBits(GPIOD, GPIO_Pin_12|GPIO_Pin_13|GPIO_Pin_14);
		//rt_kprintf("%d\n",rt_tick_get());
        
    }
}



ALIGN(RT_ALIGN_SIZE)
static char thread_usbmsc_stack[1024];
struct rt_thread thread_usbmsc;
static void rt_thread_entry_usbmsc(void* parameter)
{
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




int rt_application_init()
{
    rt_thread_t tid;


    tid = rt_thread_create("init",
                            rt_init_thread_entry, RT_NULL,
                            2048, RT_THREAD_PRIORITY_MAX-2, 20);


    if (tid != RT_NULL)  rt_thread_startup(tid);

    //------- init led1 thread
    rt_thread_init(&thread_led1,
                   "led1",
                   rt_thread_entry_led1,
                   RT_NULL,
                   &thread_led1_stack[0],
                   sizeof(thread_led1_stack),11,5);
    rt_thread_startup(&thread_led1);



    rt_thread_init(&thread_usbmsc,
                   "usbmsc",
                   rt_thread_entry_usbmsc,
                   RT_NULL,
                   &thread_usbmsc_stack[0],
                   sizeof(thread_usbmsc_stack),8,5);
    rt_thread_startup(&thread_usbmsc);

	

    return 0;
}

/*@}*/
