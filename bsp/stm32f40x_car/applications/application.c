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


void rt_init_thread_entry(void* parameter)
{

  //  rt_usb_host_init();
  //  rt_hw_susb_init();
  //  rt_thread_delay(50);
  //  rt_device_init_all();
  printer_driver_init();
  gsm_init(NULL);

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
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD|RCC_AHB1Periph_GPIOE, ENABLE);

    /* Configure PD12, PD13, PD14 and PD15 in output pushpull mode */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOD, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_Init(GPIOE, &GPIO_InitStructure);


    while (1)
    {
        rt_thread_delay(RT_TICK_PER_SECOND/2);
        GPIO_ResetBits(GPIOD, GPIO_Pin_9);
        rt_thread_delay(RT_TICK_PER_SECOND/2);
        GPIO_SetBits(GPIOD, GPIO_Pin_9);
		//rt_kprintf("%d\n",rt_tick_get());
        
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

	

    return 0;
}

/*@}*/
