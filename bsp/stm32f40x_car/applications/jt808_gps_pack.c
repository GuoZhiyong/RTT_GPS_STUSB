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
#include "jt808_gps.h"
#include <string.h>


/*
   正常1秒 517字节  压缩后 260
   近似 100个扇区 保存25分钟原始数据
 */
#define GPS_PACK_START		0x0003F000
#define GPS_PACK_SECTORS	100


/*
   每4k写入一次dataFlash
   扇区头包括  uint32_t id;		递增序号
 */
static uint8_t	pack_buf[4096];
static uint16_t pack_buf_pos;   /*写入的偏移*/
static uint32_t pack_buf_id;    /*id*/
static uint8_t	sect_index;


/*压缩gps信息
   五种种语句
   GGA
   GSA
   GSV
   RMC
   GLL


   三种类型
   GN
   BD
   GP

   TXT语句单独处理
   $GNTXT,

 */

#define MN 0xbadbad00

static char *start[15] = {
	"$GNGSV,",
	"$GNGSA,",
	"$GNGGA,",
	"$GNRMC,",
	"$GNGLL,",
	"$GPGSV,",
	"$GPGSA,",
	"$GPGGA,",
	"$GPRMC,",
	"$GPGLL,",
	"$BDGSV,",
	"$BDGSA,",
	"$BDGGA,",
	"$BDRMC,",
	"$BDGLL,",
};


/*
   static char* gntxt	= "$GNTXT,01,01,01,ANTENNA OK*2B";
   static char * gptxt = "$GPTXT,01,01,01,ANTENNA OK*35";
   static char * bdtxt = "$BDTXT,01,01,01,ANTENNA OK*24";
 */

static char* txt[] = {
	{ "$GNTXT,01,01,01,ANTENNA OK*2B"	 },
	{ "$GPTXT,01,01,01,ANTENNA OK*35"	 },
	{ "$BDTXT,01,01,01,ANTENNA OK*24"	 },
	{ "$GNTXT,01,01,01,ANTENNA OPEN*3B"	 },
	{ "$GPTXT,01,01,01,ANTENNA OPEN*25"	 },
	{ "$BDTXT,01,01,01,ANTENNA OPEN*34"	 },
	{ "$GNTXT,01,01,01,ANTENNA SHORT*7D" },
	{ "$GPTXT,01,01,01,ANTENNA SHORT*63" },
	{ "$BDTXT,01,01,01,ANTENNA SHORT*72" },
};

