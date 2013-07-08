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

#define VDR_DEBUG

//#define TEST_BKPSRAM

typedef uint32_t MYTIME;

/*ת��hex��bcd�ı���*/
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

static struct rt_timer tmr_200ms;

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

#define BYTESWAP2( val )    \
    ( ( ( val & 0xff ) << 8 ) |   \
      ( ( val & 0xff00 ) >> 8 ) )

#define BYTESWAP4( val )    \
    ( ( ( val & 0xff ) << 24 ) |   \
      ( ( val & 0xff00 ) << 8 ) |  \
      ( ( val & 0xff0000 ) >> 8 ) |  \
      ( ( val & 0xff000000 ) >> 24 ) )



uint8_t vdr_signal_status = 0x01; /*�г���¼�ǵ�״̬�ź�*/

/*��ӳ����ź�*/
__IO uint16_t	IC2Value	= 0;
__IO uint16_t	DutyCycle	= 0;
__IO uint32_t	Frequency	= 0;

/*����PA.0 ��Ϊ�ⲿ�������*/
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
/*�ǲ��Ƿ����·?*/
		DutyCycle	= ( IC2Value * 100 ) / TIM_GetCapture1( TIM5 );
		Frequency	= ( RCC_Clocks.HCLK_Frequency ) / 2 / TIM_GetCapture1( TIM5 );
	}else
	{
		DutyCycle	= 0;
		Frequency	= 0;
	}
}

#define SPEED_LIMIT				5   /*�ٶ����� ���ڴ�ֵ��Ϊ������С�ڴ�ֵ��Ϊֹͣ*/
#define SPEED_LIMIT_DURATION	10  /*�ٶ����޳���ʱ��*/

#define SPEED_STATUS_ACC	0x01    /*acc״̬ 0:�� 1:��*/
#define SPEED_STATUS_BRAKE	0x02    /*ɲ��״̬ 0:�� 1:��*/

#define SPEED_JUDGE_ACC		0x04    /*�Ƿ��ж�ACC*/
#define SPEED_JUDGE_BRAKE	0x08    /*�Ƿ��ж�BRAKE ɲ���ź�*/

#define SPEED_USE_PULSE 0x10        /*ʹ�������ź� 0:��ʹ�� 1:ʹ��*/
#define SPEED_USE_GPS	0x20        /*ʹ��gps�ź� 0:��ʹ�� 1:ʹ��*/

enum _stop_run
{
	STOP=0,
	RUN =1,
};

struct _vehicle_status
{
	enum _stop_run	stop_run;       /*��ǰ����״̬ 0:ֹͣ 1:����*/
	uint8_t			logic;          /*��ǰ�߼�״̬*/
	uint8_t			pulse_speed;    /*�ٶ�ֵ*/
	uint32_t		pulse_duration; /*����ʱ��-��*/
	uint8_t			gps_speed;      /*�ٶ�ֵ����ǰ�ٶ�ֵ*/
	uint32_t		gps_duration;   /*����ʱ��-��*/
} car_status =
{
	STOP, ( SPEED_USE_GPS ), 0, 0, 0, 0
};

/*200ms������ٶ�״̬��Ϣ*/
static uint8_t	speed_status[100][2];
static uint8_t	speed_status_index	= 0;
static uint32_t utc_speed_status	= 0; /*������汣���¼��ʱ��*/


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

static uint32_t utc_car_stop	= 0;        /*������ʼͣʻʱ��*/
static uint32_t utc_car_run		= 0;        /*������ʼ��ʻʱ��*/


/*
   ��buf�л�ȡʱ����Ϣ
   �����0xFF ��ɵ�!!!!!!!����Դ�

 */
static MYTIME mytime_from_buf( uint8_t* buf )
{
	uint8_t year, month, day, hour, minute, sec;
	uint8_t *psrc = buf;
	if( *psrc == 0xFF ) /*������Ч������*/
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

/*��ʱ��浽buffer��  4byte=>6byte*/
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

#if 0
VDR_USERDATA vdr_09_userdata;


/*
   ��˳��洢,û����ô����

 */
void vdr_08_init( void )
{
	uint32_t	addr, addr_max;
	MYTIME		curr, old = 0;
	uint8_t		buf[16];

	memset( vdr_08_info, 0xFF, sizeof( vdr_08_info ) ); /*�µļ�¼����ʼ��Ϊ0xFF*/

	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );

	/*�����������*/
	for( addr = VDR_08_START; addr < VDR_08_END; addr += 4096 )
	{
		//	sst25_erase_4k( addr );
	}
	/*����һ�� ÿ���� 32��¼*/

	for( addr = VDR_08_START; addr < VDR_08_END; addr += 128 )                      /*ÿ��sector�ĵ�һ��128 ����ð�Сʱ����Ϣ,��һ����Ч��¼��ʱ��*/
	{
		sst25_read( addr, buf, 4 );
		curr = ( buf[0] << 24 ) | ( buf[1] << 16 ) | ( buf[2] << 8 ) | ( buf[3] );  /*ǰ4���ֽڴ���ʱ��*/
		if( curr != 0xFFFFFFFF )                                                    /*����Ч��¼*/
		{
			if( curr >= old )                                                       /*�ҵ����µļ�¼*/
			{
				old			= curr;
				addr_max	= addr;                                                 /*��ǰ��¼�ĵ�ַ*/
			}
		}
	}

	rt_sem_release( &sem_dataflash );
	if( old )                                                                       /*����ȡʱ�䣬�����Ӱ���װ����ݵ�vdr_08_put*/
	{
		vdr_08_addr = addr_max;                                                     /*��ǰ�м�¼����¼�ĵ�ַ*/
	}
	rt_kprintf( ">vdr08(%08x) %02d-%02d-%02d %02d:%02d:%02d\r\n", vdr_08_addr, YEAR( old ), MONTH( old ), DAY( old ), HOUR( old ), MINUTE( old ), SEC( old ) );
}

