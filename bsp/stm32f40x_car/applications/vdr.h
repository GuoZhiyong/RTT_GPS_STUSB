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
#ifndef _H_VDR_
#define _H_VDR_
#include "stm32f4xx.h"
#include <rtthread.h>


rt_err_t vdr_rx_gps(void);
rt_err_t vdr_init( void );

void vdr_rx_8700(uint8_t *pmsg);




#endif
/************************************** The End Of File **************************************/
