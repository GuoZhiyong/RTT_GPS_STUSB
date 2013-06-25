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


/*
   4MB serial flash 0x400000
 */

/*转换hex到bcd的编码*/
#define HEX_TO_BCD( A ) ( ( ( ( A ) / 10 ) << 4 ) | ( ( A ) % 10 ) )

static rt_thread_t tid_usb_vdr = RT_NULL;

#define VDR_08H_09H_START	0x300000
#define VDR_08H_09H_END		0x32FFFF

#define VDR_BASE 0x300000

#define VDR_08_START	VDR_BASE
#define VDR_08_SECTORS	92  /*48小时 单位分钟内的速度 48*60*128/4096=90 ，防止删除整个4K时数据不足，要多预留一些*/

#define VDR_09_START	( VDR_08_START + VDR_08_SECTORS * 4096 )
#define VDR_09_SECTORS	86  /*360小时 每分钟位置速度 360*60*16*/

#define VDR_10_START	( VDR_09_START + VDR_09_SECTORS * 4096 )
#define VDR_10_SECTORS	8   /*100条事故疑点 100*234  实际 128*256 */

#define VDR_11_START	( VDR_10_START + VDR_10_SECTORS * 4096 )
#define VDR_11_SECTORS	3   /*100条超时驾驶记录 100*50 实际 128*64,保留一个扇区，删除时仍有数据*/

#define VDR_12_START	( VDR_11_START + VDR_11_SECTORS * 4096 )
#define VDR_12_SECTORS	3   /*200条驾驶人身份记录 200*25 实际200*32 */

#define VDR_13_START	( VDR_12_START + VDR_12_SECTORS * 4096 )
#define VDR_13_SECTORS	2   /*100条 外部供电记录100*7 实际 100*8*/

#define VDR_14_START	( VDR_13_START + VDR_13_SECTORS * 4096 )
#define VDR_14_SECTORS	2   /*100条 参数修改记录 100*7 实际100*8*/

#define VDR_15_START	( VDR_14_START + VDR_14_SECTORS * 4096 )
#define VDR_15_SECTORS	2   /*10条速度状态日志 10*133 实际 10*256*/

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

#define MYDATETIME( year, month, day, hour, minute, sec )	( (uint32_t)( year << 26 ) | (uint32_t)( month << 22 ) | (uint32_t)( day << 17 ) | (uint32_t)( hour << 12 ) | (uint32_t)( minute << 6 ) | sec )
#define YEAR( datetime )									( ( datetime >> 26 ) & 0x3F )
#define MONTH( datetime )									( ( datetime >> 22 ) & 0xF )
#define DAY( datetime )										( ( datetime >> 17 ) & 0x1F )
#define HOUR( datetime )									( ( datetime >> 12 ) & 0x1F )
#define MINUTE( datetime )									( ( datetime >> 6 ) & 0x3F )
#define SEC( datetime )										( datetime & 0x3F )

uint8_t vdr_signal_status = 0x01; /*行车记录仪的状态信号*/

typedef __packed struct _rec_08
{
	uint8_t		flag;
	uint32_t	datetime;
	uint8_t		speed_status[60][2];
} STU_REC_08;
STU_REC_08 stu_rec_08;


/*
   经度、纬度分别为4个字节组成一
   个32位的有符号数，表示经度或纬
   度，单位为0.0001分每比特
 */
typedef __packed struct _rec_09
{
	uint8_t		flag;
	uint32_t	datetime;
	uint32_t	lati;
	uint32_t	longi;
	uint16_t	speed;
} STU_REC_09;

STU_REC_09 stu_rec_09;

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
/*08数据,没有判断59秒时的情况，只要ymdhm不同，就保存*/

	if( ( stu_rec_08.datetime & 0xFFFFFFC0 ) != ( datetime & 0xFFFFFFC0 ) ) /*不是在当前的一分钟内*/
	{
		if( stu_rec_08.datetime != 0xFFFFFFFF )                             /*是有效的数据,要保存*/
		{
			stu_rec_08.flag = '8';
			vdr_save_rec( 8, (uint8_t*)&stu_rec_08, sizeof( STU_REC_08 ) );
			memset( (uint8_t*)&stu_rec_08, 0xFF, sizeof( STU_REC_08 ) );    /*新的记录，初始化为0xFF*/
		}
	}
	stu_rec_08.datetime				= datetime;
	stu_rec_08.speed_status[sec][0] = gps_speed;
	stu_rec_08.speed_status[sec][1] = vdr_signal_status;

