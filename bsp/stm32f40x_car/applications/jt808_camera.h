#ifndef _H_JT808_CAMERA
#define _H_JT808_CAMERA
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


rt_err_t Cam_jt808_0x0801( JT808_TX_NODEDATA *nodedata, u32 mdeia_id, u8 media_delete );

rt_err_t Cam_jt808_0x0800(u32 mdeia_id ,u8 media_delete);
rt_err_t Cam_jt808_0x8801(uint8_t linkno,uint8_t *pmsg);
rt_err_t Cam_jt808_0x8802(uint8_t linkno,uint8_t *pmsg);
rt_err_t Cam_jt808_0x8803(uint8_t linkno,uint8_t *pmsg);
rt_err_t Cam_jt808_0x8805(uint8_t linkno,uint8_t *pmsg);

void cam_ok( struct _Style_Cam_Requset_Para *para, uint32_t pic_id );
void cam_end( struct _Style_Cam_Requset_Para *para );


#endif
/************************************** The End Of File **************************************/
