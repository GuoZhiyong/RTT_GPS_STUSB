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
#ifndef _H_VDR_
#define _H_VDR_
#include "stm32f4xx.h"
#include <rtthread.h>


void vdr_rx_gps(void);
void vdr_init( void );

void vdr_rx_8700(uint8_t *pmsg);
void vdr_rx_8701(uint8_t *pmsg);


#endif
/************************************** The End Of File **************************************/
