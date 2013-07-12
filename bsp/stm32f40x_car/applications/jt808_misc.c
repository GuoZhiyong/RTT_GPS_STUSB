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
#include <string.h>
#include "jt808_misc.h"
#include "jt808.h"
#include "jt808_gps.h"
#include "sst25.h"

#define TEXTMSG_START		0x3B3000
#define TEXTMSG_SECTORS		2
#define TEXTMSG_BLOCK_SIZE	256
#define TEXTMSG_END			( TEXTMSG_START + TEXTMSG_SECTORS * 4096 )

#define EVENT_START		( TEXTMSG_END )
#define EVENT_SECTORS	1
#define EVENT_END		( EVENT_START + EVENT_SECTORS * 4096 )

#define ASK_START	( EVENT_END )
#define ASK_SECTORS 2
#define ASK_END		( ASK_START + ASK_SECTORS * 4096 )

uint32_t	textmsg_curr_addr;  /*��ǰд���������ݵ���Ϣ��ַ*/
uint32_t	textmsg_curr_id;
uint8_t		textmsg_count = 0;

#if 0
struct _sector_info
{
	uint32_t	start;          /*��ʼ��ַ*/
	uint8_t		sectors;        /*ռ�õ�������*/
	uint8_t		block_size;     /*ÿ����¼���С*/
	uint32_t	addr_wr;        /*��ǰд���ַ*/
	uint32_t	addr_rd;        /*��ǰ������ַ*/
} sector_info[4] =
{
	{ TEXTMSG_START, TEXTMSG_SECTORS, 128, TEXTMSG_END, TEXTMSG_END },
};
#endif


/*
   �ı���Ϣ����
   Ҫ��Ҫ��֤һ����Ϣ��һ�������ڣ��������������
   �������棬������infoͷ�����洢TEXTMSG_HEAD,������д������
 */
uint8_t jt808_textmsg_put( uint8_t* pinfo )
{
	uint8_t		* psrc	= pinfo;
	uint16_t	len		= ( ( psrc[2] << 8 ) | psrc[3] ) & 0x3FF - 1; /*�и���־�ֽ�*/
	uint8_t		flag	= psrc[12];

	uint32_t	addr;
	uint8_t		count, count_need;

	TEXTMSG		textmsg;

	memset( (uint8_t*)&textmsg, 0, 256 );

	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );
	textmsg_curr_addr += 256; /*��λ����һ��*/
	if( textmsg_curr_addr >= TEXTMSG_END )
	{
		textmsg_curr_addr = TEXTMSG_START;
	}
	textmsg_curr_id++;
	textmsg.id			= textmsg_curr_id;
	textmsg.datetime	= mytime_now;
	if( len > ( 256 - 9 ) )
	{
		len = 256 - 9;
	}
	textmsg.len = len;

/*�����¼,������ʼ�ı�־λ*/
	memcpy( textmsg.body, pinfo + 13, len );
/*�����¼����*/
	sst25_write_back( textmsg_curr_addr, (uint8_t*)&textmsg, 256 );
	rt_sem_release( &sem_dataflash );

	if( textmsg_count <= TEXTMSG_SECTORS * 4096 / 256 )
	{
		textmsg_count++;
	}
}

/*
   ��ȡһ���ı���Ϣ
   �Ե�ǰΪ��׼��ƫ�Ƶļ�¼��
   ����ֵ 0:û�ҵ�
    1:�ҵ�
 */
void jt808_textmsg_get( uint8_t index, TEXTMSG* pout )
{
	uint8_t		count;
	uint32_t	addr;
	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );

	addr = textmsg_curr_addr;
	for( count = 0; count < index; count++ )
	{
		if( addr == TEXTMSG_START )
		{
			addr = TEXTMSG_END;
		}
		addr -= 256;
	}
	sst25_read( addr, (uint8_t*)pout, 256 );
	rt_sem_release( &sem_dataflash );
}

/*�յ������·���0x8300��Ϣ*/
void jt808_misc_0x8300( uint8_t *pmsg )
{
	uint8_t		flag	= pmsg[12];
	uint16_t	id		= ( ( pmsg[0] << 8 ) | pmsg[1] );
	uint16_t	seq		= ( ( pmsg[10] << 8 ) | pmsg[11] );
	uint16_t	len		= ( ( pmsg[2] << 8 ) | pmsg[3] ) & 0x3FF;
	uint8_t		* ptts, *p, *psrc;
	uint16_t	i;
	if( flag & 0x01 )               /*������ֱ�ӵ���*/
	{
	}
	if( flag & 0x04 )               /*�ն���ʾ����ʾ,ָʾ��δ����Ϣ*/
	{
		jt808_textmsg_put( pmsg );
	}
	if( flag & 0x08 )               /*TTS����*/
	{
#if 0
		ptts = rt_malloc( len );    /*����-��־�ֽ�=>ת��hexasc,������˽�����*/
		if( ptts != RT_NULL )
		{
			p		= ptts;
			psrc	= pmsg + 13;
			for( i = 0; i < ( len - 1 ); i++ )
			{
				*p++ = *psrc++;
			}
			*p = '\0';
			tts_write( ptts );
			rt_free( ptts );
		}
#else
		tts_write( pmsg + 13, len - 1 );
#endif
	}
	if( flag & 0x10 )   /*�����*/
	{
	}
	if( flag & 0x20 )   /*����*/
	{
	}
/*����Ӧ��*/
	jt808_tx_0x0001( seq, id, 0x0 );
}

