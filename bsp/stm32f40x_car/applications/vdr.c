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

#define VDR_DEBUG

//#define TEST_BKPSRAM

typedef uint32_t MYTIME;

/*ת��hex��bcd�ı���*/
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
#define VDR_08_SECTORS	100 /*ÿСʱ2��sector,����50Сʱ*/
#define VDR_08_END		( VDR_08_START + VDR_08_SECTORS * 4096 )

#define VDR_09_START	( VDR_08_START + VDR_08_SECTORS * 4096 )
#define VDR_09_SECTORS	64  /*16�죬ÿ��4sector*/
#define VDR_09_END		( VDR_09_START + VDR_09_SECTORS * 4096 )

#define VDR_10_START	( VDR_09_START + VDR_09_SECTORS * 4096 )
#define VDR_10_SECTORS	8   /*100���¹��ɵ� 100*234  ʵ�� 128*256 */
#define VDR_10_END		( VDR_10_START + VDR_10_SECTORS * 4096 )

#define VDR_11_START	( VDR_10_START + VDR_10_SECTORS * 4096 )
#define VDR_11_SECTORS	3   /*100����ʱ��ʻ��¼ 100*50 ʵ�� 128*64,����һ��������ɾ��ʱ��������*/
#define VDR_11_END		( VDR_11_START + VDR_11_SECTORS * 4096 )

#define VDR_12_START	( VDR_11_START + VDR_11_SECTORS * 4096 )
#define VDR_12_SECTORS	3   /*200����ʻ����ݼ�¼ 200*25 ʵ��200*32 */
#define VDR_12_END		( VDR_12_START + VDR_12_SECTORS * 4096 )

#define VDR_13_START	( VDR_12_START + VDR_12_SECTORS * 4096 )
#define VDR_13_SECTORS	2   /*100�� �ⲿ�����¼100*7 ʵ�� 100*8*/
#define VDR_13_END		( VDR_13_START + VDR_13_SECTORS * 4096 )

#define VDR_14_START	( VDR_13_START + VDR_13_SECTORS * 4096 )
#define VDR_14_SECTORS	2   /*100�� �����޸ļ�¼ 100*7 ʵ��100*8*/
#define VDR_14_END		( VDR_14_START + VDR_14_SECTORS * 4096 )

#define VDR_15_START	( VDR_14_START + VDR_14_SECTORS * 4096 )
#define VDR_15_SECTORS	2   /*10���ٶ�״̬��־ 10*133 ʵ�� 10*256*/
#define VDR_15_END		( VDR_15_START + VDR_15_SECTORS * 4096 )

static struct rt_timer tmr_200ms;

struct _sect_info
{
	uint8_t		flag;       /*��־*/
	uint32_t	start_addr; /*��ʼ�ĵ�ַ*/
	uint32_t	addr;       /*���һ����Ч��¼�ĵ�ַ*/
	uint16_t	rec_size;   /*��¼��С*/
	uint8_t		sectors;    /*��¼��������*/
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

	uint16_t	blocks;         /*����ÿ���ϴ����ٸ����ݿ�*/
	uint16_t	blocks_remain;  /*��ǰ��֯�ϴ����ǻ���Ҫ�ĵ�blocks*/
}VDR_CMD;

VDR_CMD vdr_cmd;


/*����д���ļ�����Ϣ
   0...3  д��SerialFlash�ĵ�ַ
   4...31 �ļ���
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

#define SPEED_LIMIT				10          /*�ٶ����� ���ڴ�ֵ��Ϊ������С�ڴ�ֵ��Ϊֹͣ*/
#define SPEED_LIMIT_DURATION	10          /*�ٶ����޳���ʱ��*/

#define SPEED_STATUS_ACC	0x01            /*acc״̬ 0:�� 1:��*/
#define SPEED_STATUS_BRAKE	0x02            /*ɲ��״̬ 0:�� 1:��*/

#define SPEED_JUDGE_ACC		0x04            /*�Ƿ��ж�ACC*/
#define SPEED_JUDGE_BRAKE	0x08            /*�Ƿ��ж�BRAKE ɲ���ź�*/

#define SPEED_USE_PULSE 0x10                /*ʹ�������ź� 0:��ʹ�� 1:ʹ��*/
#define SPEED_USE_GPS	0x20                /*ʹ��gps�ź� 0:��ʹ�� 1:ʹ��*/

