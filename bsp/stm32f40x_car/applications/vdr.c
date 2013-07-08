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
#include "jt808_param.h"

#define VDR_DEBUG

//#define TEST_BKPSRAM

typedef uint32_t MYTIME;

/*转换hex到bcd的编码*/
#define HEX2BCD( A )	( ( ( ( A ) / 10 ) << 4 ) | ( ( A ) % 10 ) )
#define BCD2HEX( x )	( ( ( ( x ) >> 4 ) * 10 ) + ( ( x ) & 0x0f ) )

#define PACK_BYTE( buf, byte ) ( *( buf ) = ( byte ) )
#define PACK_WORD( buf, word ) \
    do { \
		*( ( buf ) )		= ( word ) >> 8; \
		*( ( buf ) + 1 )	= ( word ) & 0xff; \
	} \
    while( 0 )

#define PACK_INT( buf, byte4 ) \
    do { \
		*( ( buf ) )		= ( byte4 ) >> 24; \
		*( ( buf ) + 1 )	= ( byte4 ) >> 16; \
		*( ( buf ) + 2 )	= ( byte4 ) >> 8; \
		*( ( buf ) + 3 )	= ( byte4 ) & 0xff; \
	} while( 0 )


/*
   4MB serial flash 0x400000
 */

static rt_thread_t tid_usb_vdr = RT_NULL;

#define VDR_BASE 0x300000

#define VDR_08_START	VDR_BASE
#define VDR_08_SECTORS	100         /*每小时2个sector,保留50小时*/
#define VDR_08_END		( VDR_08_START + VDR_08_SECTORS * 4096 )

#define VDR_09_START	( VDR_08_START + VDR_08_SECTORS * 4096 )
#define VDR_09_SECTORS	64          /*16天，每天4sector*/
#define VDR_09_END		( VDR_09_START + VDR_09_SECTORS * 4096 )

#define VDR_10_START	( VDR_09_START + VDR_09_SECTORS * 4096 )
#define VDR_10_SECTORS	8           /*100条事故疑点 100*234  实际 128*256 */
#define VDR_10_END		( VDR_10_START + VDR_10_SECTORS * 4096 )

#define VDR_11_START	( VDR_10_START + VDR_10_SECTORS * 4096 )
#define VDR_11_SECTORS	3           /*100条超时驾驶记录 100*50 实际 128*64,保留一个扇区，删除时仍有数据*/
#define VDR_11_END		( VDR_11_START + VDR_11_SECTORS * 4096 )

#define VDR_12_START	( VDR_11_START + VDR_11_SECTORS * 4096 )
#define VDR_12_SECTORS	3           /*200条驾驶人身份记录 200*25 实际200*32 */
#define VDR_12_END		( VDR_12_START + VDR_12_SECTORS * 4096 )

#define VDR_13_14_15_START		( VDR_12_START + VDR_12_SECTORS * 4096 )
#define VDR_13_14_15_SECTORS	1   /*100条 外部供电记录 100条 参数修改记录 10条速度状态日志 */
#define VDR_13_14_15_END		( VDR_13_14_15_START + VDR_13_14_15_SECTORS * 4096 )

static struct rt_timer tmr_200ms;

struct _sect_info
{
	uint8_t		flag;               /*标志*/
	uint32_t	addr_start;         /*开始的地址*/
	uint8_t		sector_count;       /*记录的扇区数，占用的扇区数*/
	uint16_t	record_per_sector;  /*每扇区记录数*/
	uint16_t	record_size;        /*记录大小*/
	uint16_t	data_size;          /*用户数据大小*/
	uint8_t		record_per_packet;  /*多包上传时，每包的最大记录数*/
	uint8_t		sector;             /*当前所在的扇区*/
	uint8_t		index;              /*当前所在的扇区内记录位置索引*/
} sect_info[5] =
{
	{ '8', VDR_08_START, VDR_08_SECTORS, 30,  136, 126, 5,	VDR_08_SECTORS, 30,	 },
	{ '9', VDR_09_START, VDR_09_SECTORS, 6,	  680, 666, 1,	VDR_09_SECTORS, 6,	 },
	{ 'A', VDR_10_START, VDR_10_SECTORS, 16,  256, 234, 2,	VDR_10_SECTORS, 16,	 },
	{ 'B', VDR_11_START, VDR_11_SECTORS, 64,  64,  50,	10, VDR_11_SECTORS, 64,	 },
	{ 'C', VDR_12_START, VDR_12_SECTORS, 128, 32,  25,	20, VDR_12_SECTORS, 128, },
};

/*生成UTC时间*/
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

#define SPEED_LIMIT				5   /*速度门限 大于此值认为启动，小于此值认为停止*/
#define SPEED_LIMIT_DURATION	10  /*速度门限持续时间*/

#define SPEED_STATUS_ACC	0x01    /*acc状态 0:关 1:开*/
#define SPEED_STATUS_BRAKE	0x02    /*刹车状态 0:关 1:开*/

#define SPEED_JUDGE_ACC		0x04    /*是否判断ACC*/
#define SPEED_JUDGE_BRAKE	0x08    /*是否判断BRAKE 刹车信号*/

#define SPEED_USE_PULSE 0x10        /*使用脉冲信号 0:不使用 1:使用*/
#define SPEED_USE_GPS	0x20        /*使用gps信号 0:不使用 1:使用*/

enum _stop_run
{
	STOP=0,
	RUN =1,
};

struct _vehicle_status
{
	enum _stop_run	stop_run;       /*当前车辆状态 0:停止 1:启动*/
	uint8_t			logic;          /*当前逻辑状态*/
	uint8_t			pulse_speed;    /*速度值*/
	uint32_t		pulse_duration; /*持续时间-秒*/
	uint8_t			gps_speed;      /*速度值，当前速度值*/
	uint32_t		gps_duration;   /*持续时间-秒*/
} car_status =
{
	STOP, ( SPEED_USE_GPS ), 0, 0, 0, 0
};

/*200ms间隔的速度状态信息*/
static uint8_t	speed_status[100][2];
static uint8_t	speed_status_index	= 0;
static uint32_t utc_speed_status	= 0; /*最近保存保存记录的时刻*/


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


/*
   每200毫秒检测并更新状态
 */
static void cb_tmr_200ms( void* parameter )
{
	__IO uint8_t i;

	i									= ( speed_status_index + 99 ) % 100;    /*计算上一个位置，带环回*/
	speed_status[speed_status_index][0] = speed_status[i][0];                   /*等于上一次的值*/
	speed_status[speed_status_index][1] = vdr_signal_status;

	speed_status_index++;
	if( speed_status_index > 99 )
	{
		speed_status_index = 0;
	}
}

static MYTIME	vdr_08_time = 0;            /*当前时间戳*/
static uint8_t	vdr_08_info[130];           /*保存要写入的信息*/
static MYTIME	vdr_09_time = 0;
static MYTIME	vdr_10_time = 0;

static uint32_t utc_car_stop	= 0;        /*车辆开始停驶时刻*/
static uint32_t utc_car_run		= 0;        /*车辆开始行驶时刻*/


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

