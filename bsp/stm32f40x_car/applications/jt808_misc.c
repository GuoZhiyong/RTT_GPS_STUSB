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
#include "menu_include.h"

/*�ı���Ϣ*/
#define TEXTMSG_START		0x34000
#define TEXTMSG_SECTORS		2
#define TEXTMSG_BLOCK_SIZE	256
#define TEXTMSG_END			( TEXTMSG_START + TEXTMSG_SECTORS * 4096 )

/*�¼�����*/
#define EVENT_START		( TEXTMSG_END )
#define EVENT_SECTORS	1
#define EVENT_END		( EVENT_START + EVENT_SECTORS * 4096 )

/*��������*/
#define CENTER_ASK_START	( EVENT_END )
#define CENTER_ASK_SECTORS	2
#define CENTER_ASK_END		( CENTER_ASK_START + CENTER_ASK_SECTORS * 4096 )

/*��Ϣ�㲥*/
#define INFO_ONDEMAND_START		( CENTER_ASK_END )
#define INFO_ONDEMAND_SECTORS	1
#define INFO_ONDEMAND_END		( INFO_ONDEMAND_START + INFO_ONDEMAND_SECTORS * 4096 )

/*�绰��*/
#define PHONEBOOK_START		( INFO_ONDEMAND_END )
#define PHONEBOOK_SECTORS	1
#define PHONEBOOK_END		( PHONEBOOK_START + PHONEBOOK_SECTORS * 4096 )

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

uint32_t	textmsg_curr_addr;  /*��ǰд���������ݵ���Ϣ��ַ*/
uint32_t	textmsg_curr_id;
uint8_t		textmsg_count = 0;


/*
   �ı���Ϣ����
   Ҫ��Ҫ��֤һ����Ϣ��һ�������ڣ��������������
   �������棬������infoͷ�����洢TEXTMSG_HEAD,������д������
 */
void jt808_textmsg_put( uint8_t* pinfo )
{
	uint8_t		* psrc	= pinfo;
	uint16_t	len		= ( ( ( psrc[2] << 8 ) | psrc[3] ) & 0x3FF ) - 1;   /*�и���־�ֽ�*/

	TEXTMSG		textmsg;

	memset( (uint8_t*)&textmsg, 0, 256 );

	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );
	textmsg_curr_addr += 256;                                               /*��λ����һ��*/
	if( textmsg_curr_addr >= TEXTMSG_END )
	{
		textmsg_curr_addr = TEXTMSG_START;
	}
	textmsg_curr_id++;
	textmsg.id			= textmsg_curr_id;
	textmsg.datetime	= mytime_now;
	if( len > ( 256 - 10 ) )
	{
		len = 256 - 10;
	}
	textmsg.len		= len;
	textmsg.flag	= 1; /*δ��*/

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
uint32_t jt808_textmsg_get( uint8_t index, TEXTMSG* pout )
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
	return addr;
}

