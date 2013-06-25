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


/*
   4MB serial flash 0x400000
 */

/*ת��hex��bcd�ı���*/
#define HEX_TO_BCD( A ) ( ( ( ( A ) / 10 ) << 4 ) | ( ( A ) % 10 ) )

static rt_thread_t tid_usb_vdr = RT_NULL;

#define VDR_08H_09H_START	0x300000
#define VDR_08H_09H_END		0x32FFFF

#define VDR_BASE 0x300000

#define VDR_08_START	VDR_BASE
#define VDR_08_SECTORS	92  /*48Сʱ ��λ�����ڵ��ٶ� 48*60*128/4096=90 ����ֹɾ������4Kʱ���ݲ��㣬Ҫ��Ԥ��һЩ*/

#define VDR_09_START	( VDR_08_START + VDR_08_SECTORS * 4096 )
#define VDR_09_SECTORS	86  /*360Сʱ ÿ����λ���ٶ� 360*60*16*/

#define VDR_10_START	( VDR_09_START + VDR_09_SECTORS * 4096 )
#define VDR_10_SECTORS	8   /*100���¹��ɵ� 100*234  ʵ�� 128*256 */

#define VDR_11_START	( VDR_10_START + VDR_10_SECTORS * 4096 )
#define VDR_11_SECTORS	3   /*100����ʱ��ʻ��¼ 100*50 ʵ�� 128*64,����һ��������ɾ��ʱ��������*/

#define VDR_12_START	( VDR_11_START + VDR_11_SECTORS * 4096 )
#define VDR_12_SECTORS	3   /*200����ʻ����ݼ�¼ 200*25 ʵ��200*32 */

#define VDR_13_START	( VDR_12_START + VDR_12_SECTORS * 4096 )
#define VDR_13_SECTORS	2   /*100�� �ⲿ�����¼100*7 ʵ�� 100*8*/

#define VDR_14_START	( VDR_13_START + VDR_13_SECTORS * 4096 )
#define VDR_14_SECTORS	2   /*100�� �����޸ļ�¼ 100*7 ʵ��100*8*/

#define VDR_15_START	( VDR_14_START + VDR_14_SECTORS * 4096 )
#define VDR_15_SECTORS	2   /*10���ٶ�״̬��־ 10*133 ʵ�� 10*256*/

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

#define MYDATETIME( year, month, day, hour, minute, sec )	( (uint32_t)( year << 26 ) | (uint32_t)( month << 22 ) | (uint32_t)( day << 17 ) | (uint32_t)( hour << 12 ) | (uint32_t)( minute << 6 ) | sec )
#define YEAR( datetime )									( ( datetime >> 26 ) & 0x3F )
#define MONTH( datetime )									( ( datetime >> 22 ) & 0xF )
#define DAY( datetime )										( ( datetime >> 17 ) & 0x1F )
#define HOUR( datetime )									( ( datetime >> 12 ) & 0x1F )
#define MINUTE( datetime )									( ( datetime >> 6 ) & 0x3F )
#define SEC( datetime )										( datetime & 0x3F )

uint8_t vdr_signal_status = 0x01; /*�г���¼�ǵ�״̬�ź�*/

typedef __packed struct _rec_08
{
	uint8_t		flag;
	uint32_t	datetime;
	uint8_t		speed_status[60][2];
} STU_REC_08;
STU_REC_08 stu_rec_08;


/*
   ���ȡ�γ�ȷֱ�Ϊ4���ֽ����һ
   ��32λ���з���������ʾ���Ȼ�γ
   �ȣ���λΪ0.0001��ÿ����
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
/*08����,û���ж�59��ʱ�������ֻҪymdhm��ͬ���ͱ���*/

	if( ( stu_rec_08.datetime & 0xFFFFFFC0 ) != ( datetime & 0xFFFFFFC0 ) ) /*�����ڵ�ǰ��һ������*/
	{
		if( stu_rec_08.datetime != 0xFFFFFFFF )                             /*����Ч������,Ҫ����*/
		{
			stu_rec_08.flag = '8';
			vdr_save_rec( 8, (uint8_t*)&stu_rec_08, sizeof( STU_REC_08 ) );
			memset( (uint8_t*)&stu_rec_08, 0xFF, sizeof( STU_REC_08 ) );    /*�µļ�¼����ʼ��Ϊ0xFF*/
		}
	}
	stu_rec_08.datetime				= datetime;
	stu_rec_08.speed_status[sec][0] = gps_speed;
	stu_rec_08.speed_status[sec][1] = vdr_signal_status;