typedef __packed struct _vdr_userdata
{
	uint8_t		id;             /*对应的vdr类型 8-15*/
	uint16_t	record_total;   /*总得记录数*/
	uint16_t	record_remain;  /*还没有发送的记录数*/

	uint32_t	sector_from;    /*信息开始的扇区*/
	uint32_t	index_from;     /*信息开始的扇区内记录号*/

	uint32_t	sector_to;      /*信息结束的扇区*/
	uint32_t	index_to;       /*信息结束的扇区内记录号*/

	uint32_t	sector;         /*当前操作信息的扇区*/
	uint32_t	index;          /*当前操作信息的扇区内记录号*/

	uint32_t addr;              /*当前要读的地址*/

	uint16_t	rec_per_packet; /*每包的记录数*/
	uint16_t	packet_total;   /*总包数*/
	uint16_t	packet_curr;    /*当前包数*/
}VDR_USERDATA;

#if 0
VDR_USERDATA vdr_09_userdata;


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
	/*遍历一遍 每扇区 32记录*/

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

	pdata_head = pnodedata->tag_data; /*真实发送数据的开始位置->消息头*/
	rt_kprintf( "pdata_head=%p\r\n", pdata_head );

	pdata_head[0]	= pnodedata->head_id >> 8;
	pdata_head[1]	= pnodedata->head_id & 0xFF;

	pnodedata->head_sn++;
	pdata_head[10]	= pnodedata->head_sn >> 8;
	pdata_head[11]	= pnodedata->head_sn & 0xFF;
	memcpy( pdata_head + 4, mobile, 6 );    /*拷贝终端号码,此时可能还没有mobileh号码*/

	read_data_size += 7;                    /*增加VDR的数据头和校验*/
	node_datalen( pnodedata, read_data_size );

/*控制发送状态*/
	pnodedata->max_retry	= 1;
	pnodedata->retry		= 0;
	pnodedata->timeout		= RT_TICK_PER_SECOND * 3;
	pnodedata->state		= IDLE;

#if 1                                                               /*dump 数据*/
	rt_kprintf( "%d>生成 %d bytes\r\n", rt_tick_get( ), pnodedata->msg_len );
//	rt_kprintf( "linkno=%d\r\n", pnodedata->linkno );               /*传输使用的link,包括了协议和远端socket*/
//	rt_kprintf( "multipacket=%d\r\n", pnodedata->multipacket );     /*是不是多包发送*/
//	rt_kprintf( "type=%d\r\n", pnodedata->type );                   /*发送消息的类型*/
//	rt_kprintf( "state=%d\r\n", pnodedata->state );                 /*发送状态*/
//	rt_kprintf( "retry=%d\r\n", pnodedata->retry );                 /*重传次数,递增，递减找不到*/
//	rt_kprintf( "max_retry=%d\r\n", pnodedata->max_retry );         /*最大重传次数*/
//	rt_kprintf( "timeout=%d\r\n", pnodedata->timeout );             /*超时时间*/
//	rt_kprintf( "timeout_tick=%x\r\n", pnodedata->timeout_tick );   /*达到超时的tick值*/
//	rt_kprintf( "head_id=%04x\r\n", pnodedata->head_id );           /*消息ID*/
//	rt_kprintf( "head_sn=%04x\r\n", pnodedata->head_sn );           /*消息流水号*/

	rt_kprintf( "puserdata->addr=%08x\r\n", puserdata->addr );
	rt_kprintf( "puserdata->addr_from=%08x\r\n", puserdata->addr_from );
	rt_kprintf( "puserdata->addr_to=%08x\r\n", puserdata->addr_to );
	rt_kprintf( "puserdata->record_total=%d\r\n", puserdata->record_total );
	rt_kprintf( "puserdata->record_remain=%d\r\n", puserdata->record_remain );

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
	pnodedata->retry++;
	if( pnodedata->retry >= pnodedata->max_retry )
	{
		if( vdr_08_fill_data( pnodedata ) == 0 )
		{
			return WAIT_DELETE;
		}
		return IDLE;
	}
	return IDLE;
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

	if( rec_count == 0 ) /*没有记录，也要上报*/
	{
		return;
	}
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
	pnodedata->packet_num		= ( rec_count + 3 ) / 4; /*每四条记录形成一包*/
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
	uint32_t	addr, addr_max = 0xFFFFFFFF;
	uint8_t		find = 0;
	uint8_t		buf[32];
	uint32_t	mytime_curr		= 0;
	uint32_t	mytime_vdr_09	= 0;

	uint16_t	sector	= 0;
	uint8_t		count	= 0;

	for( vdr_09_sect = 0; vdr_09_sect < VDR_09_SECTORS; vdr_09_sect++ )
	{
		sst25_erase_4k( VDR_09_START + vdr_09_sect * 4096 );
	}
	for( vdr_09_sect = 0; vdr_09_sect < VDR_09_SECTORS; vdr_09_sect++ )
	{
		addr = VDR_09_START + vdr_09_sect * 4096;
		for( count = 0; count < 6; count++ )                                                /*每扇区6条记录*/
		{
			sst25_read( addr, buf, 4 );
			mytime_curr = ( buf[0] << 24 ) | ( buf[1] << 16 ) | ( buf[2] << 8 ) | buf[3];   /*前4个字节代表时间*/
			if( mytime_curr != 0xFFFFFFFF )                                                 /*是有效记录*/
			{
				if( mytime_curr >= mytime_vdr_09 )
				{
					vdr_09_time		= mytime_curr;
					vdr_09_index	= count;
				}
			}
			addr += 680;
		}
	}
	rt_kprintf( "%d>vdr_09 sect=%08x time=%08x index=%d\r\n", rt_tick_get( ), vdr_09_sect, vdr_09_time, vdr_09_index );
}

/*
   每扇区存6个小时的数据
   一个小时666byte 占用680byte
 */
void vdr_09_put( MYTIME datetime )
{
	uint8_t		buf[16];
	uint32_t	i;
	uint32_t	addr;
	uint8_t		minute;

	if( ( vdr_09_time & 0xFFFFFFC0 ) == ( datetime & 0xFFFFFFC0 ) ) /*在同一分钟内*/
	{
		return;
	}

	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );
	if( ( vdr_09_time & 0xFFFFF000 ) != ( datetime & 0xFFFFF000 ) ) /*不是是在当前的小时内,新纪录*/
	{
		vdr_09_index++;                                             /*要写入新的小时记录每680字节*/
		if( vdr_09_index > 5 )                                      /*每个sector存6个小时 0-5,越过一个扇区*/
		{
			vdr_09_index = 0;
			vdr_09_sect++;
			if( vdr_09_sect >= VDR_09_SECTORS )
			{
				vdr_09_sect = 0;
			}
			addr = VDR_09_START + vdr_09_sect * 4096;
			sst25_erase_4k( addr );
		}

		addr = VDR_09_START + vdr_09_sect * 4096 + vdr_09_index * 680;  /*每扇区680字节*/
		PACK_INT( buf, ( datetime & 0xFFFFF000 ) );                     /*写入新的记录头*/
		buf[4]	= HEX2BCD( YEAR( datetime ) );
		buf[5]	= HEX2BCD( MONTH( datetime ) );
		buf[6]	= HEX2BCD( DAY( datetime ) );
		buf[7]	= HEX2BCD( HOUR( datetime ) );
		buf[8]	= 0;
		buf[9]	= 0;
		sst25_write_through( addr, buf, 10 );
	}
	vdr_09_time = datetime;
	PACK_INT( buf, ( gps_longi * 6 ) );
	PACK_INT( buf + 4, ( gps_lati * 6 ) );
	PACK_WORD( buf + 8, ( gps_alti ) );
	buf[10] = gps_speed;

	minute	= MINUTE( datetime );
	addr	= VDR_09_START + vdr_09_sect * 4096 + vdr_09_index * 680 + minute * 11 + 10; /*跳到指定的位置,开始有10字节头*/
	sst25_write_through( addr, buf, 11 );
	rt_sem_release( &sem_dataflash );

	rt_kprintf( "%d>save 09 data addr=%08x\r\n", rt_tick_get( ), addr );
}

