/*
   vdr Vichle Driver Record 车辆行驶记录
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

/*转换hex到bcd的编码*/
#define HEX2BCD( A )	( ( ( ( A ) / 10 ) << 4 ) | ( ( A ) % 10 ) )
#define BCD2HEX( x )	( ( ( ( x ) >> 4 ) * 10 ) + ( ( x ) & 0x0f ) )

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
#define VDR_08_SECTORS	100 /*每小时2个sector,保留50小时*/
#define VDR_08_END		( VDR_08_START + VDR_08_SECTORS * 4096 )

#define VDR_09_START	( VDR_08_START + VDR_08_SECTORS * 4096 )
#define VDR_09_SECTORS	64  /*16天，每天4sector*/
#define VDR_09_END		( VDR_09_START + VDR_09_SECTORS * 4096 )

#define VDR_10_START	( VDR_09_START + VDR_09_SECTORS * 4096 )
#define VDR_10_SECTORS	8   /*100条事故疑点 100*234  实际 128*256 */
#define VDR_10_END		( VDR_10_START + VDR_10_SECTORS * 4096 )

#define VDR_11_START	( VDR_10_START + VDR_10_SECTORS * 4096 )
#define VDR_11_SECTORS	3   /*100条超时驾驶记录 100*50 实际 128*64,保留一个扇区，删除时仍有数据*/
#define VDR_11_END		( VDR_11_START + VDR_11_SECTORS * 4096 )

#define VDR_12_START	( VDR_11_START + VDR_11_SECTORS * 4096 )
#define VDR_12_SECTORS	3   /*200条驾驶人身份记录 200*25 实际200*32 */
#define VDR_12_END		( VDR_12_START + VDR_12_SECTORS * 4096 )

#define VDR_13_START	( VDR_12_START + VDR_12_SECTORS * 4096 )
#define VDR_13_SECTORS	2   /*100条 外部供电记录100*7 实际 100*8*/
#define VDR_13_END		( VDR_13_START + VDR_13_SECTORS * 4096 )

#define VDR_14_START	( VDR_13_START + VDR_13_SECTORS * 4096 )
#define VDR_14_SECTORS	2   /*100条 参数修改记录 100*7 实际100*8*/
#define VDR_14_END		( VDR_14_START + VDR_14_SECTORS * 4096 )

#define VDR_15_START	( VDR_14_START + VDR_14_SECTORS * 4096 )
#define VDR_15_SECTORS	2   /*10条速度状态日志 10*133 实际 10*256*/
#define VDR_15_END		( VDR_15_START + VDR_15_SECTORS * 4096 )

static struct rt_timer tmr_200ms;

struct _sect_info
{
	uint8_t		flag;       /*标志*/
	uint32_t	start_addr; /*开始的地址*/
	uint32_t	addr;       /*最近一条有效记录的地址*/
	uint16_t	rec_size;   /*记录大小*/
	uint8_t		sectors;    /*记录的扇区数*/
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

	uint16_t	blocks;         /*定义每次上传多少个数据块*/
	uint16_t	blocks_remain;  /*当前组织上传包是还需要的的blocks*/
}VDR_CMD;

VDR_CMD vdr_cmd;


/*传递写入文件的信息
   0...3  写入SerialFlash的地址
   4...31 文件名
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

#define MYDATETIME( year, month, day, hour, minute, sec ) \
    ( (uint32_t)( ( year ) << 26 ) | \
      (uint32_t)( ( month ) << 22 ) | \
      (uint32_t)( ( day ) << 17 ) | \
      (uint32_t)( ( hour ) << 12 ) | \
      (uint32_t)( ( minute ) << 6 ) | ( sec ) )
#define YEAR( datetime )	( ( datetime >> 26 ) & 0x3F )
#define MONTH( datetime )	( ( datetime >> 22 ) & 0xF )
#define DAY( datetime )		( ( datetime >> 17 ) & 0x1F )
#define HOUR( datetime )	( ( datetime >> 12 ) & 0x1F )
#define MINUTE( datetime )	( ( datetime >> 6 ) & 0x3F )
#define SEC( datetime )		( datetime & 0x3F )

uint8_t vdr_signal_status = 0x01; /*行车记录仪的状态信号*/

/*外接车速信号*/
__IO uint16_t	IC2Value	= 0;
__IO uint16_t	DutyCycle	= 0;
__IO uint32_t	Frequency	= 0;

/*采用PA.0 作为外部脉冲计数*/
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
/*是不是反向电路?*/
		DutyCycle	= ( IC2Value * 100 ) / TIM_GetCapture1( TIM5 );
		Frequency	= ( RCC_Clocks.HCLK_Frequency ) / 2 / TIM_GetCapture1( TIM5 );
	}else
	{
		DutyCycle	= 0;
		Frequency	= 0;
	}
}

#define SPEED_LIMIT				10          /*速度门限 大于此值认为启动，小于此值认为停止*/
#define SPEED_LIMIT_DURATION	10          /*速度门限持续时间*/

#define SPEED_STATUS_ACC	0x01            /*acc状态 0:关 1:开*/
#define SPEED_STATUS_BRAKE	0x02            /*刹车状态 0:关 1:开*/

#define SPEED_JUDGE_ACC		0x04            /*是否判断ACC*/
#define SPEED_JUDGE_BRAKE	0x08            /*是否判断BRAKE 刹车信号*/

#define SPEED_USE_PULSE 0x10                /*使用脉冲信号 0:不使用 1:使用*/
#define SPEED_USE_GPS	0x20                /*使用gps信号 0:不使用 1:使用*/

struct _vehicle_status
{
	uint8_t status;                         /*当前车辆状态 0:停止 1:启动*/
	uint8_t logic;                          /*当前逻辑状态*/

