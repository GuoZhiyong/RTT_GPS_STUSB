/*
   vdr Vichle Driver Record ������ʻ��¼
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

#define SPEED_LIMIT				5   /*�ٶ����� ���ڴ�ֵ��Ϊ������С�ڴ�ֵ��Ϊֹͣ*/
#define SPEED_LIMIT_DURATION	10  /*�ٶ����޳���ʱ��*/

#define VEHICLE_FLAG_NORMAL		0   /*����״̬��ֹͣ����ʻ*/
#define VEHICLE_FLAG_OVERSPEED	1   /*������ʻ*/
#define VEHICLE_FLAG_OVERTIME	2   /*��ʱ��ʻ*/
#define VEHICLE_FlAG_MAXSTOP	8   /*�ֹͣʱ�䣬��ʱ��ʻ�ж���*/

//#define TEST_BKPSRAM

#define VDR_BASE 0x03B000

#define VDR_16_START	VDR_BASE
#define VDR_16_SECTORS	3           /*ÿСʱ2��sector,����50Сʱ*/
#define VDR_16_END		( VDR_16_START + VDR_16_SECTORS * 4096 )

#define VDR_08_START	( VDR_16_END )
#define VDR_08_SECTORS	100         /*ÿСʱ2��sector,����50Сʱ*/
#define VDR_08_END		( VDR_08_START + VDR_08_SECTORS * 4096 )

#define VDR_09_START	( VDR_08_START + VDR_08_SECTORS * 4096 )
#define VDR_09_SECTORS	64          /*16�죬ÿ��4sector*/
#define VDR_09_END		( VDR_09_START + VDR_09_SECTORS * 4096 )

#define VDR_10_START	( VDR_09_START + VDR_09_SECTORS * 4096 )
#define VDR_10_SECTORS	8           /*100���¹��ɵ� 100*234  ʵ�� 128*256 */
#define VDR_10_END		( VDR_10_START + VDR_10_SECTORS * 4096 )

#define VDR_11_START	( VDR_10_START + VDR_10_SECTORS * 4096 )
#define VDR_11_SECTORS	3           /*100����ʱ��ʻ��¼ 100*50 ʵ�� 128*64,����һ��������ɾ��ʱ��������*/
#define VDR_11_END		( VDR_11_START + VDR_11_SECTORS * 4096 )

#define VDR_12_START	( VDR_11_START + VDR_11_SECTORS * 4096 )
#define VDR_12_SECTORS	3           /*200����ʻ����ݼ�¼ 200*25 ʵ��200*32 */
#define VDR_12_END		( VDR_12_START + VDR_12_SECTORS * 4096 )

#define VDR_13_14_15_START		( VDR_12_START + VDR_12_SECTORS * 4096 )
#define VDR_13_14_15_SECTORS	1   /*100�� �ⲿ�����¼ 100�� �����޸ļ�¼ 10���ٶ�״̬��־ */
#define VDR_13_14_15_END		( VDR_13_14_15_START + VDR_13_14_15_SECTORS * 4096 )


/*
   4MB serial flash 0x400000
 */

static rt_thread_t		tid_usb_vdr = RT_NULL;

static struct rt_timer	tmr_200ms;

struct _sect_info
{
	uint8_t		flag;               /*��־*/
	uint32_t	addr_start;         /*��ʼ�ĵ�ַ*/
	uint8_t		sector_count;       /*��¼����������ռ�õ�������*/
	uint16_t	record_per_sector;  /*ÿ������¼��*/
	uint16_t	record_size;        /*��¼��С*/
	uint16_t	data_size;          /*�û����ݴ�С*/
	uint8_t		record_per_packet;  /*����ϴ�ʱ��ÿ��������¼��*/
	uint8_t		sector;             /*��ǰ���ڵ�����*/
	uint8_t		index;              /*��ǰ���ڵ������ڼ�¼λ������*/
} sect_info[5] =
{
	{ '8', VDR_08_START, VDR_08_SECTORS, 30,  136, 126, 5,	VDR_08_SECTORS, 30,	 },
	{ '9', VDR_09_START, VDR_09_SECTORS, 6,	  680, 666, 1,	VDR_09_SECTORS, 6,	 },
	{ 'A', VDR_10_START, VDR_10_SECTORS, 16,  256, 234, 2,	VDR_10_SECTORS, 16,	 },
	{ 'B', VDR_11_START, VDR_11_SECTORS, 64,  64,  50,	10, VDR_11_SECTORS, 64,	 },
	{ 'C', VDR_12_START, VDR_12_SECTORS, 128, 32,  25,	20, VDR_12_SECTORS, 128, },
};
#if 0
/*����UTCʱ��*/
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

/*��ʱ��ƣ�ͼ�ʻ����*/
static MYTIME	vdr11_mytime_start	= 0;
static uint32_t vdr11_lati_start	= 0;
static uint32_t vdr11_longi_start	= 0;
static uint16_t vdr11_alti_start	= 0;


