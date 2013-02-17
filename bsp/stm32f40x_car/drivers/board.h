/*
 * File      : board.h
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2009, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2009-09-22     Bernard      add board.h to this bsp
 */

// <<< Use Configuration Wizard in Context Menu >>>
#ifndef __BOARD_H__
#define __BOARD_H__

#include <stm32f4xx.h>



/*
�ֿⶨ��
*/

#define FONT_BASE_ADDR		0x08040000

#define FONT_ASC0612_SIZE	0x480
#define FONT_HZ1212_SIZE	0x294F0
#define FONT_ASC1224_SIZE	0x1200
#define FONT_HZ2424_SIZE	0x7bed0




#define FONT_ASC0612_ADDR	(FONT_BASE_ADDR)
#define FONT_HZ1212_ADDR	(FONT_BASE_ADDR+FONT_ASC0612_SIZE)
#define FONT_ASC1224_ADDR	(FONT_BASE_ADDR+FONT_ASC0612_SIZE+FONT_HZ1212_SIZE)
#define FONT_HZ2424_ADDR	(FONT_BASE_ADDR+FONT_ASC0612_SIZE+FONT_HZ1212_SIZE+FONT_ASC1224_SIZE)



#define STORAGE_PARAM_ADDR		0x00000000
#define STORAGE_PARAM_SIZE		0x00001000	//(4K)



/* board configuration */
// <o> SDCard Driver <1=>SDIO sdcard <0=>SPI MMC card
// 	<i>Default: 1
#define STM32_USE_SDIO			0

/* whether use board external SRAM memory */
// <e>Use external SRAM memory on the board
// 	<i>Enable External SRAM memory
#define STM32_EXT_SRAM          0
//	<o>Begin Address of External SRAM
//		<i>Default: 0x68000000
#define STM32_EXT_SRAM_BEGIN    0x68000000 /* the begining address of external SRAM */
//	<o>End Address of External SRAM
//		<i>Default: 0x68080000
#define STM32_EXT_SRAM_END      0x68080000 /* the end address of external SRAM */
// </e>

// <o> Internal SRAM memory size[Kbytes] <8-64>
//	<i>Default: 64
#define STM32_SRAM_SIZE         128
#define STM32_SRAM_END          (0x20000000 + STM32_SRAM_SIZE * 1024)

#define RT_USING_UART1
//#define RT_USING_UART2
//#define RT_USING_UART3
//#define RT_USING_UART4
//#define RT_USING_UART5
//#define RT_USING_UART6





// <o> Console on USART: <0=> no console <1=>USART 1 <2=>USART 2 <3=> USART 3
// 	<i>Default: 1

void rt_hw_board_init(void);

#define CONSOLE_DEVICE "vuart"


#define FINSH_DEVICE_NAME   CONSOLE_DEVICE

void rt_hw_usart_init(void);

/* SD Card init function */
void rt_hw_msd_init(void);

#endif

// <<< Use Configuration Wizard in Context Menu >>>