	uint8_t		pulse_speed_judge_duration; /*速度门限持续时间*/
	uint8_t		pulse_speed;                /*速度值*/
	uint32_t	pulse_duration;             /*持续时间-秒*/

	uint8_t		gps_judge_duration;         /*速度门限持续时间*/
	uint8_t		gps_speed;                  /*速度值，当前速度值*/
	uint32_t	gps_duration;               /*持续时间-秒*/
} car_status =
{
	0, ( SPEED_USE_GPS ), 0, 0, 0, 0, 0, 0
};


/*
   200ms定时器
   监测迈速，事故疑点和速度校准

   记录仪应能以0.2s的时间间隔持续记录并存储行驶结束前20s实时时间对应的行驶状态数据，该行
   驶状态数据为：车辆行驶速度、制动等状态信号和行驶结束时的位置信息。

   在车辆行驶状态下记录仪外部供电断开时，记录仪应能以0.2s的时间间隔持续记录并存储断电前
   20s内的车辆行驶状态数据，该行驶状态数据为：车辆行驶速度、车辆制动等状态信号及断电时的
   位置信息。

   在车辆处于行驶状态且有效位置信息10s内无变化时，记录仪应能以0.2s的时间间隔持续记录并存
   储断电前20s内的车辆行驶状态数据，该行驶状态数据为：车辆行驶速度、车辆制动等状态信号及
   断电时的位置信息。

 */
static void cb_tmr_200ms( void* parameter )
{
/*判断驾驶状态*/
}

#if 0


/*
   初始化记录数据
   找到最近一条记录的位置
 */
static uint32_t vdr_init_byid( uint8_t id, uint8_t *p )
{
	uint16_t	sect, offset;
	uint8_t		*prec;
	uint32_t	mytime_curr = 0;
	uint32_t	mytime_vdr	= 0;
	uint32_t	addr;
	uint8_t		flag; /*标志*/
	uint16_t	rec_size;

	uint8_t		i = id - 8;

	addr		= sect_info[i].start_addr;
	flag		= sect_info[i].flag;
	rec_size	= sect_info[i].rec_size;

	for( sect = 0; sect < sect_info[i].sectors; sect++ )
	{
		sst25_read( sect_info[i].start_addr + sect * 4096, p, 4096 );                               /*一次读入4096字节*/
		for( offset = 0; offset < 4096; offset += rec_size )                                        /*按照记录大小遍历*/
		{
			prec = p + offset;
			if( prec[0] == flag )                                                                   /*每个记录头都是 <flag><mydatetime(4byte)>  有效数据*/                                                                   /*是有效的数据包*/
			{
				/*注意存储的顺序，不能简单的BYTESWAP4*/
				mytime_curr = ( prec[4] << 24 ) | ( prec[3] << 16 ) | ( prec[2] << 8 ) | prec[1];   /*整分钟时刻*/
				if( mytime_curr > mytime_vdr )
				{
					mytime_vdr	= mytime_curr;
					addr		= sect_info[i].start_addr + sect * 4096 + offset;
				}
			}else if( prec[0] != 0xFF )                                                             /*错误的记录头*/
			{
				rt_kprintf( "%d>vdr_init err i=%d,addr=%08x\r\n", i, prec );
			}
		}
	}
	sect_info[i].addr = addr;
	rt_kprintf( "\r\n%d>sect:%02d addr=%08x datetime:%02d-%02d-%02d %02d:%02d:%02d", rt_tick_get( ), id, addr, YEAR( mytime_vdr ), MONTH( mytime_vdr ), DAY( mytime_vdr ), HOUR( mytime_vdr ), MINUTE( mytime_vdr ), SEC( mytime_vdr ) );
}

#endif

static uint32_t vdr_08_addr = VDR_08_END;   /*当前要写入的地址*/
static MYTIME	vdr_08_time = 0xFFFFFFFF;   /*当前时间戳*/
static uint8_t	vdr_08_info[126];           /*保存要写入的信息*/
static uint8_t	vdr_08_sect_index = 0;      /*在一个sect内的记录索引,每个sector存32个*/

static MYTIME	vdr_09_time			= 0xFFFFFFFF;
static uint16_t vdr_09_sect			= 0;    /*当前要写入的sect号*/
static uint8_t	vdr_09_sect_index	= 0;    /*在一个sect内的记录索引,每个sector存6个*/


/*
   从buf中获取时间信息
   如果是0xFF 组成的!!!!!!!特殊对待

 */
static MYTIME mytime_from_buf( uint8_t* buf )
{
	uint8_t year, month, day, hour, minute, sec;
	uint8_t *psrc = buf;
	if( *psrc == 0xFF ) /*不是有效的数据*/
	{
		return 0xFFFFFFFF;
	}
	year	= BCD2HEX( *psrc++ );
	month	= BCD2HEX( *psrc++ );
	day		= BCD2HEX( *psrc++ );
	hour	= BCD2HEX( *psrc++ );
	minute	= BCD2HEX( *psrc++ );
	sec		= BCD2HEX( *psrc );
	return MYDATETIME( year, month, day, hour, minute, sec );
}

/*把时间存到buffer中  4byte=>6byte*/
static void mytime_to_buf( MYTIME time, uint8_t* buf )
{
	uint8_t *psrc = buf;

	*psrc++ = HEX2BCD( YEAR( time ) );
	*psrc++ = HEX2BCD( MONTH( time ) );
	*psrc++ = HEX2BCD( DAY( time ) );
	*psrc++ = HEX2BCD( HOUR( time ) );
	*psrc++ = HEX2BCD( MINUTE( time ) );
	*psrc	= HEX2BCD( SEC( time ) );
}

