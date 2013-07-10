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

#define TEXTMSG_START	0x60000
#define TEXTMSG_SECTORS 2
#define TEXTMSG_END		( TEXTMSG_START + TEXTMSG_SECTORS * 4096)

#define AFFAIR_START	(TEXTMSG_END)
#define AFFAIR_SECTORS 1
#define AFFAIR_END		( AFFAIR_START + AFFAIR_SECTORS * 4096 )



typedef __packed struct
{
	uint16_t	mn;                             /*���� TEXT 0x5445*/
	uint32_t	id;								/*�������*/
	MYTIME		datetime;						/*�յ���ʱ��*/
	uint16_t	len;							/*����*/
}TEXTMSG_HEAD;

MYTIME		textmsg_mytime	= 0xFFFFFFFF;       /*��ǰ��Ϣ��ʱ��*/
//uint32_t	textmsg_addr_curr = TEXTMSG_START;    /*��ǰҪ���ʵ���Ϣ��ַ*/
//uint32_t	textmsg_addr_rd = TEXTMSG_START;    /*��ǰ�Ķ���Ϣ*/

uint8_t textmsg_sect_curr=0;
uint8_t textmsg_block_curr=0;
uint8_t textmsg_sect_rd=0;
uint8_t textmsg_block_rd=0;

TEXTMSG_HEAD textmsg_head_curr;
TEXTMSG_HEAD textmsg_head_visit;



struct _sector_info
{
	uint32_t	start;                          /*��ʼ��ַ*/
	uint8_t		sectors;                        /*ռ�õ�������*/
	uint8_t		block_size;                     /*ÿ����¼���С*/
	uint32_t	addr_wr;                        /*��ǰд���ַ*/
	uint32_t	addr_rd;                        /*��ǰ������ַ*/
} sector_info[4] =
{
	{TEXTMSG_START,TEXTMSG_SECTORS,128,TEXTMSG_END,TEXTMSG_END},
};

/*
�ı���Ϣ����
Ҫ��Ҫ��֤һ����Ϣ��һ�������ڣ��������������
�������棬������infoͷ�����洢TEXTMSG_HEAD,������д������
*/
uint8_t jt808_textmsg_put( uint8_t* pinfo)
{
	uint8_t		* psrc	= pinfo;
	uint16_t	len		= ( ( psrc[2] << 8 ) | psrc[3] ) & 0x3FF;
	uint8_t		flag	= psrc[12];

	uint32_t addr;
	uint8_t count,count_need;

	rt_sem_take(&sem_dataflash,RT_TICK_PER_SECOND*2);
	if(textmsg_head_curr.mn==0x5445) /*���м�¼*/
	{
		count_need=(len+sizeof(TEXTMSG_HEAD)+127)>>7;	/*��ǰҪռ�õ�block��*/
		count=(textmsg_head_curr.len+sizeof(TEXTMSG_HEAD)+127)>>7; /*��ǰ��Ϣռ�õ�block��*/
		if(count+textmsg_block_curr+count_need>31)  /*һ������32��block [0..31] װ����*/
		{
			textmsg_block_curr=0;
			textmsg_sect_curr++;
			textmsg_sect_curr%=TEXTMSG_SECTORS;
		}
		else
		{
			textmsg_block_curr+=count;
		}
	}
	addr=TEXTMSG_START+textmsg_sect_curr*4096+textmsg_block_curr*128;
	textmsg_head_curr.mn=0x5445;
	textmsg_head_curr.id++;
	textmsg_head_curr.datetime=mytime_now;
	textmsg_head_curr.len=len;

/*����808����Ϣͷ�����¼ͷ*/
	memcpy(psrc,(uint8_t*)&textmsg_head_curr,sizeof(TEXTMSG_HEAD));
/*�����¼����*/
	psrc=pinfo;
	sst25_write_back(addr,psrc,sizeof(TEXTMSG_HEAD)+len);
	
	rt_sem_release(&sem_dataflash);
}

/*
��ȡһ���ı���Ϣ

���� index : �������α�
	0:�ӵ�ǰλ�ÿ�ʼ
	1:ǰ�����
	-1:�������

����ֵ 0:û�ҵ�
	   1:�ҵ�
*/
uint8_t jt808_textmsg_get(int8_t index,uint8_t *pout )
{
	uint8_t count;
	rt_sem_take(&sem_dataflash,RT_TICK_PER_SECOND*2);
	if(index==0)	/*��ʼ����*/
	{
	//	textmsg_sect_rd=textmsg_head_curr;
	//	textmsg_block_rd=textmsg_block_curr;
	//	memcpy(textmsg_head_visit,textmsg_head_curr,sizeof(TEXTMSG_HEAD));
	}
	if(index==1) /*��ǰ���ң����ϵ�*/
	{
		count=(textmsg_head_visit.len+12+127)>>7;
		textmsg_block_rd+=count;
		if(textmsg_block_rd>31)
		textmsg_sect_rd%=TEXTMSG_SECTORS;
	}



	rt_sem_release(&sem_dataflash);
}

/*�յ������·���0x8300��Ϣ*/
void jt808_misc_0x8300( uint8_t *pmsg )
{
	uint8_t flag=pmsg[12];
	if( flag & 0x01 )   /*������ֱ�ӵ���*/
	{
	}
	if( flag & 0x04 )   /*�ն���ʾ����ʾ,ָʾ��δ����Ϣ*/
	{
		jt808_textmsg_put(pmsg );
	}
	if( flag & 0x08 )   /*TTS����*/
	{
		tts_write(pmsg[13]);
	}
	if( flag & 0x10 )   /*�����*/
	{
	}
	if( flag & 0x20 )   /*����*/
	{
	}
}


/*���õ绰��*/
void jt808_misc_0x8401(uint8_t *pmsg)
{


}

/*
������ǰ�ļ�¼
128�ֽ�һ��block
Ҫ��50��

*/
uint8_t jt808_textmsg_init( void )
{
	uint8_t sect,block;
	uint32_t addr,id=0;
	TEXTMSG_HEAD head;
	textmsg_head_curr.mn=0;
	rt_sem_take(&sem_dataflash,RT_TICK_PER_SECOND*2);
	for(sect=0;sect<TEXTMSG_SECTORS;sect++)
	{
		for(block=0;block<32;block++)
		{
			addr=TEXTMSG_START+sect*4096+block*128;
			sst25_read(addr,(uint8_t *)&head,sizeof(TEXTMSG_HEAD));
			if(head.mn==0x5445) /* TEXT 54 45 58 54*/
			{
				if(head.id>id)
				{
					memcpy((uint8_t*)&textmsg_head_curr,(uint8_t*)&head,sizeof(TEXTMSG_HEAD));
				}
			}
		}
	}
	rt_sem_release(&sem_dataflash);
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