/*读取数据*/
uint8_t vdr_09_fill_data( JT808_TX_NODEDATA *pnodedata )
{
	uint32_t		i, count = 0;
	uint8_t			*pdata_body;
	uint8_t			*pdata_head;
	uint8_t			*pdata;
	VDR_USERDATA	* puserdata;
	uint32_t		addr;
	//uint8_t			buf[666+7];
	MYTIME			mytime;
	uint8_t			fcs				= 0;
	uint16_t		read_data_size	= 0;

	puserdata = (VDR_USERDATA* )( pnodedata->user_para );   /*指向用户数据区*/
	if( puserdata->record_remain == 0 )                     /*已经发送完数据*/
	{
		return 0;
	}
	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );  /*准备获取数据*/

	if( pnodedata->multipacket )
	{
		pdata_body = pnodedata->tag_data + 16;
	}else
	{
		pdata_body = pnodedata->tag_data + 12;
	}
	addr	= puserdata->addr;                                                      /*从当前地址开始倒序读,每次4个record*/
	pdata	= pdata_body;
	/*获取数据*/

	addr = VDR_09_START + puserdata->sector * 4096 + puserdata->index * 680 + 4;    /*开始4字节为自己的时间*/
	sst25_read( addr, pdata + 6, 666 );                                             /*开始6个字节的55 7A。。。。*/
	rt_sem_release( &sem_dataflash );

	/*整理一下数据,无效的位置信息填写0x7FFFFFFF*/
	pdata = pdata_body + 6 + 6;                                                     /*跳过开始的6字节55 7A和BCD时间头*/
	for( i = 0; i < 660; i += 11 )
	{
		if( pdata[i] == 0xff )
		{
			pdata[i]		= 0x7F;
			pdata[i + 4]	= 0x7F;
		}
	}

	/*指向下一个要读的sect,index*/
	puserdata->record_remain--;

	if( puserdata->index == 0 )
	{
		if( puserdata->sector == 0 )
		{
			puserdata->sector = VDR_09_SECTORS;
		}
		puserdata->sector--;
		puserdata->index = 6;
	}
	puserdata->index--;

	/*计算VDR的FCS*/
	pdata		= pdata_body;
	pdata[0]	= 0x55;
	pdata[1]	= 0x7A;
	pdata[2]	= 0x08;
	pdata[3]	= ( 666 ) >> 8; /*并不知道实际发送的大小*/
	pdata[4]	= ( 666 ) & 0xFF;
	pdata[5]	= 0;            /*预留*/

	for( i = 0; i < ( 666 + 6 ); i++ )
	{
		fcs ^= *pdata++;
	}
	*pdata = fcs;

	pdata_head = pnodedata->tag_data; /*真实发送数据的开始位置->消息头*/
	rt_kprintf( "pdata_head=%p\r\n", pdata_head );

	pdata_head[0]	= pnodedata->head_id >> 8;
	pdata_head[1]	= pnodedata->head_id & 0xFF;

	pnodedata->head_sn++;
	pdata_head[10]	= pnodedata->head_sn >> 8;
	pdata_head[11]	= pnodedata->head_sn & 0xFF;
	memcpy( pdata_head + 4, mobile, 6 ); /*拷贝终端号码,此时可能还没有mobileh号码*/

	node_datalen( pnodedata, 666 + 7 );

/*控制发送状态*/
	pnodedata->max_retry	= 1;
	pnodedata->retry		= 0;
	pnodedata->timeout		= RT_TICK_PER_SECOND * 3;
	pnodedata->state		= IDLE;

	/*dump 数据*/
	rt_kprintf( "%d>生成 %d bytes\r\n", rt_tick_get( ), pnodedata->msg_len );
	rt_kprintf( "puserdata->addr=%08x\r\n", addr );
	rt_kprintf( "puserdata->record_total=%d\r\n", puserdata->record_total );
	rt_kprintf( "puserdata->record_remain=%d\r\n", puserdata->record_remain );

	return 1;
}

/*收到应答的处理函数*/
static JT808_MSG_STATE vdr_09_tx_response( JT808_TX_NODEDATA * nodedata, uint8_t *pmsg )
{
	vdr_09_fill_data( nodedata );
}

/*超时后的处理函数*/
static JT808_MSG_STATE vdr_09_tx_timeout( JT808_TX_NODEDATA * nodedata )
{
	nodedata->retry++;
	if( nodedata->retry >= nodedata->max_retry )
	{
		if( vdr_09_fill_data( nodedata ) == 0 )
		{
			return WAIT_DELETE;
		}
		return IDLE;
	}
	return IDLE;
}

/*生成09发送的数据包*/
void vdr_09_get_ready( MYTIME start, MYTIME end, uint16_t totalrecord )
{
	uint32_t			addr;
	JT808_TX_NODEDATA	*pnodedata;
	VDR_USERDATA		*puserdata;
	uint32_t			i;
	uint16_t			rec_count = 0; /*找到的记录数*/
	uint8_t				buf[126 * 4];

	uint8_t				sector;
	uint8_t				index;

	uint8_t				sector_from = 0xFF;
	uint8_t				index_from;
	uint32_t			time_from = 0xFFFFFFFF;

	uint8_t				sector_to;
	uint8_t				index_to;
	uint32_t			time_to = 0xFFFFFFFF;

	MYTIME				mytime;

	/*从当前位置开始逆序查找*/
	start	&= 0xFFFFF000;                                  /*忽略分秒*/
	end		&= 0xFFFFF000;

	rt_kprintf( "%d>开始遍历数据\r\n", rt_tick_get( ) );

	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );

	sector	= vdr_09_sect;                                  /*从当前位置开始查找*/
	index	= 5;
	for( i = 0; i < VDR_09_SECTORS; i++ )                   /*就是一个计数*/
	{
		addr = VDR_09_START + sector * 4096 + index * 680;  /*找到每个小时头*/
		sst25_read( addr, buf, 4 );
		mytime = ( buf[0] << 24 ) | ( buf[1] << 16 ) | ( buf[2] << 8 ) | buf[3];
		if( ( mytime >= start ) && ( mytime <= end ) )      /*在有效时间段内*/
		{
			rec_count++;
			if( rec_count >= totalrecord )                  /*收到足够的记录数*/
			{
				break;
			}
			if( time_from == 0xFFFFFFFF )                   /*第一个找到的就是最近的*/
			{
				sector_from = sector;
				index_from	= index;
			}
			if( mytime <= time_to )
			{
				sector_to	= sector;
				index_to	= index;
			}
		}

		if( index == 0 )
		{
			index = 6; /*下面做了递减*/
			if( sector == 0 )
			{
				sector = VDR_09_SECTORS;
			}
			sector--;
		}
		index--;
	}
	rt_sem_release( &sem_dataflash );

	rt_kprintf( "%d>09遍历结束(%d条) FROM %d:%d TO %08x\r\n", rt_tick_get( ), rec_count, sector_from, index_from, sector_to, index_to );

	if( rec_count == 0 ) /*没有记录，也要上报*/
	{
		return;
	}
	puserdata = rt_malloc( sizeof( VDR_USERDATA ) );

	if( puserdata == RT_NULL )
	{
		return;
	}
	puserdata->record_total		= rec_count;
	puserdata->record_remain	= rec_count;

	puserdata->sector_from	= sector_from;
	puserdata->sector_to	= sector_to;

	puserdata->index_from	= index_from;
	puserdata->index_to		= index_to;

	puserdata->sector	= sector_from;
	puserdata->index	= index_from;

	puserdata->packet_total = ( rec_count + 3 ) / 4; /*每四条记录形成一包*/
	puserdata->packet_curr	= 0;

	pnodedata = node_begin( 1, MULTI, 0x0700, 0xF000, 666 + 7 );
	if( pnodedata == RT_NULL )
	{
		rt_free( puserdata );
		puserdata = RT_NULL;
		rt_kprintf( "无法分配\r\n" );
	}
	pnodedata->packet_num		= rec_count;
	pnodedata->packet_no		= 0;
	pnodedata->user_para		= puserdata;
	pnodedata->cb_tx_timeout	= vdr_09_tx_timeout;
	pnodedata->cb_tx_response	= vdr_09_tx_response;
	vdr_09_fill_data( pnodedata );
	node_end( pnodedata );
}

