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
#include "jt808_util.h"

#define VDR_DEBUG

#define SPEED_LIMIT				5   /*速度门限 大于此值认为启动，小于此值认为停止*/
#define SPEED_LIMIT_DURATION	10  /*速度门限持续时间*/

#define VEHICLE_FLAG_NORMAL		0   /*正常状态，停止或行驶*/
#define VEHICLE_FLAG_OVERSPEED	1   /*超速行驶*/
#define VEHICLE_FLAG_OVERTIME	2   /*超时行驶*/
#define VEHICLE_FlAG_MAXSTOP	8   /*最长停止时间，超时驾驶判断用*/

//#define TEST_BKPSRAM

#define VDR_BASE 0x03B000

#define VDR_16_START	VDR_BASE
#define VDR_16_SECTORS	3           /*每小时2个sector,保留50小时*/
#define VDR_16_END		( VDR_16_START + VDR_16_SECTORS * 4096 )

#define VDR_08_START	( VDR_16_END )
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


/*
   4MB serial flash 0x400000
 */

static rt_thread_t		tid_usb_vdr = RT_NULL;

static struct rt_timer	tmr_200ms;

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
#if 0
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

#endif

/**/
static void vdr_pack_byte( uint8_t* buf, uint8_t byte, uint8_t* fcs )
{
	*buf	= byte;
	*fcs	^= byte;
}

/**/
static void vdr_pack_word( uint8_t* buf, uint16_t word, uint8_t* fcs )
{
	buf[0]	= ( word >> 8 );
	*fcs	^= ( word >> 8 );
	buf[1]	= ( word & 0xFF );
	*fcs	^= ( word & 0xFF );
}

/**/
static void vdr_pack_buf( uint8_t* dst, uint8_t* src, uint16_t len, uint8_t* fcs )
{
	uint16_t count = len;
	while( count-- )
	{
		*dst++	= *src;
		*fcs	^= *src;
		src++;
	}
}

/*超时、疲劳驾驶参数*/
static MYTIME	vdr11_mytime_start	= 0;
static uint32_t vdr11_lati_start	= 0;
static uint32_t vdr11_longi_start	= 0;
static uint16_t vdr11_alti_start	= 0;


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
uint8_t vdr_signal_status = 0x01;           /*行车记录仪的状态信号*/

/*200ms间隔的速度状态信息*/
static uint8_t	speed_status[100][2];
static uint8_t	speed_status_index	= 0;
static uint32_t utc_speed_status	= 0;    /*最近保存保存记录的时刻*/


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

enum _stop_run
{
	STOP=0,
	RUN =1,
};

static enum _stop_run	car_stop_run	= STOP;
static uint32_t			utc_car_stop	= 0;    /*车辆开始停驶时刻*/
static uint32_t			utc_car_run		= 0;    /*车辆开始行驶时刻*/


/*
   获取id下一个可用的地址
   并没有做搽除操作，交由写之前处理
   20130801:数据要位于4k边界处，不能跨扇区，否则环回写入时会出现问题

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
			rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );
			if( ( addr & 0xFFF ) == 0 ) /*在4k边界处*/
			{
				sst25_erase_4k( addr );
			}
			sst25_write_through( addr, vdr_08_info, sizeof( vdr_08_info ) );
			rt_sem_release( &sem_dataflash );
			rt_kprintf( "\n%d>写入08 地址:%08x(sect:count=%d:%d) 值:%08x", rt_tick_get( ), addr, sect_info[0].sector, sect_info[0].index, vdr_08_time );

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

	rt_kprintf( "\n%d>save 09 data addr=%08x", rt_tick_get( ), addr );
}

/*
   234 byte
   这个不是秒单次触发的，而是车辆停驶产生的
 */
void vdr_10_put( MYTIME datetime )
{
	uint32_t	i, j;
	uint32_t	addr;
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
	memcpy( buf + 10, jt808_param.id_0xF009, 18 );  /*驾驶证号码*/
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
	rt_kprintf( "\n%d>save 10 data addr=%08x", rt_tick_get( ), addr );
}

/*
   超时,疲劳驾驶记录
   有可能超时了，还在驾驶，这时要动态生成
   等到车停以后，休息了足够的时间，才是一次完整的超时驾驶记录
   如何保存?
 */