/*还原压缩的数据，并增加校验*/
uint8_t jt808_gps_unpack( uint8_t *pinfo, char *pout )
{
	uint8_t			*psrc = pinfo;
	uint8_t			*pdst;
	uint8_t			count;
	char			buf[80];
	unsigned char	i, id, len, c[2];
	unsigned char	fcs;
	char			tbl[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

	id = psrc[3];

	if( id > 0x0F )
	{
		strcpy( pout, txt[*psrc - 0x10] );
		return 29;
	}

	memcpy( buf, start[id], 7 );

	len = psrc[4];

	psrc	= pinfo + 5;
	pdst	= buf + 7;
	count	= 0;

	while( count <= len )
	{
		c[0]	= *psrc >> 4;
		c[1]	= *psrc & 0xf;
		psrc++;
		for( i = 0; i < 2; i++ )
		{
			switch( c[i] )
			{
				case 0x0a:                  /* A 或 -*/
					if( ( id % 5 ) == 2 )   /*GGA*/
					{
						*pdst++ = '-';
					}else /*RMC GSA*/
					{
						*pdst++ = 'A';
					}
					break;
				case 0x0b:
					if( ( id % 5 ) == 2 )   /*GGA*/
					{
						*pdst++ = 'M';
					}else
					{
						*pdst++ = 'V';      /*RMC GLL*/
					}
					break;
				case 0x0c:
					*pdst++ = 'N';
					break;
				case 0x0d:
					*pdst++ = '.';
					break;
				case 0x0e:
					*pdst++ = 'E';
					break;
				case 0x0f:
					*pdst++ = ',';
					break;
				default:
					*pdst++ = ( c[i] + '0' );
					break;
			}
			count++;
			if( count > len )
			{
				break;
			}
		}
	}

	fcs = 0;
	for( i = 1; i < ( count + 6 ); i++ )
	{
		fcs ^= buf[i];
	}
	buf[count + 6]	= '*';
	buf[count + 7]	= tbl[fcs >> 4];
	buf[count + 8]	= tbl[fcs & 0xF];
	buf[count + 9]	= 0;
#if 0
	rt_kprintf( "\ndecode>%s\n", buf );
#endif
	return count + 10;
}

/*保存记录*/
static void save_rec( uint8_t *p, uint8_t len )
{
	if( ( 4096 - pack_buf_pos ) < len ) /*不足以保留整个信息,*/
	{
		rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );
		sst25_write_through(GPS_PACK_START+sect_index*4096,pack_buf,4096);
		sect_index++;
		sect_index%=GPS_PACK_SECTORS;
		sst25_erase_4k(GPS_PACK_START+sect_index*4096);
		rt_sem_release( &sem_dataflash );
		memset(pack_buf,0xFF,4096);
		pack_buf_id++;
		pack_buf[0]=pack_buf_id>>24;
		pack_buf[1]=pack_buf_id>>16;
		pack_buf[2]=pack_buf_id>>8;
		pack_buf[3]=pack_buf_id&0xFF;
		pack_buf_pos=4;
	}else
	{
		memcpy( pack_buf, p, len );
		pack_buf_pos+=len;
	}
}

/*压缩数据*/
uint8_t jt808_gps_pack( char *pinfo, uint8_t len )
{
	char		*p = pinfo;
	uint8_t		*pdst;
	uint8_t		buf[80];
	uint8_t		count;
	uint8_t		i, j, c;
	uint32_t	id;
	i = 0;
	if( p[5] == 'T' )                       /*TXT单独处理*/
	{
		switch( p[2] )
		{
			case 'N':
				if( p[25] == 'K' )          /*OK*/
				{
					i = 0x10;
				} else if( p[25] == 'P' )   /*OPEN*/
				{
					i = 0x13;
				} else if( p[25] == 'H' )   /*SHORT*/
				{
					i = 0x16;
				}
				break;
			case 'P':

				if( p[25] == 'K' )          /*OK*/
				{
					i = 0x11;
				} else if( p[25] == 'P' )   /*OPEN*/
				{
					i = 0x14;
				} else if( p[25] == 'H' )   /*SHORT*/
				{
					i = 0x17;
				}
				break;
			case 'D':

				if( p[25] == 'K' )          /*OK*/
				{
					i = 0x12;
				} else if( p[25] == 'P' )   /*OPEN*/
				{
					i = 0x15;
				} else if( p[25] == 'H' )   /*SHORT*/
				{
					i = 0x18;
				}
				break;
			default:
				return 0;
		}
	}
	if( i )
	{
		memcpy( buf, "\xba\xdb\xad\x00\x00", 5 );
		buf[4] = i;
		save_rec( buf, 5 );
		return 4;
	}

	switch( p[2] )
	{
		case 'N': i = 0; break;
		case 'P': i = 1; break;
		case 'D': i = 2; break;
		default:
			rt_kprintf( "unknow\n" );
			return 0;
	}
	switch( p[4] )
	{
		case 'S':
			if( p[5] == 'V' )           /* GSV*/
			{
				j = 0;
			} else if( p[5] == 'A' )    /* GSA*/
			{
				j = 1;
			} else
			{
				rt_kprintf( "unknow\n" );
				return 0;
			}
			break;
		case 'G': j = 2; break; /*GGA*/
		case 'M': j = 3; break; /*RMC*/
		case 'L': j = 4; break; /*GLL*/
		default:
			rt_kprintf( "unknow\n" );
			return 0;
	}
	id		= MN | ( i * 5 + j );
	buf[0]	= id >> 24;
	buf[1]	= id >> 16;
	buf[2]	= id >> 8;
	buf[3]	= id & 0xFF;

	pdst	= buf + 5;      /*buf+4 留作数据长度*/
	p		= pinfo + 7;    /*跳过语句前的,*/
	count	= 0;
	c		= 0;
	while( *p != '*' )
	{
		c <<= 4;
		if( ( *p >= '0' ) && ( *p <= '9' ) )
		{
			c |= ( *p - '0' );
		}else
		{
			switch( *p )
			{
				case 'A':
				case '-': c |= 0xa; break;
				case 'V':
				case 'M': c |= 0xb; break;
				case 'N': c |= 0xc; break;
				case '.': c |= 0xd; break;
				case 'E': c |= 0xe; break;
				case ',': c |= 0xf; break;
				default:
					rt_kprintf( "unknow\n" );
					return 0;
			}
		}
		count++;
		if( ( count & 0x01 ) == 0 ) /*每两个合并一下*/
		{
			*pdst++ = c;
			c		= 0;
		}
		p++;
	}

	if( count & 0x01 )  /*还剩一个*/
	{
		*pdst = ( c << 4 );
	}
	buf[4] = count;     /*原始的数据长度*/

#if 0
	j = ( count >> 1 ) + ( count & 0x01 );
	rt_kprintf( "Encode>" );
	for( i = 0; i < j + 5; i++ )
	{
		rt_kprintf( "%02x", buf[i] );
	}
	rt_kprintf( "\n" );

	{
		char buf_out[80];
		jt808_gps_unpack( buf, buf_out );
	}
#endif
	save_rec( buf, count+5 );
	return count + 5;
}