/*�յ������·���0x8300��Ϣ*/
void jt808_misc_0x8300( uint8_t *pmsg )
{
	uint8_t		flag	= pmsg[12];
	uint16_t	id		= ( ( pmsg[0] << 8 ) | pmsg[1] );
	uint16_t	seq		= ( ( pmsg[10] << 8 ) | pmsg[11] );
	uint16_t	len		= ( ( pmsg[2] << 8 ) | pmsg[3] ) & 0x3FF;
	jt808_textmsg_put( pmsg );

	if( flag & 0x01 )   /*������ֱ�ӵ���*/
	{
		Menu_2_3_CentreTextStor.parent	= pMenuItem;
		pMenuItem						= &Menu_2_3_CentreTextStor;
		pMenuItem->show( );
	}
	if( flag & 0x04 )   /*�ն���ʾ����ʾ,ָʾ��δ����Ϣ*/
	{
	}
	if( flag & 0x08 )   /*TTS����*/
	{
		tts_write( (char*)( pmsg + 13 ), len - 1 );
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

/*��������ָ��ָ���ָ�������ݲ��� ;-( */
uint8_t* event_buf = RT_NULL;


/*
   ͨ��id�����¼�
   ����ҵ��ˣ������ҵ��ĵ�ַ
   û�ҵ������ص�һ�����õĵ�ַ�����û�п��ã�����0xFFFF
 */
uint8_t event_find_by_id( uint8_t *peventbuf, uint8_t id, uint16_t *addr )
{
	uint8_t		*pinfo = peventbuf;
	uint32_t	i;
	EVENT		*pevent;

	*addr = 0xFFFF;
	for( i = 0; i < 4096; i += 64 )
	{
		pevent = (EVENT*)( pinfo + i );
		if( pevent->flag == 'E' )
		{
			if( pevent->id == id )
			{
				*addr = i;
				return 1;
			}
		}else /*��һ�����õĵ�ַ*/
		{
			if( *addr == 0xFFFF )
			{
				*addr = i;
			}
		}
	}
	return 0;
}

/*
   ��ȡȫ���¼�,��̬����.
   ���� ��¼��
 */
uint8_t jt808_event_get( void )
{
	uint8_t		count = 0;
	uint8_t		buf[16];
	uint32_t	addr;

	if( event_buf != RT_NULL )
	{
		rt_free( event_buf );
	}
	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );
	for( addr = EVENT_START; addr < EVENT_END; addr += 64 )
	{
		sst25_read( addr, buf, 16 );
		if( buf[0] == 'E' )
		{
			count++;
		}else
		{
			break;
		}
	}

	if( count )
	{
		event_buf = rt_malloc( count * 64 );
		if( event_buf == RT_NULL )
		{
			count = 0;
		}else
		{
			sst25_read( EVENT_START, event_buf, count * 64 );
		}
	}
	rt_sem_release( &sem_dataflash );
	return count;
}

/*�յ������·���0x8301�¼�������Ϣ*/
void jt808_misc_0x8301( uint8_t *pmsg )
{
	uint8_t		type	= pmsg[12];
	uint16_t	id		= ( ( pmsg[0] << 8 ) | pmsg[1] );
	uint16_t	seq		= ( ( pmsg[10] << 8 ) | pmsg[11] );
	uint16_t	i, j;
	uint8_t		* pdata;
	uint8_t		*pevtbuf;
	uint8_t		count, res = 0, len = 0;
	uint16_t	addr;
	uint16_t	tmpbuf[64];

	pevtbuf = rt_malloc( 4096 );
	if( pevtbuf == RT_NULL )
	{
		res = 1;
		goto lbl_end_8301;
	}
	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );
	sst25_read( EVENT_START, pevtbuf, 4096 );

	if( type == 0 )                                 /*ɾ�����������¼�*/
	{
		sst25_erase_4k( EVENT_START );
		goto lbl_end_8301;
	}else if( type == 4 )                           /*ɾ���ض��¼�*/
	{
		pdata = pmsg + 14;
		for( count = 0; count < pmsg[13]; count++ ) /*�¼�����*/
		{
			if( event_find_by_id( pevtbuf, *pdata, &addr ) )
			{
				memset( pevtbuf + addr, 0xff, sizeof( EVENT ) );
			}
			pdata++;
		}
	}else /*1.���� 2 ׷�� 3 �޸� ����idһ���ľ͸��£�û�еľ�����*/
	{
		pdata = pmsg + 14;
		for( count = 0; count < pmsg[13]; count++ ) /*�¼�����*/
		{
			len = pdata[1];
			event_find_by_id( pevtbuf, *pdata, &addr );
			if( addr != 0xFFFF ) /*�����ҵ�û�ҵ��������и����õĵ�ַ*/
			{
				if( len > 62 )
				{
					len = 62;
				}
				memset( pevtbuf + addr, 0, 64 );
				pevtbuf[addr] = 'E';
				memcpy( pevtbuf + addr + 1, pdata, len + 2 );
			}
			pdata += ( len + 2 );
		}
	}
/*��������,ȥ�����е�*/
	addr	= 0xFFFF;               /*��һ��Ϊ�յĵ�ַ*/
	count	= 0;                    /*��¼��*/
	for( i = 0; i < 4096; i += 64 )
	{
		if( pevtbuf[i] == 'E' )     /*����Ч��ַ*/
		{
			count++;
			if( addr != 0xFFFF )    /*�пյ�*/
			{
				memcpy( pevtbuf + addr, pevtbuf + i, sizeof( EVENT ) );
				memset( pevtbuf + i, 0xFF, sizeof( EVENT ) );
				addr = i;           /*��Ϊ�µ�δ�õ�*/
			}
		}else /*û���õĵ�ַ*/
		{
			if( addr == 0xFFFF )
			{
				addr = i;
			}
		}
	}


/*����
   for(int i = 0;i<cnt;i++)
   {
   for(j=0;j<cnt-1;j++)
   {
        if(a[j]>a[j+1])
        {
               tmp = a[j];
               a[j] = a[j+1];
               a[j+1] = tmp;
     }
   }
   }
 */
	for( i = 0; i < count; i++ )
	{
		for( j = 0; j < ( count - 1 ); j++ )
		{
			if( pevtbuf[j * 64 + 1] > pevtbuf[j * 64 + 65] )
			{
				memcpy( tmpbuf, pevtbuf + j * 64, 64 );
				memcpy( pevtbuf + j * 64, pevtbuf + j * 64 + 64, 64 );
				memcpy( pevtbuf + j * 64 + 64, tmpbuf, 64 );
			}
		}
	}
	sst25_write_back( EVENT_START, pevtbuf, 4096 );

lbl_end_8301:
	rt_sem_release( &sem_dataflash );
	jt808_tx_0x0001( seq, id, res ); /*����Ӧ��*/
}