/*
   200ms��ʱ��
   ������٣��¹��ɵ���ٶ�У׼

   ��¼��Ӧ����0.2s��ʱ����������¼���洢��ʻ����ǰ20sʵʱʱ���Ӧ����ʻ״̬���ݣ�����
   ʻ״̬����Ϊ��������ʻ�ٶȡ��ƶ���״̬�źź���ʻ����ʱ��λ����Ϣ��

   �ڳ�����ʻ״̬�¼�¼���ⲿ����Ͽ�ʱ����¼��Ӧ����0.2s��ʱ����������¼���洢�ϵ�ǰ
   20s�ڵĳ�����ʻ״̬���ݣ�����ʻ״̬����Ϊ��������ʻ�ٶȡ������ƶ���״̬�źż��ϵ�ʱ��
   λ����Ϣ��

   �ڳ���������ʻ״̬����Чλ����Ϣ10s���ޱ仯ʱ����¼��Ӧ����0.2s��ʱ����������¼����
   ���ϵ�ǰ20s�ڵĳ�����ʻ״̬���ݣ�����ʻ״̬����Ϊ��������ʻ�ٶȡ������ƶ���״̬�źż�
   �ϵ�ʱ��λ����Ϣ��

 */
uint8_t vdr_signal_status = 0x01;           /*�г���¼�ǵ�״̬�ź�*/

/*200ms������ٶ�״̬��Ϣ*/
static uint8_t	speed_status[100][2];
static uint8_t	speed_status_index	= 0;
static uint32_t utc_speed_status	= 0;    /*������汣���¼��ʱ��*/


/*
   ÿ200�����Ⲣ����״̬
 */
static void cb_tmr_200ms( void* parameter )
{
	__IO uint8_t i;

	i									= ( speed_status_index + 99 ) % 100;    /*������һ��λ�ã�������*/
	speed_status[speed_status_index][0] = speed_status[i][0];                   /*������һ�ε�ֵ*/
	speed_status[speed_status_index][1] = vdr_signal_status;

	speed_status_index++;
	if( speed_status_index > 99 )
	{
		speed_status_index = 0;
	}
}

static MYTIME	vdr_08_time = 0;            /*��ǰʱ���*/
static uint8_t	vdr_08_info[130];           /*����Ҫд�����Ϣ*/
static MYTIME	vdr_09_time = 0;
static MYTIME	vdr_10_time = 0;

typedef __packed struct _vdr_08_userdata
{
//	MYTIME		start;          /*��ʼʱ��*/
//	MYTIME		end;            /*����ʱ��*/
	uint16_t	record_total;   /*�ܵü�¼��*/
	uint16_t	record_remain;  /*��û�з��͵ļ�¼��*/

	uint32_t	addr_from;      /*��Ϣ��ʼ�ĵ�ַ*/
	uint32_t	addr_to;        /*��Ϣ�����ĵ�ַ*/
	uint32_t	addr;           /*��ǰҪ���ĵ�ַ*/

	uint16_t	rec_per_packet; /*ÿ���ļ�¼��*/
	uint16_t	packet_total;   /*�ܰ���*/
	uint16_t	packet_curr;    /*��ǰ����*/
}VDR_08_USERDATA;

VDR_08_USERDATA vdr_08_userdata;

typedef __packed struct _vdr_userdata
{
	uint8_t		id;             /*��Ӧ��vdr���� 8-15*/
	uint16_t	record_total;   /*�ܵü�¼��*/
	uint16_t	record_remain;  /*��û�з��͵ļ�¼��*/

	uint32_t	sector_from;    /*��Ϣ��ʼ������*/
	uint32_t	index_from;     /*��Ϣ��ʼ�������ڼ�¼��*/

	uint32_t	sector_to;      /*��Ϣ����������*/
	uint32_t	index_to;       /*��Ϣ�����������ڼ�¼��*/

	uint32_t	sector;         /*��ǰ������Ϣ������*/
	uint32_t	index;          /*��ǰ������Ϣ�������ڼ�¼��*/

	uint32_t addr;              /*��ǰҪ���ĵ�ַ*/

	uint16_t	rec_per_packet; /*ÿ���ļ�¼��*/
	uint16_t	packet_total;   /*�ܰ���*/
	uint16_t	packet_curr;    /*��ǰ����*/
}VDR_USERDATA;

enum _stop_run
{
	STOP=0,
	RUN =1,
};

static enum _stop_run	car_stop_run	= STOP;
static uint32_t			utc_car_stop	= 0;    /*������ʼͣʻʱ��*/
static uint32_t			utc_car_run		= 0;    /*������ʼ��ʻʱ��*/