/*
初始化，
是上电后就开始做
还是进入要上报原始信息的区域才开始
*/
void gps_pack_init( void )
{
	uint32_t	i;
	uint8_t		buf[128];
	uint32_t	addr, id;
	uint16_t	pos;

	pack_buf_id = 0;
	sect_index	= 0xFF;
	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );
	for( i = 0; i < GPS_PACK_SECTORS; i++ )
	{
		addr = GPS_PACK_START + i * 4096;
		sst25_read( addr, buf, 4 );
		id = ( buf[0] << 24 ) | ( buf[1] << 16 ) | ( buf[2] << 8 ) | buf[3];
		if( id != 0xFFFFFFFF )
		{
			if( id > pack_buf_id )
			{
				sect_index	= i;
				pack_buf_id = id;
			}
		}
	}
	if( sect_index == 0xFF )            /*没有找到有数据的扇区*/
	{
		sect_index		= 0;            /**/
		pack_buf_pos	= 4;            /*空出开始的四个字节保存id*/
	}else /*要在特定的扇区中查找，并不是太方便,因为是变长的*/
	{
		/*读入到4k的ram*/
		sst25_read( addr + sect_index * 4096, pack_buf, 4096 );
		pos = 4;
		while( pos < 4096 )
		{
			if( ( buf[pos] == 0xba ) && ( buf[pos + 1] == 0xdb ) && ( buf[pos + 2] == 0xad ) )
			{
				pos += ( buf[pos + 4] + 5 );
			}else
			{
				break;
			}
		}
		if( pos > 4095 ) /*出错了? why?*/
		{
			rt_kprintf( "gps_pack_init fault\n" );
			for( i = 0; i < GPS_PACK_SECTORS; i++ )
			{
				sst25_erase_4k( GPS_PACK_START + i * 4096 );
			}
			pack_buf_id		= 0;
			pack_buf_pos	= 4;
			sect_index		= 0;
		}else
		{
			pack_buf_pos = pos;
		}
	}
	rt_sem_release( &sem_dataflash );
	rt_kprintf( "sect=%d id=%d pos=%d\n", sect_index, pack_buf_id, pack_buf_pos );
}

/************************************** The End Of File **************************************/
