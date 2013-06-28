/*
   vdr Vichle Driver Record ³µÁ¾ÐÐÊ»¼ÇÂ¼
 */

#include <rtthread.h>
#include <finsh.h>
#include "stm32f4xx.h"

#include <rtdevice.h>
#include <dfs_posix.h>

#include <time.h>

#include "sst25.h"

#include "jt808.h"
#include "vdr.h"
#include "jt808_gps.h"

#define VDR_DEBUG

//#define TEST_BKPSRAM

typedef uint32_t MYTIME;

/*×ª»»hexµ½bcdµÄ±àÂë*/
#define HEX2BCD( A ) ( ( ( ( A ) / 10 ) << 4 ) | ( ( A ) % 10 ) )

#define PACK_BYTE( buf, byte ) ( *( buf ) = ( byte ) )
#define PACK_WORD( buf, word ) \
    do { \
		*( buf )		= ( word ) >> 8; \
		*( buf + 1 )	= ( word ) & 0xff; \
	} \
    while( 0 )

#define PACK_INT( buf, byte4 ) \
    do { \
		*( buf )		= ( byte4 ) >> 24; \
		*( buf + 1 )	= ( byte4 ) >> 16; \
		*( buf + 2 )	= ( byte4 ) >> 8; \
		*( buf + 3 )	= ( byte4 ) & 0xff; \
	} while( 0 )


/*
   4MB serial flash 0x400000
 */

static rt_thread_t tid_usb_vdr = RT_NULL;

#define VDR_BASE 0x300000

#define VDR_08_START	VDR_BASE
#define VDR_08_SECTORS	100 /*Ã¿Ð¡Ê±2¸ösector,±£Áô50Ð¡Ê±*/
#define VDR_08_END		( VDR_08_START + VDR_08_SECTORS * 4096 )

#define VDR_09_START	( VDR_08_START + VDR_08_SECTORS * 4096 )
#define VDR_09_SECTORS	64  /*16Ìì£¬Ã¿Ìì4sector*/
#define VDR_09_END		( VDR_09_START + VDR_09_SECTORS * 4096 )

#define VDR_10_START	( VDR_09_START + VDR_09_SECTORS * 4096 )
#define VDR_10_SECTORS	8   /*100ÌõÊÂ¹ÊÒÉµã 100*234  Êµ¼Ê 128*256 */
#define VDR_10_END		( VDR_10_START + VDR_10_SECTORS * 4096 )

#define VDR_11_START	( VDR_10_START + VDR_10_SECTORS * 4096 )
#define VDR_11_SECTORS	3   /*100Ìõ³¬Ê±¼ÝÊ»¼ÇÂ¼ 100*50 Êµ¼Ê 128*64,±£ÁôÒ»¸öÉÈÇø£¬É¾³ýÊ±ÈÔÓÐÊý¾Ý*/
#define VDR_11_END		( VDR_11_START + VDR_11_SECTORS * 4096 )

#define VDR_12_START	( VDR_11_START + VDR_11_SECTORS * 4096 )
#define VDR_12_SECTORS	3   /*200Ìõ¼ÝÊ»ÈËÉí·Ý¼ÇÂ¼ 200*25 Êµ¼Ê200*32 */
#define VDR_12_END		( VDR_12_START + VDR_12_SECTORS * 4096 )

#define VDR_13_START	( VDR_12_START + VDR_12_SECTORS * 4096 )
#define VDR_13_SECTORS	2   /*100Ìõ Íâ²¿¹©µç¼ÇÂ¼100*7 Êµ¼Ê 100*8*/
#define VDR_13_END		( VDR_13_START + VDR_13_SECTORS * 4096 )

#define VDR_14_START	( VDR_13_START + VDR_13_SECTORS * 4096 )
#define VDR_14_SECTORS	2   /*100Ìõ ²ÎÊýÐÞ¸Ä¼ÇÂ¼ 100*7 Êµ¼Ê100*8*/
#define VDR_14_END		( VDR_14_START + VDR_14_SECTORS * 4096 )

#define VDR_15_START	( VDR_14_START + VDR_14_SECTORS * 4096 )
#define VDR_15_SECTORS	2   /*10ÌõËÙ¶È×´Ì¬ÈÕÖ¾ 10*133 Êµ¼Ê 10*256*/
#define VDR_15_END		( VDR_15_START + VDR_15_SECTORS * 4096 )

static struct rt_timer tmr_200ms;

struct _sect_info
{
	uint8_t		flag;       /*±êÖ¾*/
	uint32_t	start_addr; /*¿ªÊ¼µÄµØÖ·*/
	uint32_t	addr;       /*×î½üÒ»ÌõÓÐÐ§¼ÇÂ¼µÄµØÖ·*/
	uint16_t	rec_size;   /*¼ÇÂ¼´óÐ¡*/
	uint8_t		sectors;    /*¼ÇÂ¼µÄÉÈÇøÊý*/
} sect_info[8] =
{
	{ '8', VDR_08_START, VDR_08_START, 128, 92 },
	{ '9', VDR_09_START, VDR_09_START, 16,	86 },
	{ 'A', VDR_10_START, VDR_10_START, 256, 8  },
	{ 'B', VDR_11_START, VDR_11_START, 64,	3  },
	{ 'C', VDR_12_START, VDR_12_START, 32,	3  },
	{ 'D', VDR_13_START, VDR_13_START, 8,	2  },
	{ 'E', VDR_14_START, VDR_14_START, 8,	2  },
	{ 'F', VDR_15_START, VDR_15_START, 256, 2  },
};

typedef struct
{
	uint8_t cmd;

	uint32_t	ymdh_start;
	uint8_t		minute_start;

	uint32_t	ymdh_end;
	uint8_t		minute_end;

	uint32_t	ymdh_curr;
	uint8_t		minute_curr;
	uint32_t	addr;

	uint16_t	blocks;         /*¶¨ÒåÃ¿´ÎÉÏ´«¶àÉÙ¸öÊý¾Ý¿é*/
	uint16_t	blocks_remain;  /*µ±Ç°×éÖ¯ÉÏ´«°üÊÇ»¹ÐèÒªµÄµÄblocks*/
}VDR_CMD;

VDR_CMD vdr_cmd;


/*´«µÝÐ´ÈëÎÄ¼þµÄÐÅÏ¢
   0...3  Ð´ÈëSerialFlashµÄµØÖ·
   4...31 ÎÄ¼þÃû
 */