/***********************************************************
* Function:
* Description:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
uint8_t jt808_event_init( void )
{
	return RT_EOK;
}

uint32_t	center_ask_curr_addr; /*��ǰд���������ݵ���Ϣ��ַ*/
uint32_t	center_ask_curr_id;
uint8_t		center_ask_count = 0;


/*
   �ı���Ϣ����
   Ҫ��Ҫ��֤һ����Ϣ��һ�������ڣ��������������
   �������棬������infoͷ�����洢TEXTMSG_HEAD,������д������
 */
void jt808_center_ask_put( uint8_t* pinfo )
{
	uint8_t		* psrc	= pinfo;
	uint16_t	len		= ( ( ( psrc[2] << 8 ) | psrc[3] ) & 0x3FF ) - 1;   /*�и���־�ֽ�*/

	CENTER_ASK	center_ask_msg;

	memset( (uint8_t*)&center_ask_msg, 0, 256 );

	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );
	center_ask_curr_addr += 256;                                            /*��λ����һ��*/
	if( center_ask_curr_addr >= CENTER_ASK_END )
	{
		center_ask_curr_addr = CENTER_ASK_START;
	}
	center_ask_curr_id++;
	center_ask_msg.id		= center_ask_curr_id;
	center_ask_msg.datetime = mytime_now;
	center_ask_msg.seq[0]	= pinfo[10];
	center_ask_msg.seq[1]	= pinfo[11];
	if( len > ( 256 - 12 ) )
	{
		len = 256 - 12;
	}
	center_ask_msg.len	= len;
	center_ask_msg.flag = 1; /*δ�ش�*/

/*�����¼,������ʼ�ı�־λ*/
	memcpy( center_ask_msg.body, pinfo + 13, len );
/*�����¼����*/
	sst25_write_back( center_ask_curr_addr, (uint8_t*)&center_ask_msg, 256 );
	rt_sem_release( &sem_dataflash );

	if( center_ask_count <= CENTER_ASK_SECTORS * 4096 / 256 )
	{
		center_ask_count++;
	}
}

