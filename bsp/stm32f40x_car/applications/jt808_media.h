#ifndef _JT808_MEDIA_
#define _JT808_MEDIA_






typedef __packed struct
{
	uint32_t	mn;                     /*��������Ч�ļ�¼ͷ*/
	uint32_t	id;                     /*��ˮ��*/
	uint32_t	attr;					/*����,���ϱ�*/
	MYTIME		mytime;                 /*����ʱ��*/
	uint8_t		ch;                     /*ͨ��*/
	CAM_TRIGGER trigger;                /*������ʽ*/
}PHOTO_HEAD;



typedef __packed struct
{
	uint32_t	mn;                     /*��������Ч�ļ�¼ͷ*/
	uint32_t	id;                     /*��ˮ��*/
	uint32_t	attr;					/*����,���ϱ�*/
	MYTIME		mytime;                 /*¼��ʱ��*/
	CAM_TRIGGER trigger;                /*������ʽ*/
}VOICE_HEAD;


#endif