typedef __packed struct _vdr_08_userdata
{
//	MYTIME		start;          /*开始时刻*/
//	MYTIME		end;            /*结束时刻*/
	uint16_t	record_total;   /*总得记录数*/
	uint16_t	record_remain;  /*还没有发送的记录数*/

	uint32_t	addr_from;      /*信息开始的地址*/
	uint32_t	addr_to;        /*信息结束的地址*/
	uint32_t	addr;           /*当前要读的地址*/

	uint16_t	rec_per_packet; /*每包的记录数*/
	uint16_t	packet_total;   /*总包数*/
	uint16_t	packet_curr;    /*当前包数*/
}VDR_08_USERDATA;

VDR_08_USERDATA vdr_08_userdata;


/*
   就顺序存储,没有那么复杂

 */
void vdr_08_init( void )
{
	uint32_t	addr, addr_max;
	MYTIME		curr, old = 0;
	uint8_t		buf[16];

	memset( vdr_08_info, 0xFF, sizeof( vdr_08_info ) ); /*新的记录，初始化为0xFF*/

	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );

	/*先做个搽除吧*/
	for( addr = VDR_08_START; addr < VDR_08_END; addr += 4096 )
	{
		//	sst25_erase_4k( addr );
	}
	/*遍历一遍*/
	for( addr = VDR_08_START; addr < VDR_08_END; addr += 128 )                      /*每个sector的第一个128 保存该半小时的信息,第一个有效记录的时刻*/
	{
		sst25_read( addr, buf, 4 );
		curr = ( buf[0] << 24 ) | ( buf[1] << 16 ) | ( buf[2] << 8 ) | ( buf[3] );  /*前4个字节代表时间*/
		if( curr != 0xFFFFFFFF )                                                    /*是有效记录*/
		{
			if( curr >= old )                                                       /*找到更新的记录*/
			{
				old			= curr;
				addr_max	= addr;                                                 /*当前记录的地址*/
			}
		}
	}

	rt_sem_release( &sem_dataflash );
	if( old )                                                                       /*不获取时间，否则会影响首包数据的vdr_08_put*/
	{
		vdr_08_addr = addr_max;                                                     /*当前有记录，记录的地址*/
	}
	rt_kprintf( ">vdr08(%08x) %02d-%02d-%02d %02d:%02d:%02d\r\n", vdr_08_addr, YEAR( old ), MONTH( old ), DAY( old ), HOUR( old ), MINUTE( old ), SEC( old ) );
}

/*保存数据 以1秒的间隔保存数据能否处理的过来**/
void vdr_08_put( MYTIME datetime, uint8_t speed, uint8_t status )
{
	uint32_t i, sec;

	if( ( vdr_08_time & 0xFFFFFFC0 ) != ( datetime & 0xFFFFFFC0 ) ) /*不是在当前的一分钟内*/
	{
		if( vdr_08_time != 0xFFFFFFFF )                             /*是有效的数据,要保存，注意第一个数据的处理*/
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

			vdr_08_addr += 128;                 /*增加一条新纪录*/
			if( vdr_08_addr >= VDR_08_END )     /*判断是否环回*/
			{
				vdr_08_addr = VDR_08_START;
			}
			rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );
			if( ( vdr_08_addr & 0xFFF ) == 0 )  /*在4k边界处*/
			{
				sst25_erase_4k( vdr_08_addr );
			}
			sst25_write_through( vdr_08_addr, vdr_08_info, sizeof( vdr_08_info ) );
			rt_sem_release( &sem_dataflash );
			rt_kprintf( "%d>写入08 地址:%08x 值:%08x\r\n", rt_tick_get( ), vdr_08_addr, vdr_08_time );

#endif
			memset( vdr_08_info, 0xFF, sizeof( vdr_08_info ) ); /*新的记录，初始化为0xFF*/
		}
	}
	sec							= SEC( datetime );
	vdr_08_time					= datetime;
	vdr_08_info[sec * 2 + 4]	= gps_speed;
	vdr_08_info[sec * 2 + 5]	= vdr_signal_status;
}

/*
   获取08数据
   约束条件，地址范围  ，起始结束时间，单包数据大小
 */