/*
   ��ȡһ���ı���Ϣ
   �Ե�ǰΪ��׼��ƫ�Ƶļ�¼��
   ����ֵ ��ǰ�����ĵ�ַ�����ڲ��������
 */
uint32_t jt808_center_ask_get( uint8_t index, CENTER_ASK* pout )
{
	uint8_t		count;
	uint32_t	addr;
	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );

	addr = center_ask_curr_addr;
	for( count = 0; count < index; count++ )
	{
		if( addr == CENTER_ASK_START )
		{
			addr = CENTER_ASK_END;
		}
		addr -= 256;
	}
	sst25_read( addr, (uint8_t*)pout, 256 );
	rt_sem_release( &sem_dataflash );
	return addr;
}

/*
   �����·�
   ������Ӧ��
 */
void jt808_misc_0x8302( uint8_t *pmsg )
{
	uint8_t		flag	= pmsg[12];
	uint16_t	len		= ( ( pmsg[2] << 8 ) | pmsg[3] ) & 0x3FF;
	uint16_t	ans_len = 0;
	uint8_t		*p;
	uint16_t	ask_len = 0;
	uint16_t	ans_id	= 0;
	uint16_t	pos;
	char		buf[255];
	uint8_t		i;

	jt808_center_ask_put( pmsg );

	if( flag & 0x08 )                               /*TTS�������ָ�һ��,*/
	{
		tts_write( "�����·�����", 12 );
		ask_len = pmsg[13];                         /*�������ݳ���*/
		tts_write( (char*)( pmsg + 14 ), ask_len ); /*����*/
		tts_write( "��ѡ��", 6 );
		pos = 2 + ask_len;                          /*ָ����ע��ƫ�ƿ�ʼ��ַ������+����*/
		rt_kprintf( "pos=%d len=%d\n", pos, len );
		while( pos < len )
		{
			p		= pmsg + 12 + pos;              /*ָ���ѡ���б�*/
			ans_id	= p[0];                         /*��ID��Ϊ�ɲ�����*/
			ans_len = ( p[1] << 8 ) | p[2];         /*�𰸳���*/
			memset( buf, 0, 255 );
			i = sprintf( buf, "%d,", ans_id );
			strncpy( buf + i, (char*)( p + 3 ), ans_len );
			tts_write( buf, ans_len + i );

			pos = pos + ans_len + 3;                /*��3����Ϊ��ID�ʹ����ݳ���û�м���*/
		}
		//tts_write( (char*)( pmsg + 14 + ask_len ), len - ask_len - 2 );
	}
	if( flag & 0x01 )                               /*������ֱ�ӵ���*/
	{
		Menu_3_1_CenterQuesSend.parent	= pMenuItem;
		pMenuItem						= &Menu_3_1_CenterQuesSend;
		pMenuItem->show( );
	}
	if( flag & 0x10 )                               /*�����*/
	{
	}
}

uint8_t* info_buffer;

/*��Ϣ�㲥�˵�����*/
uint8_t info_find_by_id( uint8_t *peventbuf, uint8_t id, uint16_t *addr )
{
	uint8_t		*pinfo = peventbuf;
	uint32_t	i;
	EVENT		*pevent;

	*addr = 0xFFFF;
	for( i = 0; i < 4096; i += 64 )
	{
		pevent = (EVENT*)( pinfo + i );
		if( pevent->flag == 'I' )
		{
			if( pevent->id == id )
			{
				*addr = i;
				return 1;
			}
		}else /*��һ�����õĵ�ַ*/
		{
			if( *addr == 0xFFFF )
			{
				*addr = i;
			}
		}
	}
	return 0;
}