/*
   ��ȡid��һ�����õĵ�ַ
   ��û�����������������д֮ǰ����
   20130801:����Ҫλ��4k�߽紦�����ܿ����������򻷻�д��ʱ���������

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

/*�������� ��1��ļ�����������ܷ���Ĺ���**/
void vdr_08_put( MYTIME datetime, uint8_t speed, uint8_t status )
{
	uint32_t	sec;
	uint32_t	addr;

	if( ( vdr_08_time & 0xFFFFFFC0 ) != ( datetime & 0xFFFFFFC0 ) ) /*�����ڵ�ǰ��һ������*/
	{
		if( vdr_08_time != 0 )                                      /*����Ч������,Ҫ���棬ע���һ�����ݵĴ���*/
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
			if( ( addr & 0xFFF ) == 0 ) /*��4k�߽紦*/
			{
				sst25_erase_4k( addr );
			}
			sst25_write_through( addr, vdr_08_info, sizeof( vdr_08_info ) );
			rt_sem_release( &sem_dataflash );
			rt_kprintf( "\n%d>д��08 ��ַ:%08x(sect:count=%d:%d) ֵ:%08x", rt_tick_get( ), addr, sect_info[0].sector, sect_info[0].index, vdr_08_time );

#endif
			memset( vdr_08_info, 0xFF, sizeof( vdr_08_info ) ); /*�µļ�¼����ʼ��Ϊ0xFF*/
		}
	}
	sec							= SEC( datetime );
	vdr_08_time					= datetime;
	vdr_08_info[sec * 2 + 10]	= gps_speed;
	vdr_08_info[sec * 2 + 11]	= vdr_signal_status;
}

/*
   ÿ������6��Сʱ������
   һ��Сʱ666byte ռ��680byte
 */
void vdr_09_put( MYTIME datetime )
{
	uint8_t		buf[16];
	uint32_t	addr;
	uint8_t		minute;

	if( ( vdr_09_time & 0xFFFFFFC0 ) == ( datetime & 0xFFFFFFC0 ) ) /*��ͬһ������*/
	{
		return;
	}

	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );
	if( ( vdr_09_time & 0xFFFFF000 ) != ( datetime & 0xFFFFF000 ) ) /*�������ڵ�ǰ��Сʱ��,�¼�¼*/
	{
		addr = vdr_08_12_get_nextaddr( 9 );
		if( ( addr & 0xFFF ) == 0 )                                 /*��4k�߽紦*/
		{
			sst25_erase_4k( addr );
		}

		PACK_INT( buf, ( datetime & 0xFFFFF000 ) );                 /*д���µļ�¼ͷ*/
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
	addr	= VDR_09_START + sect_info[1].sector * 4096 + sect_info[1].index * 680 + minute * 11 + 10; /*����ָ����λ��,��ʼ��10�ֽ�ͷ*/
	sst25_write_through( addr, buf, 11 );
	rt_sem_release( &sem_dataflash );

	rt_kprintf( "\n%d>save 09 data addr=%08x", rt_tick_get( ), addr );
}

/*
   234 byte
   ��������뵥�δ����ģ����ǳ���ͣʻ������
 */
void vdr_10_put( MYTIME datetime )
{
	uint32_t	i, j;
	uint32_t	addr;
	uint8_t		buf[234 + 4]; /*����Ҫд�����Ϣ*/
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
	memcpy( buf + 10, jt808_param.id_0xF009, 18 );  /*��ʻ֤����*/
	PACK_INT( buf + 228, ( gps_longi * 6 ) );
	PACK_INT( buf + 232, ( gps_lati * 6 ) );
	PACK_WORD( buf + 236, ( gps_alti ) );

	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );
	addr = vdr_08_12_get_nextaddr( 10 );
	if( ( addr & 0xFFF ) == 0 )                     /*��4k�߽紦*/
	{
		sst25_erase_4k( addr );
	}
	sst25_write_through( addr, buf, 238 );

	rt_sem_release( &sem_dataflash );
	rt_kprintf( "\n%d>save 10 data addr=%08x", rt_tick_get( ), addr );
}

/*
   ��ʱ,ƣ�ͼ�ʻ��¼
   �п��ܳ�ʱ�ˣ����ڼ�ʻ����ʱҪ��̬����
   �ȵ���ͣ�Ժ���Ϣ���㹻��ʱ�䣬����һ�������ĳ�ʱ��ʻ��¼
   ��α���?
 */
void vdr_11_put( MYTIME datetime )
{
	uint32_t	addr;
	uint8_t		buf[64];                            /*����Ҫд�����Ϣ*/

	buf[0]	= datetime >> 24;
	buf[1]	= datetime >> 16;
	buf[2]	= datetime >> 8;
	buf[3]	= datetime & 0xFF;
	memcpy( buf + 4, jt808_param.id_0xF009, 18 );   /*��ʻ֤����*/

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
	if( ( addr & 0xFFF ) == 0 ) /*��4k�߽紦*/
	{
		sst25_erase_4k( addr );
	}
	sst25_write_through( addr, buf, 54 );

	rt_sem_release( &sem_dataflash );
	rt_kprintf( "\n%d>save 11 data addr=%08x", rt_tick_get( ), addr );
}