JT808_MSG_STATE vdr_08_fill_data_old( JT808_TX_NODEDATA *pnodedata )
{
	uint32_t		i, count = 0;
	uint8_t			*pdata_body;
	uint8_t			*pdata_head;
	uint8_t			*pdata;
	VDR_08_USERDATA * puserdata;
	uint32_t		addr;
	uint8_t			buf[126];
	MYTIME			mytime;
	uint8_t			fcs				= 0;
	uint16_t		read_data_size	= 0;

	if( pnodedata == RT_NULL )
	{
		return;
	}

	puserdata = (VDR_08_USERDATA* )( pnodedata->user_para ); /*指向用户数据区*/
#if 0
	rt_kprintf( "puserdata->addr=%08x\r\n", puserdata->addr );
	rt_kprintf( "puserdata->addr_from=%08x\r\n", puserdata->addr_from );
	rt_kprintf( "puserdata->addr_to=%08x\r\n", puserdata->addr_to );
	rt_kprintf( "puserdata->record_total=%d\r\n", puserdata->record_total );
	rt_kprintf( "puserdata->record_remain=%d\r\n", puserdata->record_remain ); 3
#endif


	/*
	   判断是否已发送完成
	   可以判断
	   puserdata->packet_curr
	   或puserdata->record_remain
	 */
	if( puserdata->record_remain == 0 ) /*已经发送完数据*/
	{
		return IDLE;
	}
	/*准备获取数据*/
	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );

	pdata_head = pnodedata->tag_data;   /*真实发送数据的开始位置->消息头*/
	rt_kprintf( "pdata_head=%p\r\n", pdata_head );

	pdata_head[0]	= pnodedata->head_id >> 8;
	pdata_head[1]	= pnodedata->head_id & 0xFF;
	pnodedata->head_sn++;
	pdata_head[10]	= pnodedata->head_sn >> 8;
	pdata_head[11]	= pnodedata->head_sn & 0xFF;
	memcpy( pdata_head + 4, mobile, 6 );    /*拷贝终端号码,此时可能还没有mobileh号码*/
	if( pnodedata->multipacket )            /*多包发送*/
	{
		pdata_head[12]	= puserdata->packet_total >> 8;
		pdata_head[13]	= puserdata->packet_total & 0xFF;
		puserdata->packet_curr++;
		pdata_head[14]	= puserdata->packet_curr >> 8;
		pdata_head[15]	= puserdata->packet_curr & 0xFF;

		pdata_head[2]	|= 0x20; /*多包发送*/
		pdata_body		= pdata_head + 16;
	}else
	{
		pdata_head[2]	= 0;
		pdata_body		= pdata_head + 12;
	}
	rt_kprintf( "pdata_body=%p\r\n", pdata_body );

	addr	= puserdata->addr;                              /*从当前地址开始倒序读,每次4个record*/
	pdata	= pdata_body;

	/*获取数据*/
	for( count = 0; count < 4; count++ )
	{
		sst25_read( addr, buf + 2, 124 );                   /*在存数据时没有存BCD的日期，而是MYTIME的格式*/
		mytime	= (uint32_t)( buf[2] << 24 ) | (uint32_t)( buf[3] << 16 ) | (uint32_t)( buf[4] << 8 ) | (uint32_t)( buf[5] );
		buf[0]	= HEX2BCD( YEAR( mytime ) );
		buf[1]	= HEX2BCD( MONTH( mytime ) );
		buf[2]	= HEX2BCD( DAY( mytime ) );
		buf[3]	= HEX2BCD( HOUR( mytime ) );
		buf[4]	= HEX2BCD( MINUTE( mytime ) );
		buf[5]	= 0;                                        /*按照要求秒为0*/
		if( puserdata->record_total > 4 )                   /*多包*/
		{
			memcpy( pdata + 6 + count * 126, buf, 126 );    /*拷贝126个到发送区*/
		}
		read_data_size	+= 126;
		addr			-= 128;                             /*定位到前一个记录*/
		if( addr < VDR_08_START )
		{
			addr = VDR_08_END - 128;
		}
		puserdata->addr = addr;                             /*指向新的地址*/
		/*不能拿地址puserdata->addr_to判断是否完成，如果addr_to=VDR_08_START?,用记录数判断*/
		puserdata->record_remain--;
		if( puserdata->record_remain == 0 )
		{
			break;
		}
	}
	rt_sem_release( &sem_dataflash );
	/*计算VDR的FCS*/
	pdata		= pdata_body;
	pdata[0]	= 0x55;
	pdata[1]	= 0x7A;
	pdata[2]	= 0x08;
	pdata[3]	= ( read_data_size ) >> 8;  /*并不知道实际发送的大小*/
	pdata[4]	= ( read_data_size ) & 0xFF;
	pdata[5]	= 0;                        /*预留*/

	for( i = 0; i < ( read_data_size + 6 ); i++ )
	{
		fcs ^= *pdata++;
	}
	*pdata = fcs;

	/*补充发送信息*/
	if( pnodedata->multipacket )
	{
		pnodedata->msg_len = 12 + 4 + 7 + read_data_size;
	}else
	{
		pnodedata->msg_len = 12 + 7 + read_data_size;
	}
	pdata_head[2]	= 0x20 | ( pnodedata->msg_len >> 8 );
	pdata_head[3]	= pnodedata->msg_len & 0xFF;

/*控制发送状态*/
	pnodedata->max_retry	= 1;
	pnodedata->retry		= 0;
	pnodedata->timeout		= RT_TICK_PER_SECOND * 3;

	pnodedata->state = IDLE;

#if 1                                                               /*dump 数据*/
	rt_kprintf( "%d>生成 %d bytes\r\n", rt_tick_get( ), pnodedata->msg_len );
	rt_kprintf( "linkno=%d\r\n", pnodedata->linkno );               /*传输使用的link,包括了协议和远端socket*/
	rt_kprintf( "multipacket=%d\r\n", pnodedata->multipacket );     /*是不是多包发送*/
	rt_kprintf( "type=%d\r\n", pnodedata->type );                   /*发送消息的类型*/
	rt_kprintf( "state=%d\r\n", pnodedata->state );                 /*发送状态*/
	rt_kprintf( "retry=%d\r\n", pnodedata->retry );                 /*重传次数,递增，递减找不到*/
	rt_kprintf( "max_retry=%d\r\n", pnodedata->max_retry );         /*最大重传次数*/
	rt_kprintf( "timeout=%d\r\n", pnodedata->timeout );             /*超时时间*/
//	rt_kprintf( "timeout_tick=%x\r\n", pnodedata->timeout_tick );   /*达到超时的tick值*/
	rt_kprintf( "head_id=%04x\r\n", pnodedata->head_id );           /*消息ID*/
	rt_kprintf( "head_sn=%04x\r\n", pnodedata->head_sn );           /*消息流水号*/

	rt_kprintf( "puserdata->addr=%08x\r\n", puserdata->addr );
	rt_kprintf( "puserdata->addr_from=%08x\r\n", puserdata->addr_from );
	rt_kprintf( "puserdata->addr_to=%08x\r\n", puserdata->addr_to );
	rt_kprintf( "puserdata->record_total=%d\r\n", puserdata->record_total );
	rt_kprintf( "puserdata->record_remain=%d\r\n", puserdata->record_remain );

	pdata = pnodedata->tag_data;
	//for( i = 0; i < pnodedata->msg_len; i++ )
	for( i = 0; i < 32; i++ )
	{
		if( ( i % 16 ) == 0 )
		{
			rt_kprintf( "\r\n" );
		}
		rt_kprintf( "%02x ", *pdata++ );
	}

#endif
}