static uint8_t file_rec[32];


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static unsigned long linux_mktime( uint32_t year, uint32_t mon, uint32_t day, uint32_t hour, uint32_t min, uint32_t sec )
{
	if( 0 >= (int)( mon -= 2 ) )
	{
		mon		+= 12;
		year	-= 1;
	}
	return ( ( ( (unsigned long)( year / 4 - year / 100 + year / 400 + 367 * mon / 12 + day ) + year * 365 - 719499 ) * 24 + hour * 60 + min ) * 60 + sec );
}

#define BYTESWAP2( val )    \
    ( ( ( val & 0xff ) << 8 ) |   \
      ( ( val & 0xff00 ) >> 8 ) )

#define BYTESWAP4( val )    \
    ( ( ( val & 0xff ) << 24 ) |   \
      ( ( val & 0xff00 ) << 8 ) |  \
      ( ( val & 0xff0000 ) >> 8 ) |  \
      ( ( val & 0xff000000 ) >> 24 ) )

#define MYDATETIME( year, month, day, hour, minute, sec )\
	( (uint32_t)( ( year ) << 26 ) | \
	(uint32_t)( ( month ) << 22 ) | \
	(uint32_t)( ( day ) << 17 ) | \
	(uint32_t)( ( hour ) << 12 ) | \
	(uint32_t)( ( minute ) << 6 ) | ( sec ) )
#define YEAR( datetime )									( ( datetime >> 26 ) & 0x3F )
#define MONTH( datetime )									( ( datetime >> 22 ) & 0xF )
#define DAY( datetime )										( ( datetime >> 17 ) & 0x1F )
#define HOUR( datetime )									( ( datetime >> 12 ) & 0x1F )
#define MINUTE( datetime )									( ( datetime >> 6 ) & 0x3F )
#define SEC( datetime )										( datetime & 0x3F )

uint8_t vdr_signal_status = 0x01; /*ÐÐ³µ¼ÇÂ¼ÒÇµÄ×´Ì¬ÐÅºÅ*/

/*Íâ½Ó³µËÙÐÅºÅ*/
__IO uint16_t	IC2Value	= 0;
__IO uint16_t	DutyCycle	= 0;
__IO uint32_t	Frequency	= 0;

/*²ÉÓÃPA.0 ×÷ÎªÍâ²¿Âö³å¼ÆÊý*/
void pulse_init( void )
{
	GPIO_InitTypeDef	GPIO_InitStructure;
	NVIC_InitTypeDef	NVIC_InitStructure;
	TIM_ICInitTypeDef	TIM_ICInitStructure;

	/* TIM5 clock enable */
	RCC_APB1PeriphClockCmd( RCC_APB1Periph_TIM5, ENABLE );

	/* GPIOA clock enable */
	RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOA, ENABLE );

	/* TIM5 chennel1 configuration : PA.0 */
	GPIO_InitStructure.GPIO_Pin		= GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed	= GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_OType	= GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd	= GPIO_PuPd_UP;
	GPIO_Init( GPIOA, &GPIO_InitStructure );

	/* Connect TIM pin to AF0 */
	GPIO_PinAFConfig( GPIOA, GPIO_PinSource0, GPIO_AF_TIM5 );

	/* Enable the TIM5 global Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel						= TIM5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority	= 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority			= 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd					= ENABLE;
	NVIC_Init( &NVIC_InitStructure );

	TIM_ICInitStructure.TIM_Channel		= TIM_Channel_1;
	TIM_ICInitStructure.TIM_ICPolarity	= TIM_ICPolarity_Rising;
	TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
	TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
	TIM_ICInitStructure.TIM_ICFilter	= 0x0;

	TIM_PWMIConfig( TIM5, &TIM_ICInitStructure );

	/* Select the TIM5 Input Trigger: TI1FP1 */
	TIM_SelectInputTrigger( TIM5, TIM_TS_TI1FP1 );

	/* Select the slave Mode: Reset Mode */
	TIM_SelectSlaveMode( TIM5, TIM_SlaveMode_Reset );
	TIM_SelectMasterSlaveMode( TIM5, TIM_MasterSlaveMode_Enable );

	/* TIM enable counter */
	TIM_Cmd( TIM5, ENABLE );

	/* Enable the CC2 Interrupt Request */
	TIM_ITConfig( TIM5, TIM_IT_CC2, ENABLE );
}

/*TIM5_CH1*/
void TIM5_IRQHandler( void )
{
	RCC_ClocksTypeDef RCC_Clocks;
	RCC_GetClocksFreq( &RCC_Clocks );

	TIM_ClearITPendingBit( TIM5, TIM_IT_CC2 );

	/* Get the Input Capture value */
	IC2Value = TIM_GetCapture2( TIM5 );

	if( IC2Value != 0 )
	{
		/* Duty cycle computation */
		//DutyCycle = ( TIM_GetCapture1( TIM5 ) * 100 ) / IC2Value;
		/* Frequency computation   TIM4 counter clock = (RCC_Clocks.HCLK_Frequency)/2 */
		//Frequency = (RCC_Clocks.HCLK_Frequency)/2 / IC2Value;
/*ÊÇ²»ÊÇ·´ÏòµçÂ·?*/
		DutyCycle	= ( IC2Value * 100 ) / TIM_GetCapture1( TIM5 );
		Frequency	= ( RCC_Clocks.HCLK_Frequency ) / 2 / TIM_GetCapture1( TIM5 );
	}else
	{
		DutyCycle	= 0;
		Frequency	= 0;
	}
}

#define SPEED_LIMIT				10          /*ËÙ¶ÈÃÅÏÞ ´óÓÚ´ËÖµÈÏÎªÆô¶¯£¬Ð¡ÓÚ´ËÖµÈÏÎªÍ£Ö¹*/
#define SPEED_LIMIT_DURATION	10          /*ËÙ¶ÈÃÅÏÞ³ÖÐøÊ±¼ä*/

#define SPEED_STATUS_ACC	0x01            /*acc×´Ì¬ 0:¹Ø 1:¿ª*/
#define SPEED_STATUS_BRAKE	0x02            /*É²³µ×´Ì¬ 0:¹Ø 1:¿ª*/

#define SPEED_JUDGE_ACC		0x04            /*ÊÇ·ñÅÐ¶ÏACC*/
#define SPEED_JUDGE_BRAKE	0x08            /*ÊÇ·ñÅÐ¶ÏBRAKE É²³µÐÅºÅ*/

#define SPEED_USE_PULSE 0x10                /*Ê¹ÓÃÂö³åÐÅºÅ 0:²»Ê¹ÓÃ 1:Ê¹ÓÃ*/
#define SPEED_USE_GPS	0x20                /*Ê¹ÓÃgpsÐÅºÅ 0:²»Ê¹ÓÃ 1:Ê¹ÓÃ*/