/*09数据 每分钟的位置速度，只保存第一次有效的*/
	if( ( stu_rec_09.datetime & 0xFFFFFFC0 ) != ( datetime & 0xFFFFFFC0 ) ) /*不是是在当前的一分钟内*/
	{
		stu_rec_09.datetime = datetime;                                     /*置位相等*/
		stu_rec_09.flag		= '9';
		stu_rec_09.longi	= BYTESWAP4( gps_longi * 6 );                   /* 看含义*/
		stu_rec_09.lati		= BYTESWAP4( gps_lati * 6 );
		stu_rec_09.speed	= BYTESWAP2( gps_speed );
		vdr_save_rec( 9, (uint8_t*)&stu_rec_09, sizeof( STU_REC_09 ) );
	}
/*10数据 事故疑点*/
	if( car_status.status == 0 )                                            /*认为车辆停止,判断启动*/
	{
		if( gps_speed >= SPEED_LIMIT )                                      /*速度大于门限值*/
		{
			car_status.gps_judge_duration++;
			if( car_status.gps_judge_duration >= SPEED_LIMIT_DURATION )     /*超过了持续时间*/
			{
				car_status.gps_duration			= SPEED_LIMIT_DURATION;
				car_status.status				= 1;                        /*认为车辆行驶*/
				car_status.gps_judge_duration	= 0;
				rt_kprintf( "%d>车辆行驶\r\n", rt_tick_get( ) );
			}else
			{
				car_status.gps_duration++;                                  /*停车累计时间*/
			}
		}else
		{
			car_status.gps_duration++;                                      /*停车累计时间*/
			/*在此判断停车超时*/
			car_status.gps_judge_duration = 0;
		}
	}else /*车辆已启动*/
	{
		if( gps_speed <= SPEED_LIMIT )                                      /*速度小于门限值*/
		{
			car_status.gps_judge_duration++;
			if( car_status.gps_judge_duration >= SPEED_LIMIT_DURATION )     /*超过了持续时间*/
			{
				car_status.gps_duration			= SPEED_LIMIT_DURATION;
				car_status.status				= 0;                        /*认为车辆停驶*/
				car_status.gps_judge_duration	= 0;
				rt_kprintf( "%d>车辆停驶\r\n", rt_tick_get( ) );
			}else
			{
				car_status.gps_duration++;                                  /*行驶累计时间*/
			}
		}else
		{
			car_status.gps_duration++;                                      /*行驶累计时间*/
			/*判断疲劳驾驶*/
			car_status.gps_judge_duration = 0;
		}
	}

/*11数据超时驾驶记录*/
}

/*获取08数据*/
void vdr_get_08( )
{
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

	rt_free( pbuf );
	pbuf = RT_NULL;
/*初始化一个50ms的定时器，用作事故疑点判断*/
	rt_timer_init( &tmr_200ms, "tmr_200ms",     /* 定时器名字是 tmr_50ms */
	               cb_tmr_200ms,                /* 超时时回调的处理函数 */
	               RT_NULL,                     /* 超时函数的入口参数 */
	               RT_TICK_PER_SECOND / 5,      /* 定时长度，以OS Tick为单位 */
	               RT_TIMER_FLAG_PERIODIC );    /* 周期性定时器 */
	rt_timer_start( &tmr_200ms );
}

/*行车记录仪数据采集命令*/
void vdr_rx_8700( uint8_t *pmsg )
{
	uint8_t		* psrc;
	uint8_t		buf[500];

	uint16_t	seq = JT808HEAD_SEQ( pmsg );
	uint16_t	len = JT808HEAD_LEN( pmsg );
	uint8_t		cmd = *( pmsg + 13 );   /*跳过前面12字节的头*/

	switch( cmd )
	{
		case 0:                         /*采集记录仪执行标准版本*/
			buf[0]	= seq >> 8;
			buf[1]	= seq & 0xff;
			buf[2]	= cmd;
			memcpy(buf+3,"\x55\x7A\x00\x00\x02\x00\x12\x00",8);
			jt808_add_tx_data_single( 1, TERMINAL_ACK, 0x0700, 11, buf, RT_NULL, RT_NULL );
			break;
		case 1:
			buf[0]	= seq >> 8;
			buf[1]	= seq & 0xff;
			buf[2]	= cmd;
			memcpy(buf+3,"\x55\x7A\x01\x00\x12\x00120221123456789\x00\x00\x00\x00",25);
			jt808_add_tx_data_single( 1, TERMINAL_ACK, 0x0700, 28, buf, RT_NULL, RT_NULL );
		case 2:  /*行车记录仪时间*/
			buf[0]	= seq >> 8;
			buf[1]	= seq & 0xff;
			buf[2]	= cmd;
			sprintf(buf+3,"\x55\x7A\x02\x00\x06\x00%6s",gps_baseinfo.datetime);
			jt808_add_tx_data_single( 1, TERMINAL_ACK, 0x0700, 15, buf, RT_NULL, RT_NULL );
		case 3:
			
	}
}

/************************************** The End Of File **************************************/