/*
   ��ȡ08����
   Լ����������ַ��Χ  ����ʼ����ʱ�䣬�������ݴ�С
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

	puserdata = (VDR_08_USERDATA* )( pnodedata->user_para );    /*ָ���û�������*/
	if( puserdata->record_remain == 0 )                         /*�Ѿ�����������*/
	{
		return 0;
	}
	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );      /*׼����ȡ����*/
	if( pnodedata->multipacket )
	{
		pdata_body = pnodedata->tag_data + 16;
	}else
	{
		pdata_body = pnodedata->tag_data + 12;
	}
	addr	= puserdata->addr;                              /*�ӵ�ǰ��ַ��ʼ�����,ÿ��4��record*/
	pdata	= pdata_body;
	/*��ȡ����*/
	for( count = 0; count < 4; count++ )
	{
		sst25_read( addr, buf + 2, 124 );                   /*�ڴ�����ʱû�д�BCD�����ڣ�����MYTIME�ĸ�ʽ*/
		mytime	= (uint32_t)( buf[2] << 24 ) | (uint32_t)( buf[3] << 16 ) | (uint32_t)( buf[4] << 8 ) | (uint32_t)( buf[5] );
		buf[0]	= HEX2BCD( YEAR( mytime ) );
		buf[1]	= HEX2BCD( MONTH( mytime ) );
		buf[2]	= HEX2BCD( DAY( mytime ) );
		buf[3]	= HEX2BCD( HOUR( mytime ) );
		buf[4]	= HEX2BCD( MINUTE( mytime ) );
		buf[5]	= 0;                                        /*����Ҫ����Ϊ0*/
		if( puserdata->record_total > 4 )                   /*���*/
		{
			memcpy( pdata + 6 + count * 126, buf, 126 );    /*����126����������*/
		}
		read_data_size	+= 126;
		addr			-= 128;                             /*��λ��ǰһ����¼*/
		if( addr < VDR_08_START )
		{
			addr = VDR_08_END - 128;
		}
		puserdata->addr = addr;                             /*ָ���µĵ�ַ*/
		/*�����õ�ַpuserdata->addr_to�ж��Ƿ���ɣ����addr_to=VDR_08_START?,�ü�¼���ж�*/
		puserdata->record_remain--;
		if( puserdata->record_remain == 0 )
		{
			break;
		}
	}
	rt_sem_release( &sem_dataflash );
	/*����VDR��FCS*/
	pdata		= pdata_body;
	pdata[0]	= 0x55;
	pdata[1]	= 0x7A;
	pdata[2]	= 0x08;
	pdata[3]	= ( read_data_size ) >> 8;  /*����֪��ʵ�ʷ��͵Ĵ�С*/
	pdata[4]	= ( read_data_size ) & 0xFF;
	pdata[5]	= 0;                        /*Ԥ��*/

	for( i = 0; i < ( read_data_size + 6 ); i++ )
	{
		fcs ^= *pdata++;
	}
	*pdata = fcs;

	pdata_head = pnodedata->tag_data; /*��ʵ�������ݵĿ�ʼλ��->��Ϣͷ*/
	rt_kprintf( "pdata_head=%p\r\n", pdata_head );

	pdata_head[0]	= pnodedata->head_id >> 8;
	pdata_head[1]	= pnodedata->head_id & 0xFF;

	pnodedata->head_sn++;
	pdata_head[10]	= pnodedata->head_sn >> 8;
	pdata_head[11]	= pnodedata->head_sn & 0xFF;
	memcpy( pdata_head + 4, mobile, 6 );    /*�����ն˺���,��ʱ���ܻ�û��mobileh����*/

	read_data_size += 7;                    /*����VDR������ͷ��У��*/
	node_datalen( pnodedata, read_data_size );

/*���Ʒ���״̬*/
	pnodedata->max_retry	= 1;
	pnodedata->retry		= 0;
	pnodedata->timeout		= RT_TICK_PER_SECOND * 3;
	pnodedata->state		= IDLE;

#if 1                                                               /*dump ����*/
	rt_kprintf( "%d>���� %d bytes\r\n", rt_tick_get( ), pnodedata->msg_len );
