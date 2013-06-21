#ifndef _H_JT808_CAMERA
#define _H_JT808_CAMERA
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


rt_err_t Cam_jt808_0x801(u32 mdeia_id ,u8 media_delete);
rt_err_t Cam_jt808_0x800(u32 mdeia_id ,u8 media_delete);
rt_err_t Cam_jt808_0x8801(uint8_t linkno,uint8_t *pmsg);
rt_err_t Cam_jt808_0x8802(uint8_t linkno,uint8_t *pmsg);
rt_err_t Cam_jt808_0x8803(uint8_t linkno,uint8_t *pmsg);

#endif
/************************************** The End Of File **************************************/
