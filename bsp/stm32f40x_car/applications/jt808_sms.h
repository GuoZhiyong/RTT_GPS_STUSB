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
#ifndef _H_SMS
#define _H_SMS

#define SMS_TYPE_PDU

#define GSM_7BIT	0x00
#define GSM_UCS2	0x08

#define  SMS_ACK_msg          1      // ��Ӵ���ض�Ϣ
#define  SMS_ACK_none         0      // ����Ҫ���ض�Ϣ

#ifndef true
#define true	1
#define false	0
#endif


/////////////////////////////////////////////////////////////////////////////////////////////////
///�ӿں���
/////////////////////////////////////////////////////////////////////////////////////////////////
extern rt_err_t sms_init(void);
/*
jt808���̵߳���
*/
void SMS_Process(void);
/*m66�����̵߳���*/
u8 	SMS_rx_pro(char *psrc,u16 len);
#endif
/************************************** The End Of File **************************************/