//	rt_kprintf( "linkno=%d\r\n", pnodedata->linkno );               /*����ʹ�õ�link,������Э���Զ��socket*/
//	rt_kprintf( "multipacket=%d\r\n", pnodedata->multipacket );     /*�ǲ��Ƕ������*/
//	rt_kprintf( "type=%d\r\n", pnodedata->type );                   /*������Ϣ������*/
//	rt_kprintf( "state=%d\r\n", pnodedata->state );                 /*����״̬*/
//	rt_kprintf( "retry=%d\r\n", pnodedata->retry );                 /*�ش�����,�������ݼ��Ҳ���*/
//	rt_kprintf( "max_retry=%d\r\n", pnodedata->max_retry );         /*����ش�����*/
//	rt_kprintf( "timeout=%d\r\n", pnodedata->timeout );             /*��ʱʱ��*/
//	rt_kprintf( "timeout_tick=%x\r\n", pnodedata->timeout_tick );   /*�ﵽ��ʱ��tickֵ*/
//	rt_kprintf( "head_id=%04x\r\n", pnodedata->head_id );           /*��ϢID*/
//	rt_kprintf( "head_sn=%04x\r\n", pnodedata->head_sn );           /*��Ϣ��ˮ��*/

	rt_kprintf( "puserdata->addr=%08x\r\n", puserdata->addr );
	rt_kprintf( "puserdata->addr_from=%08x\r\n", puserdata->addr_from );
	rt_kprintf( "puserdata->addr_to=%08x\r\n", puserdata->addr_to );
	rt_kprintf( "puserdata->record_total=%d\r\n", puserdata->record_total );
	rt_kprintf( "puserdata->record_remain=%d\r\n", puserdata->record_remain );

	return 1;

#endif
}

/*�յ�Ӧ��Ĵ�����*/
static JT808_MSG_STATE vdr_08_tx_response( JT808_TX_NODEDATA * pnodedata, uint8_t *pmsg )
{
	pnodedata->retry++;
	if( pnodedata->retry >= pnodedata->max_retry )
	{
		return WAIT_DELETE;
	}
	return vdr_08_fill_data( pnodedata );
}

/*��ʱ��Ĵ�����*/
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

/*��λ���ݵĵ�ַ?
   ��ָ���Ľ���ʱ��֮ǰ����ĵ�1���ӵ���ʻ�ٶȼ�¼��ʼ
   Ҫ��Ҫ���ݽ�nodedata?
   ׼��Ҫ���͵����ݣ�Լ������:��ʼ����ʱ�̣��ܵ�block��

 */
void vdr_08_get_ready( MYTIME start, MYTIME end, uint16_t totalrecord )
{
	uint32_t			addr;
	JT808_TX_NODEDATA	*pnodedata;
	VDR_08_USERDATA		*puserdata;
	uint32_t			i;
	uint16_t			rec_count = 0;              /*�ҵ��ļ�¼��*/
	uint8_t				buf[126 * 4];

	uint32_t			addr_from	= 0xFFFFFFFF;   /*��ʼ�ĵ�ַ*/
	uint32_t			addr_to		= 0xFFFFFFFF;   /*�����ĵ�ַ*/

	uint32_t			time_from	= 0xFFFFFFFF;   /*�ϱ���¼ ��ʼ��ʱ�̣������*/
	uint32_t			time_to		= 0xFFFFFFFF;   /*�ϱ���¼ ������ʱ�̣���Զ��*/

	MYTIME				mytime;

/*�ӵ�ǰλ�ÿ�ʼ�������*/
	start	&= 0xFFFFFFC0;                          /*������*/
	end		&= 0xFFFFFFC0;

	rt_kprintf( "%d>��ʼ��������\r\n", rt_tick_get( ) );

	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );
	addr = vdr_08_addr + 128;                       /*�ж������˵�ַ�ݼ�*/
	for( i = 0; i < VDR_08_SECTORS * 32; i++ )      /*�ܹ��ҵļ�¼������ȫʹ�õ�ַ�жϻ᲻����*/
	{
		addr -= 128;
		if( addr < VDR_08_START )                   /*����*/
		{
			addr = VDR_08_END - 128;
		}
		sst25_read( addr, buf, 4 );
		mytime = (uint32_t)( buf[0] << 24 ) | (uint32_t)( buf[1] << 16 ) | (uint32_t)( buf[2] << 8 ) | (uint32_t)( buf[3] );
		if( mytime == 0xFFFFFFFF )
		{
			continue;                                   /*������Ч��¼��������*/
		}
		mytime &= 0xFFFFFFC0;                           /*������,��Ӧ������*/
		if( ( mytime <= end ) && ( mytime >= start ) )  /*����ʱ�����[0..0xffffffff]*/
		{
			rec_count++;                                /*��Ч��¼*/
			if( rec_count >= totalrecord )              /*�յ��㹻�ļ�¼��*/
			{
				break;
			}
			if( time_from == 0xFFFFFFFF )               /*��¼��ʼ�ĵ�ַ,��һ���ҵ��ľ��������*/
			{
				time_from	= mytime;
				addr_from	= addr;
			}
			if( mytime < time_to )                      /*����*/
			{
				time_to = mytime;
				addr_to = addr;
			}
		}
	}

	rt_sem_release( &sem_dataflash );

	rt_kprintf( "%d>��������(%d��) form:%08x to:%08x\r\n", rt_tick_get( ), rec_count, addr_from, addr_to );

	if( rec_count == 0 ) /*û�м�¼��ҲҪ�ϱ�*/
	{
		return;
	}