/*
   获取08数据
   约束条件，地址范围  ，起始结束时间，单包数据大小
 */
uint8_t vdr_08_fill_data( JT808_TX_NODEDATA *pnodedata )
{
	uint32_t		i, count = 0;
	uint8_t			*pdata_body;
	uint8_t			*pdata_head;
	uint8_t			*pdata;
	VDR_08_USERDATA * puserdata;
	uint32_t		addr;
	uint8_t			buf[126];
	MYTIME			mytime;
	uint8_t			fcs				= 0;
	uint16_t		read_data_size	= 0;

	puserdata = (VDR_08_USERDATA* )( pnodedata->user_para );    /*指向用户数据区*/
	if( puserdata->record_remain == 0 )                         /*已经发送完数据*/
	{
		return 0;
	}
	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );      /*准备获取数据*/
	if( pnodedata->multipacket )
	{
		pdata_body = pnodedata->tag_data + 16;
	}else
	{
		pdata_body = pnodedata->tag_data + 12;
	}
	addr	= puserdata->addr;                              /*从当前地址开始倒序读,每次4个record*/
	pdata	= pdata_body;
	/*获取数据*/
	for( count = 0; count < 4; count++ )
	{
		sst25_read( addr, buf + 2, 124 );                   /*在存数据时没有存BCD的日期，而是MYTIME的格式*/
		mytime	= (uint32_t)( buf[2] << 24 ) | (uint32_t)( buf[3] << 16 ) | (uint32_t)( buf[4] << 8 ) | (uint32_t)( buf[5] );
		buf[0]	= HEX2BCD( YEAR( mytime ) );
		buf[1]	= HEX2BCD( MONTH( mytime ) );
		buf[2]	= HEX2BCD( DAY( mytime ) );
		buf[3]	= HEX2BCD( HOUR( mytime ) );
		buf[4]	= HEX2BCD( MINUTE( mytime ) );
		buf[5]	= 0;                                        /*按照要求秒为0*/
		if( puserdata->record_total > 4 )                   /*多包*/
		{
			memcpy( pdata + 6 + count * 126, buf, 126 );    /*拷贝126个到发送区*/
		}
		read_data_size	+= 126;
		addr			-= 128;                             /*定位到前一个记录*/
		if( addr < VDR_08_START )
		{
			addr = VDR_08_END - 128;
		}
		puserdata->addr = addr;                             /*指向新的地址*/
		/*不能拿地址puserdata->addr_to判断是否完成，如果addr_to=VDR_08_START?,用记录数判断*/
		puserdata->record_remain--;
		if( puserdata->record_remain == 0 )
		{
			break;
		}
	}
	rt_sem_release( &sem_dataflash );
	/*计算VDR的FCS*/
	pdata		= pdata_body;
	pdata[0]	= 0x55;
	pdata[1]	= 0x7A;
	pdata[2]	= 0x08;
	pdata[3]	= ( read_data_size ) >> 8;  /*并不知道实际发送的大小*/
	pdata[4]	= ( read_data_size ) & 0xFF;
	pdata[5]	= 0;                        /*预留*/

	for( i = 0; i < ( read_data_size + 6 ); i++ )
	{
		fcs ^= *pdata++;
	}
	*pdata = fcs;

	pdata_head = pnodedata->tag_data;                        /*真实发送数据的开始位置->消息头*/
	rt_kprintf( "pdata_head=%p\r\n", pdata_head );

	pdata_head[0]	= pnodedata->head_id >> 8;
	pdata_head[1]	= pnodedata->head_id & 0xFF;
	pnodedata->head_sn++;
	pdata_head[10]	= pnodedata->head_sn >> 8;
	pdata_head[11]	= pnodedata->head_sn & 0xFF;
	memcpy( pdata_head + 4, mobile, 6 );    /*拷贝终端号码,此时可能还没有mobileh号码*/

	read_data_size	+= 7;                   /*增加VDR的数据头和校验*/
	pdata_head[2]	= read_data_size >> 8;
	pdata_head[3]	= read_data_size & 0xFF;

	pnodedata->msg_len = read_data_size + 12;
	if( pnodedata->multipacket )            /*多包发送*/
	{
		pdata_head[12]	= puserdata->packet_total >> 8;
		pdata_head[13]	= puserdata->packet_total & 0xFF;
		puserdata->packet_curr++;
		pdata_head[14]		= puserdata->packet_curr >> 8;
		pdata_head[15]		= puserdata->packet_curr & 0xFF;
		pdata_head[2]		|= 0x20;        /*多包发送*/
		pnodedata->msg_len	= read_data_size + 16;
	}
/*控制发送状态*/
	pnodedata->max_retry	= 1;
	pnodedata->retry		= 0;
	pnodedata->timeout		= RT_TICK_PER_SECOND * 3;
	pnodedata->state		= IDLE;

#if 1                                                               /*dump 数据*/
	rt_kprintf( "%d>生成 %d bytes\r\n", rt_tick_get( ), pnodedata->msg_len );
	rt_kprintf( "linkno=%d\r\n", pnodedata->linkno );               /*传输使用的link,包括了协议和远端socket*/
	rt_kprintf( "multipacket=%d\r\n", pnodedata->multipacket );     /*是不是多包发送*/
	rt_kprintf( "type=%d\r\n", pnodedata->type );                   /*发送消息的类型*/
	rt_kprintf( "state=%d\r\n", pnodedata->state );                 /*发送状态*/
	rt_kprintf( "retry=%d\r\n", pnodedata->retry );                 /*重传次数,递增，递减找不到*/
	rt_kprintf( "max_retry=%d\r\n", pnodedata->max_retry );         /*最大重传次数*/
	rt_kprintf( "timeout=%d\r\n", pnodedata->timeout );             /*超时时间*/