/*���ټ�ʻ��¼*/
void vdr_16_put( MYTIME start, MYTIME end, uint16_t speed_min, uint16_t speed_max, uint16_t speed_avg )
{
	uint32_t	addr;
	uint8_t		buf[64]; /*����Ҫд�����Ϣ*/

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
	if( ( addr & 0xFFF ) == 0 ) /*��4k�߽紦*/
	{
		sst25_erase_4k( addr );
	}
	sst25_write_through( addr, buf, 22 );

	rt_sem_release( &sem_dataflash );
	rt_kprintf( "\n%d>save 16 data addr=%08x", rt_tick_get( ), addr );
}

/*
   ÿ���жϳ���״̬,gps�е�RMC��䴥��
   ��ʻ���
   Ĭ�϶���ʹ��GPS�ٶȣ����gpsδ��λ��Ҳ����������
 */

/*gps��Ч����¼�鳵����ʻ״̬*/
void process_overtime( void )
{
/*�жϳ�����ʻ״̬*/
	if( car_stop_run == STOP )                                          /*��Ϊ����ֹͣ,�ж�����*/
	{
		if( utc_car_stop == 0 )                                         /*ͣ��ʱ����δ��ʼ����Ĭ��ͣʻ*/
		{
			utc_car_stop = utc_now;
		}
		if( gps_speed >= SPEED_LIMIT )                                  /*�ٶȴ�������ֵ*/
		{
			if( utc_car_run == 0 )                                      /*��û�м�¼��ʻ��ʱ��*/
			{
				utc_car_run = utc_now;                                  /*��¼��ʼʱ��*/
			}

			if( ( utc_now - utc_car_run ) >= SPEED_LIMIT_DURATION )     /*�����˳���ʱ��*/
			{
				car_stop_run = RUN;                                     /*��Ϊ������ʻ*/
				rt_kprintf( "\n%d>������ʻ", rt_tick_get( ) );
				utc_car_stop = 0;                                       /*�ȴ��ж�ͣʻ*/
			}
		}else
		{
			if( utc_now - utc_car_stop > jt808_param.id_0x005A )        /*�ж�ͣ���ʱ��*/
			{
				//rt_kprintf( "�ﵽͣ���ʱ��\r\n" );
				jt808_alarm|=BIT_ALARM_STOP_OVERTIME;
			}
			utc_car_run = 0;
		}
	}else                                                               /*����������*/
	{
		if( gps_speed <= SPEED_LIMIT )                                  /*�ٶ�С������ֵ*/
		{
			if( utc_car_stop == 0 )
			{
				utc_car_stop = utc_now;
			}
			if( ( utc_now - utc_car_stop ) >= SPEED_LIMIT_DURATION )    /*�����˳���ʱ��*/
			{
				car_stop_run = STOP;                                    /*��Ϊ����ͣʻ*/
				rt_kprintf( "\n%d>����ͣʻ", rt_tick_get( ) );
				utc_car_run = 0;
				vdr_10_put( mytime_now );                               /*����VDR_10����,�¹��ɵ�*/
			}
		}else
		{
			if( utc_now - utc_car_run > jt808_param.id_0x0057 )         /*�ж�������ʻ�ʱ��*/
			{
				//rt_kprintf( "�ﵽ������ʻ�ʱ��\r\n" );
			}
			utc_car_stop = 0;
		}
	}
}

/*���١�����Ԥ���ж�*/
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

	if( gps_speed >= jt808_param.id_0x0055 )                                /*��������ٶ�*/
	{
		if( utc_overspeed_start )                                           /*�жϳ��ٳ����¼�*/
		{
			if( utc_now - utc_overspeed_start >= jt808_param.id_0x0056 )    /*�Ѿ���������*/
			{
				if( ( jt808_param.id_0x0050 & BIT_ALARM_OVERSPEED ) == 0 )  /*����������*/
				{
					jt808_alarm |= BIT_ALARM_OVERSPEED;
					beep( 5, 1, 1 );
				}
				overspeed_flag	= 2;                                        /*�Ѿ�������*/
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
		}else                                                               /*û�г��ٻ���Ԥ������¼��ʼ���٣�*/
		{
			utc_overspeed_start		= utc_now;                              /*��¼���ٿ�ʼ��ʱ��*/
			mytime_overspeed_start	= mytime_now;
		}
	}else
	{
		if( gps_speed_10x >= ( limit_speed_10x - jt808_param.id_0x005B ) )  /*����Ԥ��*/
		{
			if( ( jt808_param.id_0x0050 & BIT_ALARM_PRE_OVERSPEED ) == 0 )  /*����������*/
			{
				jt808_alarm |= ( BIT_ALARM_PRE_OVERSPEED );
				jt808_alarm &= ~( BIT_ALARM_OVERSPEED );
				beep( 10, 1, 1 );
			}
			overspeed_flag = 1;
		}else                                                               /*û�г��٣�Ҳû��Ԥ��,�����־λ*/
		{
			if( overspeed_flag > 1 )                                        /*�Ѿ�������,�����ٶ�������Ҫ��¼��ǰ�ĳ��ټ�¼*/
			{
				vdr_16_put( mytime_overspeed_start, mytime_now, overspeed_min, overspeed_max, overspeed_sum / overspeed_count );
			}
			overspeed_flag	= 0;
			jt808_alarm		&= ~( BIT_ALARM_OVERSPEED | BIT_ALARM_PRE_OVERSPEED );
		}
		utc_overspeed_start = 0;                                            /*׼����¼���ٵ�ʱ��*/
		overspeed_max		= 0;
		overspeed_min		= 0xFF;
		overspeed_count		= 0;
		overspeed_sum		= 0;
	}
}