void vdr_11_put( MYTIME datetime )
{
	uint32_t	addr;
	uint8_t		buf[64];                            /*保存要写入的信息*/

	buf[0]	= datetime >> 24;
	buf[1]	= datetime >> 16;
	buf[2]	= datetime >> 8;
	buf[3]	= datetime & 0xFF;
	memcpy( buf + 4, jt808_param.id_0xF009, 18 );   /*驾驶证号码*/

	mytime_to_hex( buf + 22, vdr11_mytime_start );
	mytime_to_hex( buf + 28, datetime );

	PACK_INT( buf + 34, ( vdr11_longi_start * 6 ) );
	PACK_INT( buf + 38, ( vdr11_lati_start * 6 ) );
	PACK_WORD( buf + 42, vdr11_alti_start );

	PACK_INT( buf + 44, ( gps_longi * 6 ) );
	PACK_INT( buf + 48, ( gps_lati * 6 ) );
	PACK_WORD( buf + 52, gps_alti );

	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 5 );
	addr = vdr_08_12_get_nextaddr( 11 );
	if( ( addr & 0xFFF ) == 0 ) /*在4k边界处*/
	{
		sst25_erase_4k( addr );
	}
	sst25_write_through( addr, buf, 54 );

	rt_sem_release( &sem_dataflash );
	rt_kprintf( "\n%d>save 11 data addr=%08x", rt_tick_get( ), addr );
}

/*超速驾驶记录*/
void vdr_16_put( MYTIME start, MYTIME end, uint16_t speed_min, uint16_t speed_max, uint16_t speed_avg )
{
	uint32_t	addr;
	uint8_t		buf[64]; /*保存要写入的信息*/

	buf[0]	= end >> 24;
	buf[1]	= end >> 16;
	buf[2]	= end >> 8;
	buf[3]	= end & 0xFF;
	mytime_to_hex( buf + 4, start );
	mytime_to_hex( buf + 10, end );
	PACK_WORD( buf + 16, speed_min );
	PACK_WORD( buf + 18, speed_max );
	PACK_WORD( buf + 20, speed_avg );

	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 5 );
	//addr = vdr_08_12_get_nextaddr( 11 );
	if( ( addr & 0xFFF ) == 0 ) /*在4k边界处*/
	{
		sst25_erase_4k( addr );
	}
	sst25_write_through( addr, buf, 22 );

	rt_sem_release( &sem_dataflash );
	rt_kprintf( "\n%d>save 16 data addr=%08x", rt_tick_get( ), addr );
}

/*
   每秒判断车辆状态,gps中的RMC语句触发
   行驶里程
   默认都是使用GPS速度，如果gps未定位，也有秒语句输出
 */

