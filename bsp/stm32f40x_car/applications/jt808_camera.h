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


rt_err_t Cam_jt808_0x801(u32 mdeia_id ,u8 media_delete);
rt_err_t Cam_jt808_0x800(u32 mdeia_id ,u8 media_delete);
rt_err_t Cam_jt808_0x8801(uint8_t linkno,uint8_t *pmsg);
rt_err_t Cam_jt808_0x8802(uint8_t linkno,uint8_t *pmsg);
rt_err_t Cam_jt808_0x8803(uint8_t linkno,uint8_t *pmsg);

#endif
/************************************** The End Of File **************************************/
