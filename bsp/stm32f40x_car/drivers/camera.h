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
#ifndef _CAMERAPRO_H_
#define _CAMERAPRO_H_

//#include "uffs_types.h"
#include "jt808_util.h"

#define nop
#ifndef BIT
#define BIT( i ) ( (unsigned long)( 1 << i ) )
#endif

typedef enum
{
	Cam_TRIGGER_PLANTFORM = 0,                                                              ///平台下发
	Cam_TRIGGER_TIMER,                                                                      ///定时动作
	Cam_TRIGGER_ROBBERY,                                                                    ///抢劫报警
	Cam_TRIGGER_HIT,                                                                        ///碰撞
	Cam_TRIGGER_OPENDOR,                                                                    ///开门
	Cam_TRIGGER_CLOSEDOR,                                                                   ///关门
	Cam_TRIGGER_LOWSPEED,                                                                   ///低速超过20分钟
	Cam_TRIGGER_SPACE,                                                                      ///定距离拍照
	Cam_TRIGGER_OTHER,                                                                      ///其他
}CAM_TRIGGER;

typedef __packed struct
{
	u32 Address;                                                                            ///地址
	u32 Len;                                                                                ///长度
	u32 Data_ID;                                                                            ///数据ID
}TypeDF_PackageInfo;

typedef __packed struct
{
	u16					Number;                                                             ///图片数量
	TypeDF_PackageInfo	First;                                                           ///第一个图片
	TypeDF_PackageInfo	Last;                                                            ///最后一个图片
}TypeDF_PICPara;

typedef  __packed struct _Style_Cam_Requset_Para
{
	CAM_TRIGGER TiggerStyle;                                                                ///触发拍照的信号源
	u16			Channel_ID;                                                                 ///照相通道的ID号
	u16			PhotoTotal;                                                                 ///需要拍照的总数量
	u16			PhotoNum;                                                                   ///当前拍照成功的数量
	u32			PhoteSpace;                                                                 ///拍照间隔，单位为内核TICK
	u8			SendPhoto;                                                                  ///拍照结束后是否发送，1表示发送，0表示不发送
	u8			SavePhoto;                                                                  ///拍照结束后是否保存，1表示保存，0表示不保存
	u32			start_tick;                                                                 ///照片拍照开始时间
	void		*user_para;                                                                 ///和用户回调函数相关的数据参数
	void ( *cb_response_cam_ok )( struct _Style_Cam_Requset_Para *para, uint32_t pic_id );  ///一张照片拍照成功回调函数
	void ( *cb_response_cam_end )( struct _Style_Cam_Requset_Para *para );                  ///所有照片拍照结束回调函数
}Style_Cam_Requset_Para;

typedef __packed struct
{
	u32		Head;                                                                             ///幻数部分，表示当前数据区域为某固定数据开始
	u32		id;                                                                             ///数据ID,顺序递增方式记录
	u32		Len;                                                                            ///数据长度，包括数据头部分内容,数据头部分固定为64字节
	u8		State;                                                                          ///表示图片状态标记，0xFF为初始化状态，bit0==0表示已经删除,bit1==0表示成功上传,bit2==0表示该数据为不存盘数据
	MYTIME	Time;                                                                           ///记录数据的时间，BCD码表示，YY-MM-DD-hh-mm-ss
	u8		Media_Format;                                                                   ///0：JPEG；1：TIF；2：MP3；3：WAV；4：WMV； 其他保留
	u8		Media_Style;                                                                    ///数据类型:0：图像；1：音频；2：视频；
	u8		Channel_ID;                                                                     ///数据通道ID
	u8		TiggerStyle;                                                                    ///触发方式
	u8		position[28];                                                                   ///位置信息
}TypeDF_PackageHead;

u32 Cam_Flash_FindPicID( u32 id, TypeDF_PackageHead *p_head );


rt_err_t Cam_Flash_DelPic( u32 id );


rt_err_t Cam_Flash_TransOkSet( u32 id );


u16 Cam_Flash_SearchPic( MYTIME start_time, MYTIME end_time, TypeDF_PackageHead *para, u8 *pdest );


rt_err_t Cam_Flash_RdPic( void *pData, u16 *len, u32 id, u8 offset );


TypeDF_PICPara Cam_get_state( void );


rt_err_t take_pic_request( Style_Cam_Requset_Para *para );


void Cam_Device_init( void );


u8 Camera_Process( void );


#endif
/************************************** The End Of File **************************************/
