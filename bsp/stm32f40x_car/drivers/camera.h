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
	Cam_TRIGGER_PLANTFORM = 0,                                                              ///ƽ̨�·�
	Cam_TRIGGER_TIMER,                                                                      ///��ʱ����
	Cam_TRIGGER_ROBBERY,                                                                    ///���ٱ���
	Cam_TRIGGER_HIT,                                                                        ///��ײ
	Cam_TRIGGER_OPENDOR,                                                                    ///����
	Cam_TRIGGER_CLOSEDOR,                                                                   ///����
	Cam_TRIGGER_LOWSPEED,                                                                   ///���ٳ���20����
	Cam_TRIGGER_SPACE,                                                                      ///����������
	Cam_TRIGGER_OTHER,                                                                      ///����
}CAM_TRIGGER;

typedef __packed struct
{
	u32 Address;                                                                            ///��ַ
	u32 Len;                                                                                ///����
	u32 Data_ID;                                                                            ///����ID
}TypeDF_PackageInfo;

typedef __packed struct
{
	u16					Number;                                                             ///ͼƬ����
	TypeDF_PackageInfo	First;                                                           ///��һ��ͼƬ
	TypeDF_PackageInfo	Last;                                                            ///���һ��ͼƬ
}TypeDF_PICPara;

typedef  __packed struct _Style_Cam_Requset_Para
{
	CAM_TRIGGER TiggerStyle;                                                                ///�������յ��ź�Դ
	u16			Channel_ID;                                                                 ///����ͨ����ID��
	u16			PhotoTotal;                                                                 ///��Ҫ���յ�������
	u16			PhotoNum;                                                                   ///��ǰ���ճɹ�������
	u32			PhoteSpace;                                                                 ///���ռ������λΪ�ں�TICK
	u8			SendPhoto;                                                                  ///���ս������Ƿ��ͣ�1��ʾ���ͣ�0��ʾ������
	u8			SavePhoto;                                                                  ///���ս������Ƿ񱣴棬1��ʾ���棬0��ʾ������
	u32			start_tick;                                                                 ///��Ƭ���տ�ʼʱ��
	void		*user_para;                                                                 ///���û��ص�������ص����ݲ���
	void ( *cb_response_cam_ok )( struct _Style_Cam_Requset_Para *para, uint32_t pic_id );  ///һ����Ƭ���ճɹ��ص�����
	void ( *cb_response_cam_end )( struct _Style_Cam_Requset_Para *para );                  ///������Ƭ���ս����ص�����
}Style_Cam_Requset_Para;

typedef __packed struct
{
	u32		Head;                                                                             ///�������֣���ʾ��ǰ��������Ϊĳ�̶����ݿ�ʼ
	u32		id;                                                                             ///����ID,˳�������ʽ��¼
	u32		Len;                                                                            ///���ݳ��ȣ���������ͷ��������,����ͷ���̶ֹ�Ϊ64�ֽ�
	u8		State;                                                                          ///��ʾͼƬ״̬��ǣ�0xFFΪ��ʼ��״̬��bit0==0��ʾ�Ѿ�ɾ��,bit1==0��ʾ�ɹ��ϴ�,bit2==0��ʾ������Ϊ����������
	MYTIME	Time;                                                                           ///��¼���ݵ�ʱ�䣬BCD���ʾ��YY-MM-DD-hh-mm-ss
	u8		Media_Format;                                                                   ///0��JPEG��1��TIF��2��MP3��3��WAV��4��WMV�� ��������
	u8		Media_Style;                                                                    ///��������:0��ͼ��1����Ƶ��2����Ƶ��
	u8		Channel_ID;                                                                     ///����ͨ��ID
	u8		TiggerStyle;                                                                    ///������ʽ
	u8		position[28];                                                                   ///λ����Ϣ
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