/*09���� ÿ���ӵ�λ���ٶȣ�ֻ�����һ����Ч��*/
	if( ( stu_rec_09.datetime & 0xFFFFFFC0 ) != ( datetime & 0xFFFFFFC0 ) ) /*�������ڵ�ǰ��һ������*/
	{
		stu_rec_09.datetime = datetime;                                     /*��λ���*/
		stu_rec_09.flag		= '9';
		stu_rec_09.longi	= BYTESWAP4( gps_longi * 6 );                   /* ������*/
		stu_rec_09.lati		= BYTESWAP4( gps_lati * 6 );
		stu_rec_09.speed	= BYTESWAP2( gps_speed );
		vdr_save_rec( 9, (uint8_t*)&stu_rec_09, sizeof( STU_REC_09 ) );
	}
/*10���� �¹��ɵ�*/
	if( car_status.status == 0 )                                            /*��Ϊ����ֹͣ,�ж�����*/
	{
		if( gps_speed >= SPEED_LIMIT )                                      /*�ٶȴ�������ֵ*/
		{
			car_status.gps_judge_duration++;
			if( car_status.gps_judge_duration >= SPEED_LIMIT_DURATION )     /*�����˳���ʱ��*/
			{
				car_status.gps_duration			= SPEED_LIMIT_DURATION;
				car_status.status				= 1;                        /*��Ϊ������ʻ*/
				car_status.gps_judge_duration	= 0;
				rt_kprintf( "%d>������ʻ\r\n", rt_tick_get( ) );
			}else
			{
				car_status.gps_duration++;                                  /*ͣ���ۼ�ʱ��*/
			}
		}else
		{
			car_status.gps_duration++;                                      /*ͣ���ۼ�ʱ��*/
			/*�ڴ��ж�ͣ����ʱ*/
			car_status.gps_judge_duration = 0;
		}
	}else /*����������*/
	{
		if( gps_speed <= SPEED_LIMIT )                                      /*�ٶ�С������ֵ*/
		{
			car_status.gps_judge_duration++;
			if( car_status.gps_judge_duration >= SPEED_LIMIT_DURATION )     /*�����˳���ʱ��*/
			{
				car_status.gps_duration			= SPEED_LIMIT_DURATION;
				car_status.status				= 0;                        /*��Ϊ����ͣʻ*/
				car_status.gps_judge_duration	= 0;
				rt_kprintf( "%d>����ͣʻ\r\n", rt_tick_get( ) );
			}else
			{
				car_status.gps_duration++;                                  /*��ʻ�ۼ�ʱ��*/
			}
		}else
		{
			car_status.gps_duration++;                                      /*��ʻ�ۼ�ʱ��*/
			/*�ж�ƣ�ͼ�ʻ*/
			car_status.gps_judge_duration = 0;
		}
	}

/*11���ݳ�ʱ��ʻ��¼*/
}

/*��ȡ08����*/
void vdr_get_08( )
{
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

	rt_free( pbuf );
	pbuf = RT_NULL;
/*��ʼ��һ��50ms�Ķ�ʱ���������¹��ɵ��ж�*/
	rt_timer_init( &tmr_200ms, "tmr_200ms",     /* ��ʱ�������� tmr_50ms */
	               cb_tmr_200ms,                /* ��ʱʱ�ص��Ĵ����� */
	               RT_NULL,                     /* ��ʱ��������ڲ��� */
	               RT_TICK_PER_SECOND / 5,      /* ��ʱ���ȣ���OS TickΪ��λ */
	               RT_TIMER_FLAG_PERIODIC );    /* �����Զ�ʱ�� */
	rt_timer_start( &tmr_200ms );
}

/*�г���¼�����ݲɼ�����*/
void vdr_rx_8700( uint8_t *pmsg )
{
	uint8_t		* psrc;
	uint8_t		buf[500];

	uint16_t	seq = JT808HEAD_SEQ( pmsg );
	uint16_t	len = JT808HEAD_LEN( pmsg );
	uint8_t		cmd = *( pmsg + 13 );   /*����ǰ��12�ֽڵ�ͷ*/

	switch( cmd )
	{
		case 0:                         /*�ɼ���¼��ִ�б�׼�汾*/
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
		case 2:  /*�г���¼��ʱ��*/
			buf[0]	= seq >> 8;
			buf[1]	= seq & 0xff;
			buf[2]	= cmd;
			sprintf(buf+3,"\x55\x7A\x02\x00\x06\x00%6s",gps_baseinfo.datetime);
			jt808_add_tx_data_single( 1, TERMINAL_ACK, 0x0700, 15, buf, RT_NULL, RT_NULL );
		case 3:
			
	}
}

/************************************** The End Of File **************************************/
