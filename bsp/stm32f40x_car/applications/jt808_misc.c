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
	uint16_t	mn;                             /*幻数 TEXT 0x5445*/
	uint32_t	id;								/*单增序号*/
	MYTIME		datetime;						/*收到的时间*/
	uint16_t	len;							/*长度*/
}TEXTMSG_HEAD;

MYTIME		textmsg_mytime	= 0xFFFFFFFF;       /*当前消息的时刻*/
//uint32_t	textmsg_addr_curr = TEXTMSG_START;    /*当前要访问的消息地址*/
//uint32_t	textmsg_addr_rd = TEXTMSG_START;    /*当前的读消息*/

uint8_t textmsg_sect_curr=0;
uint8_t textmsg_block_curr=0;
uint8_t textmsg_sect_rd=0;
uint8_t textmsg_block_rd=0;

TEXTMSG_HEAD textmsg_head_curr;
TEXTMSG_HEAD textmsg_head_visit;



struct _sector_info
{
	uint32_t	start;                          /*起始地址*/
	uint8_t		sectors;                        /*占用的扇区数*/
	uint8_t		block_size;                     /*每个记录块大小*/
	uint32_t	addr_wr;                        /*当前写入地址*/
	uint32_t	addr_rd;                        /*当前读出地址*/
} sector_info[4] =
{
	{TEXTMSG_START,TEXTMSG_SECTORS,128,TEXTMSG_END,TEXTMSG_END},
};

/*
文本信息保存
要不要保证一包信息在一个扇区内，不会跨扇区保存
在这里面，借用了info头部来存储TEXTMSG_HEAD,这样回写更方便
*/
uint8_t jt808_textmsg_put( uint8_t* pinfo)
{
	uint8_t		* psrc	= pinfo;
	uint16_t	len		= ( ( psrc[2] << 8 ) | psrc[3] ) & 0x3FF;
	uint8_t		flag	= psrc[12];

	uint32_t addr;
	uint8_t count,count_need;

	rt_sem_take(&sem_dataflash,RT_TICK_PER_SECOND*2);
	if(textmsg_head_curr.mn==0x5445) /*已有记录*/
	{
		count_need=(len+sizeof(TEXTMSG_HEAD)+127)>>7;	/*当前要占用的block数*/
		count=(textmsg_head_curr.len+sizeof(TEXTMSG_HEAD)+127)>>7; /*当前信息占用的block数*/
		if(count+textmsg_block_curr+count_need>31)  /*一个扇区32个block [0..31] 装不下*/
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

/*利用808的消息头保存记录头*/
	memcpy(psrc,(uint8_t*)&textmsg_head_curr,sizeof(TEXTMSG_HEAD));
/*保存记录内容*/
	psrc=pinfo;
	sst25_write_back(addr,psrc,sizeof(TEXTMSG_HEAD)+len);
	
	rt_sem_release(&sem_dataflash);
}

/*
读取一条文本信息

输入 index : 索引或游标
	0:从当前位置开始
	1:前向查找
	-1:后向查找

返回值 0:没找到
	   1:找到
*/
uint8_t jt808_textmsg_get(int8_t index,uint8_t *pout )
{
	uint8_t count;
	rt_sem_take(&sem_dataflash,RT_TICK_PER_SECOND*2);
	if(index==0)	/*开始查找*/
	{
	//	textmsg_sect_rd=textmsg_head_curr;
	//	textmsg_block_rd=textmsg_block_curr;
	//	memcpy(textmsg_head_visit,textmsg_head_curr,sizeof(TEXTMSG_HEAD));
	}
	if(index==1) /*向前查找，更老的*/
	{
		count=(textmsg_head_visit.len+12+127)>>7;
		textmsg_block_rd+=count;
		if(textmsg_block_rd>31)
		textmsg_sect_rd%=TEXTMSG_SECTORS;
	}



	rt_sem_release(&sem_dataflash);
}

/*收到中心下发的0x8300信息*/
void jt808_misc_0x8300( uint8_t *pmsg )
{
	uint8_t flag=pmsg[12];
	if( flag & 0x01 )   /*紧急，直接弹出*/
	{
	}
	if( flag & 0x04 )   /*终端显示器显示,指示有未读信息*/
	{
		jt808_textmsg_put(pmsg );
	}
	if( flag & 0x08 )   /*TTS播报*/
	{
		tts_write(pmsg[13]);
	}
	if( flag & 0x10 )   /*广告屏*/
	{
	}
	if( flag & 0x20 )   /*紧急*/
	{
	}
}


/*设置电话本*/
void jt808_misc_0x8401(uint8_t *pmsg)
{


}

/*
遍历当前的记录
128字节一个block
要求50条

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

/*电话簿*/
uint8_t jt808_phonebook_init( void )
{
}

/*初始化吧*/
void jt808_misc_init( void )
{
}

/************************************** The End Of File **************************************/