/*��Ϣ�㲥�˵�����*/
void jt808_misc_0x8303( uint8_t *pmsg )
{
	uint8_t		type	= pmsg[12];
	uint16_t	id		= ( ( pmsg[0] << 8 ) | pmsg[1] );
	uint16_t	seq		= ( ( pmsg[10] << 8 ) | pmsg[11] );
	uint16_t	i, j;
	uint8_t		* pdata;
	uint8_t		*pevtbuf;
	uint8_t		count, res = 0;
	uint16_t	len = 0;
	uint16_t	addr;
	uint16_t	tmpbuf[64];

	pevtbuf = rt_malloc( 4096 );
	if( pevtbuf == RT_NULL )
	{
		res = 1;
		goto lbl_end_8303;
	}

	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );
	sst25_read( INFO_ONDEMAND_START, pevtbuf, 4096 );

	if( type == 0 )                                 /*ɾ�����������¼�*/
	{
		sst25_erase_4k( INFO_ONDEMAND_START );
		goto lbl_end_8303;
	}else if( type == 4 )                           /*ɾ���ض��¼�*/
	{
		pdata = pmsg + 14;
		for( count = 0; count < pmsg[13]; count++ ) /*�¼�����*/
		{
			if( info_find_by_id( pevtbuf, *pdata, &addr ) )
			{
				memset( pevtbuf + addr, 0xff, sizeof( EVENT ) );
			}
			pdata++;
		}
	}else if( type == 1 )                           /*1.���� ɾ�����еģ�׷���µ�*/
	{
		memset( pevtbuf, 0, 4096 );
		pdata = pmsg + 14;
		for( count = 0; count < pmsg[13]; count++ ) /*�¼�����*/
		{
			len = ( pdata[1] << 8 ) | pdata[2];
			if( len > 62 )                          /*�ض̣��ճ�β����0*/
			{
				len = 62;
			}
			pevtbuf[count * 64] = 'I';
			memcpy( pevtbuf + count * 64 + 1, pdata, len );
			pdata += ( len + 3 );
		}
	}else /* 2 ׷�� 3 �޸� ����idһ���ľ͸��£�û�еľ�����*/
	{
		pdata = pmsg + 14;
		for( count = 0; count < pmsg[13]; count++ ) /*�¼�����*/
		{
			len = pdata[1];
			len = ( pdata[1] << 8 ) | pdata[2];
			info_find_by_id( pevtbuf, *pdata, &addr );
			if( addr != 0xFFFF ) /*�����ҵ�û�ҵ��������и����õĵ�ַ*/
			{
				if( len > 62 )
				{
					len = 62;
				}
				pevtbuf[addr] = 'I';
				memcpy( pevtbuf + addr + 1, pdata, len );
			}
			pdata += ( len + 3 );
		}
	}
	/*��������,ȥ�����е�*/
	addr	= 0xFFFF;               /*��һ��Ϊ�յĵ�ַ*/
	count	= 0;                    /*��¼��*/
	for( i = 0; i < 4096; i += 64 )
	{
		if( pevtbuf[i] == 'I' )     /*����Ч��ַ*/
		{
			count++;
			if( addr != 0xFFFF )    /*�пյ�*/
			{
				memcpy( pevtbuf + addr, pevtbuf + i, sizeof( EVENT ) );
				memset( pevtbuf + i, 0xFF, sizeof( EVENT ) );
				addr = i;           /*��Ϊ�µ�δ�õ�*/
			}
		}else /*û���õĵ�ַ*/
		{
			if( addr == 0xFFFF )
			{
				addr = i;
			}
		}
	}


	/*����
	   for(int i = 0;i<cnt;i++)
	   {
	   for(j=0;j<cnt-1;j++)
	   {
	   if(a[j]>a[j+1])
	   {
	      tmp = a[j];
	      a[j] = a[j+1];
	      a[j+1] = tmp;
	   }
	   }
	   }
	 */
	for( i = 0; i < count; i++ )
	{
		for( j = 0; j < ( count - 1 ); j++ )
		{
			if( pevtbuf[j * 64 + 1] > pevtbuf[j * 64 + 65] )
			{
				memcpy( tmpbuf, pevtbuf + j * 64, 64 );
				memcpy( pevtbuf + j * 64, pevtbuf + j * 64 + 64, 64 );
				memcpy( pevtbuf + j * 64 + 64, tmpbuf, 64 );
			}
		}
	}
	sst25_write_back( INFO_ONDEMAND_START, pevtbuf, 4096 );