/*gps有效情况下检查车辆行驶状态*/
void process_overtime( void )
{
/*判断车辆行驶状态*/
	if( car_stop_run == STOP )                                          /*认为车辆停止,判断启动*/
	{
		if( utc_car_stop == 0 )                                         /*停车时刻尚未初始化，默认停驶*/
		{
			utc_car_stop = utc_now;
		}
		if( gps_speed >= SPEED_LIMIT )                                  /*速度大于门限值*/
		{
			if( utc_car_run == 0 )                                      /*还没有记录行驶的时刻*/
			{
				utc_car_run = utc_now;                                  /*记录开始时刻*/
			}

			if( ( utc_now - utc_car_run ) >= SPEED_LIMIT_DURATION )     /*超过了持续时间*/
			{
				car_stop_run = RUN;                                     /*认为车辆行驶*/
				rt_kprintf( "\n%d>车辆行驶", rt_tick_get( ) );
				utc_car_stop = 0;                                       /*等待判断停驶*/
			}
		}else
		{
			if( utc_now - utc_car_stop > jt808_param.id_0x005A )        /*判断停车最长时间*/
			{
				//rt_kprintf( "达到停车最长时间\r\n" );
				jt808_alarm|=BIT_ALARM_STOP_OVERTIME;
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
				car_stop_run = STOP;                                    /*认为车辆停驶*/
				rt_kprintf( "\n%d>车辆停驶", rt_tick_get( ) );
				utc_car_run = 0;
				vdr_10_put( mytime_now );                               /*生成VDR_10数据,事故疑点*/
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
}

/*超速、超速预警判断*/
void process_overspeed( void )
{
	static uint8_t	overspeed_flag = 0;
	static uint32_t utc_overspeed_start;
	static MYTIME	mytime_overspeed_start;
	static uint32_t overspeed_sum	= 0;
	static uint32_t overspeed_count = 0;
	static uint16_t overspeed_min	= 0xFF;
	static uint16_t overspeed_max	= 0;

	uint16_t		gps_speed_10x	= gps_speed * 10;
	uint16_t		limit_speed_10x = jt808_param.id_0x0055 * 10;

	if( gps_speed >= jt808_param.id_0x0055 )                                /*超过最高速度*/
	{
		if( utc_overspeed_start )                                           /*判断超速持续事件*/
		{
			if( utc_now - utc_overspeed_start >= jt808_param.id_0x0056 )    /*已经持续超速*/
			{
				if( ( jt808_param.id_0x0050 & BIT_ALARM_OVERSPEED ) == 0 )  /*报警屏蔽字*/
				{
					jt808_alarm |= BIT_ALARM_OVERSPEED;
					beep( 5, 1, 1 );
				}
				overspeed_flag	= 2;                                        /*已经超速了*/
				overspeed_sum	+= gps_speed;
				overspeed_count++;
				if( gps_speed > overspeed_max )
				{
					overspeed_max = gps_speed;
				}
				if( gps_speed < overspeed_min )
				{
					overspeed_min = gps_speed;
				}
			}
		}else                                                               /*没有超速或超速预警，记录开始超速，*/
		{
			utc_overspeed_start		= utc_now;                              /*记录超速开始的时刻*/
			mytime_overspeed_start	= mytime_now;
		}
	}else
	{
		if( gps_speed_10x >= ( limit_speed_10x - jt808_param.id_0x005B ) )  /*超速预警*/
		{
			if( ( jt808_param.id_0x0050 & BIT_ALARM_PRE_OVERSPEED ) == 0 )  /*报警屏蔽字*/
			{
				jt808_alarm |= ( BIT_ALARM_PRE_OVERSPEED );
				jt808_alarm &= ~( BIT_ALARM_OVERSPEED );
				beep( 10, 1, 1 );
			}
			overspeed_flag = 1;
		}else                                                               /*没有超速，也没有预警,清除标志位*/
		{
			if( overspeed_flag > 1 )                                        /*已经超速了,现在速度正常，要记录当前的超速记录*/
			{
				vdr_16_put( mytime_overspeed_start, mytime_now, overspeed_min, overspeed_max, overspeed_sum / overspeed_count );
			}
			overspeed_flag	= 0;
			jt808_alarm		&= ~( BIT_ALARM_OVERSPEED | BIT_ALARM_PRE_OVERSPEED );
		}
		utc_overspeed_start = 0;                                            /*准备记录超速的时刻*/
		overspeed_max		= 0;
		overspeed_min		= 0xFF;
		overspeed_count		= 0;
		overspeed_sum		= 0;
	}
}

/*
   收到已定位gps数据的处理，
   存储位置信息，
   速度判断，校准
 */

void vdr_rx_gps( void )
{
	uint32_t datetime;

#ifdef TEST_BKPSRAM
	uint8_t buf[128];
	uint8_t *pbkpsram;
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

	datetime = mytime_from_hex( gps_datetime );         /*可以直接使用mytime_now*/

	vdr_08_put( datetime, gps_speed, vdr_signal_status );
	vdr_09_put( datetime );

	/*保存数据,停车前20ms数据*/
	if( utc_now - utc_speed_status > 20 )               /*超过20秒的没有数据的间隔*/
	{
		speed_status_index = 0;                         /*重新记录*/
	}
	speed_status[speed_status_index][0] = gps_speed;    /*当前位置写入速度,200ms任务中操作*/
	utc_speed_status					= utc_now;

	process_overspeed( );
	process_overtime( );
}

/*
   初始化08 09 10 11 12的记录
   返回 记录的时刻
     0:没有记录
 */
MYTIME vdr_08_12_init( uint8_t vdr_id, uint8_t format )
{
	uint32_t	addr;
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
	rt_kprintf( "\n%d>vdr%02d time=%08x sect=%d index=%d", rt_tick_get( ), vdr_id, mytime_ret, sect_info[id].sector, sect_info[id].index );
	return mytime_ret;
}

FINSH_FUNCTION_EXPORT_ALIAS( vdr_08_12_init, vdr_format, format vdr record );

/*获取08_12的数据*/
uint16_t vdr_08_12_getdata( VDR_USERDATA *userdata, uint8_t *pout )
{
}

/*读取数据*/
uint8_t vdr_08_12_fill_data( JT808_TX_NODEDATA *pnodedata )
{
	uint32_t		i, count = 0;
	uint8_t			*pdata_body;
	uint8_t			*pdata_head;
	uint8_t			*pdata;
	VDR_USERDATA	* puserdata;
	uint32_t		addr;
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

	if( pnodedata->type > SINGLE_ACK )
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

	if( puserdata->id == 9 )        /*整理一下09数据,无效的位置信息填写0x7FFFFFFF*/
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
	rt_kprintf( "\npdata_head=%p", pdata_head );

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
	rt_kprintf( "\n%d>生成 %d bytes", rt_tick_get( ), pnodedata->msg_len );
	rt_kprintf( "\npuserdata->addr=%08x", addr );
	rt_kprintf( "\npuserdata->record_total=%d", puserdata->record_total );
	rt_kprintf( "\npuserdata->record_remain=%d", puserdata->record_remain );

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

   行车记录仪数据有校验

 */
void vdr_08_12_get_ready( uint8_t vdr_id, uint16_t seq, MYTIME start, MYTIME end, uint16_t totalrecord )
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
	rt_kprintf( "\n%d>开始遍历数据", rt_tick_get( ) );

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

	rt_kprintf( "\n%d>vdr%02d遍历结束(%d条) FROM %d:%d TO %d:%d",
	            rt_tick_get( ),
	            vdr_id,
	            rec_count,
	            sector_from, index_from,
	            sector_to, index_to );

	if( rec_count == 0 )    /*没有找到记录，也要上报*/
	{
		buf[0]	= seq >> 8;
		buf[1]	= seq & 0xff;
		buf[2]	= vdr_id;
		buf[3]	= 0x55;     /*vdr相关*/
		buf[4]	= 0x7a;
		buf[5]	= vdr_id;   /*命令*/
		buf[6]	= 0x0;      /*长度*/
		buf[7]	= 0x0;
		buf[8]	= 0x0;      /*保留*/
		buf[9]	= ( 0x55 ^ 0x7a ^ vdr_id );
		jt808_tx_ack( 0x0700, buf, 10 );
		return;
	}

	puserdata = rt_malloc( sizeof( VDR_USERDATA ) );

	if( puserdata == RT_NULL )
	{
		buf[0]	= seq >> 8;
		buf[1]	= seq & 0xff;
		buf[2]	= vdr_id;
		buf[3]	= 0x55; /*vdr相关*/
		buf[4]	= 0x7a;
		buf[5]	= 0xFA; /*采集出错命令*/
		buf[6]	= 0x0;  /*保留*/
		buf[7]	= ( 0x55 ^ 0x7a ^ 0xFA );
		jt808_tx_ack( 0x0700, buf, 8 );
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

	/*每包用户数据大小，加7 是因为第一包有vdr的头和校验*/
	i = sect_info[id].record_per_packet * sect_info[id].data_size + 7;

	/*计算要发送的数据总数，和单包最大字节数*/
	if( rec_count > sect_info[id].record_per_packet ) /*多包发送,*/
	{
		pnodedata = node_begin( 1, MULTI_CMD, 0x0700, 0xF000, i );
	} else
	{
		pnodedata = node_begin( 1, SINGLE_CMD, 0x0700, 0xF000, i );
	}
	if( pnodedata == RT_NULL )
	{
		rt_free( puserdata );
		puserdata	= RT_NULL;
		buf[0]		= seq >> 8;
		buf[1]		= seq & 0xff;
		buf[2]		= vdr_id;
		buf[3]		= 0x55; /*vdr相关*/
		buf[4]		= 0x7a;
		buf[5]		= 0xFA; /*采集出错命令*/
		buf[6]		= 0x0;  /*保留*/
		buf[7]		= ( 0x55 ^ 0x7a ^ 0xFA );
		jt808_tx_ack( 0x0700, buf, 8 );
		rt_kprintf( "\n无法分配" );
		return;
	}
	/*计算需要的数据包数*/
	pnodedata->packet_num	= ( rec_count + sect_info[id].record_per_packet - 1 ) / sect_info[id].record_per_packet;
	pnodedata->packet_no	= 0;
	rt_kprintf( "\n填充VDR数据" );

	vdr_08_12_fill_data( pnodedata );

	node_end( pnodedata, vdr_08_12_tx_timeout, vdr_08_12_tx_response, puserdata );
}

FINSH_FUNCTION_EXPORT_ALIAS( vdr_08_12_get_ready, vdr_get, get_vdr_data );


/*
   初始化记录区数据
   因为是属于固定时间段存储的
   需要记录开始时刻的sector位置(相对的sector偏移)
 */
void vdr_init( void )
{
	uint8_t* pbuf;

	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 5 );

	vdr_08_time = vdr_08_12_init( 8, 0 );
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
	uint8_t		buf[100];
	uint8_t		tmpbuf[32];

	uint16_t	seq = JT808HEAD_SEQ( pmsg );
	uint16_t	len = JT808HEAD_LEN( pmsg );
	uint8_t		cmd = *( pmsg + 12 ); /*跳过前面12字节的头*/
	MYTIME		start, end;
	uint16_t	blocks;
	uint32_t	i;
	uint8_t		fcs;

	switch( cmd )
	{
		case 0x00: /*采集记录仪执行标准版本*/
			buf[0]	= seq >> 8;
			buf[1]	= seq & 0xff;
			buf[2]	= cmd;
			fcs		= 0;
			vdr_pack_buf( buf + 3, "\x55\x7A\x00\x00\x02\x00\x12\x00", 8, &fcs );
			buf[11] = fcs;
			jt808_tx_ack( 0x0700, buf, 12 );
			break;
		case 0x01:
			buf[0]	= seq >> 8;
			buf[1]	= seq & 0xff;
			buf[2]	= cmd;
			fcs		= 0;
			vdr_pack_buf( buf + 3, "\x55\x7A\x01\x00\x12\x00" "120221123456789\x00\x00\x00\x00", 25, &fcs );
			buf[28] = fcs;
			jt808_tx_ack( 0x0700, buf, 29 );
			break;
		case 0x02: /*行车记录仪时间*/
			buf[0]	= seq >> 8;
			buf[1]	= seq & 0xff;
			buf[2]	= cmd;
			fcs		= 0;
			vdr_pack_buf( buf + 3, "\x55\x7A\x02\x00\x06\x00", 6, &fcs );
			vdr_pack_buf( buf + 9, gps_baseinfo.datetime, 6, &fcs );
			buf[15] = fcs;
			jt808_tx_ack( 0x0700, buf, 16 );
			break;
		case 0x03:
			buf[0]	= seq >> 8;
			buf[1]	= seq & 0xff;
			buf[2]	= cmd;
			fcs		= 0;
			vdr_pack_buf( buf + 3, "\x55\x7A\x03\x00\x14\x00", 6, &fcs );

			mytime_to_bcd( tmpbuf, mytime_now );            /*实时时间*/
			vdr_pack_buf( buf + 9, tmpbuf, 6, &fcs );

			mytime_to_bcd( tmpbuf, jt808_param.id_0xF030 ); /*首次安装时间*/
			vdr_pack_buf( buf + 15, tmpbuf, 6, &fcs );
			i			= jt808_param.id_0xF032 * 10;       /*初次安装里程 0.1KM BCD码 00-99999999*/
			tmpbuf[0]	= ( ( ( i / 10000000 ) % 10 ) << 4 ) | ( ( i / 1000000 ) % 10 );
			tmpbuf[1]	= ( ( ( i / 100000 ) % 10 ) << 4 ) | ( ( i / 10000 ) % 10 );
			tmpbuf[2]	= ( ( ( i / 1000 ) % 10 ) << 4 ) | ( ( i / 100 ) % 10 );
			tmpbuf[3]	= ( ( ( i / 10 ) % 10 ) << 4 ) | ( i % 10 );
			vdr_pack_buf( buf + 21, tmpbuf, 4, &fcs );
			i			= jt808_param.id_0xF020 * 10;       /*总里程 0.1KM BCD码 00-99999999*/
			tmpbuf[0]	= ( ( ( i / 10000000 ) % 10 ) << 4 ) | ( ( i / 1000000 ) % 10 );
			tmpbuf[1]	= ( ( ( i / 100000 ) % 10 ) << 4 ) | ( ( i / 10000 ) % 10 );
			tmpbuf[2]	= ( ( ( i / 1000 ) % 10 ) << 4 ) | ( ( i / 100 ) % 10 );
			tmpbuf[3]	= ( ( ( i / 10 ) % 10 ) << 4 ) | ( i % 10 );
			vdr_pack_buf( buf + 25, tmpbuf, 4, &fcs );
			buf[29] = fcs;
			jt808_tx_ack( 0x0700, buf, 30 );
			break;
		case 0x04: /*特征系数*/
			buf[0]	= seq >> 8;
			buf[1]	= seq & 0xff;
			buf[2]	= cmd;
			fcs		= 0;
			vdr_pack_buf( buf + 3, "\x55\x7A\x04\x00\x08\x00", 6, &fcs );
			vdr_pack_buf( buf + 9, gps_baseinfo.datetime, 6, &fcs );
			vdr_pack_word( buf + 15, jt808_param.id_0xF033, &fcs );
			buf[17] = fcs;
			jt808_tx_ack( 0x0700, buf, 18 );
			break;
		case 0x05: /*车辆信息  41byte*/
			buf[0]	= seq >> 8;
			buf[1]	= seq & 0xff;
			buf[2]	= cmd;
			fcs		= 0;
			vdr_pack_buf( buf + 3, "\x55\x7A\x05\x00\x29\x00", 6, &fcs );
			vdr_pack_buf( buf + 9, jt808_param.id_0x0083, 12, &fcs );
			vdr_pack_buf( buf + 21, "大型汽车    ", 12, &fcs );
			buf[33] = fcs;
			jt808_tx_ack( 0x0700, buf, 34 );
			break;
		case 0x06:                              /*状态信号配置信息 87byte*/
			buf[0]	= seq >> 8;
			buf[1]	= seq & 0xff;
			buf[2]	= cmd;
			fcs		= 0;
			vdr_pack_buf( buf + 3, "\x55\x7A\x06\x00\x57\x00", 6, &fcs );
			vdr_pack_buf( buf + 9, gps_baseinfo.datetime, 6, &fcs );
			vdr_pack_byte( buf + 15, 1, &fcs ); /*状态信号字节个数*/
			vdr_pack_buf( buf + 16, "用户自定义", 10, &fcs );
			vdr_pack_buf( buf + 26, "用户自定义", 10, &fcs );
			vdr_pack_buf( buf + 36, "用户自定义", 10, &fcs );
			vdr_pack_buf( buf + 46, "近光\0\0\0\0\0\0", 10, &fcs );
			vdr_pack_buf( buf + 56, "远光\0\0\0\0\0\0", 10, &fcs );
			vdr_pack_buf( buf + 66, "右转向\0\0\0\0", 10, &fcs );
			vdr_pack_buf( buf + 76, "左转向\0\0\0\0", 10, &fcs );
			vdr_pack_buf( buf + 86, "制动\0\0\0\0\0\0", 10, &fcs );
			buf[97] = fcs;
			jt808_tx_ack( 0x0700, buf, 98 );
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
				start	= mytime_from_bcd( psrc );
				end		= mytime_from_bcd( psrc + 6 );
				blocks	= ( *( psrc + 32 ) << 8 ) | ( *( psrc + 33 ) );
			}else
			{
				rt_kprintf( "\n%d>8700的格式不识别", rt_tick_get( ) );
				return;
			}
			vdr_08_12_get_ready( cmd, seq, start, end, blocks );
			break;
		case 0x13:
			break;
		case 0x14:
			break;
		case 0x15:
			break;
		case 0x82:          /*设置车辆信息*/
			if( len == 1 )  /*消息体为空*/
			{
				break;
			}
		case 0x83:          /*设置初次安装日期*/
			break;
		case 0x84:          /*设置状态量配置*/
			break;

		case 0xC2:          /*设置记录仪时间*/
			if( len == 7 )  /*命令字+时间BCD*/
			{
			}
			break;
		case 0xC3:          /*设置脉冲系数*/
			break;
		case 0xC4:          /*设置初始里程*/
			break;
	}
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
void vdr_rx_8701( uint8_t * pmsg )
{
}

/************************************** The End Of File **************************************/