struct _vehicle_status
{
	uint8_t status;                         /*��ǰ����״̬ 0:ֹͣ 1:����*/
	uint8_t logic;                          /*��ǰ�߼�״̬*/

	uint8_t		pulse_speed_judge_duration; /*�ٶ����޳���ʱ��*/
	uint8_t		pulse_speed;                /*�ٶ�ֵ*/
	uint32_t	pulse_duration;             /*����ʱ��-��*/

	uint8_t		gps_judge_duration;         /*�ٶ����޳���ʱ��*/
	uint8_t		gps_speed;                  /*�ٶ�ֵ����ǰ�ٶ�ֵ*/
	uint32_t	gps_duration;               /*����ʱ��-��*/
} car_status =
{
	0, ( SPEED_USE_GPS ), 0, 0, 0, 0, 0, 0
};


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
static void cb_tmr_200ms( void* parameter )
{
/*�жϼ�ʻ״̬*/
}

#if 0


/*
   ��ʼ����¼����
   �ҵ����һ����¼��λ��
 */
static uint32_t vdr_init_byid( uint8_t id, uint8_t *p )
{
	uint16_t	sect, offset;
	uint8_t		*prec;
	uint32_t	mytime_curr = 0;
	uint32_t	mytime_vdr	= 0;
	uint32_t	addr;
	uint8_t		flag; /*��־*/
	uint16_t	rec_size;

	uint8_t		i = id - 8;

	addr		= sect_info[i].start_addr;
	flag		= sect_info[i].flag;
	rec_size	= sect_info[i].rec_size;

	for( sect = 0; sect < sect_info[i].sectors; sect++ )
	{
		sst25_read( sect_info[i].start_addr + sect * 4096, p, 4096 );                               /*һ�ζ���4096�ֽ�*/
		for( offset = 0; offset < 4096; offset += rec_size )                                        /*���ռ�¼��С����*/
		{
			prec = p + offset;
			if( prec[0] == flag )                                                                   /*ÿ����¼ͷ���� <flag><mydatetime(4byte)>  ��Ч����*/                                                                   /*����Ч�����ݰ�*/
			{
				/*ע��洢��˳�򣬲��ܼ򵥵�BYTESWAP4*/
				mytime_curr = ( prec[4] << 24 ) | ( prec[3] << 16 ) | ( prec[2] << 8 ) | prec[1];   /*������ʱ��*/
				if( mytime_curr > mytime_vdr )
				{
					mytime_vdr	= mytime_curr;
					addr		= sect_info[i].start_addr + sect * 4096 + offset;
				}
			}else if( prec[0] != 0xFF )                                                             /*����ļ�¼ͷ*/
			{
				rt_kprintf( "%d>vdr_init err i=%d,addr=%08x\r\n", i, prec );
			}
		}
	}
	sect_info[i].addr = addr;
	rt_kprintf( "\r\n%d>sect:%02d addr=%08x datetime:%02d-%02d-%02d %02d:%02d:%02d", rt_tick_get( ), id, addr, YEAR( mytime_vdr ), MONTH( mytime_vdr ), DAY( mytime_vdr ), HOUR( mytime_vdr ), MINUTE( mytime_vdr ), SEC( mytime_vdr ) );
}

#endif

static uint32_t vdr_08_addr = VDR_08_END;   /*��ǰҪд��ĵ�ַ*/
static MYTIME	vdr_08_time = 0xFFFFFFFF;   /*��ǰʱ���*/
static uint8_t	vdr_08_info[126];           /*����Ҫд�����Ϣ*/
static uint8_t	vdr_08_sect_index = 0;      /*��һ��sect�ڵļ�¼����,ÿ��sector��32��*/

static MYTIME	vdr_09_time			= 0xFFFFFFFF;
static uint16_t vdr_09_sect			= 0;    /*��ǰҪд���sect��*/
static uint8_t	vdr_09_sect_index	= 0;    /*��һ��sect�ڵļ�¼����,ÿ��sector��6��*/

/*��buf�л�ȡʱ����Ϣ*/
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