lbl_end_8303:
	rt_sem_release( &sem_dataflash );
	jt808_tx_0x0001( seq, id, res ); /*����Ӧ��*/
}

/*��Ϣ����*/
void jt808_misc_0x8304( uint8_t *pmsg )
{
}

uint8_t* phonebook_buf;


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
uint8_t jt808_phonebook_get( void )
{
	uint8_t		count = 0;
	uint8_t		buf[16];
	uint32_t	addr;

	if( phonebook_buf != RT_NULL )
	{
		rt_free( phonebook_buf );
	}
	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );
	for( addr = PHONEBOOK_START; addr < PHONEBOOK_END; addr += 64 )
	{
		sst25_read( addr, buf, 16 );
		if( buf[0] == 'P' )
		{
			count++;
		}else
		{
			break;
		}
	}

	if( count )
	{
		phonebook_buf = rt_malloc( count * 64 );
		if( phonebook_buf == RT_NULL )
		{
			count = 0;
		}else
		{
			sst25_read( PHONEBOOK_START, phonebook_buf, count * 64 );
		}
	}
	rt_sem_release( &sem_dataflash );
	return count;
}

/*�绰�ز�*/
void jt808_misc_0x8400( uint8_t *pmsg )
{
	char		dialbuf[64];
	uint16_t	seq = ( pmsg[10] << 8 ) | pmsg[11];
	uint16_t	len = ( ( pmsg[2] << 8 ) | pmsg[3] ) & 0x3FF;

	jt808_tx_0x0001( seq, 0x8400, 0 );          /*����Ӧ��*/

	strcpy( dialbuf, "ATD" );
	strncpy( dialbuf + 3, (char*)( pmsg + 13 ), len - 1 );
	strcat( dialbuf, ";\r\n" );
	if( pmsg[12] == 0 )                         /*��ͨͨ��*/
	{
		GPIO_ResetBits( GPIOD, GPIO_Pin_9 );    /*������*/
	}
	at( dialbuf );
}

