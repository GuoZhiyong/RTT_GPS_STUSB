#ifndef _H_JT808_AREA
#define _H_JT808_AREA
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
rt_err_t area_jt808_0x8600(uint8_t linkno,uint8_t *pmsg);
rt_err_t area_jt808_0x8601(uint8_t linkno,uint8_t *pmsg);
rt_err_t area_jt808_0x8602(uint8_t linkno,uint8_t *pmsg);
rt_err_t area_jt808_0x8603(uint8_t linkno,uint8_t *pmsg);
rt_err_t area_jt808_0x8604(uint8_t linkno,uint8_t *pmsg);
rt_err_t area_jt808_0x8605(uint8_t linkno,uint8_t *pmsg);
rt_err_t area_jt808_0x8606(uint8_t linkno,uint8_t *pmsg);
rt_err_t area_jt808_0x8607(uint8_t linkno,uint8_t *pmsg);

/*计算两点间距离*/
u32 dis_Point2Point( s32 Lati_1, s32 Longi_1, s32 Lati_2, s32 Longi_2 );

void area_init(void);
void area_process(void);
u32  area_get_alarm(u8 *pdestbuf,u16* destbuflen);
#endif
/************************************** The End Of File **************************************/