/*��ʱ��浽buffer��  4byte=>6byte*/
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
	MYTIME		start;          /*��ʼʱ��*/
	MYTIME		end;            /*����ʱ��*/
	uint16_t	totalrecord;    /*�ܵü�¼��*/
	uint32_t	addr_from;      /*��ǰ���ĵ�ַ*/
	uint32_t	addr_to;        /*��ǰ���ĵ�ַ*/
	uint8_t		rec_per_packet; /*ÿ���ļ�¼��*/
	uint16_t	packet_total;   /*�ܰ���*/
	uint16_t	packet_curr;    /*��ǰ����*/
}VDR_08_USERDATA;

VDR_08_USERDATA vdr_08_userdata;


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
	/*����һ��*/
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

/*�������� ��1��ļ�����������ܷ���Ĺ���**/
void vdr_08_put( MYTIME datetime, uint8_t speed, uint8_t status )
{
	uint32_t i, sec;

	if( ( vdr_08_time & 0xFFFFFFC0 ) != ( datetime & 0xFFFFFFC0 ) ) /*�����ڵ�ǰ��һ������*/
	{
		if( vdr_08_time != 0xFFFFFFFF )                             /*����Ч������,Ҫ���棬ע���һ�����ݵĴ���*/
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

			vdr_08_addr += 128;                 /*����һ���¼�¼*/
			if( vdr_08_addr >= VDR_08_END )     /*�ж��Ƿ񻷻�*/
			{
				vdr_08_addr = VDR_08_START;
			}
			rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );
			if( ( vdr_08_addr & 0xFFF ) == 0 )  /*��4k�߽紦*/
			{
				sst25_erase_4k( vdr_08_addr );
			}
			sst25_write_through( vdr_08_addr, vdr_08_info, sizeof( vdr_08_info ) );
			rt_sem_release( &sem_dataflash );
			rt_kprintf( "%d>д��08 ��ַ:%08x ֵ:%08x\r\n", rt_tick_get( ), vdr_08_addr, vdr_08_time );

#endif
			memset( vdr_08_info, 0xFF, sizeof( vdr_08_info ) ); /*�µļ�¼����ʼ��Ϊ0xFF*/
		}
	}
	sec							= SEC( datetime );
	vdr_08_time					= datetime;
	vdr_08_info[sec * 2 + 4]	= gps_speed;
	vdr_08_info[sec * 2 + 5]	= vdr_signal_status;
}

/*
   ��ȡ08����
   Լ����������ַ��Χ  ����ʼ����ʱ�䣬�������ݴ�С
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

/*�ӵ�ǰ��ַ��ʼ�����,ÿ��4��record*/
	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );
	addr = puserdata->addr_from;
	for( i = 0; i < 4; i++ )
	{
		sst25_read( addr, buf + 2, 124 ); /*�ڴ�����ʱû�д�BCD�����ڣ�����MYTIME�ĸ�ʽ*/
	}

	rt_sem_release( &sem_dataflash );
}

/*�յ�Ӧ��Ĵ�����*/
static void jt808_tx_response( JT808_TX_NODEDATA * nodedata, uint8_t *pmsg )
{
}

/*��ʱ��Ĵ�����*/
static void jt808_tx_timeout( JT808_TX_NODEDATA * nodedata )
{
}

/*��λ���ݵĵ�ַ�
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
	uint8_t				buf[4];

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

	rt_kprintf( "%d>�������� form:%08x@%08x to:%08x@%08x\r\n", rt_tick_get( ), time_from, addr_from, time_to, addr_to );

/*�����õ������Ƕ������*/
	pnodedata = node_begin( 126 * 4 + 4 + sizeof( VDR_08_USERDATA ) );                  /*126�ֽ�һ����¼(4����¼),4�ֽ���Ϣ�����*/
	if( pnodedata == RT_NULL )                                                          /*����ʧ��*/
	{
		rt_kprintf( "\d>vdr����08��������\r\n", rt_tick_get( ) );
		return;
	}

	puserdata				= (VDR_08_USERDATA*)( pnodedata->tag_data + 126 * 4 + 4 );  /*ָ���û�����*/
	puserdata->totalrecord	= rec_count;
	puserdata->addr_from	= addr_from;
	puserdata->addr_to		= addr_to;
	puserdata->packet_total = (rec_count+3)/4;		/*ÿ������¼�γ�һ��*/
	puserdata->packet_curr = 1;
	//vdr_08_get( pnodedata );                                                            /*׼�����ݲ�����*/
}