struct _vehicle_status
{
	uint8_t status;                         /*µ±Ç°³µÁ¾×´Ì¬ 0:Í£Ö¹ 1:Æô¶¯*/
	uint8_t logic;                          /*µ±Ç°Âß¼­×´Ì¬*/

	uint8_t		pulse_speed_judge_duration; /*ËÙ¶ÈÃÅÏÞ³ÖÐøÊ±¼ä*/
	uint8_t		pulse_speed;                /*ËÙ¶ÈÖµ*/
	uint32_t	pulse_duration;             /*³ÖÐøÊ±¼ä-Ãë*/

	uint8_t		gps_judge_duration;         /*ËÙ¶ÈÃÅÏÞ³ÖÐøÊ±¼ä*/
	uint8_t		gps_speed;                  /*ËÙ¶ÈÖµ£¬µ±Ç°ËÙ¶ÈÖµ*/
	uint32_t	gps_duration;               /*³ÖÐøÊ±¼ä-Ãë*/
} car_status =
{
	0, ( SPEED_USE_GPS ), 0, 0, 0, 0, 0, 0
};


/*
   200ms¶¨Ê±Æ÷
   ¼à²âÂõËÙ£¬ÊÂ¹ÊÒÉµãºÍËÙ¶ÈÐ£×¼

   ¼ÇÂ¼ÒÇÓ¦ÄÜÒÔ0.2sµÄÊ±¼ä¼ä¸ô³ÖÐø¼ÇÂ¼²¢´æ´¢ÐÐÊ»½áÊøÇ°20sÊµÊ±Ê±¼ä¶ÔÓ¦µÄÐÐÊ»×´Ì¬Êý¾Ý£¬¸ÃÐÐ
   Ê»×´Ì¬Êý¾ÝÎª£º³µÁ¾ÐÐÊ»ËÙ¶È¡¢ÖÆ¶¯µÈ×´Ì¬ÐÅºÅºÍÐÐÊ»½áÊøÊ±µÄÎ»ÖÃÐÅÏ¢¡£

   ÔÚ³µÁ¾ÐÐÊ»×´Ì¬ÏÂ¼ÇÂ¼ÒÇÍâ²¿¹©µç¶Ï¿ªÊ±£¬¼ÇÂ¼ÒÇÓ¦ÄÜÒÔ0.2sµÄÊ±¼ä¼ä¸ô³ÖÐø¼ÇÂ¼²¢´æ´¢¶ÏµçÇ°
   20sÄÚµÄ³µÁ¾ÐÐÊ»×´Ì¬Êý¾Ý£¬¸ÃÐÐÊ»×´Ì¬Êý¾ÝÎª£º³µÁ¾ÐÐÊ»ËÙ¶È¡¢³µÁ¾ÖÆ¶¯µÈ×´Ì¬ÐÅºÅ¼°¶ÏµçÊ±µÄ
   Î»ÖÃÐÅÏ¢¡£

   ÔÚ³µÁ¾´¦ÓÚÐÐÊ»×´Ì¬ÇÒÓÐÐ§Î»ÖÃÐÅÏ¢10sÄÚÎÞ±ä»¯Ê±£¬¼ÇÂ¼ÒÇÓ¦ÄÜÒÔ0.2sµÄÊ±¼ä¼ä¸ô³ÖÐø¼ÇÂ¼²¢´æ
   ´¢¶ÏµçÇ°20sÄÚµÄ³µÁ¾ÐÐÊ»×´Ì¬Êý¾Ý£¬¸ÃÐÐÊ»×´Ì¬Êý¾ÝÎª£º³µÁ¾ÐÐÊ»ËÙ¶È¡¢³µÁ¾ÖÆ¶¯µÈ×´Ì¬ÐÅºÅ¼°
   ¶ÏµçÊ±µÄÎ»ÖÃÐÅÏ¢¡£

 */
static void cb_tmr_200ms( void* parameter )
{
/*ÅÐ¶Ï¼ÝÊ»×´Ì¬*/
}

#if 0


/*
   ³õÊ¼»¯¼ÇÂ¼Êý¾Ý
   ÕÒµ½×î½üÒ»Ìõ¼ÇÂ¼µÄÎ»ÖÃ
 */
static uint32_t vdr_init_byid( uint8_t id, uint8_t *p )
{
	uint16_t	sect, offset;
	uint8_t		*prec;
	uint32_t	mytime_curr = 0;
	uint32_t	mytime_vdr	= 0;
	uint32_t	addr;
	uint8_t		flag; /*±êÖ¾*/
	uint16_t	rec_size;

	uint8_t		i = id - 8;

	addr		= sect_info[i].start_addr;
	flag		= sect_info[i].flag;
	rec_size	= sect_info[i].rec_size;

	for( sect = 0; sect < sect_info[i].sectors; sect++ )
	{
		sst25_read( sect_info[i].start_addr + sect * 4096, p, 4096 );                               /*Ò»´Î¶ÁÈë4096×Ö½Ú*/
		for( offset = 0; offset < 4096; offset += rec_size )                                        /*°´ÕÕ¼ÇÂ¼´óÐ¡±éÀú*/
		{
			prec = p + offset;
			if( prec[0] == flag )                                                                   /*Ã¿¸ö¼ÇÂ¼Í·¶¼ÊÇ <flag><mydatetime(4byte)>  ÓÐÐ§Êý¾Ý*/                                                                   /*ÊÇÓÐÐ§µÄÊý¾Ý°ü*/
			{
				/*×¢Òâ´æ´¢µÄË³Ðò£¬²»ÄÜ¼òµ¥µÄBYTESWAP4*/
				mytime_curr = ( prec[4] << 24 ) | ( prec[3] << 16 ) | ( prec[2] << 8 ) | prec[1];   /*Õû·ÖÖÓÊ±¿Ì*/
				if( mytime_curr > mytime_vdr )
				{
					mytime_vdr	= mytime_curr;
					addr		= sect_info[i].start_addr + sect * 4096 + offset;
				}
			}else if( prec[0] != 0xFF )                                                             /*´íÎóµÄ¼ÇÂ¼Í·*/
			{
				rt_kprintf( "%d>vdr_init err i=%d,addr=%08x\r\n", i, prec );
			}
		}
	}
	sect_info[i].addr = addr;
	rt_kprintf( "\r\n%d>sect:%02d addr=%08x datetime:%02d-%02d-%02d %02d:%02d:%02d", rt_tick_get( ), id, addr, YEAR( mytime_vdr ), MONTH( mytime_vdr ), DAY( mytime_vdr ), HOUR( mytime_vdr ), MINUTE( mytime_vdr ), SEC( mytime_vdr ) );
}

#endif