/*Ĭ�ϰ��������*/
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
	puserdata->packet_total		= ( rec_count + 3 ) / 4; /*ÿ������¼�γ�һ��*/
	puserdata->packet_curr		= 0;

	pnodedata = node_begin( 1, MULTI, 0x0700, 0xF000, 126 * 4 + 7 );
	if( pnodedata == RT_NULL )
	{
		rt_free( puserdata );
		puserdata = RT_NULL;
		rt_kprintf( "�޷�����\r\n" );
	}
	pnodedata->packet_num		= ( rec_count + 3 ) / 4; /*ÿ������¼�γ�һ��*/
	pnodedata->user_para		= puserdata;
	pnodedata->cb_tx_timeout	= vdr_08_tx_timeout;
	pnodedata->cb_tx_response	= vdr_08_tx_response;
	vdr_08_fill_data( pnodedata );
	node_end( pnodedata );
}

FINSH_FUNCTION_EXPORT_ALIAS( vdr_08_get_ready, get_08, get_08_data );


/*
   ��λСʱ��ÿ���ӵ�λ����Ϣ���ٶ�
   ���Ƶ�ʣ�ÿ����һ������
   360Сʱ=15��
   ÿ��4��sector ÿ��sector��Ӧ6Сʱ��ÿСʱ  680�ֽ�
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
		for( count = 0; count < 6; count++ )                                                /*ÿ����6����¼*/
		{
			sst25_read( addr, buf, 4 );
			mytime_curr = ( buf[0] << 24 ) | ( buf[1] << 16 ) | ( buf[2] << 8 ) | buf[3];   /*ǰ4���ֽڴ���ʱ��*/
			if( mytime_curr != 0xFFFFFFFF )                                                 /*����Ч��¼*/
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
   ÿ������6��Сʱ������
   һ��Сʱ666byte ռ��680byte
 */
void vdr_09_put( MYTIME datetime )
{
	uint8_t		buf[16];
	uint32_t	i;
	uint32_t	addr;
	uint8_t		minute;

	if( ( vdr_09_time & 0xFFFFFFC0 ) == ( datetime & 0xFFFFFFC0 ) ) /*��ͬһ������*/
	{
		return;
	}

	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );
	if( ( vdr_09_time & 0xFFFFF000 ) != ( datetime & 0xFFFFF000 ) ) /*�������ڵ�ǰ��Сʱ��,�¼�¼*/
	{
		vdr_09_index++;                                             /*Ҫд���µ�Сʱ��¼ÿ680�ֽ�*/
		if( vdr_09_index > 5 )                                      /*ÿ��sector��6��Сʱ 0-5,Խ��һ������*/
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

		addr = VDR_09_START + vdr_09_sect * 4096 + vdr_09_index * 680;  /*ÿ����680�ֽ�*/
		PACK_INT( buf, ( datetime & 0xFFFFF000 ) );                     /*д���µļ�¼ͷ*/
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
	addr	= VDR_09_START + vdr_09_sect * 4096 + vdr_09_index * 680 + minute * 11 + 10; /*����ָ����λ��,��ʼ��10�ֽ�ͷ*/
	sst25_write_through( addr, buf, 11 );
	rt_sem_release( &sem_dataflash );

	rt_kprintf( "%d>save 09 data addr=%08x\r\n", rt_tick_get( ), addr );
}

/*��ȡ����*/
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

	puserdata = (VDR_USERDATA* )( pnodedata->user_para );   /*ָ���û�������*/
	if( puserdata->record_remain == 0 )                     /*�Ѿ�����������*/
	{
		return 0;
	}
	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );  /*׼����ȡ����*/

	if( pnodedata->multipacket )
	{
		pdata_body = pnodedata->tag_data + 16;
	}else
	{
		pdata_body = pnodedata->tag_data + 12;
	}
	addr	= puserdata->addr;                                                      /*�ӵ�ǰ��ַ��ʼ�����,ÿ��4��record*/
	pdata	= pdata_body;
	/*��ȡ����*/

	addr = VDR_09_START + puserdata->sector * 4096 + puserdata->index * 680 + 4;    /*��ʼ4�ֽ�Ϊ�Լ���ʱ��*/
	sst25_read( addr, pdata + 6, 666 );                                             /*��ʼ6���ֽڵ�55 7A��������*/
	rt_sem_release( &sem_dataflash );

	/*����һ������,��Ч��λ����Ϣ��д0x7FFFFFFF*/
	pdata = pdata_body + 6 + 6;                                                     /*������ʼ��6�ֽ�55 7A��BCDʱ��ͷ*/
	for( i = 0; i < 660; i += 11 )
	{
		if( pdata[i] == 0xff )
		{
			pdata[i]		= 0x7F;
			pdata[i + 4]	= 0x7F;
		}
	}

	/*ָ����һ��Ҫ����sect,index*/
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

	/*����VDR��FCS*/
	pdata		= pdata_body;
	pdata[0]	= 0x55;
	pdata[1]	= 0x7A;
	pdata[2]	= 0x08;
	pdata[3]	= ( 666 ) >> 8; /*����֪��ʵ�ʷ��͵Ĵ�С*/
	pdata[4]	= ( 666 ) & 0xFF;
	pdata[5]	= 0;            /*Ԥ��*/

	for( i = 0; i < ( 666 + 6 ); i++ )
	{
		fcs ^= *pdata++;
	}
	*pdata = fcs;

	pdata_head = pnodedata->tag_data; /*��ʵ�������ݵĿ�ʼλ��->��Ϣͷ*/
	rt_kprintf( "pdata_head=%p\r\n", pdata_head );

	pdata_head[0]	= pnodedata->head_id >> 8;
	pdata_head[1]	= pnodedata->head_id & 0xFF;

	pnodedata->head_sn++;
	pdata_head[10]	= pnodedata->head_sn >> 8;
	pdata_head[11]	= pnodedata->head_sn & 0xFF;
	memcpy( pdata_head + 4, mobile, 6 ); /*�����ն˺���,��ʱ���ܻ�û��mobileh����*/

	node_datalen( pnodedata, 666 + 7 );