FINSH_FUNCTION_EXPORT_ALIAS( vdr_09_get_ready, get_09, get_09_data );


/*
   事故疑点记录
 */
void vdr_10_init( void )
{
	uint32_t	addr, addr_max = 0xFFFFFFFF;
	uint8_t		find = 0;
	uint8_t		buf[32];
	uint32_t	mytime_curr		= 0;
	uint32_t	mytime_vdr_10	= 0;

	uint16_t	sector	= 0;
	uint8_t		count	= 0;

	for( vdr_10_sect = 0; vdr_10_sect < VDR_10_SECTORS; vdr_10_sect++ )
	{
		sst25_erase_4k( VDR_10_START + vdr_10_sect * 4096 );
	}
	for( vdr_10_sect = 0; vdr_10_sect < VDR_10_SECTORS; vdr_10_sect++ )
	{
		addr = VDR_10_START + vdr_10_sect * 4096;
		for( count = 0; count < 16; count++ )                                               /*每扇区6条记录*/
		{
			sst25_read( addr, buf, 4 );
			mytime_curr = ( buf[0] << 24 ) | ( buf[1] << 16 ) | ( buf[2] << 8 ) | buf[3];   /*前4个字节代表时间*/
			if( mytime_curr != 0xFFFFFFFF )                                                 /*是有效记录*/
			{
				if( mytime_curr >= mytime_vdr_10 )
				{
					vdr_10_time		= mytime_curr;
					vdr_10_index	= count;
				}
			}
			addr += 256;
		}
	}
	rt_kprintf( "%d>vdr_10 sect=%08x time=%08x index=%d\r\n", rt_tick_get( ), vdr_10_sect, vdr_10_time, vdr_10_index );
}

/*
   234 byte
   这个不是秒单次触发的，而是车辆停驶产生的
 */
void vdr_10_put( MYTIME datetime )
{
	uint32_t	i, j;
	uint32_t	addr;
	uint8_t		minute;
	uint8_t		buf[234 + 4]; /*保存要写入的信息*/
	uint8_t		* pdata;

	j		= speed_status_index;
	pdata	= buf + 28;
	for( i = 0; i < 100; i++ )
	{
		*pdata++	= speed_status[j][0];
		*pdata++	= speed_status[j][1];
		if( j == 0 )
		{
			j = 100;
		}
		j--;
	}
	buf[0]	= datetime >> 24;
	buf[1]	= datetime >> 24;
	buf[2]	= datetime >> 24;
	buf[3]	= datetime >> 24;
	buf[4]	= HEX2BCD( YEAR( datetime ) );
	buf[5]	= HEX2BCD( MONTH( datetime ) );
	buf[6]	= HEX2BCD( DAY( datetime ) );
	buf[7]	= HEX2BCD( HOUR( datetime ) );
	buf[8]	= HEX2BCD( MINUTE( datetime ) );
	buf[9]	= HEX2BCD( SEC( datetime ) );
	memcpy( buf + 10, "12010419800101234", 18 );    /*驾驶证号码*/
	PACK_INT( buf + 228, ( gps_longi * 6 ) );
	PACK_INT( buf + 232, ( gps_lati * 6 ) );
	PACK_WORD( buf + 236, ( gps_alti ) );

	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );
	vdr_10_index++;                                 /*要写入新的小时记录每680字节*/
	if( vdr_10_index > 5 )                          /*每个sector存6个小时 0-5,越过一个扇区*/
	{
		vdr_10_index = 0;
		vdr_10_sect++;
		if( vdr_10_sect >= VDR_10_SECTORS )
		{
			vdr_10_sect = 0;
		}
		addr = VDR_10_START + vdr_10_sect * 4096;
		sst25_erase_4k( addr );
	}

	addr = VDR_10_START + vdr_10_sect * 4096 + vdr_10_index * 256; /*每扇区680字节*/
	sst25_write_through( addr, buf, 238 );

	rt_sem_release( &sem_dataflash );
	rt_kprintf( "%d>save 10 data addr=%08x\r\n", rt_tick_get( ), addr );
}

/*读取数据*/
uint8_t vdr_10_fill_data( JT808_TX_NODEDATA *pnodedata )
{
	uint32_t		i, count = 0;
	uint8_t			*pdata_body;
	uint8_t			*pdata_head;
	uint8_t			*pdata;
	VDR_USERDATA	* puserdata;
	uint32_t		addr;
	//uint8_t			buf[666+7];
	MYTIME			mytime;
	uint8_t			fcs				= 0;
	uint16_t		read_data_size	= 0;

	puserdata = (VDR_USERDATA* )( pnodedata->user_para );   /*指向用户数据区*/
	if( puserdata->record_remain == 0 )                     /*已经发送完数据*/
	{
		return 0;
	}
	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );  /*准备获取数据*/

	if( pnodedata->multipacket )
	{
		pdata_body = pnodedata->tag_data + 16;
	}else
	{
		pdata_body = pnodedata->tag_data + 12;
	}
	addr	= puserdata->addr;                                                      /*从当前地址开始倒序读,每次4个record*/
	pdata	= pdata_body;
	/*获取数据*/

	addr = VDR_09_START + puserdata->sector * 4096 + puserdata->index * 680 + 4;    /*开始4字节为自己的时间*/
	sst25_read( addr, pdata + 6, 666 );                                             /*开始6个字节的55 7A。。。。*/
	rt_sem_release( &sem_dataflash );

	/*整理一下数据,无效的位置信息填写0x7FFFFFFF*/
	pdata = pdata_body + 6 + 6;                                                     /*跳过开始的6字节55 7A和BCD时间头*/
	for( i = 0; i < 660; i += 11 )
	{
		if( pdata[i] == 0xff )
		{
			pdata[i]		= 0x7F;
			pdata[i + 4]	= 0x7F;
		}
	}

	/*指向下一个要读的sect,index*/
	puserdata->record_remain--;

	if( puserdata->index == 0 )
	{
		if( puserdata->sector == 0 )
		{
			puserdata->sector = VDR_09_SECTORS;
		}
		puserdata->sector--;
		puserdata->index = 6;
	}
	puserdata->index--;

	/*计算VDR的FCS*/
	pdata		= pdata_body;
	pdata[0]	= 0x55;
	pdata[1]	= 0x7A;
	pdata[2]	= 0x08;
	pdata[3]	= ( 666 ) >> 8; /*并不知道实际发送的大小*/
	pdata[4]	= ( 666 ) & 0xFF;
	pdata[5]	= 0;            /*预留*/

	for( i = 0; i < ( 666 + 6 ); i++ )
	{
		fcs ^= *pdata++;
	}
	*pdata = fcs;

	pdata_head = pnodedata->tag_data; /*真实发送数据的开始位置->消息头*/
	rt_kprintf( "pdata_head=%p\r\n", pdata_head );

	pdata_head[0]	= pnodedata->head_id >> 8;
	pdata_head[1]	= pnodedata->head_id & 0xFF;

	pnodedata->head_sn++;
	pdata_head[10]	= pnodedata->head_sn >> 8;
	pdata_head[11]	= pnodedata->head_sn & 0xFF;
	memcpy( pdata_head + 4, mobile, 6 ); /*拷贝终端号码,此时可能还没有mobileh号码*/

	node_datalen( pnodedata, 666 + 7 );