/*
   �յ��Ѷ�λgps���ݵĴ���
   �洢λ����Ϣ��
   �ٶ��жϣ�У׼
 */

void vdr_rx_gps( void )
{
	uint32_t datetime;

#ifdef TEST_BKPSRAM
	uint8_t buf[128];
	uint8_t *pbkpsram;
/*�ϵ����Ҫд�����ʷ���� 08*/
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

	datetime = mytime_from_hex( gps_datetime );         /*����ֱ��ʹ��mytime_now*/

	vdr_08_put( datetime, gps_speed, vdr_signal_status );
	vdr_09_put( datetime );

	/*��������,ͣ��ǰ20ms����*/
	if( utc_now - utc_speed_status > 20 )               /*����20���û�����ݵļ��*/
	{
		speed_status_index = 0;                         /*���¼�¼*/
	}
	speed_status[speed_status_index][0] = gps_speed;    /*��ǰλ��д���ٶ�,200ms�����в���*/
	utc_speed_status					= utc_now;

	process_overspeed( );
	process_overtime( );
}

/*
   ��ʼ��08 09 10 11 12�ļ�¼
   ���� ��¼��ʱ��
     0:û�м�¼
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

		for( count = 0; count < sect_info[id].record_per_sector; count++ )                  /*ÿ����6����¼*/
		{
			sst25_read( addr, buf, 4 );
			mytime_curr = ( buf[0] << 24 ) | ( buf[1] << 16 ) | ( buf[2] << 8 ) | buf[3];   /*ǰ4���ֽڴ���ʱ��*/
			if( mytime_curr != 0xFFFFFFFF )                                                 /*����Ч��¼*/
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

/*��ȡ08_12������*/
uint16_t vdr_08_12_getdata( VDR_USERDATA *userdata, uint8_t *pout )
{
}

/*��ȡ����*/
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

	puserdata = (VDR_USERDATA* )( pnodedata->user_para );   /*ָ���û�������*/

	id = puserdata->id - 8;

	if( puserdata->record_remain == 0 )                     /*�Ѿ�����������*/
	{
		return 0;
	}

	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );  /*׼����ȡ����*/

	if( pnodedata->type > SINGLE_ACK )
	{
		pdata_body = pnodedata->tag_data + 16;
	}else
	{
		pdata_body = pnodedata->tag_data + 12;
	}

	/*��ȡҪ��ȡ�ļ�¼��,�п����Ƕ��������һ��*/
	rec_count = sect_info[id].record_per_packet;
	if( puserdata->record_remain < sect_info[id].record_per_packet )    /*���㹹��1�����Ͱ�*/
	{
		rec_count = puserdata->record_remain;
	}

	pdata = pdata_body + 6;                                             /*������ʼ��6���ֽڵ�vdrͷ 55 7A*/

	for( i = 0; i < rec_count; i++ )
	{
		addr = sect_info[id].addr_start +
		       puserdata->sector * 4096 +
		       puserdata->index * sect_info[id].record_size + 4;        /*��ʼ4�ֽ�Ϊ�Լ���ʱ��*/
		sst25_read( addr, pdata, sect_info[id].data_size );

		if( puserdata->index == 0 )                                     /*������һ��Ҫ����λ��*/
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
		puserdata->record_remain--; /*ʣ���¼��-1*/
	}
	rt_sem_release( &sem_dataflash );

	if( puserdata->id == 9 )        /*����һ��09����,��Ч��λ����Ϣ��д0x7FFFFFFF*/
	{
		pdata = pdata_body + 6 + 6; /*������ʼ��6�ֽ�55 7A��BCDʱ��ͷ*/
		for( i = 0; i < 660; i += 11 )
		{
			if( pdata[i] == 0xff )
			{
				pdata[i]		= 0x7F;
				pdata[i + 4]	= 0x7F;
			}
		}
	}
	/*����VDR��FCS*/
	count		= rec_count * sect_info[id].data_size;  /*��ȡ�ļ�¼��*/
	pdata		= pdata_body;
	pdata[0]	= 0x55;
	pdata[1]	= 0x7A;
	pdata[2]	= puserdata->id;
	pdata[3]	= count >> 8;                           /*����֪��ʵ�ʷ��͵Ĵ�С*/
	pdata[4]	= count & 0xFF;
	pdata[5]	= 0;                                    /*Ԥ��*/

	for( i = 0; i < ( count + 6 ); i++ )
	{
		fcs ^= *pdata++;
	}
	*pdata = fcs;

	pdata_head = pnodedata->tag_data; /*��ʵ�������ݵĿ�ʼλ��->��Ϣͷ*/
	rt_kprintf( "\npdata_head=%p", pdata_head );

	pdata_head[0]	= pnodedata->head_id >> 8;
	pdata_head[1]	= pnodedata->head_id & 0xFF;

	pnodedata->head_sn++;
	pdata_head[10]	= pnodedata->head_sn >> 8;
	pdata_head[11]	= pnodedata->head_sn & 0xFF;
	memcpy( pdata_head + 4, mobile, 6 ); /*�����ն˺���,��ʱ���ܻ�û��mobileh����*/

	node_datalen( pnodedata, count + 7 );

