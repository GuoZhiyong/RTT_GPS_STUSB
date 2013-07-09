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
#include "jt808_misc.h"
#include "jt808.h"
#include "sst25.h"

#define TEXTMSG_START	0x60000
#define TEXTMSG_SECTORS 1
#define TEXTMSG_END	(TEXTMSG_START+TEXTMSG_SECTORS*4096)




typedef __packed struct
{
	uint32_t	mn;		/*���� TEXT*/
	uint32_t 	id;
	MYTIME		datetime;
	uint8_t		flag;
}TEXTMSG_HEAD;

MYTIME textmsg_mytime=0xFFFFFFFF;  /*��ǰ��Ϣ��ʱ��*/
uint32_t textmsg_addr_visit=TEXTMSG_START; /*��ǰҪ���ʵ���Ϣ��ַ*/
uint32_t textmsg_addr_curr=TEXTMSG_START; /*��ǰ����Ϣ*/




/*�ı���Ϣ����*/
uint8_t jt808_textmsg_put( uint8_t* info,uint16_t len )
{
/*�����¼ͷ*/

/*�����¼����*/
}

/*��ȡһ���ı���Ϣ*/
uint8_t jt808_textmsg_get( void )
{


}



/*�յ������·���0x8300��Ϣ*/
void jt808_misc_0x8300( uint8_t *pmsg )
{
	uint8_t		* psrc	= pmsg;
	uint16_t	len		= ( ( psrc[2] << 8 ) | psrc[3] ) & 0x3FF;
	uint8_t		flag	= psrc[12];
	if( flag & 0x01 )   /*������ֱ�ӵ���*/
	{
	}
	if( flag & 0x04 )   /*�ն���ʾ����ʾ,ָʾ��δ����Ϣ*/
	{
		jt808_textmsg_put(psrc+12,len);
	
	}
	if( flag & 0x08 )   /*TTS����*/
	{
	}
	if( flag & 0x10 )   /*�����*/
	{
	}
	if( flag & 0x20 )   /*����*/
	{
	}
}

/**/
uint8_t text_msg_init( void )
{
}

/*�绰��*/
uint8_t jt808_phonebook_init( void )
{
}

/*��ʼ����*/
void jt808_misc_init( void )
{
}

/************************************** The End Of File **************************************/