/*���Ʒ���״̬*/
	pnodedata->max_retry	= 1;
	pnodedata->retry		= 0;
	pnodedata->timeout		= RT_TICK_PER_SECOND * 3;
	pnodedata->state		= IDLE;

	/*dump ����*/
	rt_kprintf( "%d>���� %d bytes\r\n", rt_tick_get( ), pnodedata->msg_len );
	rt_kprintf( "puserdata->addr=%08x\r\n", addr );
	rt_kprintf( "puserdata->record_total=%d\r\n", puserdata->record_total );
	rt_kprintf( "puserdata->record_remain=%d\r\n", puserdata->record_remain );

	return 1;
}

/*�յ�Ӧ��Ĵ�����*/
static JT808_MSG_STATE vdr_09_tx_response( JT808_TX_NODEDATA * nodedata, uint8_t *pmsg )
{
	vdr_09_fill_data( nodedata );
}

/*��ʱ��Ĵ�����*/
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

/*����09���͵����ݰ�*/
void vdr_09_get_ready( MYTIME start, MYTIME end, uint16_t totalrecord )
{
	uint32_t			addr;
	JT808_TX_NODEDATA	*pnodedata;
	VDR_USERDATA		*puserdata;
	uint32_t			i;
	uint16_t			rec_count = 0; /*�ҵ��ļ�¼��*/
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

	/*�ӵ�ǰλ�ÿ�ʼ�������*/
	start	&= 0xFFFFF000;                                  /*���Է���*/
	end		&= 0xFFFFF000;

	rt_kprintf( "%d>��ʼ��������\r\n", rt_tick_get( ) );

	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );

	sector	= vdr_09_sect;                                  /*�ӵ�ǰλ�ÿ�ʼ����*/
	index	= 5;
	for( i = 0; i < VDR_09_SECTORS; i++ )                   /*����һ������*/
	{
		addr = VDR_09_START + sector * 4096 + index * 680;  /*�ҵ�ÿ��Сʱͷ*/
		sst25_read( addr, buf, 4 );
		mytime = ( buf[0] << 24 ) | ( buf[1] << 16 ) | ( buf[2] << 8 ) | buf[3];
		if( ( mytime >= start ) && ( mytime <= end ) )      /*����Чʱ�����*/
		{
			rec_count++;
			if( rec_count >= totalrecord )                  /*�յ��㹻�ļ�¼��*/
			{
				break;
			}
			if( time_from == 0xFFFFFFFF )                   /*��һ���ҵ��ľ��������*/
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
			index = 6; /*�������˵ݼ�*/
			if( sector == 0 )
			{
				sector = VDR_09_SECTORS;
			}
			sector--;
		}
		index--;
	}
	rt_sem_release( &sem_dataflash );

	rt_kprintf( "%d>09��������(%d��) FROM %d:%d TO %08x\r\n", rt_tick_get( ), rec_count, sector_from, index_from, sector_to, index_to );

	if( rec_count == 0 ) /*û�м�¼��ҲҪ�ϱ�*/
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

	puserdata->packet_total = ( rec_count + 3 ) / 4; /*ÿ������¼�γ�һ��*/
	puserdata->packet_curr	= 0;

	pnodedata = node_begin( 1, MULTI, 0x0700, 0xF000, 666 + 7 );
	if( pnodedata == RT_NULL )
	{
		rt_free( puserdata );
		puserdata = RT_NULL;
		rt_kprintf( "�޷�����\r\n" );
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
   �¹��ɵ��¼
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
		for( count = 0; count < 16; count++ )                                               /*ÿ����6����¼*/
		{
			sst25_read( addr, buf, 4 );
			mytime_curr = ( buf[0] << 24 ) | ( buf[1] << 16 ) | ( buf[2] << 8 ) | buf[3];   /*ǰ4���ֽڴ���ʱ��*/
			if( mytime_curr != 0xFFFFFFFF )                                                 /*����Ч��¼*/
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
   ��������뵥�δ����ģ����ǳ���ͣʻ������
 */
void vdr_10_put( MYTIME datetime )
{
	uint32_t	i, j;
	uint32_t	addr;
	uint8_t		minute;
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
	buf[1]	= datetime >> 24;
	buf[2]	= datetime >> 24;
	buf[3]	= datetime >> 24;
	buf[4]	= HEX2BCD( YEAR( datetime ) );
	buf[5]	= HEX2BCD( MONTH( datetime ) );
	buf[6]	= HEX2BCD( DAY( datetime ) );
	buf[7]	= HEX2BCD( HOUR( datetime ) );
	buf[8]	= HEX2BCD( MINUTE( datetime ) );
	buf[9]	= HEX2BCD( SEC( datetime ) );
	memcpy( buf + 10, "12010419800101234", 18 );    /*��ʻ֤����*/
	PACK_INT( buf + 228, ( gps_longi * 6 ) );
	PACK_INT( buf + 232, ( gps_lati * 6 ) );
	PACK_WORD( buf + 236, ( gps_alti ) );

	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );
	vdr_10_index++;                                 /*Ҫд���µ�Сʱ��¼ÿ680�ֽ�*/
	if( vdr_10_index > 5 )                          /*ÿ��sector��6��Сʱ 0-5,Խ��һ������*/
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

	addr = VDR_10_START + vdr_10_sect * 4096 + vdr_10_index * 256; /*ÿ����680�ֽ�*/
	sst25_write_through( addr, buf, 238 );

	rt_sem_release( &sem_dataflash );
	rt_kprintf( "%d>save 10 data addr=%08x\r\n", rt_tick_get( ), addr );
}

/*��ȡ����*/
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

	puserdata = (VDR_USERDATA* )( pnodedata->user_para );   /*ָ���û�������*/
	if( puserdata->record_remain == 0 )                     /*�Ѿ�����������*/
	{
		return 0;
	}
	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );  /*׼����ȡ����*/

	if( pnodedata->multipacket )
	{
		pdata_body = pnodedata->tag_data + 16;
	}else
	{
		pdata_body = pnodedata->tag_data + 12;
	}
	addr	= puserdata->addr;                                                      /*�ӵ�ǰ��ַ��ʼ�����,ÿ��4��record*/
	pdata	= pdata_body;
	/*��ȡ����*/

	addr = VDR_09_START + puserdata->sector * 4096 + puserdata->index * 680 + 4;    /*��ʼ4�ֽ�Ϊ�Լ���ʱ��*/
	sst25_read( addr, pdata + 6, 666 );                                             /*��ʼ6���ֽڵ�55 7A��������*/
	rt_sem_release( &sem_dataflash );

	/*����һ������,��Ч��λ����Ϣ��д0x7FFFFFFF*/
	pdata = pdata_body + 6 + 6;                                                     /*������ʼ��6�ֽ�55 7A��BCDʱ��ͷ*/
	for( i = 0; i < 660; i += 11 )
	{
		if( pdata[i] == 0xff )
		{
			pdata[i]		= 0x7F;
			pdata[i + 4]	= 0x7F;
		}
	}

	/*ָ����һ��Ҫ����sect,index*/
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

	/*����VDR��FCS*/
	pdata		= pdata_body;
	pdata[0]	= 0x55;
	pdata[1]	= 0x7A;
	pdata[2]	= 0x08;
	pdata[3]	= ( 666 ) >> 8; /*����֪��ʵ�ʷ��͵Ĵ�С*/
	pdata[4]	= ( 666 ) & 0xFF;
	pdata[5]	= 0;            /*Ԥ��*/

	for( i = 0; i < ( 666 + 6 ); i++ )
	{
		fcs ^= *pdata++;
	}
	*pdata = fcs;

	pdata_head = pnodedata->tag_data; /*��ʵ�������ݵĿ�ʼλ��->��Ϣͷ*/
	rt_kprintf( "pdata_head=%p\r\n", pdata_head );

	pdata_head[0]	= pnodedata->head_id >> 8;
	pdata_head[1]	= pnodedata->head_id & 0xFF;

	pnodedata->head_sn++;
	pdata_head[10]	= pnodedata->head_sn >> 8;
	pdata_head[11]	= pnodedata->head_sn & 0xFF;
	memcpy( pdata_head + 4, mobile, 6 ); /*�����ն˺���,��ʱ���ܻ�û��mobileh����*/

	node_datalen( pnodedata, 666 + 7 );