static uint32_t vdr_08_addr = VDR_08_END;   /*µ±Ç°ÒªÐ´ÈëµÄµØÖ·*/
static MYTIME	vdr_08_time = 0xFFFFFFFF;   /*µ±Ç°Ê±¼ä´Á*/
static uint8_t	vdr_08_info[126];           /*±£´æÒªÐ´ÈëµÄÐÅÏ¢*/
static uint8_t	vdr_08_sect_index = 0;      /*ÔÚÒ»¸ösectÄÚµÄ¼ÇÂ¼Ë÷Òý,Ã¿¸ösector´æ32¸ö*/

static MYTIME	vdr_09_time			= 0xFFFFFFFF;
static uint16_t vdr_09_sect			= 0;    /*µ±Ç°ÒªÐ´ÈëµÄsectºÅ*/
static uint8_t	vdr_09_sect_index	= 0;    /*ÔÚÒ»¸ösectÄÚµÄ¼ÇÂ¼Ë÷Òý,Ã¿¸ösector´æ6¸ö*/

/*´ÓbufÖÐ»ñÈ¡Ê±¼äÐÅÏ¢*/
static MYTIME mytime_from_buf( uint8_t* buf )
{
	uint8_t year, month, day, hour, minute, sec;
	uint8_t *psrc = buf;

	year	= *psrc++;
	month	= *psrc++;
	day		= *psrc++;
	hour	= *psrc++;
	minute	= *psrc++;
	sec		= *psrc;
	return MYDATETIME( year, month, day, hour, minute, sec );
}

/*°ÑÊ±¼ä´æµ½bufferÖÐ  4byte=>6byte*/
static void mytime_to_buf( MYTIME time, uint8_t* buf )
{
	uint8_t *psrc = buf;

	*psrc++ = YEAR( time );
	*psrc++ = MONTH( time );
	*psrc++ = DAY( time );
	*psrc++ = HOUR( time );
	*psrc++ = MINUTE( time );
	*psrc	= SEC( time );
}

typedef struct _vdr_08_userdata
{
	MYTIME		start;          /*¿ªÊ¼Ê±¿Ì*/
	MYTIME		end;            /*½áÊøÊ±¿Ì*/
	uint16_t	totalrecord;    /*×ÜµÃ¼ÇÂ¼Êý*/
	uint32_t	addr_from;      /*µ±Ç°¶ÁµÄµØÖ·*/
	uint32_t	addr_to;        /*µ±Ç°¶ÁµÄµØÖ·*/
	uint8_t		rec_per_packet; /*Ã¿°üµÄ¼ÇÂ¼Êý*/
	uint16_t	packet_total;   /*×Ü°üÊý*/
	uint16_t	packet_curr;    /*µ±Ç°°üÊý*/
}VDR_08_USERDATA;

VDR_08_USERDATA vdr_08_userdata;


/*
   ¾ÍË³Ðò´æ´¢,Ã»ÓÐÄÇÃ´¸´ÔÓ

 */
void vdr_08_init( void )
{
	uint32_t	addr, addr_max;
	MYTIME		curr, old = 0;
	uint8_t		buf[16];

	memset( vdr_08_info, 0xFF, sizeof( vdr_08_info ) ); /*ÐÂµÄ¼ÇÂ¼£¬³õÊ¼»¯Îª0xFF*/

	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );

	/*ÏÈ×ö¸ö²ë³ý°É*/
	for( addr = VDR_08_START; addr < VDR_08_END; addr += 4096 )
	{
	//	sst25_erase_4k( addr );
	}
	/*±éÀúÒ»±é*/
	for( addr = VDR_08_START; addr < VDR_08_END; addr += 128 )                      /*Ã¿¸ösectorµÄµÚÒ»¸ö128 ±£´æ¸Ã°ëÐ¡Ê±µÄÐÅÏ¢,µÚÒ»¸öÓÐÐ§¼ÇÂ¼µÄÊ±¿Ì*/
	{
		sst25_read( addr, buf, 4 );
		curr = ( buf[0] << 24 ) | ( buf[1] << 16 ) | ( buf[2] << 8 ) | ( buf[3] );  /*Ç°4¸ö×Ö½Ú´ú±íÊ±¼ä*/
		if( curr != 0xFFFFFFFF )                                                    /*ÊÇÓÐÐ§¼ÇÂ¼*/
		{
			if( curr >= old )                                                       /*ÕÒµ½¸üÐÂµÄ¼ÇÂ¼*/
			{
				old			= curr;
				addr_max	= addr;                                                 /*µ±Ç°¼ÇÂ¼µÄµØÖ·*/
			}
		}
	}

	rt_sem_release( &sem_dataflash );
	if( old )                                                                       /*²»»ñÈ¡Ê±¼ä£¬·ñÔò»áÓ°ÏìÊ×°üÊý¾ÝµÄvdr_08_put*/
	{
		vdr_08_addr = addr_max;                                                     /*µ±Ç°ÓÐ¼ÇÂ¼£¬¼ÇÂ¼µÄµØÖ·*/
	}
	rt_kprintf( ">vdr08(%08x) %02d-%02d-%02d %02d:%02d:%02d\r\n", vdr_08_addr, YEAR( old ), MONTH( old ), DAY( old ), HOUR( old ), MINUTE( old ), SEC( old ) );
}

/*±£´æÊý¾Ý ÒÔ1ÃëµÄ¼ä¸ô±£´æÊý¾ÝÄÜ·ñ´¦ÀíµÄ¹ýÀ´**/
void vdr_08_put( MYTIME datetime, uint8_t speed, uint8_t status )
{
	uint32_t i, sec;

	if( ( vdr_08_time & 0xFFFFFFC0 ) != ( datetime & 0xFFFFFFC0 ) ) /*²»ÊÇÔÚµ±Ç°µÄÒ»·ÖÖÓÄÚ*/
	{
		if( vdr_08_time != 0xFFFFFFFF )                             /*ÊÇÓÐÐ§µÄÊý¾Ý,Òª±£´æ£¬×¢ÒâµÚÒ»¸öÊý¾ÝµÄ´¦Àí*/
		{
			vdr_08_info[0]	= vdr_08_time >> 24;
			vdr_08_info[1]	= vdr_08_time >> 16;
			vdr_08_info[2]	= vdr_08_time >> 8;
			vdr_08_info[3]	= vdr_08_time & 0xFF;
//#ifdef TEST_BKPSRAM
#if 0
			for( i = 0; i < sizeof( vdr_08_info ); i++ )
			{
				*(__IO uint8_t*)( BKPSRAM_BASE + i + 1 ) = vdr_08_info[i];
			}
			*(__IO uint8_t*)( BKPSRAM_BASE ) = 1;
#else

			vdr_08_addr += 128;                 /*Ôö¼ÓÒ»ÌõÐÂ¼ÍÂ¼*/
			if( vdr_08_addr >= VDR_08_END )     /*ÅÐ¶ÏÊÇ·ñ»·»Ø*/
			{
				vdr_08_addr = VDR_08_START;
			}
			rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );
			if( ( vdr_08_addr & 0xFFF ) == 0 )  /*ÔÚ4k±ß½ç´¦*/
			{
				sst25_erase_4k( vdr_08_addr );
			}
			sst25_write_through( vdr_08_addr, vdr_08_info, sizeof( vdr_08_info ) );
			rt_sem_release( &sem_dataflash );
			rt_kprintf( "%d>Ð´Èë08 µØÖ·:%08x Öµ:%08x\r\n", rt_tick_get( ), vdr_08_addr, vdr_08_time );

#endif
			memset( vdr_08_info, 0xFF, sizeof( vdr_08_info ) ); /*ÐÂµÄ¼ÇÂ¼£¬³õÊ¼»¯Îª0xFF*/
		}
	}
	sec							= SEC( datetime );
	vdr_08_time					= datetime;
	vdr_08_info[sec * 2 + 4]	= gps_speed;
	vdr_08_info[sec * 2 + 5]	= vdr_signal_status;
}

