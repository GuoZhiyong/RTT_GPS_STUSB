#ifndef _CAMERAPRO_H_
#define _CAMERAPRO_H_


//#include "uffs_types.h"


#define nop 

typedef __packed struct
{
 u8 years;
 u8 months;
 u8 days;
 u8 hours;
 u8 minutes;
 u8 seconds;
} T_TIMES;



typedef enum
{
	Cam_TRIGGER_PLANTFORM=0,	///ƽ̨�·�
	Cam_TRIGGER_TIMER,			///��ʱ����
	Cam_TRIGGER_ROBBERY,		///���ٱ���
	Cam_TRIGGER_HIT,			///��ײ
	Cam_TRIGGER_OPENDOR,		///����
	Cam_TRIGGER_CLOSEDOR,		///����
	Cam_TRIGGER_LOWSPEED,		///���ٳ���20����
	Cam_TRIGGER_SPACE,			///����������
	Cam_TRIGGER_OTHER,			///����
}Cam_Trigger;


typedef  __packed struct
{
	Cam_Trigger	TiggerStyle;		///�������յ��ź�Դ
	u16			Channel_ID;			///����ͨ����ID��
	u16			PhotoTotal;			///��Ҫ���յ�������
	u16			PhotoNum;			///��ǰ���ճɹ�������
	u16			PhoteSpace;			///���ռ������λΪ��
	u8			SendPhoto;			///���ս������Ƿ��ͣ�1��ʾ���ͣ�0��ʾ������
	u8			SavePhoto;			///���ս������Ƿ񱣴棬1��ʾ���棬0��ʾ������
}Style_Cam_Requset_Para;





extern rt_err_t df_rd_pic(void *pData,u16 *len, u32 id,u8 offset );
extern rt_err_t Take_Pic_Requset( Style_Cam_Requset_Para *para);
extern void 	Cam_init( void );
extern u8 		Camera_Process(void);
#endif