//	rt_kprintf( "timeout_tick=%x\r\n", pnodedata->timeout_tick );   /*达到超时的tick值*/
	rt_kprintf( "head_id=%04x\r\n", pnodedata->head_id );           /*消息ID*/
	rt_kprintf( "head_sn=%04x\r\n", pnodedata->head_sn );           /*消息流水号*/

	rt_kprintf( "puserdata->addr=%08x\r\n", puserdata->addr );
	rt_kprintf( "puserdata->addr_from=%08x\r\n", puserdata->addr_from );
	rt_kprintf( "puserdata->addr_to=%08x\r\n", puserdata->addr_to );
	rt_kprintf( "puserdata->record_total=%d\r\n", puserdata->record_total );
	rt_kprintf( "puserdata->record_remain=%d\r\n", puserdata->record_remain );

	pdata = pnodedata->tag_data;
#if 0
	for( i = 0; i < 32; i++ )
	{
		if( ( i % 16 ) == 0 )
		{
			rt_kprintf( "\r\n" );
		}
		rt_kprintf( "%02x ", *pdata++ );
	}
#endif

	return 1;

#endif
}

/*收到应答的处理函数*/
static JT808_MSG_STATE vdr_08_tx_response( JT808_TX_NODEDATA * pnodedata, uint8_t *pmsg )
{
	pnodedata->retry++;
	if( pnodedata->retry >= pnodedata->max_retry )
	{
		return WAIT_DELETE;
	}
	return vdr_08_fill_data( pnodedata );
}

/*超时后的处理函数*/
static JT808_MSG_STATE vdr_08_tx_timeout( JT808_TX_NODEDATA * pnodedata )
{
	vdr_08_fill_data( pnodedata );
}

/*定位数据的地址?
   从指定的结束时间之前最近的第1分钟的行驶速度记录开始
   要不要传递进nodedata?
   准备要发送的数据，约束条件:开始结束时刻，总的block数

 */
void vdr_08_get_ready( MYTIME start, MYTIME end, uint16_t totalrecord )
{
	uint32_t			addr;
	JT808_TX_NODEDATA	*pnodedata;
	VDR_08_USERDATA		*puserdata;
	uint32_t			i;
	uint16_t			rec_count = 0;              /*找到的记录数*/
	uint8_t				buf[126 * 4];

	uint32_t			addr_from	= 0xFFFFFFFF;   /*开始的地址*/
	uint32_t			addr_to		= 0xFFFFFFFF;   /*结束的地址*/

	uint32_t			time_from	= 0xFFFFFFFF;   /*上报记录 开始的时刻，最近的*/
	uint32_t			time_to		= 0xFFFFFFFF;   /*上报记录 结束的时刻，最远的*/

	MYTIME				mytime;

/*从当前位置开始逆序查找*/
	start	&= 0xFFFFFFC0;                          /*忽略秒*/
	end		&= 0xFFFFFFC0;

	rt_kprintf( "%d>开始遍历数据\r\n", rt_tick_get( ) );

	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );
	addr = vdr_08_addr + 128;                       /*判断中做了地址递减*/
	for( i = 0; i < VDR_08_SECTORS * 32; i++ )      /*总共找的记录数，完全使用地址判断会不方便*/
	{
		addr -= 128;
		if( addr < VDR_08_START )                   /*环回*/
		{
			addr = VDR_08_END - 128;
		}
		sst25_read( addr, buf, 4 );
		mytime = (uint32_t)( buf[0] << 24 ) | (uint32_t)( buf[1] << 16 ) | (uint32_t)( buf[2] << 8 ) | (uint32_t)( buf[3] );
		if( mytime == 0xFFFFFFFF )
		{
			continue;                                   /*不是有效记录继续查找*/
		}
		mytime &= 0xFFFFFFC0;                           /*忽略秒,对应到分钟*/
		if( ( mytime <= end ) && ( mytime >= start ) )  /*是在时间段内[0..0xffffffff]*/
		{
			rec_count++;                                /*有效记录*/
			if( rec_count >= totalrecord )              /*收到足够的记录数*/
			{
				break;
			}
			if( time_from == 0xFFFFFFFF )               /*记录开始的地址,第一个找到的就是最近的*/
			{
				time_from	= mytime;
				addr_from	= addr;
			}
			if( mytime < time_to )                      /*逆序*/
			{
				time_to = mytime;
				addr_to = addr;
			}
		}
	}

	rt_sem_release( &sem_dataflash );

	rt_kprintf( "%d>遍历结束(%d条) form:%08x to:%08x\r\n", rt_tick_get( ), rec_count, addr_from, addr_to );

/*默认按多包分配*/
	puserdata = rt_malloc( sizeof( VDR_08_USERDATA ) );
	if( puserdata == RT_NULL )
	{
		return;
	}
	puserdata->record_total		= rec_count;
	puserdata->record_remain	= rec_count;
	puserdata->addr_from		= addr_from;
	puserdata->addr_to			= addr_to;
	puserdata->addr				= addr_from;
	puserdata->packet_total		= ( rec_count + 3 ) / 4; /*每四条记录形成一包*/
	puserdata->packet_curr		= 0;

	pnodedata = node_begin( 1, MULTI, 0x0700, 0xF000, 126 * 4 + 7 );
	if( pnodedata == RT_NULL )
	{
		rt_free( puserdata );
		puserdata = RT_NULL;
		rt_kprintf( "无法分配\r\n" );
	}
	pnodedata->user_para		= puserdata;
	pnodedata->cb_tx_timeout	= vdr_08_tx_timeout;
	pnodedata->cb_tx_response	= vdr_08_tx_response;
	vdr_08_fill_data( pnodedata );
	node_end( pnodedata );
}

FINSH_FUNCTION_EXPORT_ALIAS( vdr_08_get_ready, get_08, get_08_data );