/*
   »ñÈ¡08Êý¾Ý
   Ô¼ÊøÌõ¼þ£¬µØÖ··¶Î§  £¬ÆðÊ¼½áÊøÊ±¼ä£¬µ¥°üÊý¾Ý´óÐ¡
 */
void vdr_08_get( JT808_TX_NODEDATA *pnodedata )
{
	uint8_t			i;
	VDR_08_USERDATA * puserdata;
	uint32_t		addr;
	uint8_t			buf[126];

	if( pnodedata == RT_NULL )
	{
		return;
	}

	puserdata = (VDR_08_USERDATA* )( pnodedata->tag_data + 126 * 4 + 4 );

/*´Óµ±Ç°µØÖ·¿ªÊ¼µ¹Ðò¶Á,Ã¿´Î4¸örecord*/
	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );
	addr = puserdata->addr_from;
	for( i = 0; i < 4; i++ )
	{
		sst25_read( addr, buf + 2, 124 ); /*ÔÚ´æÊý¾ÝÊ±Ã»ÓÐ´æBCDµÄÈÕÆÚ£¬¶øÊÇMYTIMEµÄ¸ñÊ½*/
	}

	rt_sem_release( &sem_dataflash );
}

/*ÊÕµ½Ó¦´ðµÄ´¦Àíº¯Êý*/
static void jt808_tx_response( JT808_TX_NODEDATA * nodedata, uint8_t *pmsg )
{
}

/*³¬Ê±ºóµÄ´¦Àíº¯Êý*/
static void jt808_tx_timeout( JT808_TX_NODEDATA * nodedata )
{
}

/*¶¨Î»Êý¾ÝµÄµØÖ·£
   ´ÓÖ¸¶¨µÄ½áÊøÊ±¼äÖ®Ç°×î½üµÄµÚ1·ÖÖÓµÄÐÐÊ»ËÙ¶È¼ÇÂ¼¿ªÊ¼
   Òª²»Òª´«µÝ½ønodedata?
   ×¼±¸Òª·¢ËÍµÄÊý¾Ý£¬Ô¼ÊøÌõ¼þ:¿ªÊ¼½áÊøÊ±¿Ì£¬×ÜµÄblockÊý

 */
void vdr_08_get_ready( MYTIME start, MYTIME end, uint16_t totalrecord )
{
	uint32_t			addr;
	JT808_TX_NODEDATA	*pnodedata;
	VDR_08_USERDATA		*puserdata;
	uint32_t			i;
	uint16_t			rec_count = 0;              /*ÕÒµ½µÄ¼ÇÂ¼Êý*/
	uint8_t				buf[4];

	uint32_t			addr_from	= 0xFFFFFFFF;   /*¿ªÊ¼µÄµØÖ·*/
	uint32_t			addr_to		= 0xFFFFFFFF;   /*½áÊøµÄµØÖ·*/

	uint32_t			time_from	= 0xFFFFFFFF;   /*ÉÏ±¨¼ÇÂ¼ ¿ªÊ¼µÄÊ±¿Ì£¬×î½üµÄ*/
	uint32_t			time_to		= 0xFFFFFFFF;   /*ÉÏ±¨¼ÇÂ¼ ½áÊøµÄÊ±¿Ì£¬×îÔ¶µÄ*/

	MYTIME				mytime;

/*´Óµ±Ç°Î»ÖÃ¿ªÊ¼ÄæÐò²éÕÒ*/
	start	&= 0xFFFFFFC0;                          /*ºöÂÔÃë*/
	end		&= 0xFFFFFFC0;

	rt_kprintf( "%d>¿ªÊ¼±éÀúÊý¾Ý\r\n", rt_tick_get( ) );

	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );
	addr = vdr_08_addr + 128;                       /*ÅÐ¶ÏÖÐ×öÁËµØÖ·µÝ¼õ*/
	for( i = 0; i < VDR_08_SECTORS * 32; i++ )      /*×Ü¹²ÕÒµÄ¼ÇÂ¼Êý£¬ÍêÈ«Ê¹ÓÃµØÖ·ÅÐ¶Ï»á²»·½±ã*/
	{
		addr -= 128;
		if( addr < VDR_08_START )                   /*»·»Ø*/
		{
			addr = VDR_08_END - 128;
		}
		sst25_read( addr, buf, 4 );
		mytime = (uint32_t)( buf[0] << 24 ) | (uint32_t)( buf[1] << 16 ) | (uint32_t)( buf[2] << 8 ) | (uint32_t)( buf[3] );
		if( mytime == 0xFFFFFFFF )
		{
			continue;                                   /*²»ÊÇÓÐÐ§¼ÇÂ¼¼ÌÐø²éÕÒ*/
		}
		mytime &= 0xFFFFFFC0;                           /*ºöÂÔÃë,¶ÔÓ¦µ½·ÖÖÓ*/
		if( ( mytime <= end ) && ( mytime >= start ) )  /*ÊÇÔÚÊ±¼ä¶ÎÄÚ[0..0xffffffff]*/
		{
			rec_count++;                                /*ÓÐÐ§¼ÇÂ¼*/
			if( rec_count >= totalrecord )              /*ÊÕµ½×ã¹»µÄ¼ÇÂ¼Êý*/
			{
				break;
			}
			if( time_from == 0xFFFFFFFF )               /*¼ÇÂ¼¿ªÊ¼µÄµØÖ·,µÚÒ»¸öÕÒµ½µÄ¾ÍÊÇ×î½üµÄ*/
			{
				time_from	= mytime;
				addr_from	= addr;
			}
			if( mytime < time_to )                      /*ÄæÐò*/
			{
				time_to = mytime;
				addr_to = addr;
			}
		}
	}

	rt_sem_release( &sem_dataflash );

	rt_kprintf( "%d>±éÀú½áÊø form:%08x@%08x to:%08x@%08x\r\n", rt_tick_get( ), time_from, addr_from, time_to, addr_to );