/*控制发送状态*/
	pnodedata->max_retry	= 1;
	pnodedata->retry		= 0;
	pnodedata->timeout		= RT_TICK_PER_SECOND * 3;
	pnodedata->state		= IDLE;

	/*dump 数据*/
	rt_kprintf( "%d>生成 %d bytes\r\n", rt_tick_get( ), pnodedata->msg_len );
	rt_kprintf( "puserdata->addr=%08x\r\n", addr );
	rt_kprintf( "puserdata->record_total=%d\r\n", puserdata->record_total );
	rt_kprintf( "puserdata->record_remain=%d\r\n", puserdata->record_remain );

	return 1;
}

/*收到应答的处理函数*/
static void vdr_10_tx_response( JT808_TX_NODEDATA * nodedata, uint8_t *pmsg )
{
	vdr_10_fill_data( nodedata );
}

/*超时后的处理函数*/
static JT808_MSG_STATE vdr_10_tx_timeout( JT808_TX_NODEDATA * nodedata )
{
	nodedata->retry++;
	if( nodedata->retry >= nodedata->max_retry )
	{
		if( vdr_10_fill_data( nodedata ) == 0 )
		{
			return WAIT_DELETE;
		}
		return IDLE;
	}
	return IDLE;
}

/*生成10发送的数据包*/
void vdr_10_get_ready( MYTIME start, MYTIME end, uint16_t totalrecord )
{
	uint32_t			addr;
	JT808_TX_NODEDATA	*pnodedata;
	VDR_USERDATA		*puserdata;
	uint32_t			i;
	uint16_t			rec_count = 0; /*找到的记录数*/
	uint8_t				buf[126 * 4];

	uint8_t				sector;
	uint8_t				index;

	uint8_t				sector_from = 0xFF;
	uint8_t				index_from;
	uint32_t			time_from = 0xFFFFFFFF;

	uint8_t				sector_to;
	uint8_t				index_to;
	uint32_t			time_to = 0xFFFFFFFF;

	MYTIME				mytime;

	/*从当前位置开始逆序查找*/
	start	&= 0xFFFFF000;                                  /*忽略分秒*/
	end		&= 0xFFFFF000;

	rt_kprintf( "%d>开始遍历数据\r\n", rt_tick_get( ) );

	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );

	sector	= vdr_09_sect;                                  /*从当前位置开始查找*/
	index	= 5;
	for( i = 0; i < VDR_09_SECTORS; i++ )                   /*就是一个计数*/
	{
		addr = VDR_09_START + sector * 4096 + index * 680;  /*找到每个小时头*/
		sst25_read( addr, buf, 4 );
		mytime = ( buf[0] << 24 ) | ( buf[1] << 16 ) | ( buf[2] << 8 ) | buf[3];
		if( ( mytime >= start ) && ( mytime <= end ) )      /*在有效时间段内*/
		{
			rec_count++;
			if( rec_count >= totalrecord )                  /*收到足够的记录数*/
			{
				break;
			}
			if( time_from == 0xFFFFFFFF )                   /*第一个找到的就是最近的*/
			{
				sector_from = sector;
				index_from	= index;
			}
			if( mytime <= time_to )
			{
				sector_to	= sector;
				index_to	= index;
			}
		}

		if( index == 0 )
		{
			index = 6; /*下面做了递减*/
			if( sector == 0 )
			{
				sector = VDR_09_SECTORS;
			}
			sector--;
		}
		index--;
	}
	rt_sem_release( &sem_dataflash );

	rt_kprintf( "%d>09遍历结束(%d条) FROM %d:%d TO %08x\r\n", rt_tick_get( ), rec_count, sector_from, index_from, sector_to, index_to );

	if( rec_count == 0 )
	{
		return;
	}
	puserdata = rt_malloc( sizeof( VDR_USERDATA ) );
	if( puserdata == RT_NULL )
	{
		return;
	}
	puserdata->record_total		= rec_count;
	puserdata->record_remain	= rec_count;

	puserdata->sector_from	= sector_from;
	puserdata->sector_to	= sector_to;

	puserdata->index_from	= index_from;
	puserdata->index_to		= index_to;

	puserdata->sector	= sector_from;
	puserdata->index	= index_from;

	puserdata->packet_total = ( rec_count + 3 ) / 4; /*每四条记录形成一包*/
	puserdata->packet_curr	= 0;

	pnodedata = node_begin( 1, MULTI, 0x0700, 0xF000, 666 + 7 );
	if( pnodedata == RT_NULL )
	{
		rt_free( puserdata );
		puserdata = RT_NULL;
		rt_kprintf( "无法分配\r\n" );
	}
	pnodedata->packet_num		= rec_count;
	pnodedata->packet_no		= 0;
	pnodedata->user_para		= puserdata;
	pnodedata->cb_tx_timeout	= vdr_09_tx_timeout;
	pnodedata->cb_tx_response	= vdr_09_tx_response;
	vdr_10_fill_data( pnodedata );
	node_end( pnodedata );
}

FINSH_FUNCTION_EXPORT_ALIAS( vdr_10_get_ready, get_10, get_10_data );
#endif


/*
   获取id下一个可用的地址
   并没有做搽除操作，交由写之前处理

 */
static uint32_t vdr_08_12_get_nextaddr( uint8_t vdr_id )
{
	uint8_t id = vdr_id - 8;
	sect_info[id].index++;
	if( sect_info[id].index >= sect_info[id].record_per_sector )
	{
		sect_info[id].index = 0;
		sect_info[id].sector++;
		if( sect_info[id].sector >= sect_info[id].sector_count )
		{
			sect_info[id].sector = 0;
		}
	}
	return sect_info[id].addr_start + sect_info[id].sector * 4096 + sect_info[id].index * sect_info[id].record_size;
}