/*���Ʒ���״̬*/
	pnodedata->max_retry	= 1;
	pnodedata->retry		= 0;
	pnodedata->timeout		= RT_TICK_PER_SECOND * 3;
	pnodedata->state		= IDLE;

	/*dump ����*/
	rt_kprintf( "%d>���� %d bytes\r\n", rt_tick_get( ), pnodedata->msg_len );
	rt_kprintf( "puserdata->addr=%08x\r\n", addr );
	rt_kprintf( "puserdata->record_total=%d\r\n", puserdata->record_total );
	rt_kprintf( "puserdata->record_remain=%d\r\n", puserdata->record_remain );

	return 1;
}

/*�յ�Ӧ��Ĵ�����*/
static void vdr_10_tx_response( JT808_TX_NODEDATA * nodedata, uint8_t *pmsg )
{
	vdr_10_fill_data( nodedata );
}

/*��ʱ��Ĵ�����*/
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

/*����10���͵����ݰ�*/
void vdr_10_get_ready( MYTIME start, MYTIME end, uint16_t totalrecord )
{
	uint32_t			addr;
	JT808_TX_NODEDATA	*pnodedata;
	VDR_USERDATA		*puserdata;
	uint32_t			i;
	uint16_t			rec_count = 0; /*�ҵ��ļ�¼��*/
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

	/*�ӵ�ǰλ�ÿ�ʼ�������*/
	start	&= 0xFFFFF000;                                  /*���Է���*/
	end		&= 0xFFFFF000;

	rt_kprintf( "%d>��ʼ��������\r\n", rt_tick_get( ) );

	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );

	sector	= vdr_09_sect;                                  /*�ӵ�ǰλ�ÿ�ʼ����*/
	index	= 5;
	for( i = 0; i < VDR_09_SECTORS; i++ )                   /*����һ������*/
	{
		addr = VDR_09_START + sector * 4096 + index * 680;  /*�ҵ�ÿ��Сʱͷ*/
		sst25_read( addr, buf, 4 );
		mytime = ( buf[0] << 24 ) | ( buf[1] << 16 ) | ( buf[2] << 8 ) | buf[3];
		if( ( mytime >= start ) && ( mytime <= end ) )      /*����Чʱ�����*/
		{
			rec_count++;
			if( rec_count >= totalrecord )                  /*�յ��㹻�ļ�¼��*/
			{
				break;
			}
			if( time_from == 0xFFFFFFFF )                   /*��һ���ҵ��ľ��������*/
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
			index = 6; /*�������˵ݼ�*/
			if( sector == 0 )
			{
				sector = VDR_09_SECTORS;
			}
			sector--;
		}
		index--;
	}
	rt_sem_release( &sem_dataflash );

	rt_kprintf( "%d>09��������(%d��) FROM %d:%d TO %08x\r\n", rt_tick_get( ), rec_count, sector_from, index_from, sector_to, index_to );

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

	puserdata->packet_total = ( rec_count + 3 ) / 4; /*ÿ������¼�γ�һ��*/
	puserdata->packet_curr	= 0;

	pnodedata = node_begin( 1, MULTI, 0x0700, 0xF000, 666 + 7 );
	if( pnodedata == RT_NULL )
	{
		rt_free( puserdata );
		puserdata = RT_NULL;
		rt_kprintf( "�޷�����\r\n" );
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
   ��ȡid��һ�����õĵ�ַ
   ��û�����������������д֮ǰ����

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
			rt_kprintf( ">08 sect=%d index=%d\r\n", sect_info[0].sector, sect_info[0].index );

			rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );
			if( ( addr & 0xFFF ) == 0 ) /*��4k�߽紦*/
			{
				sst25_erase_4k( addr );
			}
			sst25_write_through( addr, vdr_08_info, sizeof( vdr_08_info ) );
			rt_sem_release( &sem_dataflash );
			rt_kprintf( "%d>д��08 ��ַ:%08x ֵ:%08x\r\n", rt_tick_get( ), addr, vdr_08_time );

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
	uint32_t	i;
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

	rt_kprintf( "%d>save 09 data addr=%08x\r\n", rt_tick_get( ), addr );
}