/*��������Ԫ�أ�ִ�в���*/
void foreach_event( uint8_t *pdata )
{
	uint32_t	i;
	EVENT		*pevent;

	for( i = 0; i < 128; i++ )
	{
		pevent = (EVENT*)( pdata + 32 * i );

		if( pdata[i] == 'E' )
		{
		}
	}
}

/*�յ������·���0x8300��Ϣ*/
void jt808_misc_0x8301( uint8_t *pmsg )
{
	uint8_t		type	= pmsg[12];
	uint16_t	id		= ( ( pmsg[0] << 8 ) | pmsg[1] );
	uint16_t	seq		= ( ( pmsg[10] << 8 ) | pmsg[11] );
	uint16_t	len		= ( ( pmsg[2] << 8 ) | pmsg[3] ) & 0x3FF;
	uint8_t		* ptts, *p, *psrc;
	uint16_t	i;
	uint8_t		*pevent;
	uint8_t		res = 0;

	pevent = rt_malloc( 4096 );
	if( pevent == RT_NULL )
	{
		res = 1;
		goto lbl_end_8301;
	}

	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );
	sst25_read( EVENT_START, pevent, 4096 );

	if( type == 0 ) /*ɾ�����������¼�*/
	{
		sst25_erase_4k( EVENT_START );
	}
	if( type == 1 ) /*�����¼�*/
	{
	}
	if( type == 2 ) /*׷���¼�*/
	{
	}
	if( type == 3 ) /*�޸��¼�*/
	{
	}
	if( type == 4 ) /*ɾ���ض��¼�*/
	{
	}
	rt_sem_release( &sem_dataflash );

/*����Ӧ��*/
lbl_end_8301:
	jt808_tx_0x0001( seq, id, res );
}

/*Ҫ����һ����?*/
static jt808_ask_put( uint8_t * pinfo )
{
}


/*
   �����·�
   ������Ӧ��
 */
void jt808_misc_0x8302( uint8_t *pmsg )
{
	uint8_t		flag	= pmsg[12];
	uint16_t	id		= ( ( pmsg[0] << 8 ) | pmsg[1] );
	uint16_t	seq		= ( ( pmsg[10] << 8 ) | pmsg[11] );
	uint16_t	len		= ( ( pmsg[2] << 8 ) | pmsg[3] ) & 0x3FF;
	uint16_t	ans_len = 0;
	uint8_t		*p;
	uint16_t	ask_len = 0;
	uint16_t	pos;
	if( flag & 0x01 )                       /*������ֱ�ӵ���*/
	{
	}
	if( flag & 0x08 )                       /*TTS�������ָ�һ��,*/
	{
		tts_write( "����", 4 );
		ask_len = pmsg[13];                 /*�������ݳ���*/
		tts_write( pmsg + 14, ask_len );    /*����*/
		tts_write( "��ѡ��", 6 );
		pos = 14 + ask_len;
		while( pos < len )
		{
			p		= pmsg + pos;           /*ָ���ѡ���б�*/
			p[0]	= p[0] + 0x30;          /*��ID��Ϊ�ɲ�����*/
			ans_len = ( p[1] << 8 ) | p[2]; /*�𰸳���*/
			p[1]	= ',';                  /*�滻Ϊ���ţ��ɲ���*/
			p[2]	= ',';
			pos		= pos + ans_len + 3;    /*��3����Ϊ��ID�ʹ����ݳ���û�м���*/
		}
		tts_write( pmsg + 14 + ask_len, len - ask_len - 2 );
	}
	if( flag & 0x10 )                       /*�����*/
	{
	}
	/*����Ӧ��*/
	jt808_tx_0x0001( seq, id, 0x0 );
}

/*��Ϣ�㲥�˵�����*/
void jt808_misc_0x8303( uint8_t *pmsg )
{
}

/*��Ϣ����*/
void jt808_misc_0x8304( uint8_t *pmsg )
{
}

/*�绰�ز�*/
void jt808_misc_0x8400( uint8_t *pmsg )
{
	uint8_t		dialbuf[32];
	uint8_t		flag	= pmsg[12];
	uint16_t	len		= ( ( pmsg[2] << 8 ) | pmsg[3] ) & 0x3FF;
	strcpy(dialbuf,"ATD");
	strncpy(dialbuf+3,&pmsg[13],len-1);
	strcat(dialbuf,";\r\n");
	GPIO_ResetBits( GPIOD, GPIO_Pin_9 );             /*������*/
	at(dialbuf);
	
}

/*���õ绰��*/
void jt808_misc_0x8401( uint8_t *pmsg )
{
}

/*
   ������ǰ�ļ�¼
   256�ֽ�һ��block
   Ҫ��50��

 */
uint8_t jt808_textmsg_init( void )
{
	uint32_t	addr;
	uint32_t	id = 0;
	uint8_t		buf[16];

	textmsg_curr_addr	= TEXTMSG_END;               /*ָ�����*/
	textmsg_curr_id		= 0;
	textmsg_count		= 0;

	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );
	for( addr = TEXTMSG_START; addr < TEXTMSG_END; addr += 256 )
	{
		sst25_read( addr, buf, 16 );
		id = ( buf[0] << 24 ) | ( buf[1] << 16 ) | ( buf[2] << 8 ) | ( buf[3] );
		if( id != 0xFFFFFFFF )
		{
			textmsg_count++;
			if( id > textmsg_curr_id )
			{
				textmsg_curr_addr	= addr;
				textmsg_curr_id		= id;
			}
		}
	}
	rt_sem_release( &sem_dataflash );
}

/*�绰��*/
uint8_t jt808_phonebook_init( void )
{
}

/*��ʼ����*/
void jt808_misc_init( void )
{
	jt808_textmsg_init( );
}

/************************************** The End Of File **************************************/