/*保存数据 以1秒的间隔保存数据能否处理的过来**/
void vdr_08_put( MYTIME datetime, uint8_t speed, uint8_t status )
{
	uint32_t	sec;
	uint32_t	addr;

	if( ( vdr_08_time & 0xFFFFFFC0 ) != ( datetime & 0xFFFFFFC0 ) ) /*不是在当前的一分钟内*/
	{
		if( vdr_08_time != 0 )                                      /*是有效的数据,要保存，注意第一个数据的处理*/
		{
			vdr_08_info[0]	= vdr_08_time >> 24;
			vdr_08_info[1]	= vdr_08_time >> 16;
			vdr_08_info[2]	= vdr_08_time >> 8;
			vdr_08_info[3]	= vdr_08_time & 0xFF;
			vdr_08_info[4]	= HEX2BCD( YEAR( vdr_08_time ) );
			vdr_08_info[5]	= HEX2BCD( MONTH( vdr_08_time ) );
			vdr_08_info[6]	= HEX2BCD( DAY( vdr_08_time ) );
			vdr_08_info[7]	= HEX2BCD( HOUR( vdr_08_time ) );
			vdr_08_info[8]	= HEX2BCD( MINUTE( vdr_08_time ) );
			vdr_08_info[9]	= 0;

//#ifdef TEST_BKPSRAM
#if 0
			for( i = 0; i < sizeof( vdr_08_info ); i++ )
			{
				*(__IO uint8_t*)( BKPSRAM_BASE + i + 1 ) = vdr_08_info[i];
			}
			*(__IO uint8_t*)( BKPSRAM_BASE ) = 1;
#else

			addr = vdr_08_12_get_nextaddr( 8 );
			rt_kprintf( ">08 sect=%d index=%d\r\n", sect_info[0].sector, sect_info[0].index );

			rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );
			if( ( addr & 0xFFF ) == 0 ) /*在4k边界处*/
			{
				sst25_erase_4k( addr );
			}
			sst25_write_through( addr, vdr_08_info, sizeof( vdr_08_info ) );
			rt_sem_release( &sem_dataflash );
			rt_kprintf( "%d>写入08 地址:%08x 值:%08x\r\n", rt_tick_get( ), addr, vdr_08_time );

#endif
			memset( vdr_08_info, 0xFF, sizeof( vdr_08_info ) ); /*新的记录，初始化为0xFF*/
		}
	}
	sec							= SEC( datetime );
	vdr_08_time					= datetime;
	vdr_08_info[sec * 2 + 10]	= gps_speed;
	vdr_08_info[sec * 2 + 11]	= vdr_signal_status;
}

/*
   每扇区存6个小时的数据
   一个小时666byte 占用680byte
 */
void vdr_09_put( MYTIME datetime )
{
	uint8_t		buf[16];
	uint32_t	i;
	uint32_t	addr;
	uint8_t		minute;

	if( ( vdr_09_time & 0xFFFFFFC0 ) == ( datetime & 0xFFFFFFC0 ) ) /*在同一分钟内*/
	{
		return;
	}

	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );
	if( ( vdr_09_time & 0xFFFFF000 ) != ( datetime & 0xFFFFF000 ) ) /*不是是在当前的小时内,新纪录*/
	{
		addr = vdr_08_12_get_nextaddr( 9 );
		if( ( addr & 0xFFF ) == 0 )                                 /*在4k边界处*/
		{
			sst25_erase_4k( addr );
		}

		PACK_INT( buf, ( datetime & 0xFFFFF000 ) );                 /*写入新的记录头*/
		buf[4]	= HEX2BCD( YEAR( datetime ) );
		buf[5]	= HEX2BCD( MONTH( datetime ) );
		buf[6]	= HEX2BCD( DAY( datetime ) );
		buf[7]	= HEX2BCD( HOUR( datetime ) );
		buf[8]	= 0;
		buf[9]	= 0;
		sst25_write_through( addr, buf, 10 );
	}
	vdr_09_time = datetime;
	PACK_INT( buf, ( gps_longi * 6 ) );
	PACK_INT( buf + 4, ( gps_lati * 6 ) );
	PACK_WORD( buf + 8, ( gps_alti ) );
	buf[10] = gps_speed;

	minute	= MINUTE( datetime );
	addr	= VDR_09_START + sect_info[1].sector * 4096 + sect_info[1].index * 680 + minute * 11 + 10; /*跳到指定的位置,开始有10字节头*/
	sst25_write_through( addr, buf, 11 );
	rt_sem_release( &sem_dataflash );

	rt_kprintf( "%d>save 09 data addr=%08x\r\n", rt_tick_get( ), addr );
}

/*
   234 byte
   这个不是秒单次触发的，而是车辆停驶产生的
 */
void vdr_10_put( MYTIME datetime )
{
	uint32_t	i, j;
	uint32_t	addr;
	uint8_t		minute;
	uint8_t		buf[234 + 4]; /*保存要写入的信息*/
	uint8_t		* pdata;

	j		= speed_status_index;
	pdata	= buf + 28;
	for( i = 0; i < 100; i++ )
	{
		*pdata++	= speed_status[j][0];
		*pdata++	= speed_status[j][1];
		if( j == 0 )
		{
			j = 100;
		}
		j--;
	}
	buf[0]	= datetime >> 24;
	buf[1]	= datetime >> 16;
	buf[2]	= datetime >> 8;
	buf[3]	= datetime & 0xFF;
	buf[4]	= HEX2BCD( YEAR( datetime ) );
	buf[5]	= HEX2BCD( MONTH( datetime ) );
	buf[6]	= HEX2BCD( DAY( datetime ) );
	buf[7]	= HEX2BCD( HOUR( datetime ) );
	buf[8]	= HEX2BCD( MINUTE( datetime ) );
	buf[9]	= HEX2BCD( SEC( datetime ) );
	memcpy( buf + 10, "120104198001012345", 18 );   /*驾驶证号码*/
	PACK_INT( buf + 228, ( gps_longi * 6 ) );
	PACK_INT( buf + 232, ( gps_lati * 6 ) );
	PACK_WORD( buf + 236, ( gps_alti ) );

	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );
	addr = vdr_08_12_get_nextaddr( 10 );
	if( ( addr & 0xFFF ) == 0 )                     /*在4k边界处*/
	{
		sst25_erase_4k( addr );
	}
	sst25_write_through( addr, buf, 238 );

	rt_sem_release( &sem_dataflash );
	rt_kprintf( "%d>save 10 data addr=%08x\r\n", rt_tick_get( ), addr );
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
	uint32_t	i, j;
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
	vdr_09_put( datetime );

	/*保存数据,停车前20ms数据*/
	if( utc_now - utc_speed_status > 20 )                               /*超过20秒的没有数据的间隔*/
	{
		speed_status_index = 0;                                         /*重新记录*/
	}
	speed_status[speed_status_index][0] = gps_speed;                    /*当前位置写入速度,200ms任务中操作*/
	utc_speed_status					= utc_now;