/*
   234 byte
   ��������뵥�δ����ģ����ǳ���ͣʻ������
 */
void vdr_10_put( MYTIME datetime )
{
	uint32_t	i, j;
	uint32_t	addr;
	uint8_t		minute;
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
	memcpy( buf + 10, "120104198001012345", 18 );   /*��ʻ֤����*/
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
	rt_kprintf( "%d>save 10 data addr=%08x\r\n", rt_tick_get( ), addr );
}

/*
   �յ�gps���ݵĴ����ж�λ��δ��λ
   �洢λ����Ϣ��
   �ٶ��жϣ�У׼
 */

rt_err_t vdr_rx_gps( void )
{
	uint32_t	datetime;
	uint8_t		year, month, day, hour, minute, sec;
	uint32_t	i, j;
	uint8_t		buf[128];
	uint8_t		*pbkpsram;

#ifdef TEST_BKPSRAM
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

	if( ( jt808_status & BIT_STATUS_GPS ) == 0 ) /*δ��λ*/
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

	/*��������,ͣ��ǰ20ms����*/
	if( utc_now - utc_speed_status > 20 )                               /*����20���û�����ݵļ��*/
	{
		speed_status_index = 0;                                         /*���¼�¼*/
	}
	speed_status[speed_status_index][0] = gps_speed;                    /*��ǰλ��д���ٶ�,200ms�����в���*/
	utc_speed_status					= utc_now;

/*�жϳ�����ʻ״̬*/
	if( car_status.stop_run == STOP )                                   /*��Ϊ����ֹͣ,�ж�����*/
	{
		if( utc_car_stop == 0 )                                         /*ͣ��ʱ����δ��ʼ����Ĭ��ͣʻ*/
		{
			utc_car_stop = utc_now;
		}
		if( gps_speed >= SPEED_LIMIT )                                  /*�ٶȴ�������ֵ*/
		{
			if( utc_car_run == 0 )
			{
				utc_car_run = utc_now;                                  /*��¼��ʼʱ��*/
			}

			if( ( utc_now - utc_car_run ) >= SPEED_LIMIT_DURATION )     /*�����˳���ʱ��*/
			{
				car_status.stop_run = RUN;                              /*��Ϊ������ʻ*/
				rt_kprintf( "%d>������ʻ\r\n", rt_tick_get( ) );
				utc_car_stop = 0;                                       /*�ȴ��ж�ͣʻ*/
			}
		}else
		{
			if( utc_now - utc_car_stop > jt808_param.id_0x005A )        /*�ж�ͣ���ʱ��*/
			{
				//rt_kprintf( "�ﵽͣ���ʱ��\r\n" );
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
				car_status.stop_run = STOP;                             /*��Ϊ����ͣʻ*/
				rt_kprintf( "%d>����ͣʻ\r\n", rt_tick_get( ) );
				utc_car_run = 0;
				vdr_10_put( datetime );                                 /*����VDR_10����,�¹��ɵ�*/
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

/*11���ݳ�ʱ��ʻ��¼*/
}

/*
   ��ʼ��08 09 10 11 12�ļ�¼
   ���� ��¼��ʱ��
     0:û�м�¼
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
	rt_kprintf( "%d>vdr%02d time=%08x sect=%d index=%d\r\n", rt_tick_get( ), vdr_id, mytime_ret, sect_info[id].sector, sect_info[id].index );
	return mytime_ret;
}

FINSH_FUNCTION_EXPORT_ALIAS( vdr_08_12_init, vdr_format, format vdr record );

/*��ȡ����*/
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

	puserdata = (VDR_USERDATA* )( pnodedata->user_para );   /*ָ���û�������*/

	id = puserdata->id - 8;

	if( puserdata->record_remain == 0 )                     /*�Ѿ�����������*/
	{
		return 0;
	}

	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );  /*׼����ȡ����*/

	if( pnodedata->multipacket )
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

	if( id == 1 )                   /*����һ��09����,��Ч��λ����Ϣ��д0x7FFFFFFF*/
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
	rt_kprintf( "pdata_head=%p\r\n", pdata_head );

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
	rt_kprintf( "%d>���� %d bytes\r\n", rt_tick_get( ), pnodedata->msg_len );
	rt_kprintf( "puserdata->addr=%08x\r\n", addr );
	rt_kprintf( "puserdata->record_total=%d\r\n", puserdata->record_total );
	rt_kprintf( "puserdata->record_remain=%d\r\n", puserdata->record_remain );

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

 */
void vdr_08_12_get_ready( uint8_t vdr_id, MYTIME start, MYTIME end, uint16_t totalrecord )
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
	rt_kprintf( "%d>��ʼ��������\r\n", rt_tick_get( ) );

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

	rt_kprintf( "%d>vdr%02d��������(%d��) FROM %d:%d TO %d:%d\r\n",
	            rt_tick_get( ),
	            vdr_id,
	            rec_count,
	            sector_from, index_from,
	            sector_to, index_to );

	if( rec_count == 0 ) /*û���ҵ���¼��ҲҪ�ϱ�*/
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

	/*����Ҫ���͵������������͵�������ֽ���*/
	i = sect_info[id].record_per_packet * sect_info[id].data_size + 7;  /*��7 ����Ϊ��vdr��ͷ*/

	if( rec_count * sect_info[id].data_size > 700 )                     /*�������,*/
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
		rt_kprintf( "�޷�����\r\n" );
	}
	pnodedata->packet_num		= ( rec_count + sect_info[id].record_per_packet - 1 ) / sect_info[id].record_per_packet;
	pnodedata->packet_no		= 0;
	pnodedata->user_para		= puserdata;
	pnodedata->cb_tx_timeout	= vdr_08_12_tx_timeout;
	pnodedata->cb_tx_response	= vdr_08_12_tx_response;
	rt_kprintf( "�������\r\n" );
	vdr_08_12_fill_data( pnodedata );
	node_end( pnodedata );
}