/*���Ʒ���״̬*/
	pnodedata->max_retry	= 1;
	pnodedata->retry		= 0;
	pnodedata->timeout		= RT_TICK_PER_SECOND * 3;
	pnodedata->state		= IDLE;

	/*dump ����*/
	rt_kprintf( "\n%d>���� %d bytes", rt_tick_get( ), pnodedata->msg_len );
	rt_kprintf( "\npuserdata->addr=%08x", addr );
	rt_kprintf( "\npuserdata->record_total=%d", puserdata->record_total );
	rt_kprintf( "\npuserdata->record_remain=%d", puserdata->record_remain );

	return 1;
}

/*�յ�Ӧ��Ĵ�����*/
static JT808_MSG_STATE vdr_08_12_tx_response( JT808_TX_NODEDATA * pnodedata, uint8_t *pmsg )
{
	pnodedata->retry++;
	if( pnodedata->retry >= pnodedata->max_retry )
	{
		return WAIT_DELETE;
	}
	return vdr_08_12_fill_data( pnodedata );
}

/*��ʱ��Ĵ�����*/
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
   ����08-12���͵����ݰ�
   ��ͬ������������ǲ�ͬ�ġ�

   �г���¼��������У��

 */
void vdr_08_12_get_ready( uint8_t vdr_id, uint16_t seq, MYTIME start, MYTIME end, uint16_t totalrecord )
{
	uint32_t			addr;
	uint8_t				buf[16];
	JT808_TX_NODEDATA	*pnodedata;
	VDR_USERDATA		*puserdata;
	uint32_t			i;
	uint16_t			rec_count = 0;      /*�ҵ��ļ�¼��*/

	uint8_t				sector;
	uint8_t				index;

	uint8_t				sector_from = 0xFF; /*��λû���ҵ�*/
	uint8_t				index_from;

	uint8_t				sector_to;
	uint8_t				index_to;
	uint32_t			time_to = 0xFFFFFFFF;
	MYTIME				mytime;
	uint8_t				id;

	id = vdr_id - 8;

	/*�ӵ�ǰλ�ÿ�ʼ�������*/
	rt_kprintf( "\n%d>��ʼ��������", rt_tick_get( ) );

	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );

	sector	= sect_info[id].sector;                     /*�ӵ�ǰλ�ÿ�ʼ����,����*/
	index	= sect_info[id].index;

	for( i = 0; i < sect_info[id].sector_count; i++ )   /*����һ������*/
	{
		addr = sect_info[id].addr_start + sector * 4096 + index * sect_info[id].record_size;
		sst25_read( addr, buf, 4 );
		mytime = ( buf[0] << 24 ) | ( buf[1] << 16 ) | ( buf[2] << 8 ) | buf[3];
		if( ( mytime >= start ) && ( mytime <= end ) )  /*����Чʱ�����*/
		{
			rec_count++;
			if( rec_count >= totalrecord )              /*�յ��㹻�ļ�¼��*/
			{
				break;
			}
			if( sector_from == 0xFF )                   /*��һ���ҵ��ľ��������*/
			{
				sector_from = sector;
				index_from	= index;
			}
			if( mytime <= time_to )                     /*��С�Ĳ�������ϵ�*/
			{
				sector_to	= sector;
				index_to	= index;
				time_to		= mytime;
			}
		}

		if( index == 0 )
		{
			index = sect_info[id].record_per_sector; /*�������˵ݼ�*/
			if( sector == 0 )
			{
				sector = sect_info[id].sector_count;
			}
			sector--;
		}
		index--;
	}
	rt_sem_release( &sem_dataflash );

	rt_kprintf( "\n%d>vdr%02d��������(%d��) FROM %d:%d TO %d:%d",
	            rt_tick_get( ),
	            vdr_id,
	            rec_count,
	            sector_from, index_from,
	            sector_to, index_to );

	if( rec_count == 0 )    /*û���ҵ���¼��ҲҪ�ϱ�*/
	{
		buf[0]	= seq >> 8;
		buf[1]	= seq & 0xff;
		buf[2]	= vdr_id;
		buf[3]	= 0x55;     /*vdr���*/
		buf[4]	= 0x7a;
		buf[5]	= vdr_id;   /*����*/
		buf[6]	= 0x0;      /*����*/
		buf[7]	= 0x0;
		buf[8]	= 0x0;      /*����*/
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
		buf[3]	= 0x55; /*vdr���*/
		buf[4]	= 0x7a;
		buf[5]	= 0xFA; /*�ɼ���������*/
		buf[6]	= 0x0;  /*����*/
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

	/*ÿ���û����ݴ�С����7 ����Ϊ��һ����vdr��ͷ��У��*/
	i = sect_info[id].record_per_packet * sect_info[id].data_size + 7;

	/*����Ҫ���͵������������͵�������ֽ���*/
	if( rec_count > sect_info[id].record_per_packet ) /*�������,*/
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
		buf[3]		= 0x55; /*vdr���*/
		buf[4]		= 0x7a;
		buf[5]		= 0xFA; /*�ɼ���������*/
		buf[6]		= 0x0;  /*����*/
		buf[7]		= ( 0x55 ^ 0x7a ^ 0xFA );
		jt808_tx_ack( 0x0700, buf, 8 );
		rt_kprintf( "\n�޷�����" );
		return;
	}
	/*������Ҫ�����ݰ���*/
	pnodedata->packet_num	= ( rec_count + sect_info[id].record_per_packet - 1 ) / sect_info[id].record_per_packet;
	pnodedata->packet_no	= 0;
	rt_kprintf( "\n���VDR����" );

	vdr_08_12_fill_data( pnodedata );

	node_end( pnodedata, vdr_08_12_tx_timeout, vdr_08_12_tx_response, puserdata );
}