/*¾ö¶¨ÓÃµ¥°ü»¹ÊÇ¶à°ü·¢ËÍ*/
	pnodedata = node_begin( 126 * 4 + 4 + sizeof( VDR_08_USERDATA ) );                  /*126×Ö½ÚÒ»¸ö¼ÇÂ¼(4¸ö¼ÇÂ¼),4×Ö½ÚÏûÏ¢·â°üÏî*/
	if( pnodedata == RT_NULL )                                                          /*´´½¨Ê§°Ü*/
	{
		rt_kprintf( "\d>vdrÉú³É08Êý¾ÝÓÐÎó\r\n", rt_tick_get( ) );
		return;
	}

	puserdata				= (VDR_08_USERDATA*)( pnodedata->tag_data + 126 * 4 + 4 );  /*Ö¸ÏòÓÃ»§Êý¾Ý*/
	puserdata->totalrecord	= rec_count;
	puserdata->addr_from	= addr_from;
	puserdata->addr_to		= addr_to;
	puserdata->packet_total = (rec_count+3)/4;		/*Ã¿ËÄÌõ¼ÇÂ¼ÐÎ³ÉÒ»°ü*/
	puserdata->packet_curr = 1;
	//vdr_08_get( pnodedata );                                                            /*×¼±¸Êý¾Ý²¢·¢ËÍ*/
}

FINSH_FUNCTION_EXPORT_ALIAS(vdr_08_get_ready,get_08,get_08_data);



/*
   µ¥Î»Ð¡Ê±ÄÚÃ¿·ÖÖÓµÄÎ»ÖÃÐÅÏ¢ºÍËÙ¶È
   ×î¿ìÆµÂÊ£¬Ã¿·ÖÖÓÒ»×éÊý¾Ý
   360Ð¡Ê±=15Ìì
   Ã¿Ìì4¸ösector Ã¿¸ösector¶ÔÓ¦6Ð¡Ê±£¬Ã¿Ð¡Ê±  680×Ö½Ú
 */
void vdr_09_init( void )
{
	uint32_t	addr;
	uint8_t		find = 0;
	uint8_t		buf[32];
	uint32_t	mytime_curr		= 0;
	uint32_t	mytime_vdr_08	= 0;

	uint16_t	sector = 0;
	uint8_t		i;

/*Ã¿ËÄ¸ösector´ú±íÒ»Ìì*/
	for( sector = 0; sector < VDR_09_SECTORS; sector++ )
	{
	}

	for( addr = VDR_08_START; addr < VDR_08_END; addr += 4096 )
	{
		sst25_read( addr, buf, 32 );

		mytime_curr = ( buf[0] << 24 ) | ( buf[1] << 16 ) | ( buf[2] << 8 ) | buf[3];   /*Ç°4¸ö×Ö½Ú´ú±íÊ±¼ä*/

		mytime_curr &= 0xFFFFFFC0;                                                      /*Æµ±Îµôsec*/

		if( mytime_curr != 0xFFFFFFFF )                                                 /*ÊÇÓÐÐ§¼ÇÂ¼*/
		{
			if( mytime_curr >= mytime_vdr_08 )
			{
				mytime_vdr_08	= mytime_curr;
				vdr_08_addr		= addr;
				vdr_08_time		= mytime_curr;
				find			= 1;
			}
		}
	}
	if( find == 0 ) /*Ã»ÓÐÕÒµ½ÓÐÐ§¼ÇÂ¼*/
	{
		vdr_08_addr = VDR_08_START + VDR_08_SECTORS * 4096;
	}
	rt_kprintf( "%d>vdr_08 addr=%08x datetime=%08x\r\n", vdr_08_addr, vdr_08_time );
}

/*
   Ã¿ÉÈÇø´æ6¸öÐ¡Ê±µÄÊý¾Ý
   Ò»¸öÐ¡Ê±666byte Õ¼ÓÃ680byte

 */
void vdr_09_put( MYTIME datetime )
{
	uint8_t		buf[11], bufhead[10];
	uint32_t	i;
	uint32_t	addr;
	PACK_INT( buf, ( gps_longi * 6 ) );
	PACK_INT( buf + 4, ( gps_lati * 6 ) );
	PACK_WORD( buf + 8, ( gps_alti ) );
	buf[10] = gps_speed;

	if( ( vdr_09_time & 0xFFFFF000 ) != ( datetime & 0xFFFFF000 ) ) /*²»ÊÇÊÇÔÚµ±Ç°µÄÐ¡Ê±ÄÚ*/
	{
		vdr_09_time = datetime;
		vdr_09_sect_index++;                                        /*µ÷Õû*/
		if( vdr_09_sect_index > 5 )                                 /*Ã¿¸ösector´æ6¸öÐ¡Ê± 0-5*/
		{
			vdr_09_sect_index = 0;
			vdr_09_sect++;
			if( vdr_09_sect >= VDR_09_SECTORS )
			{
				vdr_09_sect = 0;
			}
			addr = VDR_09_START + vdr_09_sect * 4096;   /*É¾³ýÉÈÇø*/
#ifdef VDR_DEBUG
			rt_kprintf( "%d>Ð´ÈëÐÂµÄÉÈÇø vdr_09_sect=%d\r\n", rt_tick_get( ), vdr_09_sect );
#else
			sst25_erase_4k( addr );
			PACK_INT( bufhead, datetime );              /*Ð´ÈëÐÂµÄ¼ÇÂ¼Í·*/
			PACK_BYTE( bufhead + 4, HEX2BCD( YEAR( datetime ) ) );
			PACK_BYTE( bufhead + 4, HEX2BCD( MONTH( datetime ) ) );
			PACK_BYTE( bufhead + 4, HEX2BCD( DAY( datetime ) ) );
			PACK_BYTE( bufhead + 4, HEX2BCD( HOUR( datetime ) ) );
			PACK_BYTE( bufhead + 4, HEX2BCD( MINUTE( datetime ) ) );
			PACK_BYTE( bufhead + 4, HEX2BCD( SEC( datetime ) ) );
#endif
		}
	}

#ifdef VDR_DEBUG


/*
   rt_kprintf( "%d>Ð´ÈëÒÑÓÐÊý¾Ý vdr_09_sect=%d\r\n", rt_tick_get( ), vdr_09_sect );
   for( i = 0; i < 11; i++ )
   {
   rt_kprintf( "%02x ", buf[i] );
   }
   rt_kprintf("\r\n");
 */
#else

#endif
}

