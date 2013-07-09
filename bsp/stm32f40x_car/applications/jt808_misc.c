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

#define TEXTMSG_START	0x60000
#define TEXTMSG_SECTORS 1
#define TEXTMSG_END	(TEXTMSG_START+TEXTMSG_SECTORS*4096)




typedef __packed struct
{
	uint32_t	mn;		/*幻数 TEXT*/
	uint32_t 	id;
	MYTIME		datetime;
	uint8_t		flag;
}TEXTMSG_HEAD;

MYTIME textmsg_mytime=0xFFFFFFFF;  /*当前消息的时刻*/
uint32_t textmsg_addr_visit=TEXTMSG_START; /*当前要访问的消息地址*/
uint32_t textmsg_addr_curr=TEXTMSG_START; /*当前的消息*/




/*文本信息保存*/
uint8_t jt808_textmsg_put( uint8_t* info,uint16_t len )
{
/*保存记录头*/

/*保存记录内容*/
}

/*读取一条文本信息*/
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
		jt808_textmsg_put(psrc+12,len);
	
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

/**/
uint8_t text_msg_init( void )
{
}

/*电话簿*/
uint8_t jt808_phonebook_init( void )
{
}

/*初始化吧*/
void jt808_misc_init( void )
{
}

/************************************** The End Of File **************************************/