FINSH_FUNCTION_EXPORT_ALIAS( vdr_08_12_get_ready, vdr_get, get_vdr_data );


/*
   ��ʼ����¼������
   ��Ϊ�����ڹ̶�ʱ��δ洢��
   ��Ҫ��¼��ʼʱ�̵�sectorλ��(��Ե�sectorƫ��)
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

/*��ʼ��һ��50ms�Ķ�ʱ���������¹��ɵ��ж�*/
	rt_timer_init( &tmr_200ms, "tmr_200ms",     /* ��ʱ�������� tmr_50ms */
	               cb_tmr_200ms,                /* ��ʱʱ�ص��Ĵ����� */
	               RT_NULL,                     /* ��ʱ��������ڲ��� */
	               RT_TICK_PER_SECOND / 5,      /* ��ʱ���ȣ���OS TickΪ��λ */
	               RT_TIMER_FLAG_PERIODIC );    /* �����Զ�ʱ�� */
	rt_timer_start( &tmr_200ms );
}

/*�г���¼�����ݲɼ�����*/
void vdr_rx_8700( uint8_t * pmsg )
{
	uint8_t		* psrc;
	uint8_t		buf[100];
	uint8_t		tmpbuf[32];

	uint16_t	seq = JT808HEAD_SEQ( pmsg );
	uint16_t	len = JT808HEAD_LEN( pmsg );
	uint8_t		cmd = *( pmsg + 12 ); /*����ǰ��12�ֽڵ�ͷ*/
	MYTIME		start, end;
	uint16_t	blocks;
	uint32_t	i;
	uint8_t		fcs;

	switch( cmd )
	{
		case 0x00: /*�ɼ���¼��ִ�б�׼�汾*/
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
		case 0x02: /*�г���¼��ʱ��*/
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

			mytime_to_bcd( tmpbuf, mytime_now );            /*ʵʱʱ��*/
			vdr_pack_buf( buf + 9, tmpbuf, 6, &fcs );

			mytime_to_bcd( tmpbuf, jt808_param.id_0xF030 ); /*�״ΰ�װʱ��*/
			vdr_pack_buf( buf + 15, tmpbuf, 6, &fcs );
			i			= jt808_param.id_0xF032 * 10;       /*���ΰ�װ��� 0.1KM BCD�� 00-99999999*/
			tmpbuf[0]	= ( ( ( i / 10000000 ) % 10 ) << 4 ) | ( ( i / 1000000 ) % 10 );
			tmpbuf[1]	= ( ( ( i / 100000 ) % 10 ) << 4 ) | ( ( i / 10000 ) % 10 );
			tmpbuf[2]	= ( ( ( i / 1000 ) % 10 ) << 4 ) | ( ( i / 100 ) % 10 );
			tmpbuf[3]	= ( ( ( i / 10 ) % 10 ) << 4 ) | ( i % 10 );
			vdr_pack_buf( buf + 21, tmpbuf, 4, &fcs );
			i			= jt808_param.id_0xF020 * 10;       /*����� 0.1KM BCD�� 00-99999999*/
			tmpbuf[0]	= ( ( ( i / 10000000 ) % 10 ) << 4 ) | ( ( i / 1000000 ) % 10 );
			tmpbuf[1]	= ( ( ( i / 100000 ) % 10 ) << 4 ) | ( ( i / 10000 ) % 10 );
			tmpbuf[2]	= ( ( ( i / 1000 ) % 10 ) << 4 ) | ( ( i / 100 ) % 10 );
			tmpbuf[3]	= ( ( ( i / 10 ) % 10 ) << 4 ) | ( i % 10 );
			vdr_pack_buf( buf + 25, tmpbuf, 4, &fcs );
			buf[29] = fcs;
			jt808_tx_ack( 0x0700, buf, 30 );
			break;
		case 0x04: /*����ϵ��*/
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
		case 0x05: /*������Ϣ  41byte*/
			buf[0]	= seq >> 8;
			buf[1]	= seq & 0xff;
			buf[2]	= cmd;
			fcs		= 0;
			vdr_pack_buf( buf + 3, "\x55\x7A\x05\x00\x29\x00", 6, &fcs );
			vdr_pack_buf( buf + 9, jt808_param.id_0x0083, 12, &fcs );
			vdr_pack_buf( buf + 21, "��������    ", 12, &fcs );
			buf[33] = fcs;
			jt808_tx_ack( 0x0700, buf, 34 );
			break;
		case 0x06:                              /*״̬�ź�������Ϣ 87byte*/
			buf[0]	= seq >> 8;
			buf[1]	= seq & 0xff;
			buf[2]	= cmd;
			fcs		= 0;
			vdr_pack_buf( buf + 3, "\x55\x7A\x06\x00\x57\x00", 6, &fcs );
			vdr_pack_buf( buf + 9, gps_baseinfo.datetime, 6, &fcs );
			vdr_pack_byte( buf + 15, 1, &fcs ); /*״̬�ź��ֽڸ���*/
			vdr_pack_buf( buf + 16, "�û��Զ���", 10, &fcs );
			vdr_pack_buf( buf + 26, "�û��Զ���", 10, &fcs );
			vdr_pack_buf( buf + 36, "�û��Զ���", 10, &fcs );
			vdr_pack_buf( buf + 46, "����\0\0\0\0\0\0", 10, &fcs );
			vdr_pack_buf( buf + 56, "Զ��\0\0\0\0\0\0", 10, &fcs );
			vdr_pack_buf( buf + 66, "��ת��\0\0\0\0", 10, &fcs );
			vdr_pack_buf( buf + 76, "��ת��\0\0\0\0", 10, &fcs );
			vdr_pack_buf( buf + 86, "�ƶ�\0\0\0\0\0\0", 10, &fcs );
			buf[97] = fcs;
			jt808_tx_ack( 0x0700, buf, 98 );
			break;
		case 8:                                 /*08��¼*/
		case 9:
		case 0x10:
		case 0x11:
		case 0x12:
			if( len == 1 )                      /*��ȡ���м�¼*/
			{
				start	= 0;
				end		= 0xffffffff;
				blocks	= 0;
			}else if( len == 22 )               /*�����ĸ�ʽ <55><7A><cmd><len(2byte)><����><data_block(14byte)><XOR>*/
			{
				psrc	= pmsg + 12 + 1 + 7;    /*12�ֽ�ͷ+1byte������+7�ֽ�vdr����*/
				start	= mytime_from_bcd( psrc );
				end		= mytime_from_bcd( psrc + 6 );
				blocks	= ( *( psrc + 32 ) << 8 ) | ( *( psrc + 33 ) );
			}else
			{
				rt_kprintf( "\n%d>8700�ĸ�ʽ��ʶ��", rt_tick_get( ) );
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
		case 0x82:          /*���ó�����Ϣ*/
			if( len == 1 )  /*��Ϣ��Ϊ��*/
			{
				break;
			}
		case 0x83:          /*���ó��ΰ�װ����*/
			break;
		case 0x84:          /*����״̬������*/
			break;

		case 0xC2:          /*���ü�¼��ʱ��*/
			if( len == 7 )  /*������+ʱ��BCD*/
			{
			}
			break;
		case 0xC3:          /*��������ϵ��*/
			break;
		case 0xC4:          /*���ó�ʼ���*/
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