/*
   ±£´æÐÐÊ»¼ÇÂ¼Êý¾Ý
   ×¢ÒâÇø·Ö²»Í¬µÄÊý¾ÝÀàÐÍ
   id=0
 */
static vdr_save_rec( uint8_t sect_id, uint8_t * pdata, uint16_t len )
{
	uint8_t id = sect_id - 8;
/*Ôö¼ÓÒ»ÌõÐÂ¼ÍÂ¼*/
	sect_info[id].addr += sect_info[id].rec_size;
/*ÅÐ¶ÏÊÇ·ñ»·»Ø*/
	if( sect_info[id].addr >= ( sect_info[id].start_addr + sect_info[id].sectors * 4096 ) )
	{
		sect_info[id].addr = sect_info[id].start_addr;
	}

	if( ( sect_info[id].addr & 0xFFF ) == 0 ) /*ÔÚ4k±ß½ç´¦*/
	{
		sst25_erase_4k( sect_info[id].addr );
	}
	sst25_write_through( sect_info[id].addr, pdata, len );
}


/*
   ÊÕµ½gpsÊý¾ÝµÄ´¦Àí£¬ÓÐ¶¨Î»ºÍÎ´¶¨Î»
   ´æ´¢Î»ÖÃÐÅÏ¢£¬
   ËÙ¶ÈÅÐ¶Ï£¬Ð£×¼
 */

rt_err_t vdr_rx_gps( void )
{
	uint32_t	datetime;
	uint8_t		year, month, day, hour, minute, sec;
	uint32_t	i;
	uint8_t		buf[128];
	uint8_t		*pbkpsram;

#ifdef TEST_BKPSRAM
/*ÉÏµçºó£¬ÓÐÒªÐ´ÈëµÄÀúÊ·Êý¾Ý 08*/
	if( *(__IO uint8_t*)( BKPSRAM_BASE ) == 1 )
	{
		pbkpsram = (__IO uint8_t*)BKPSRAM_BASE + 1;
		for( i = 0; i < 128; i++ )
		{
			buf[i] = *pbkpsram++;
		}

		*(__IO uint8_t*)( BKPSRAM_BASE ) = 0;
	}
#endif

	if( ( jt808_status & BIT_STATUS_GPS ) == 0 ) /*Î´¶¨Î»*/
	{
		return;
	}

	year	= gps_datetime[0];
	month	= gps_datetime[1];
	day		= gps_datetime[2];
	hour	= gps_datetime[3];
	minute	= gps_datetime[4];
	sec		= gps_datetime[5];
	//rt_kprintf( "%d>vdr_rx=%02d-%02d-%02d %02d:%02d:%02d\r\n", rt_tick_get( ), year, month, day, hour, minute, sec );

	datetime = MYDATETIME( year, month, day, hour, minute, sec );

	vdr_08_put( datetime, gps_speed, vdr_signal_status );


/*
   09Êý¾Ý µ¥Î»Ð¡Ê±ÄÚÃ¿·ÖÖÓµÄÎ»ÖÃËÙ¶È£¬Ö»±£´æµÚÒ»´ÎÓÐÐ§µÄ
   Ã¿Ð¡Ê±(10+1)*60+6 ×Ö½Ú
 */
	//vdr_09_put( datetime );

/*10Êý¾Ý ÊÂ¹ÊÒÉµã*/
	if( car_status.status == 0 )                                        /*ÈÏÎª³µÁ¾Í£Ö¹,ÅÐ¶ÏÆô¶¯*/
	{
		if( gps_speed >= SPEED_LIMIT )                                  /*ËÙ¶È´óÓÚÃÅÏÞÖµ*/
		{
			car_status.gps_judge_duration++;
			if( car_status.gps_judge_duration >= SPEED_LIMIT_DURATION ) /*³¬¹ýÁË³ÖÐøÊ±¼ä*/
			{
				car_status.gps_duration			= SPEED_LIMIT_DURATION;
				car_status.status				= 1;                    /*ÈÏÎª³µÁ¾ÐÐÊ»*/
				car_status.gps_judge_duration	= 0;
				rt_kprintf( "%d>³µÁ¾ÐÐÊ»\r\n", rt_tick_get( ) );
			}else
			{
				car_status.gps_duration++;                              /*Í£³µÀÛ¼ÆÊ±¼ä*/
			}
		}else
		{
			car_status.gps_duration++;                                  /*Í£³µÀÛ¼ÆÊ±¼ä*/
			                                                            /*ÔÚ´ËÅÐ¶ÏÍ£³µ³¬Ê±*/
			car_status.gps_judge_duration = 0;
		}
	}else /*³µÁ¾ÒÑÆô¶¯*/
	{
		if( gps_speed <= SPEED_LIMIT )                                  /*ËÙ¶ÈÐ¡ÓÚÃÅÏÞÖµ*/
		{
			car_status.gps_judge_duration++;
			if( car_status.gps_judge_duration >= SPEED_LIMIT_DURATION ) /*³¬¹ýÁË³ÖÐøÊ±¼ä*/
			{
				car_status.gps_duration			= SPEED_LIMIT_DURATION;
				car_status.status				= 0;                    /*ÈÏÎª³µÁ¾Í£Ê»*/
				car_status.gps_judge_duration	= 0;
				rt_kprintf( "%d>³µÁ¾Í£Ê»\r\n", rt_tick_get( ) );
			}else
			{
				car_status.gps_duration++;                              /*ÐÐÊ»ÀÛ¼ÆÊ±¼ä*/
			}
		}else
		{
			car_status.gps_duration++;                                  /*ÐÐÊ»ÀÛ¼ÆÊ±¼ä*/
			                                                            /*ÅÐ¶ÏÆ£ÀÍ¼ÝÊ»*/
			car_status.gps_judge_duration = 0;
		}
	}

/*11Êý¾Ý³¬Ê±¼ÝÊ»¼ÇÂ¼*/
}

/*
   É¾³ýÌØ¶¨ÇøÓòµÄ¼ÇÂ¼Êý¾Ý
   Ê¹ÓÃbitmaskÊÇ²»ÊÇ¸üºÃ
 */
void vdr_format( uint16_t area )
{
	uint8_t i;
	uint8_t sect;

	for( i = 8; i < 16; i++ )
	{
		if( area & ( 1 << i ) )
		{
			for( sect = 0; sect < sect_info[i - 8].sectors; sect++ )
			{
				sst25_erase_4k( sect_info[i - 8].start_addr + sect * 4096 );
			}
		}
	}
}