FINSH_FUNCTION_EXPORT_ALIAS( vdr_08_12_get_ready, vdr_get, get_vdr_data );


/*
   ��ʼ����¼������
   ��Ϊ�����ڹ̶�ʱ��δ洢��
   ��Ҫ��¼��ʼʱ�̵�sectorλ��(��Ե�sectorƫ��)
 */
rt_err_t vdr_init( void )
{
	uint8_t* pbuf;

	pulse_init( ); /*���������*/

	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 5 );

	vdr_08_time = vdr_08_12_init( 8, 0);
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
	uint8_t		buf[500];

	uint16_t	seq = JT808HEAD_SEQ( pmsg );
	uint16_t	len = JT808HEAD_LEN( pmsg );
	uint8_t		cmd = *( pmsg + 12 );   /*����ǰ��12�ֽڵ�ͷ*/
	MYTIME		start, end;
	uint16_t	blocks;

	switch( cmd )
	{
		case 0:                         /*�ɼ���¼��ִ�б�׼�汾*/
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
		case 2: /*�г���¼��ʱ��*/
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
				start	= mytime_from_buf( psrc );
				end		= mytime_from_buf( psrc + 6 );
				blocks	= ( *( psrc + 32 ) << 8 ) | ( *( psrc + 33 ) );
			}else
			{
				rt_kprintf( "%d>8700�ĸ�ʽ��ʶ��\r\n", rt_tick_get( ) );
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