/*判断车辆行驶状态*/
	if( car_status.stop_run == STOP )                                   /*认为车辆停止,判断启动*/
	{
		if( utc_car_stop == 0 )                                         /*停车时刻尚未初始化，默认停驶*/
		{
			utc_car_stop = utc_now;
		}
		if( gps_speed >= SPEED_LIMIT )                                  /*速度大于门限值*/
		{
			if( utc_car_run == 0 )
			{
				utc_car_run = utc_now;                                  /*记录开始时刻*/
			}

			if( ( utc_now - utc_car_run ) >= SPEED_LIMIT_DURATION )     /*超过了持续时间*/
			{
				car_status.stop_run = RUN;                              /*认为车辆行驶*/
				rt_kprintf( "%d>车辆行驶\r\n", rt_tick_get( ) );
				utc_car_stop = 0;                                       /*等待判断停驶*/
			}
		}else
		{
			if( utc_now - utc_car_stop > jt808_param.id_0x005A )        /*判断停车最长时间*/
			{
				//rt_kprintf( "达到停车最长时间\r\n" );
			}
			utc_car_run = 0;
		}
	}else                                                               /*车辆已启动*/
	{
		if( gps_speed <= SPEED_LIMIT )                                  /*速度小于门限值*/
		{
			if( utc_car_stop == 0 )
			{
				utc_car_stop = utc_now;
			}
			if( ( utc_now - utc_car_stop ) >= SPEED_LIMIT_DURATION )    /*超过了持续时间*/
			{
				car_status.stop_run = STOP;                             /*认为车辆停驶*/
				rt_kprintf( "%d>车辆停驶\r\n", rt_tick_get( ) );
				utc_car_run = 0;
				vdr_10_put( datetime );                                 /*生成VDR_10数据,事故疑点*/
			}
		}else
		{
			if( utc_now - utc_car_run > jt808_param.id_0x0057 )         /*判断连续驾驶最长时间*/
			{
				//rt_kprintf( "达到连续驾驶最长时间\r\n" );
			}
			utc_car_stop = 0;
		}
	}

/*11数据超时驾驶记录*/
}

/*
   初始化08 09 10 11 12的记录
   返回 记录的时刻
     0:没有记录
 */
MYTIME vdr_08_12_init( uint8_t vdr_id, uint8_t format )
{
	uint32_t	addr, addr_max = 0xFFFFFFFF;
	uint8_t		find = 0;
	uint8_t		buf[32];
	MYTIME		mytime_curr = 0;
	MYTIME		mytime_ret	= 0x0;

	uint8_t		sector	= 0;
	uint8_t		count	= 0;
	uint8_t		id;

	id = vdr_id - 8;

	if( format )
	{
		for( sector = 0; sector < sect_info[id].sector_count; sector++ )
		{
			sst25_erase_4k( sect_info[id].addr_start + sector * 4096 );
		}
	}
	for( sector = 0; sector < sect_info[id].sector_count; sector++ )
	{
		addr = sect_info[id].addr_start + sector * 4096;

		for( count = 0; count < sect_info[id].record_per_sector; count++ )                  /*每扇区6条记录*/
		{
			sst25_read( addr, buf, 4 );
			mytime_curr = ( buf[0] << 24 ) | ( buf[1] << 16 ) | ( buf[2] << 8 ) | buf[3];   /*前4个字节代表时间*/
			if( mytime_curr != 0xFFFFFFFF )                                                 /*是有效记录*/
			{
				if( mytime_curr >= mytime_ret )
				{
					mytime_ret				= mytime_curr;
					sect_info[id].sector	= sector;
					sect_info[id].index		= count;
				}
			}
			addr += sect_info[id].record_size;
		}
	}
	rt_kprintf( "%d>vdr%02d time=%08x sect=%d index=%d\r\n", rt_tick_get( ), vdr_id, mytime_ret, sect_info[id].sector, sect_info[id].index );
	return mytime_ret;
}

FINSH_FUNCTION_EXPORT_ALIAS( vdr_08_12_init, vdr_format, format vdr record );

/*读取数据*/
uint8_t vdr_08_12_fill_data( JT808_TX_NODEDATA *pnodedata )
{
	uint32_t		i, count = 0;
	uint8_t			*pdata_body;
	uint8_t			*pdata_head;
	uint8_t			*pdata;
	VDR_USERDATA	* puserdata;
	uint32_t		addr;
	MYTIME			mytime;
	uint8_t			fcs			= 0;
	uint16_t		rec_count	= 0;
	uint8_t			id;

	puserdata = (VDR_USERDATA* )( pnodedata->user_para );   /*指向用户数据区*/

	id = puserdata->id - 8;

	if( puserdata->record_remain == 0 )                     /*已经发送完数据*/
	{
		return 0;
	}

	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );  /*准备获取数据*/

	if( pnodedata->multipacket )
	{
		pdata_body = pnodedata->tag_data + 16;
	}else
	{
		pdata_body = pnodedata->tag_data + 12;
	}

	/*获取要读取的记录数,有可能是多包，或不足一包*/
	rec_count = sect_info[id].record_per_packet;
	if( puserdata->record_remain < sect_info[id].record_per_packet )    /*不足构成1个发送包*/
	{
		rec_count = puserdata->record_remain;
	}

	pdata = pdata_body + 6;                                             /*跳过开始的6个字节的vdr头 55 7A*/

	for( i = 0; i < rec_count; i++ )
	{
		addr = sect_info[id].addr_start +
		       puserdata->sector * 4096 +
		       puserdata->index * sect_info[id].record_size + 4;        /*开始4字节为自己的时间*/
		sst25_read( addr, pdata, sect_info[id].data_size );

		if( puserdata->index == 0 )                                     /*调整下一个要读的位置*/
		{
			puserdata->index = sect_info[id].record_per_sector;
			if( puserdata->sector == 0 )
			{
				puserdata->sector = sect_info[id].sector_count;
			}
			puserdata->sector--;
		}
		puserdata->index--;

		pdata += sect_info[id].data_size;
		puserdata->record_remain--; /*剩余记录数-1*/
	}
	rt_sem_release( &sem_dataflash );

	if( id == 1 )                   /*整理一下09数据,无效的位置信息填写0x7FFFFFFF*/
	{
		pdata = pdata_body + 6 + 6; /*跳过开始的6字节55 7A和BCD时间头*/
		for( i = 0; i < 660; i += 11 )
		{
			if( pdata[i] == 0xff )
			{
				pdata[i]		= 0x7F;
				pdata[i + 4]	= 0x7F;
			}
		}
	}
	/*计算VDR的FCS*/
	count		= rec_count * sect_info[id].data_size;  /*读取的记录数*/
	pdata		= pdata_body;
	pdata[0]	= 0x55;
	pdata[1]	= 0x7A;
	pdata[2]	= puserdata->id;
	pdata[3]	= count >> 8;                           /*并不知道实际发送的大小*/
	pdata[4]	= count & 0xFF;
	pdata[5]	= 0;                                    /*预留*/

	for( i = 0; i < ( count + 6 ); i++ )
	{
		fcs ^= *pdata++;
	}
	*pdata = fcs;

	pdata_head = pnodedata->tag_data; /*真实发送数据的开始位置->消息头*/
	rt_kprintf( "pdata_head=%p\r\n", pdata_head );

	pdata_head[0]	= pnodedata->head_id >> 8;
	pdata_head[1]	= pnodedata->head_id & 0xFF;

	pnodedata->head_sn++;
	pdata_head[10]	= pnodedata->head_sn >> 8;
	pdata_head[11]	= pnodedata->head_sn & 0xFF;
	memcpy( pdata_head + 4, mobile, 6 ); /*拷贝终端号码,此时可能还没有mobileh号码*/

	node_datalen( pnodedata, count + 7 );

/*控制发送状态*/
	pnodedata->max_retry	= 1;
	pnodedata->retry		= 0;
	pnodedata->timeout		= RT_TICK_PER_SECOND * 3;
	pnodedata->state		= IDLE;

	/*dump 数据*/
	rt_kprintf( "%d>生成 %d bytes\r\n", rt_tick_get( ), pnodedata->msg_len );
	rt_kprintf( "puserdata->addr=%08x\r\n", addr );
	rt_kprintf( "puserdata->record_total=%d\r\n", puserdata->record_total );
	rt_kprintf( "puserdata->record_remain=%d\r\n", puserdata->record_remain );

	return 1;
}