FINSH_FUNCTION_EXPORT( vdr_format, format vdr record );


/*
   »ñÈ¡08´æ´¢µÄ×´Ì¬ 48Ð¡Ê± µ¥Î»·ÖÖÓÄÚÃ¿ÃëµÄËÙ¶È×´Ì¬2byte
   48*60*128=2880*128=368640 (bytes)
   368640/4096=90(sectors)

   ¸ñÊ½:
   <'8'><mydatetime(4bytes><60ÃëµÄËÙ¶È×´Ì¬120bytes>
   Ñ­»·µÝÔö
 */


/*
   ³õÊ¼»¯¼ÇÂ¼ÇøÊý¾Ý
   ÒòÎªÊÇÊôÓÚ¹Ì¶¨Ê±¼ä¶Î´æ´¢µÄ
   ÐèÒª¼ÇÂ¼¿ªÊ¼Ê±¿ÌµÄsectorÎ»ÖÃ(Ïà¶ÔµÄsectorÆ«ÒÆ)
 */
rt_err_t vdr_init( void )
{
	uint8_t* pbuf;

	pulse_init( ); /*½ÓÂö³å¼ÆÊý*/

	//vdr_format( 0xff00 );

	pbuf = rt_malloc( 4096 );
	if( pbuf == RT_NULL )
	{
		return -RT_ENOMEM;
	}

	vdr_08_init( );
#if 0
	vdr_init_byid( 8, pbuf );
	sst25_read( sect_info[0].addr, (uint8_t*)&stu_rec_08, sizeof( STU_REC_08 ) ); /*¶Á³öÀ´ÊÇ·ÀÖ¹Ò»·ÖÖÓÄÚµÄÖØÆô*/
	vdr_init_byid( 9, pbuf );
	sst25_read( sect_info[1].addr, (uint8_t*)&stu_rec_09, sizeof( STU_REC_09 ) );
	vdr_init_byid( 10, pbuf );
	vdr_init_byid( 11, pbuf );
	vdr_init_byid( 12, pbuf );
	vdr_init_byid( 13, pbuf );
	vdr_init_byid( 14, pbuf );
	vdr_init_byid( 15, pbuf );
#endif
	rt_free( pbuf );
	pbuf = RT_NULL;
/*³õÊ¼»¯Ò»¸ö50msµÄ¶¨Ê±Æ÷£¬ÓÃ×÷ÊÂ¹ÊÒÉµãÅÐ¶Ï*/
	rt_timer_init( &tmr_200ms, "tmr_200ms",             /* ¶¨Ê±Æ÷Ãû×ÖÊÇ tmr_50ms */
	               cb_tmr_200ms,                        /* ³¬Ê±Ê±»Øµ÷µÄ´¦Àíº¯Êý */
	               RT_NULL,                             /* ³¬Ê±º¯ÊýµÄÈë¿Ú²ÎÊý */
	               RT_TICK_PER_SECOND / 5,              /* ¶¨Ê±³¤¶È£¬ÒÔOS TickÎªµ¥Î» */
	               RT_TIMER_FLAG_PERIODIC );            /* ÖÜÆÚÐÔ¶¨Ê±Æ÷ */
	rt_timer_start( &tmr_200ms );
}

/*ÐÐ³µ¼ÇÂ¼ÒÇÊý¾Ý²É¼¯ÃüÁî*/
void vdr_rx_8700( uint8_t * pmsg )
{
	uint8_t		* psrc;
	uint8_t		buf[500];

	uint16_t	seq = JT808HEAD_SEQ( pmsg );
	uint16_t	len = JT808HEAD_LEN( pmsg );
	uint8_t		cmd = *( pmsg + 12 );   /*Ìø¹ýÇ°Ãæ12×Ö½ÚµÄÍ·*/
	MYTIME		start, end;
	uint16_t	blocks;

	switch( cmd )
	{
		case 0:                         /*²É¼¯¼ÇÂ¼ÒÇÖ´ÐÐ±ê×¼°æ±¾*/
			buf[0]	= seq >> 8;
			buf[1]	= seq & 0xff;
			buf[2]	= cmd;
			memcpy( buf + 3, "\x55\x7A\x00\x00\x02\x00\x12\x00", 8 );
			jt808_add_tx_data_single( 1, TERMINAL_ACK, 0x0700, 11, buf, RT_NULL, RT_NULL );
			break;
		case 1:
			buf[0]	= seq >> 8;
			buf[1]	= seq & 0xff;
			buf[2]	= cmd;
			memcpy( buf + 3, "\x55\x7A\x01\x00\x12\x00120221123456789\x00\x00\x00\x00", 25 );
			jt808_add_tx_data_single( 1, TERMINAL_ACK, 0x0700, 28, buf, RT_NULL, RT_NULL );
		case 2: /*ÐÐ³µ¼ÇÂ¼ÒÇÊ±¼ä*/
			buf[0]	= seq >> 8;
			buf[1]	= seq & 0xff;
			buf[2]	= cmd;
			sprintf( buf + 3, "\x55\x7A\x02\x00\x06\x00%6s", gps_baseinfo.datetime );
			jt808_add_tx_data_single( 1, TERMINAL_ACK, 0x0700, 15, buf, RT_NULL, RT_NULL );
		case 3:
			break;
		case 4:
			break;
		case 5:
			break;
		case 6:
			break;
		case 8:                                 /*08¼ÇÂ¼*/
			if( len == 1 )                      /*»ñÈ¡ËùÓÐ¼ÇÂ¼*/
			{
				start	= 0;
				end		= 0xffffffff;
				blocks	= 0;
			}else if( len == 22 )               /*ÏÂÀ´µÄ¸ñÊ½ <55><7A><cmd><len(2byte)><±£Áô><data_block(14byte)><XOR>*/
			{
				psrc	= pmsg + 12 + 1 + 7;    /*12×Ö½ÚÍ·+1byteÃüÁî×Ö+7×Ö½ÚvdrÊý¾Ý*/
				start	= mytime_from_buf( psrc );
				end		= mytime_from_buf( psrc + 6 );
				blocks	= ( *( psrc + 32 ) << 8 ) | ( *( psrc + 33 ) );
			}else
			{
				rt_kprintf( "%d>8700µÄ¸ñÊ½²»Ê¶±ð\r\n", rt_tick_get( ) );
				return;
			}
			vdr_08_get_ready( start, end, blocks );
			break;
		case 9:
			break;
		case 10:
			break;
		case 11:
			break;
		case 12:
			break;
		case 13:
			break;
		case 14:
			break;
		case 15:
			break;
	}
}

/************************************** The End Of File **************************************/