FINSH_FUNCTION_EXPORT_ALIAS(vdr_08_get_ready,get_08,get_08_data);



/*
   ��λСʱ��ÿ���ӵ�λ����Ϣ���ٶ�
   ���Ƶ�ʣ�ÿ����һ������
   360Сʱ=15��
   ÿ��4��sector ÿ��sector��Ӧ6Сʱ��ÿСʱ  680�ֽ�
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

/*ÿ�ĸ�sector����һ��*/
	for( sector = 0; sector < VDR_09_SECTORS; sector++ )
	{
	}

	for( addr = VDR_08_START; addr < VDR_08_END; addr += 4096 )
	{
		sst25_read( addr, buf, 32 );

		mytime_curr = ( buf[0] << 24 ) | ( buf[1] << 16 ) | ( buf[2] << 8 ) | buf[3];   /*ǰ4���ֽڴ���ʱ��*/

		mytime_curr &= 0xFFFFFFC0;                                                      /*Ƶ�ε�sec*/

		if( mytime_curr != 0xFFFFFFFF )                                                 /*����Ч��¼*/
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
	if( find == 0 ) /*û���ҵ���Ч��¼*/
	{
		vdr_08_addr = VDR_08_START + VDR_08_SECTORS * 4096;
	}
	rt_kprintf( "%d>vdr_08 addr=%08x datetime=%08x\r\n", vdr_08_addr, vdr_08_time );
}