/*���õ绰��*/
void jt808_misc_0x8401( uint8_t *pmsg )
{
	uint8_t		type	= pmsg[12];
	uint16_t	id		= ( ( pmsg[0] << 8 ) | pmsg[1] );
	uint16_t	seq		= ( ( pmsg[10] << 8 ) | pmsg[11] );
	uint16_t	i;
	uint8_t		* pdata;
	uint8_t		*pbuf;
	uint8_t		count, res = 0;
	uint16_t	len = 0;
	uint16_t	addr;
	uint8_t		len_phone, len_contact;

	pbuf = rt_malloc( 4096 );
	if( pbuf == RT_NULL )
	{
		res = 1;
		goto lbl_end_8401;
	}

	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );
	sst25_read( PHONEBOOK_START, pbuf, 4096 );

	if( type == 0 )                                 /*ɾ�����������¼�*/
	{
		sst25_erase_4k( PHONEBOOK_START );
		goto lbl_end_8401;
	}else if( type == 1 )                           /*1.���� ɾ�����еģ�׷���µ�*/
	{
		memset( pbuf, 0, 4096 );
		pdata = pmsg + 14;
		for( count = 0; count < pmsg[13]; count++ ) /*��ϵ������*/
		{
			len_phone	= pdata[1];
			len_contact = pdata[len_phone + 2];
			len			= len_phone + len_contact + 3;
			if( len > 62 )
			{
				len = 62;
			}
			pbuf[count * 64] = 'P';
			memcpy( pbuf + count * 64 + 1, pdata, len );
			pdata += ( len_phone + len_contact + 3 );
		}
	}else if( type == 2 )                           /* 2 ׷�� 3 �޸� ����idһ���ľ͸��£�û�еľ�����*/
	{
		pdata = pmsg + 14;
		for( count = 0; count < pmsg[13]; count++ ) /*�¼�����*/
		{
			len_phone	= pdata[1];
			len_contact = pdata[len_phone + 2];
			len			= len_phone + len_contact + 3;
			if( len > 62 )
			{
				len = 62;
			}

			for( i = 0; i < 4096; i += 64 )
			{
				if( pbuf[i] != 'P' ) /*����Ч��ַ*/
				{
					pbuf[i] = 'P';
					memcpy( pbuf + i + 1, pdata, len );
					break;
				}
			}

			pdata += ( len_phone + len_contact + 3 );
		}
	}else if( type == 3 )                           /* 3 �޸�����ϵ��Ϊ����*/
	{
		pdata = pmsg + 14;
		for( count = 0; count < pmsg[13]; count++ ) /*�¼�����*/
		{
			len_phone	= pdata[1];
			len_contact = pdata[len_phone + 2];
			len			= len_phone + len_contact + 3;
			if( len > 62 )
			{
				len = 62;
			}

			for( i = 0; i < 4096; i += 64 )
			{
				if( strncmp( (char*)( pdata + len_phone + 3 ), (char*)( pbuf + len_phone + 4 ), len_contact ) == 0 )
				{
					pbuf[i] = 'P';
					memcpy( pbuf + i + 1, pdata, len );
					break;
				}
			}
			pdata += ( len_phone + len_contact + 3 );
		}
	}

	/*��������,ȥ�����е�*/
	addr	= 0xFFFF;               /*��һ��Ϊ�յĵ�ַ*/
	count	= 0;                    /*��¼��*/
	for( i = 0; i < 4096; i += 64 )
	{
		if( pbuf[i] == 'P' )        /*����Ч��ַ*/
		{
			count++;
			if( addr != 0xFFFF )    /*�пյ�*/
			{
				memcpy( pbuf + addr, pbuf + i, 64 );
				memset( pbuf + i, 0xFF, 64 );
				addr = i;           /*��Ϊ�µ�δ�õ�*/
			}
		}else /*û���õĵ�ַ*/
		{
			if( addr == 0xFFFF )
			{
				addr = i;
			}
		}
	}

	sst25_write_back( PHONEBOOK_START, pbuf, 4096 );

lbl_end_8401:
	rt_sem_release( &sem_dataflash );
	rt_free( pbuf );
	jt808_tx_0x0001( seq, id, res ); /*����Ӧ��*/
}

/***********************************************************
* Function:
* Description:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
void jt808_misc_0x8500( uint8_t *pmsg )
{
}

/*
   ������ǰ�ļ�¼
   256�ֽ�һ��block
   Ҫ��50��

 */
void jt808_textmsg_init( void )
{
	uint32_t	addr;
	uint32_t	id = 0;
	uint8_t		buf[16];

	textmsg_curr_addr	= TEXTMSG_END; /*ָ�����*/
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

/*
   ������ǰ�ļ�¼
   256�ֽ�һ��block
   Ҫ��50��

 */
void jt808_center_ask_init( void )
{
	uint32_t	addr;
	uint32_t	id = 0;
	uint8_t		buf[16];

	center_ask_curr_addr	= CENTER_ASK_END; /*ָ�����*/
	center_ask_curr_id		= 0;
	center_ask_count		= 0;

	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );
	for( addr = CENTER_ASK_START; addr < CENTER_ASK_END; addr += 256 )
	{
		sst25_read( addr, buf, 16 );
		id = ( buf[0] << 24 ) | ( buf[1] << 16 ) | ( buf[2] << 8 ) | ( buf[3] );
		if( id != 0xFFFFFFFF )
		{
			center_ask_count++;
			if( id > center_ask_curr_id )
			{
				center_ask_curr_addr	= addr;
				center_ask_curr_id		= id;
			}
		}
	}
	rt_sem_release( &sem_dataflash );
}

/*��ʼ����*/
void jt808_misc_init( void )
{
	jt808_center_ask_init( );
	jt808_textmsg_init( );
	jt808_event_init( );
}

/************************************** The End Of File **************************************/