/*收到应答的处理函数*/
static JT808_MSG_STATE vdr_08_12_tx_response( JT808_TX_NODEDATA * pnodedata, uint8_t *pmsg )
{
	pnodedata->retry++;
	if( pnodedata->retry >= pnodedata->max_retry )
	{
		return WAIT_DELETE;
	}
	return vdr_08_12_fill_data( pnodedata );
}

/*超时后的处理函数*/
static JT808_MSG_STATE vdr_08_12_tx_timeout( JT808_TX_NODEDATA * pnodedata )
{
	pnodedata->retry++;
	if( pnodedata->retry >= pnodedata->max_retry )
	{
		if( vdr_08_12_fill_data( pnodedata ) == 0 )
		{
			return WAIT_DELETE;
		}
		return IDLE;
	}
	return IDLE;
}

/*
   生成08-12发送的数据包
   不同的数据组包数是不同的。

 */
void vdr_08_12_get_ready( uint8_t vdr_id, MYTIME start, MYTIME end, uint16_t totalrecord )
{
	uint32_t			addr;
	uint8_t				buf[16];
	JT808_TX_NODEDATA	*pnodedata;
	VDR_USERDATA		*puserdata;
	uint32_t			i;
	uint16_t			rec_count = 0;      /*找到的记录数*/

	uint8_t				sector;
	uint8_t				index;

	uint8_t				sector_from = 0xFF; /*置位没有找到*/
	uint8_t				index_from;

	uint8_t				sector_to;
	uint8_t				index_to;
	uint32_t			time_to = 0xFFFFFFFF;
	MYTIME				mytime;
	uint8_t				id;

	id = vdr_id - 8;

	/*从当前位置开始逆序查找*/
	rt_kprintf( "%d>开始遍历数据\r\n", rt_tick_get( ) );

	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );

	sector	= sect_info[id].sector;                     /*从当前位置开始查找,倒序*/
	index	= sect_info[id].index;

	for( i = 0; i < sect_info[id].sector_count; i++ )   /*就是一个计数*/
	{
		addr = sect_info[id].addr_start + sector * 4096 + index * sect_info[id].record_size;
		sst25_read( addr, buf, 4 );
		mytime = ( buf[0] << 24 ) | ( buf[1] << 16 ) | ( buf[2] << 8 ) | buf[3];
		if( ( mytime >= start ) && ( mytime <= end ) )  /*在有效时间段内*/
		{
			rec_count++;
			if( rec_count >= totalrecord )              /*收到足够的记录数*/
			{
				break;
			}
			if( sector_from == 0xFF )                   /*第一个找到的就是最近的*/
			{
				sector_from = sector;
				index_from	= index;
			}
			if( mytime <= time_to )                     /*最小的才是最符合的*/
			{
				sector_to	= sector;
				index_to	= index;
				time_to		= mytime;
			}
		}

		if( index == 0 )
		{
			index = sect_info[id].record_per_sector; /*下面做了递减*/
			if( sector == 0 )
			{
				sector = sect_info[id].sector_count;
			}
			sector--;
		}
		index--;
	}
	rt_sem_release( &sem_dataflash );

	rt_kprintf( "%d>vdr%02d遍历结束(%d条) FROM %d:%d TO %d:%d\r\n",
	            rt_tick_get( ),
	            vdr_id,
	            rec_count,
	            sector_from, index_from,
	            sector_to, index_to );

	if( rec_count == 0 ) /*没有找到记录，也要上报*/
	{
		return;
	}

	puserdata = rt_malloc( sizeof( VDR_USERDATA ) );

	if( puserdata == RT_NULL )
	{
		return;
	}
	puserdata->id				= vdr_id;
	puserdata->record_total		= rec_count;
	puserdata->record_remain	= rec_count;

	puserdata->sector_from	= sector_from;
	puserdata->index_from	= index_from;

	puserdata->sector_to	= sector_to;
	puserdata->index_to		= index_to;

	puserdata->sector	= sector_from;
	puserdata->index	= index_from;

	puserdata->packet_total = ( rec_count + sect_info[id].record_per_packet - 1 ) / sect_info[id].record_per_packet;
	puserdata->packet_curr	= 0;

	/*计算要发送的数据总数，和单包最大字节数*/
	i = sect_info[id].record_per_packet * sect_info[id].data_size + 7;  /*加7 是因为有vdr的头*/

	if( rec_count * sect_info[id].data_size > 700 )                     /*多包发送,*/
	{
		pnodedata = node_begin( 1, MULTI_CMD, 0x0700, 0xF000, i );
	} else
	{
		pnodedata = node_begin( 1, SINGLE_CMD, 0x0700, 0xF000, i );
	}
	if( pnodedata == RT_NULL )
	{
		rt_free( puserdata );
		puserdata = RT_NULL;
		rt_kprintf( "无法分配\r\n" );
	}
	pnodedata->packet_num		= ( rec_count + sect_info[id].record_per_packet - 1 ) / sect_info[id].record_per_packet;
	pnodedata->packet_no		= 0;
	pnodedata->user_para		= puserdata;
	pnodedata->cb_tx_timeout	= vdr_08_12_tx_timeout;
	pnodedata->cb_tx_response	= vdr_08_12_tx_response;
	rt_kprintf( "填充数据\r\n" );
	vdr_08_12_fill_data( pnodedata );
	node_end( pnodedata );
}

FINSH_FUNCTION_EXPORT_ALIAS( vdr_08_12_get_ready, vdr_get, get_vdr_data );


/*
   初始化记录区数据
   因为是属于固定时间段存储的
   需要记录开始时刻的sector位置(相对的sector偏移)
 */
rt_err_t vdr_init( void )
{
	uint8_t* pbuf;

	pulse_init( ); /*接脉冲计数*/

	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 5 );

	vdr_08_time = vdr_08_12_init( 8, 0);
	memset( vdr_08_info, 0xFF, sizeof( vdr_08_info ) );
	vdr_09_time = vdr_08_12_init( 9, 0 );
	vdr_10_time = vdr_08_12_init( 10, 0 );

	rt_sem_release( &sem_dataflash );

/*初始化一个50ms的定时器，用作事故疑点判断*/
	rt_timer_init( &tmr_200ms, "tmr_200ms",     /* 定时器名字是 tmr_50ms */
	               cb_tmr_200ms,                /* 超时时回调的处理函数 */
	               RT_NULL,                     /* 超时函数的入口参数 */
	               RT_TICK_PER_SECOND / 5,      /* 定时长度，以OS Tick为单位 */
	               RT_TIMER_FLAG_PERIODIC );    /* 周期性定时器 */
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
		case 9:
		case 0x10:
		case 0x11:
		case 0x12:
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
			vdr_08_12_get_ready( cmd, start, end, blocks );
			break;
		case 0x13:
			break;
		case 0x14:
			break;
		case 0x15:
			break;
		case 0x82:
			break;
		case 0x83:
			break;
		case 0x84:
			break;
		case 0xC2:
			break;
		case 0xC3:
			break;
		case 0xC4:
			break;
	}
}

/************************************** The End Of File **************************************/
