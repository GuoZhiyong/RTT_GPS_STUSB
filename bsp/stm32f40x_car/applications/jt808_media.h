#ifndef _JT808_MEDIA_
#define _JT808_MEDIA_






typedef __packed struct
{
	uint32_t	mn;                     /*幻数，有效的记录头*/
	uint32_t	id;                     /*流水号*/
	uint32_t	attr;					/*属性,已上报*/
	MYTIME		mytime;                 /*拍照时刻*/
	uint8_t		ch;                     /*通道*/
	CAM_TRIGGER trigger;                /*触发方式*/
}PHOTO_HEAD;



typedef __packed struct
{
	uint32_t	mn;                     /*幻数，有效的记录头*/
	uint32_t	id;                     /*流水号*/
	uint32_t	attr;					/*属性,已上报*/
	MYTIME		mytime;                 /*录音时刻*/
	CAM_TRIGGER trigger;                /*触发方式*/
}VOICE_HEAD;


#endif
