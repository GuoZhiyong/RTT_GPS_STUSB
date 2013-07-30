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
#ifndef SED1520_H_
#define SED1520_H_

// define to disable busy-delays (useful for simulation)
//// #define LCD_SIM (1)
// define to enable additional debugging functions
//// #define LCD_DEBUG (1)

#include <stdint.h>
#include "menu_include.h"

/* draw modes */
#define LCD_MODE_CLEAR	0
#define LCD_MODE_SET	1
#define LCD_MODE_XOR	2
#define LCD_MODE_INVERT 3

/* command function equates for SED1520 LCD Display Controller */
#define LCD_DISP_OFF	0xAE    /* turn LCD panel OFF */
#define LCD_DISP_ON		0xAF    /* turn LCD panel ON */
#define LCD_SET_LINE	0xC0    /* set line for COM0 (4 lsbs = ST3:ST2:ST1:ST0) */
#define LCD_SET_PAGE	0xB8    /* set page address (2 lsbs = P1:P0) */
#define LCD_SET_COL		0x00    /* set column address (6 lsbs = Y4:Y4:Y3:Y2:Y1:Y0) */
#define LCD_SET_ADC_NOR 0xA0    /* ADC set for normal direction */
#define LCD_SET_ADC_REV 0xA1    /* ADC set for reverse direction */
#define LCD_STATIC_OFF	0xA4    /* normal drive */
#define LCD_STATIC_ON	0xA5    /* static drive (power save) */
#define LCD_DUTY_16		0xA8    /* driving duty 1/16 */
#define LCD_DUTY_32		0xA9    /* driving duty 1/32 */
#define LCD_SET_MODIFY	0xE0    /* start read-modify-write mode */
#define LCD_CLR_MODIFY	0xEE    /* end read-modify-write mode */
#define LCD_RESET		0xE2    /* soft reset command */

/* LCD screen and bitmap image array consants */
#define LCD_X_BYTES 122
#define LCD_Y_BYTES 4
#define SCRN_LEFT	0
#define SCRN_TOP	0
#define SCRN_RIGHT	121
#define SCRN_BOTTOM 31

#define LCD_STARTCOL_REVERSE 19

void lcd_init( void );


void lcd_fill( const unsigned char pattern );


void lcd_update( const unsigned char top, const unsigned char bottom );


void lcd_update_all( void );


void lcd_bitmap( const uint8_t left, const uint8_t top, IMG_DEF *img_ptr, const uint8_t mode );


void lcd_text12( char left, char top, char *pinfo, char len, const char mode );

void lcd_drawline( int x1, int y1, int x2, int y2, const char mode );


#endif
/************************************** The End Of File **************************************/