/*
   ÿ������6��Сʱ������
   һ��Сʱ666byte ռ��680byte

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

	if( ( vdr_09_time & 0xFFFFF000 ) != ( datetime & 0xFFFFF000 ) ) /*�������ڵ�ǰ��Сʱ��*/
	{
		vdr_09_time = datetime;
		vdr_09_sect_index++;                                        /*����*/
		if( vdr_09_sect_index > 5 )                                 /*ÿ��sector��6��Сʱ 0-5*/
		{
			vdr_09_sect_index = 0;
			vdr_09_sect++;
			if( vdr_09_sect >= VDR_09_SECTORS )
			{
				vdr_09_sect = 0;
			}
			addr = VDR_09_START + vdr_09_sect * 4096;   /*ɾ������*/
#ifdef VDR_DEBUG
			rt_kprintf( "%d>д���µ����� vdr_09_sect=%d\r\n", rt_tick_get( ), vdr_09_sect );
#else
			sst25_erase_4k( addr );
			PACK_INT( bufhead, datetime );              /*д���µļ�¼ͷ*/
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
   rt_kprintf( "%d>д���������� vdr_09_sect=%d\r\n", rt_tick_get( ), vdr_09_sect );
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
   ������ʻ��¼����
   ע�����ֲ�ͬ����������
   id=0
 */
static vdr_save_rec( uint8_t sect_id, uint8_t * pdata, uint16_t len )
{
	uint8_t id = sect_id - 8;
/*����һ���¼�¼*/
	sect_info[id].addr += sect_info[id].rec_size;
/*�ж��Ƿ񻷻�*/
	if( sect_info[id].addr >= ( sect_info[id].start_addr + sect_info[id].sectors * 4096 ) )
	{
		sect_info[id].addr = sect_info[id].start_addr;
	}

	if( ( sect_info[id].addr & 0xFFF ) == 0 ) /*��4k�߽紦*/
	{
		sst25_erase_4k( sect_info[id].addr );
	}
	sst25_write_through( sect_info[id].addr, pdata, len );
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
	uint32_t	i;
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


/*
   09���� ��λСʱ��ÿ���ӵ�λ���ٶȣ�ֻ�����һ����Ч��
   ÿСʱ(10+1)*60+6 �ֽ�
 */
	//vdr_09_put( datetime );

/*10���� �¹��ɵ�*/
	if( car_status.status == 0 )                                        /*��Ϊ����ֹͣ,�ж�����*/
	{
		if( gps_speed >= SPEED_LIMIT )                                  /*�ٶȴ�������ֵ*/
		{
			car_status.gps_judge_duration++;
			if( car_status.gps_judge_duration >= SPEED_LIMIT_DURATION ) /*�����˳���ʱ��*/
			{
				car_status.gps_duration			= SPEED_LIMIT_DURATION;
				car_status.status				= 1;                    /*��Ϊ������ʻ*/
				car_status.gps_judge_duration	= 0;
				rt_kprintf( "%d>������ʻ\r\n", rt_tick_get( ) );
			}else
			{
				car_status.gps_duration++;                              /*ͣ���ۼ�ʱ��*/
			}
		}else
		{
			car_status.gps_duration++;                                  /*ͣ���ۼ�ʱ��*/
			                                                            /*�ڴ��ж�ͣ����ʱ*/
			car_status.gps_judge_duration = 0;
		}
	}else /*����������*/
	{
		if( gps_speed <= SPEED_LIMIT )                                  /*�ٶ�С������ֵ*/
		{
			car_status.gps_judge_duration++;
			if( car_status.gps_judge_duration >= SPEED_LIMIT_DURATION ) /*�����˳���ʱ��*/
			{
				car_status.gps_duration			= SPEED_LIMIT_DURATION;
				car_status.status				= 0;                    /*��Ϊ����ͣʻ*/
				car_status.gps_judge_duration	= 0;
				rt_kprintf( "%d>����ͣʻ\r\n", rt_tick_get( ) );
			}else
			{
				car_status.gps_duration++;                              /*��ʻ�ۼ�ʱ��*/
			}
		}else
		{
			car_status.gps_duration++;                                  /*��ʻ�ۼ�ʱ��*/
			                                                            /*�ж�ƣ�ͼ�ʻ*/
			car_status.gps_judge_duration = 0;
		}
	}

/*11���ݳ�ʱ��ʻ��¼*/
}

/*
   ɾ���ض�����ļ�¼����
   ʹ��bitmask�ǲ��Ǹ���
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
   ��ȡ08�洢��״̬ 48Сʱ ��λ������ÿ����ٶ�״̬2byte
   48*60*128=2880*128=368640 (bytes)
   368640/4096=90(sectors)

   ��ʽ:
   <'8'><mydatetime(4bytes><60����ٶ�״̬120bytes>
   ѭ������
 */


/*
   ��ʼ����¼������
   ��Ϊ�����ڹ̶�ʱ��δ洢��
   ��Ҫ��¼��ʼʱ�̵�sectorλ��(��Ե�sectorƫ��)
 */
rt_err_t vdr_init( void )
{
	uint8_t* pbuf;

	pulse_init( ); /*���������*/

	//vdr_format( 0xff00 );

	pbuf = rt_malloc( 4096 );
	if( pbuf == RT_NULL )
	{
		return -RT_ENOMEM;
	}

	vdr_08_init( );
#if 0
	vdr_init_byid( 8, pbuf );
	sst25_read( sect_info[0].addr, (uint8_t*)&stu_rec_08, sizeof( STU_REC_08 ) ); /*�������Ƿ�ֹһ�����ڵ�����*/
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
/*��ʼ��һ��50ms�Ķ�ʱ���������¹��ɵ��ж�*/
	rt_timer_init( &tmr_200ms, "tmr_200ms",             /* ��ʱ�������� tmr_50ms */
	               cb_tmr_200ms,                        /* ��ʱʱ�ص��Ĵ����� */
	               RT_NULL,                             /* ��ʱ��������ڲ��� */
	               RT_TICK_PER_SECOND / 5,              /* ��ʱ���ȣ���OS TickΪ��λ */
	               RT_TIMER_FLAG_PERIODIC );            /* �����Զ�ʱ�� */
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
			jt808_add_tx_data_single( 1, TERMINAL_ACK, 0x0700, 11, buf, RT_NULL, RT_NULL );
			break;
		case 1:
			buf[0]	= seq >> 8;
			buf[1]	= seq & 0xff;
			buf[2]	= cmd;
			memcpy( buf + 3, "\x55\x7A\x01\x00\x12\x00120221123456789\x00\x00\x00\x00", 25 );
			jt808_add_tx_data_single( 1, TERMINAL_ACK, 0x0700, 28, buf, RT_NULL, RT_NULL );
		case 2: /*�г���¼��ʱ��*/
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
		case 8:                                 /*08��¼*/
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