/*
   单位小时内每分钟的位置信息和速度
   最快频率，每分钟一组数据
   360小时=15天
   每天4个sector 每个sector对应6小时，每小时  680字节
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

/*每四个sector代表一天*/
	for( sector = 0; sector < VDR_09_SECTORS; sector++ )
	{
	}

	for( addr = VDR_08_START; addr < VDR_08_END; addr += 4096 )
	{
		sst25_read( addr, buf, 32 );

		mytime_curr = ( buf[0] << 24 ) | ( buf[1] << 16 ) | ( buf[2] << 8 ) | buf[3];   /*前4个字节代表时间*/

		mytime_curr &= 0xFFFFFFC0;                                                      /*频蔽掉sec*/

		if( mytime_curr != 0xFFFFFFFF )                                                 /*是有效记录*/
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
	if( find == 0 ) /*没有找到有效记录*/
	{
		vdr_08_addr = VDR_08_START + VDR_08_SECTORS * 4096;
	}
	rt_kprintf( "%d>vdr_08 addr=%08x datetime=%08x\r\n", vdr_08_addr, vdr_08_time );
}

/*
   每扇区存6个小时的数据
   一个小时666byte 占用680byte

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

	if( ( vdr_09_time & 0xFFFFF000 ) != ( datetime & 0xFFFFF000 ) ) /*不是是在当前的小时内*/
	{
		vdr_09_time = datetime;
		vdr_09_sect_index++;                                        /*调整*/
		if( vdr_09_sect_index > 5 )                                 /*每个sector存6个小时 0-5*/
		{
			vdr_09_sect_index = 0;
			vdr_09_sect++;
			if( vdr_09_sect >= VDR_09_SECTORS )
			{
				vdr_09_sect = 0;
			}
			addr = VDR_09_START + vdr_09_sect * 4096;   /*删除扇区*/
#ifdef VDR_DEBUG
			rt_kprintf( "%d>写入新的扇区 vdr_09_sect=%d\r\n", rt_tick_get( ), vdr_09_sect );
#else
			sst25_erase_4k( addr );
			PACK_INT( bufhead, datetime );              /*写入新的记录头*/
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
   rt_kprintf( "%d>写入已有数据 vdr_09_sect=%d\r\n", rt_tick_get( ), vdr_09_sect );
   for( i = 0; i < 11; i++ )
   {
   rt_kprintf( "%02x ", buf[i] );
   }
   rt_kprintf("\r\n");
 */
#else

#endif
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
JT808_MSG_STATE vdr_09_fill_data( JT808_TX_NODEDATA *pnodedata )
{
}

/*收到应答的处理函数*/
static void vdr_09_tx_response( JT808_TX_NODEDATA * nodedata, uint8_t *pmsg )
{
	vdr_09_fill_data( nodedata );
}

/*超时后的处理函数*/
static void vdr_09_tx_timeout( JT808_TX_NODEDATA * nodedata )
{
	vdr_09_fill_data( nodedata );
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
void vdr_09_get_ready( MYTIME start, MYTIME end, uint16_t totalrecord )
{
}

/*
   保存行驶记录数据
   注意区分不同的数据类型
   id=0
 */
static vdr_save_rec( uint8_t sect_id, uint8_t * pdata, uint16_t len )
{
	uint8_t id = sect_id - 8;
/*增加一条新纪录*/
	sect_info[id].addr += sect_info[id].rec_size;
/*判断是否环回*/
	if( sect_info[id].addr >= ( sect_info[id].start_addr + sect_info[id].sectors * 4096 ) )
	{
		sect_info[id].addr = sect_info[id].start_addr;
	}

	if( ( sect_info[id].addr & 0xFFF ) == 0 ) /*在4k边界处*/
	{
		sst25_erase_4k( sect_info[id].addr );
	}
	sst25_write_through( sect_info[id].addr, pdata, len );
}


/*
   收到gps数据的处理，有定位和未定位
   存储位置信息，
   速度判断，校准
 */

rt_err_t vdr_rx_gps( void )
{
	uint32_t	datetime;
	uint8_t		year, month, day, hour, minute, sec;
	uint32_t	i;
	uint8_t		buf[128];
	uint8_t		*pbkpsram;

#ifdef TEST_BKPSRAM
/*上电后，有要写入的历史数据 08*/
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

	if( ( jt808_status & BIT_STATUS_GPS ) == 0 ) /*未定位*/
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
   09数据 单位小时内每分钟的位置速度，只保存第一次有效的
   每小时(10+1)*60+6 字节
 */
	//vdr_09_put( datetime );

/*10数据 事故疑点*/
	if( car_status.status == 0 )                                        /*认为车辆停止,判断启动*/
	{
		if( gps_speed >= SPEED_LIMIT )                                  /*速度大于门限值*/
		{
			car_status.gps_judge_duration++;
			if( car_status.gps_judge_duration >= SPEED_LIMIT_DURATION ) /*超过了持续时间*/
			{
				car_status.gps_duration			= SPEED_LIMIT_DURATION;
				car_status.status				= 1;                    /*认为车辆行驶*/
				car_status.gps_judge_duration	= 0;
				rt_kprintf( "%d>车辆行驶\r\n", rt_tick_get( ) );
			}else
			{
				car_status.gps_duration++;                              /*停车累计时间*/
			}
		}else
		{
			car_status.gps_duration++;                                  /*停车累计时间*/
			                                                            /*在此判断停车超时*/
			car_status.gps_judge_duration = 0;
		}
	}else /*车辆已启动*/
	{
		if( gps_speed <= SPEED_LIMIT )                                  /*速度小于门限值*/
		{
			car_status.gps_judge_duration++;
			if( car_status.gps_judge_duration >= SPEED_LIMIT_DURATION ) /*超过了持续时间*/
			{
				car_status.gps_duration			= SPEED_LIMIT_DURATION;
				car_status.status				= 0;                    /*认为车辆停驶*/
				car_status.gps_judge_duration	= 0;
				rt_kprintf( "%d>车辆停驶\r\n", rt_tick_get( ) );
			}else
			{
				car_status.gps_duration++;                              /*行驶累计时间*/
			}
		}else
		{
			car_status.gps_duration++;                                  /*行驶累计时间*/
			                                                            /*判断疲劳驾驶*/
			car_status.gps_judge_duration = 0;
		}
	}

/*11数据超时驾驶记录*/
}

/*
   删除特定区域的记录数据
   使用bitmask是不是更好
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
   获取08存储的状态 48小时 单位分钟内每秒的速度状态2byte
   48*60*128=2880*128=368640 (bytes)
   368640/4096=90(sectors)

   格式:
   <'8'><mydatetime(4bytes><60秒的速度状态120bytes>
   循环递增
 */


/*
   初始化记录区数据
   因为是属于固定时间段存储的
   需要记录开始时刻的sector位置(相对的sector偏移)
 */
rt_err_t vdr_init( void )
{
	uint8_t* pbuf;

	pulse_init( ); /*接脉冲计数*/

	//vdr_format( 0xff00 );

	pbuf = rt_malloc( 4096 );
	if( pbuf == RT_NULL )
	{
		return -RT_ENOMEM;
	}

	vdr_08_init( );
#if 0
	vdr_init_byid( 8, pbuf );
	sst25_read( sect_info[0].addr, (uint8_t*)&stu_rec_08, sizeof( STU_REC_08 ) ); /*读出来是防止一分钟内的重启*/
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
/*初始化一个50ms的定时器，用作事故疑点判断*/
	rt_timer_init( &tmr_200ms, "tmr_200ms",             /* 定时器名字是 tmr_50ms */
	               cb_tmr_200ms,                        /* 超时时回调的处理函数 */
	               RT_NULL,                             /* 超时函数的入口参数 */
	               RT_TICK_PER_SECOND / 5,              /* 定时长度，以OS Tick为单位 */
	               RT_TIMER_FLAG_PERIODIC );            /* 周期性定时器 */
	rt_timer_start( &tmr_200ms );
}

/*行车记录仪数据采集命令*/
void vdr_rx_8700( uint8_t * pmsg )
{
	uint8_t		* psrc;
	uint8_t		buf[500];

	uint16_t	seq = JT808HEAD_SEQ( pmsg );
	uint16_t	len = JT808HEAD_LEN( pmsg );
	uint8_t		cmd = *( pmsg + 12 );   /*跳过前面12字节的头*/
	MYTIME		start, end;
	uint16_t	blocks;

	switch( cmd )
	{
		case 0:                         /*采集记录仪执行标准版本*/
			buf[0]	= seq >> 8;
			buf[1]	= seq & 0xff;
			buf[2]	= cmd;
			memcpy( buf + 3, "\x55\x7A\x00\x00\x02\x00\x12\x00", 8 );
			//jt808_add_tx_data_single( 1, TERMINAL_ACK, 0x0700, 11, buf, RT_NULL, RT_NULL );
			jt808_tx( 0x0700, buf, 11 );
			break;
		case 1:
			buf[0]	= seq >> 8;
			buf[1]	= seq & 0xff;
			buf[2]	= cmd;
			memcpy( buf + 3, "\x55\x7A\x01\x00\x12\x00120221123456789\x00\x00\x00\x00", 25 );
			//jt808_add_tx_data_single( 1, TERMINAL_ACK, 0x0700, 28, buf, RT_NULL, RT_NULL );
			jt808_tx( 0x0700, buf, 28 );
		case 2: /*行车记录仪时间*/
			buf[0]	= seq >> 8;
			buf[1]	= seq & 0xff;
			buf[2]	= cmd;
			sprintf( buf + 3, "\x55\x7A\x02\x00\x06\x00%6s", gps_baseinfo.datetime );
			//jt808_add_tx_data_single( 1, TERMINAL_ACK, 0x0700, 15, buf, RT_NULL, RT_NULL );
			jt808_tx( 0x0700, buf, 15 );

		case 3:
			break;
		case 4:
			break;
		case 5:
			break;
		case 6:
			break;
		case 8:                                 /*08记录*/
			if( len == 1 )                      /*获取所有记录*/
			{
				start	= 0;
				end		= 0xffffffff;
				blocks	= 0;
			}else if( len == 22 )               /*下来的格式 <55><7A><cmd><len(2byte)><保留><data_block(14byte)><XOR>*/
			{
				psrc	= pmsg + 12 + 1 + 7;    /*12字节头+1byte命令字+7字节vdr数据*/
				start	= mytime_from_buf( psrc );
				end		= mytime_from_buf( psrc + 6 );
				blocks	= ( *( psrc + 32 ) << 8 ) | ( *( psrc + 33 ) );
			}else
			{
				rt_kprintf( "%d>8700的格式不识别\r\n", rt_tick_get( ) );
				return;
			}
			vdr_08_get_ready( start, end, blocks );
			break;
		case 9:
			if( len == 1 )                      /*获取所有记录*/
			{
				start	= 0;
				end		= 0xffffffff;
				blocks	= 0;
			}else if( len == 22 )               /*下来的格式 <55><7A><cmd><len(2byte)><保留><data_block(14byte)><XOR>*/
			{
				psrc	= pmsg + 12 + 1 + 7;    /*12字节头+1byte命令字+7字节vdr数据*/
				start	= mytime_from_buf( psrc );
				end		= mytime_from_buf( psrc + 6 );
				blocks	= ( *( psrc + 32 ) << 8 ) | ( *( psrc + 33 ) );
			}else
			{
				rt_kprintf( "%d>8700的格式不识别\r\n", rt_tick_get( ) );
				return;
			}
			vdr_09_get_ready( start, end, blocks );

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
