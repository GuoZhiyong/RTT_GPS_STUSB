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
#include "jt808_misc.h"
#include "jt808.h"
#include "sst25.h"

typedef __packed struct
{
	uint32_t	mn;
	MYTIME		datetime;
	uint8_t		flag;
}TEXTMSG_HEAD;


/*文本信息保存*/
uint8_t jt808_textmsg_put( char* info )
{
}

/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
uint8_t jt808_textmsg_get( void )
{
}
/*收到中心下发的0x8300信息*/
void jt808_misc_0x8300( uint8_t *pmsg )
{
	uint8_t		* psrc	= pmsg;
	uint16_t	len		= ( ( psrc[2] << 8 ) | psrc[3] ) & 0x3FF;
	uint8_t		flag	= psrc[12];
	if( flag & 0x01 )   /*紧急，直接弹出*/
	{
	}
	if( flag & 0x04 )   /*终端显示器显示,指示有未读信息*/
	{

	
	}
	if( flag & 0x08 )   /*TTS播报*/
	{
	}
	if( flag & 0x10 )   /*广告屏*/
	{
	}
	if( flag & 0x20 )   /*紧急*/
	{
	}
}

/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
uint8_t text_msg_init( void )
{
}

/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
uint8_t jt808_phonebook_init( void )
{
}

/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
void jt808_misc_init( void )
{
}

/************************************** The End Of File **************************************/
