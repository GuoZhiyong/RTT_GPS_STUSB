/************************************************************
 * Copyright (C), 2008-2012,
 * FileName:		// �ļ���
 * Author:			// ����
 * Date:			// ����
 * Description:		// ģ������
 * Version:			// �汾��Ϣ
 * Function List:	// ��Ҫ�������书��
 *     1. -------
 * History:			// ��ʷ�޸ļ�¼
 *     <author>  <time>   <version >   <desc>
 *     David    96/10/12     1.0     build this moudle
 ***********************************************************/
#include <stdio.h>

#include <board.h>
#include <rtthread.h>
#include <finsh.h>

#include "stm32f4xx.h"
#include "jt808.h"
#include "jt808_gps.h"
#include "jt808_param.h"
#include "rtc.h"
#include "jt808_util.h"
#include "jt808_vehicle.h"
#include "jt808_area.h"
#include "menu_include.h"
#include "jt808_gps_pack.h"
#include "vdr.h"
#include "math.h"

#define BIT( n ) ( 1 << n )

typedef struct _GPSPoint
{
	int sign;
	int deg;
	int min;
	int sec;
} GPSPoint;

uint32_t gps_sec_count = 0;                         /*gps���������*/

/*Ҫ��union���ͱ��棬λ�򣬷�����?*/
uint32_t		jt808_alarm			= 0x0;
uint32_t		jt808_alarm_last	= 0x0;          /*��һ�ε��ϱ�״̬*/

uint32_t		jt808_status		= 0x0;
uint32_t		jt808_status_last	= 0x0;          /*��һ�ε�״̬��Ϣ*/

static uint32_t jt808_report_interval	= 60;       /*GPS�ϱ�ʱ������Ϊ0:ֹͣ�ϱ�*/
static uint32_t jt808_report_distance	= 1000;     /*GPS�ϱ�������,Ϊ0 ֹͣ�ϱ�*/

static uint32_t distance		= 0;                /*�����ϱ���ǰ����ֵ*/
static uint32_t total_distance	= 0;                /*�ܵ��ۼ����*/

uint32_t		gps_second_count = 0;               /*gps��������*/

uint16_t		jt808_8202_track_interval	= 0;    /*jt808_8202 ��ʱλ�ø��ٿ���*/
uint32_t		jt808_8202_track_duration	= 0;
uint16_t		jt808_8202_track_counter;

uint32_t		jt808_8203_manual_ack_seq	= 0;    /*�˹�ȷ�ϱ����ı�ʶλ 0,3,20,21,22,27,28*/
uint16_t		jt808_8203_manual_ack_value = 0;

#if 0


/*
   ����Ķ���,ʹ��list�������������node����Ļ���
   RAM�Ƿ���
   ʹ��dataflash�洢����4k��Ϊcache,�������
   ÿ���λ����Ϣ��Ҫ�ж�
 */
struct
{
	uint32_t	id;                 /*����ID*/
	uint16_t	attr;               /*����*/
	uint32_t	latitude;           /*����γ��*/
	uint32_t	logitude;           /*���ľ���*/
	uint32_t	radius;             /*�뾶*/
	uint8_t		datetime_start[6];  /*��ʼʱ�̣�ʹ��utc�ǲ��Ǹ���?*/
	uint8_t		datetime_end[6];
	uint16_t	speed;
	uint8_t		duration;           /*����ʱ��*/
} circle;

struct
{
	uint32_t	id;                 /*����ID*/
	uint16_t	attr;               /*����*/
	uint32_t	latitude;           /*����γ��*/
	uint32_t	logitude;           /*���ľ���*/
	uint32_t	radius;             /*�뾶*/
	uint8_t		datetime_start[6];  /*��ʼʱ�̣�ʹ��utc�ǲ��Ǹ���?*/
	uint8_t		datetime_end[6];
	uint16_t	speed;
	uint8_t		duration;           /*����ʱ��*/
} rectangle;

#endif

uint32_t	gps_lati;
uint32_t	gps_longi;
uint16_t	gps_speed;

uint16_t	gps_cog;              /*course over ground*/
uint16_t	gps_alti;
uint8_t		gps_datetime[6];

/*��¼��һ�ε�λ�ã��������������*/
static uint32_t gps_lati_last	= 0;
static uint32_t gps_longi_last	= 0;

/*����gps����λ����Ϣ*/
GPS_BASEINFO	gps_baseinfo;
/*gps��״̬*/
GPS_STATUS		gps_status = { MODE_BDGPS, 0, 0, 0 };


/*
   Epochָ����һ���ض���ʱ�䣺1970-01-01 00:00:00 UTC
   UNIXʱ�����Unixʱ�����Ӣ��ΪUnix time, POSIX time �� Unix timestamp��
   �Ǵ�Epoch��1970��1��1��00:00:00 UTC����ʼ�����������������������롣

 */
uint32_t	utc_last	= 0;
uint32_t	utc_now		= 0;
MYTIME		mytime_now	= 0;

uint8_t		ACC_status;     /*0:ACC��   1:ACC��  */
uint32_t	ACC_ticks;      /*ACC״̬�����仯ʱ��tickֵ����ʱGPS����δ��λ*/

struct
{
	uint8_t		mode;       /*�ϱ�ģʽ 0:��ʱ 1:���� 2:��ʱ����*/
	uint8_t		userlogin;  /*�Ƿ�ʹ�õ�¼*/
	uint32_t	time_unlog;
	uint32_t	time_sleep;
	uint32_t	time_emg;
	uint32_t	time_default;
	uint32_t	distance_unlog;
	uint32_t	distance_sleep;
	uint32_t	distance_emg;
	uint32_t	distance_default;

	uint32_t	last_tick;      /*��һ���ϱ���ʱ��*/
	uint32_t	last_distance;  /*��һ���ϱ�ʱ�����*/
} jt808_report;

#define DEBUG_GPS

#ifdef DEBUG_GPS
uint8_t		speed_add	= 0;
uint32_t	speed_count = 0;
#endif

/*hmi���15�����ٶ�*/
static void process_hmi_15min_speed( void )
{
	static uint8_t	hmi_15min_speed_count	= 0;                                    /*��??��?����?????��y*/
	static uint32_t hmi_15min_speed_sum		= 0;                                    /*?��?����??��o��*/
	if( ( mytime_now & 0xFFFFFFC0 ) != hmi_15min_speed[hmi_15min_speed_curr].time ) /*D?����?��,??������?��??��*/
	{
		if( hmi_15min_speed[hmi_15min_speed_curr].time != 0 )                       /*��?��a?2??*/
		{
			//hmi_15min_speed[hmi_15min_speed_curr].speed=hmi_15min_speed_sum/hmi_15min_speed_count;
			hmi_15min_speed_curr++;
			hmi_15min_speed_curr	%= 15;
			hmi_15min_speed_sum		= 0;
			hmi_15min_speed_count	= 0;
		}
	}
	hmi_15min_speed[hmi_15min_speed_curr].time	= mytime_now & 0xFFFFFFC0;
	hmi_15min_speed_sum							+= gps_speed;
	hmi_15min_speed_count++;
	hmi_15min_speed[hmi_15min_speed_curr].speed = hmi_15min_speed_sum / hmi_15min_speed_count; /*??����?��D?*/
}

/*
   ͨ��gps��䴥����1�붨ʱ
   δ��λʱҲ��ƣ�ͼ�ʻ
 */
static void adjust_mytime_now( void )
{
	uint8_t year, month, day, hour, minute, sec;

	if( mytime_now )                  /*mytime_now����gps��λ�����ʱ*/
	{
		sec		= SEC( mytime_now );
		minute	= MINUTE( mytime_now );
		hour	= HOUR( mytime_now );
		day		= DAY( mytime_now );
		month	= MONTH( mytime_now );
		year	= YEAR( mytime_now );
		sec++;

		if( sec == 60 )
		{
			sec = 0;
			minute++;
		}
		if( minute == 60 )
		{
			minute = 0;
			hour++;
		}
		if( hour == 24 )
		{
			hour = 0;
			day++;
		}
		if( ( month == 4 ) || ( month == 6 ) || ( month == 9 ) || ( month == 11 ) )
		{
			if( day == 31 )
			{
				day = 1;
				month++;
			}
		}else if( month == 2 )
		{
			if( year % 4 == 0 ) /*����29��*/
			{
				if( day == 30 )
				{
					day = 1;
					month++;
				}
			}else
			{
				if( day == 29 )
				{
					day = 1;
					month++;
				}
			}
		}else if( day == 32 )
		{
			day = 1;
			month++;
		}
		if( month == 13 )
		{
			month = 1;
			year++;
		}
		mytime_now = MYDATETIME( year, month, day, hour, minute, sec );
	}
}

/*
   LinuxԴ���е�mktime�㷨����
 */
static __inline unsigned long linux_mktime( unsigned int year, unsigned int mon,
                                            unsigned int day, unsigned int hour,
                                            unsigned int min, unsigned int sec )
{
	if( 0 >= (int)( mon -= 2 ) )                    /**//* 1..12 -> 11,12,1..10 */
	{
		mon		+= 12;                              /**//* Puts Feb last since it has leap day */
		year	-= 1;
	}

	return ( ( ( (unsigned long)( year / 4 - year / 100 + year / 400 + 367 * mon / 12 + day ) +
	             year * 365 - 719499
	             ) * 24 + hour                      /**//* now have hours */
	           ) * 60 + min                         /**//* now have minutes */
	         ) * 60 + sec;                          /**//* finally seconds */
}

/*�������*/
uint32_t calc_distance( void )
{
	if( gps_lati_last )                             /*�״ζ�λ*/
	{
		distance				= dis_Point2Point( gps_lati_last, gps_longi_last, gps_lati, gps_longi );
		total_distance			+= distance;
		jt808_param.id_0xF020	= total_distance;   /*�����m*/
	}
	gps_lati_last	= gps_lati;
	gps_longi_last	= gps_longi;
	/**/
	return distance;
}

#if 0
/**/
static double gpsToRad( GPSPoint point )
{
	return point.sign * ( point.deg + ( point.min + point.sec / 60.0 ) / 60.0 ) * 3.141592654 / 180.0;
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
static double getDistance( GPSPoint latFrom, GPSPoint lngFrom, GPSPoint latTo, GPSPoint lngTo )
{
	double	latFromRad	= gpsToRad( latFrom );
	double	lngFromRad	= gpsToRad( lngFrom );
	double	latToRad	= gpsToRad( latTo );
	double	lngToRad	= gpsToRad( lngTo );
	double	lngDiff		= lngToRad - lngFromRad;
	double	part1		= pow( cos( latToRad ) * sin( lngDiff ), 2 );
	//double part2 = pow( cos(latFromRad)*sin(latToRad)*cos(lngDiff) , 2);
	double	part2 = pow( cos( latFromRad ) * sin( latToRad ) - sin( latFromRad ) * cos( latToRad ) * cos( lngDiff ), 2 );

	double	part3 = sin( latFromRad ) * sin( latToRad ) + cos( latFromRad ) * cos( latToRad ) * cos( lngDiff );
	//double centralAngle = atan2( sqrt(part1 + part2) / part3 );
	double	centralAngle = atan( sqrt( part1 + part2 ) / part3 );
	return 6371.01 * 1000.0 * centralAngle; //Return Distance in meter
}

#endif


/*
   ����gps��Ϣ,�ж���������ϣ��ϱ�
   ��ʱ���յ���ȡ��
 */

#define FLAG_SEND_AREA			0x80
#define FLAG_SEND_STATUS		0x01
#define FLAG_SEND_ALARM			0x02
#define FLAG_SEND_FIX_TIME		0x04
#define FLAG_SEND_FIX_DISTANCE	0x08


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static void process_gps_report( void )
{
	uint32_t	tmp;
	uint8_t		flag_send	= 0; /*Ĭ�ϲ��ϱ�*/
	uint8_t		*palarmdata = RT_NULL;
	uint16_t	alarm_length;
	uint32_t	alarm_bits;

	uint8_t		buf[300];

/*����·�ߴ���*/
	alarm_bits = area_get_alarm( palarmdata, &alarm_length );
	if( alarm_bits ) /*�и澯*/
	{
		rt_kprintf( "\n�����и澯" );
		memcpy( buf + 28, palarmdata, alarm_length );
		flag_send = FLAG_SEND_AREA;
	}
	jt808_alarm			|= alarm_bits;
	gps_baseinfo.alarm	= BYTESWAP4( jt808_alarm );
	gps_baseinfo.status = BYTESWAP4( jt808_status );
	memcpy( buf, (uint8_t*)&gps_baseinfo, 28 );

/*����׷��,ֱ���ϱ���������*/
	if( jt808_8202_track_duration ) /*Ҫ׷��*/
	{
		jt808_8202_track_counter++;
		if( jt808_8202_track_counter >= jt808_8202_track_interval )
		{
			jt808_8202_track_counter = 0;
			jt808_tx( 0x0200, buf, 28 + alarm_length );
			if( jt808_8202_track_duration > jt808_8202_track_interval )
			{
				jt808_8202_track_duration -= jt808_8202_track_interval;
			}else
			{
				jt808_8202_track_duration = 0;
			}
		}
		return;
	}

/*�����ϱ���ʽ,�����ϳ�������� */
	tmp = jt808_status ^ jt808_status_last;
	if( tmp )                                           /*״̬�����仯��Ҫ�ϱ�,*/
	{
		flag_send |= FLAG_SEND_STATUS;
	}
	/*����������¼״̬*/
	if( tmp & BIT_STATUS_ACC )                          /*ACC�仯,�޸Ļ㱨�ļ�������*/
	{
		jt808_report_distance	= 0;
		jt808_report_interval	= 0;
		if( ( jt808_param.id_0x0020 & 0x01 ) == 0x0 )   /*�ж�ʱ�ϱ�*/
		{
			if( jt808_status & BIT_STATUS_ACC )         /*��ǰ״̬ΪACC��*/
			{
				jt808_report_interval = jt808_param.id_0x0029;
			}else
			{
				jt808_report_interval = jt808_param.id_0x0027;
			}
			utc_last = utc_now;                         /*���¼�ʱ*/
		}
		if( jt808_param.id_0x0020 )                     /*�ж����ϱ�*/
		{
			if( jt808_status & BIT_STATUS_ACC )         /*��ǰ״̬ΪACC��*/
			{
				jt808_report_distance = jt808_param.id_0x002C;
			}else
			{
				jt808_report_distance = jt808_param.id_0x002E;
			}
			distance = 0;                               /*���¼������*/
		}
	}

	tmp = ( jt808_alarm ^ jt808_alarm_last );           /*�澯λ�仯*/
	if( tmp )                                           /*�澯�����仯��Ҫ�ϱ�,*/
	{
		flag_send |= FLAG_SEND_ALARM;
	}

	if( tmp & BIT_ALARM_EMG )                           /*�����澯*/
	{
		if( ( jt808_param.id_0x0020 & 0x01 ) == 0x0 )   /*�ж�ʱ�ϱ�*/
		{
			jt808_report_interval	= jt808_param.id_0x0028;
			utc_last				= utc_now;
		}
		if( jt808_param.id_0x0020 )                     /*�ж����ϱ�*/
		{
			jt808_report_distance	= jt808_param.id_0x002F;
			distance				= 0;
		}
	}

/*���㶨ʱ�ϱ�*/
	if( ( jt808_param.id_0x0020 & 0x01 ) == 0x0 ) /*�ж�ʱ�ϱ�*/
	{
		if( utc_now - utc_last >= jt808_report_interval )
		{
			flag_send	|= FLAG_SEND_FIX_TIME;
			utc_last	= utc_now;
		}
	}
/*���㶨���ϱ�*/
	if( jt808_param.id_0x0020 ) /*�ж����ϱ�*/
	{
		if( distance >= jt808_report_distance )
		{
			flag_send	|= FLAG_SEND_FIX_DISTANCE;
			distance	= 0;
		}
	}

	jt808_status_last	= jt808_status;
	jt808_alarm_last	= jt808_alarm;


/*
   if( flag_send == 0 )
   {
   return;
   }
 */
/*����Ҫ�ϱ�������*/
#if 1

	//if( gps_datetime[5] == 0 )
	if( flag_send )
	{
		jt808_tx( 0x0200, buf, 28 + alarm_length );
		rt_kprintf( "\n%d>�ϱ�gps(%02x)", rt_tick_get( ), flag_send );
	}
#endif
}

/*
   $GNRMC,074001.00,A,3905.291037,N,11733.138255,E,0.1,,171212,,,A*655220.9*3F0E
   $GNTXT,01,01,01,ANTENNA OK*2B7,N,11733.138255,E,0.1,,171212,,,A*655220.9*3F0E
   $GNGGA,074002.00,3905.291085,N,11733.138264,E,1,11,0.9,8.2,M,-1.6,M,,,1.4*68E
   $GNGLL,3905.291085,N,11733.138264,E,074002.00,A,0*02.9,8.2,M,-1.6,M,,,1.4*68E
   $GPGSA,A,3,18,05,08,02,26,29,15,,,,,,,,,,,,,,,,,,,,,,,,,,1.6,0.9,1.4,0.9*3F8E
   $BDGSA,A,3,04,03,01,07,,,,,,,,,,,,,,,,,,,,,,,,,,,,,1.6,0.9,1.4,0.9*220.9*3F8E
   $GPGSV,2,1,7,18,10,278,29,05,51,063,08,21,052,24,02,24,140,45*4C220.9*3F8E
   $GPGSV,2,2,7,26,72,055,24,29,35,244,37,15,66,224,37*76,24,140,45*4C220.9*3F8E
   $BDGSV,1,1,4,04,27,124,38,03,42,190,34,01,38,146,37,07,34,173,35*55220.9*3F8E

   ���ش�����ֶ����������ȷ�Ļ�
 */

static uint8_t process_rmc( uint8_t * pinfo )
{
	//�������������,ִ������ת��
	uint8_t		year = 0, mon = 0, day = 0, hour = 0, min = 0, sec = 0, fDateModify = 0;
	uint32_t	degrees, minutes;
	uint8_t		commacount = 0, count = 0;

	uint32_t	lati		= 0, longi = 0;
	uint16_t	speed_10x	= 0;
	uint16_t	cog			= 0;    /*course over ground*/

	uint8_t		i;
	uint8_t		buf[20];
	uint8_t		*psrc = pinfo + 6;  /*ָ��ʼλ�� $GNRMC,074001.00,A,3905.291037,N,11733.138255,E,0.1,,171212,,,A*655220.9*3F0E*/

/*��Ϊ������һ�Σ����Դ�pinfo+6��ʼ*/
	while( *psrc++ )
	{
		if( *psrc != ',' )
		{
			buf[count++]	= *psrc;
			buf[count]		= 0;
			continue;
		}

		commacount++;
		switch( commacount )
		{
			case 1: /*ʱ��*/
				if( count < 6 )
				{
					return 1;
				}

				i = ( buf[0] - 0x30 ) * 10 + ( buf[1] - 0x30 ) + 8;
				if( i > 23 )
				{
					fDateModify = 1;
					i			-= 24;
				}
				/*ת��HEX��ʽ*/
				hour	= i;
				min		= ( buf[2] - 0x30 ) * 10 + ( buf[3] - 0x30 );
				sec		= ( buf[4] - 0x30 ) * 10 + ( buf[5] - 0x30 );
				break;
			case 2:                         /*A_V*/
				if( buf[0] != 'A' )         /*δ��λ*/
				{
					jt808_status	&= ~BIT_STATUS_FIXED;
					gps_lati_last	= 0;    /*���¼������*/
					gps_longi_last	= 0;
					return 2;
				}
				jt808_status |= BIT_STATUS_FIXED;
#if 0
				if( buf[0] == 'A' )
				{
					jt808_status |= BIT_STATUS_GPS;
				} else if( buf[0] == 'V' )
				{
					jt808_status &= ~BIT_STATUS_GPS;
				} else
				{
					return 2;
				}
#endif
				break;
			case 3: /*γ�ȴ���ddmm.mmmmmm*/
				if( count < 10 )
				{
					return 3;
				}

				degrees = ( ( buf [0] - 0x30 ) * 10 + ( buf [1] - 0x30 ) ) * 1000000;
				minutes = ( buf [2] - 0x30 ) * 1000000 +
				          ( buf [3] - 0x30 ) * 100000 +
				          ( buf [5] - 0x30 ) * 10000 +
				          ( buf [6] - 0x30 ) * 1000 +
				          ( buf [7] - 0x30 ) * 100 +
				          ( buf [8] - 0x30 ) * 10 +
				          ( buf [9] - 0x30 );   /*�����һ��λ����Ҫ��֤����*/
				lati = degrees + minutes / 6;
				break;
			case 4:                             /*N_S����*/
				if( buf[0] == 'N' )
				{
					jt808_status &= ~BIT_STATUS_NS;
				} else if( buf[0] == 'S' )
				{
					jt808_status |= BIT_STATUS_NS;
				}else
				{
					return 4;
				}
				break;
			case 5: /*���ȴ���*/
				if( count < 11 )
				{
					return 5;
				}
				degrees = ( ( buf [0] - 0x30 ) * 100 + ( buf [1] - 0x30 ) * 10 + ( buf [2] - 0x30 ) ) * 1000000;
				minutes = ( buf [3] - 0x30 ) * 1000000 +
				          ( buf [4] - 0x30 ) * 100000 +
				          ( buf [6] - 0x30 ) * 10000 +
				          ( buf [7] - 0x30 ) * 1000 +
				          ( buf [8] - 0x30 ) * 100 +
				          ( buf [9] - 0x30 ) * 10 +
				          ( buf [10] - 0x30 );
				longi = degrees + minutes / 6;
				break;
			case 6: /*E_W����*/
				if( buf[0] == 'E' )
				{
					jt808_status &= ~BIT_STATUS_EW;
				} else if( buf[0] == 'W' )
				{
					jt808_status |= BIT_STATUS_EW;
				}else
				{
					return 6;
				}
				break;
			case 7: /*�ٶȴ��� */
				speed_10x = 0;
				for( i = 0; i < count; i++ )
				{
					if( buf[i] == '.' )
					{
						speed_10x += ( buf[i + 1] - 0x30 );
						break;
					}else
					{
						speed_10x	+= ( buf[i] - 0x30 );
						speed_10x	= speed_10x * 10;
					}
				}
				/*��ǰ��0.1knot => 0.1Kmh  1����=1.852Km  1852=1024+512+256+32+16+8+4*/
#ifdef DEBUG_GPS
				if( speed_count )
				{
					speed_10x += ( speed_add * 10 );
					speed_count--;
				}

#endif
				speed_10x	*= 1.852;
				gps_speed	= speed_10x / 10;
				//i=speed_10x;
				//speed_10x=(i<<10)|(i<<9)|(i<<8)|(i<<5)|(i<<4)|(i<<3)|(i<<2);
				//speed_10x/=1000;
				break;

			case 8: /*������*/
				cog = 0;
				for( i = 0; i < count; i++ )
				{
					if( buf[i] == '.' )
					{
						break;
					}else
					{
						cog = cog * 10;
						cog += ( buf[i] - 0x30 );
					}
				}
				break;

			case 9: /*���ڴ���*/
				if( count < 6 )
				{
					return 9;
				}

				day		= ( ( buf [0] - 0x30 ) * 10 ) + ( buf [1] - 0x30 );
				mon		= ( ( buf [2] - 0x30 ) * 10 ) + ( buf [3] - 0x30 );
				year	= ( ( buf [4] - 0x30 ) * 10 ) + ( buf [5] - 0x30 );

				if( fDateModify )
				{
					day++;
					if( mon == 2 )
					{
						if( ( year % 4 ) == 0 ) /*û�п�������ʱ��Ҫ��400������NM��2100��*/
						{
							if( day == 30 )
							{
								day = 1; mon++;
							}
						} else
						if( day == 29 )
						{
							day = 1; mon++;
						}
					} else
					if( ( mon == 4 ) || ( mon == 6 ) || ( mon == 9 ) || ( mon == 11 ) )
					{
						if( day == 31 )
						{
							mon++; day = 1;
						}
					} else
					{
						if( day == 32 )
						{
							mon++; day = 1;
						}
						if( mon == 13 )
						{
							mon = 1; year++;
						}
					}
				}

				/*���������˸��� gps_baseinfo,û�и߳���Ϣ*/
				gps_lati		= lati;
				gps_longi		= longi;
				gps_speed		= speed_10x / 10;
				gps_cog			= cog;
				gps_datetime[0] = year;
				gps_datetime[1] = mon;
				gps_datetime[2] = day;
				gps_datetime[3] = hour;
				gps_datetime[4] = min;
				gps_datetime[5] = sec;

				gps_baseinfo.alarm		= BYTESWAP4( jt808_alarm );
				gps_baseinfo.status		= BYTESWAP4( jt808_status );
				gps_baseinfo.latitude	= BYTESWAP4( lati );
				gps_baseinfo.longitude	= BYTESWAP4( longi );
				gps_baseinfo.speed_10x	= BYTESWAP2( speed_10x );
				gps_baseinfo.cog		= BYTESWAP2( cog );

				utc_now						= linux_mktime( year, mon, day, hour, min, sec );
				mytime_now					= MYDATETIME( year, mon, day, hour, min, sec );
				gps_baseinfo.datetime[0]	= HEX2BCD( year );
				gps_baseinfo.datetime[1]	= HEX2BCD( mon );
				gps_baseinfo.datetime[2]	= HEX2BCD( day );
				gps_baseinfo.datetime[3]	= HEX2BCD( hour );
				gps_baseinfo.datetime[4]	= HEX2BCD( min );
				gps_baseinfo.datetime[5]	= HEX2BCD( sec );

				/*�״ζ�λ,Уʱ*/
				if( ( jt808_status_last & BIT_STATUS_FIXED ) == 0 )
				{
					date_set( year, mon, day );
					time_set( hour, min, sec );
					rt_kprintf( "\n%d>rtc sync %02d-%02d-%02d %02d:%02d:%02d", rt_tick_get( ), year, mon, day, hour, min, sec );
				}
				/*��СʱУ׼*/
				if( ( gps_datetime[4] == 0 ) && ( gps_datetime[5] == 0 ) )
				{
					date_set( year, mon, day );
					time_set( hour, min, sec );
				}

				return 0;
		}
		count	= 0;
		buf[0]	= 0;
	}
	return 10;
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
uint8_t process_gga( uint8_t * pinfo )
{
	//�������������,ִ������ת��
	uint8_t		NoSV;
	uint8_t		i;
	uint8_t		buf[20];
	uint8_t		commacount	= 0, count = 0;
	uint8_t		*psrc		= pinfo + 7; //ָ��ʼλ��
	uint16_t	altitute;

	while( *psrc++ )
	{
		if( *psrc != ',' )
		{
			buf[count++]	= *psrc;
			buf[count]		= 0;
			continue;
		}
		commacount++;
		switch( commacount )
		{
			case 1: /*ʱ�䴦�� */
				if( count < 6 )
				{
					return 1;
				}
				break;

			case 2: /*γ�ȴ���ddmm.mmmmmm*/
				if( count < 10 )
				{
					return 2;
				}
				break;

			case 3: /*N_S����*/
				break;

			case 4: /*���ȴ���*/

				break;
			case 5: /*E_W����*/
				break;
			case 6: /*��λ����*/
				break;
			case 7: /*NoSV,������*/
				NoSV = 0;
				for( i = 0; i < count; i++ )
				{
					NoSV	= NoSV * 10;
					NoSV	+= ( buf[i] - 0x30 );
				}
				gps_status.NoSV = NoSV;
				break;
			case 8: /*HDOP*/
				return 0;

			case 9: /*MSL Altitute*/
				altitute = 0;
				for( i = 0; i < count; i++ )
				{
					if( buf[i] == '.' )
					{
						break;
					}
					altitute	= altitute * 10;
					altitute	+= ( buf[i] - '0' );
				}
				gps_baseinfo.altitude	= altitute;
				gps_alti				= altitute;
				return 0;
		}
		count	= 0;
		buf[0]	= 0;
	}
	return 9;
}

/***********************************************************
* Function:
* Description:gps�յ���Ϣ��Ĵ���ͷ�����ֽ�Ϊ����
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
void gps_rx( uint8_t * pinfo, uint16_t length )
{
	uint8_t ret;
	char	* psrc;
	psrc				= (char*)pinfo;
	*( psrc + length )	= 0;
	/*�Ƿ����ԭʼ��Ϣ*/
	if( gps_status.Raw_Output )
	{
		rt_kprintf( "\n%d gps<%s", rt_tick_get( ), psrc );
	}
	/*����RAW����*/
	jt808_gps_pack( (char*)pinfo, length );

	if( strncmp( psrc + 3, "GGA,", 4 ) == 0 )
	{
		process_gga( (uint8_t*)psrc );
	}

	//if( ( strncmp( psrc, "$GNRMC,", 7 ) == 0 ) || ( strncmp( psrc, "$BDRMC,", 7 ) == 0 ) || ( strncmp( psrc, "$GPRMC,", 7 ) == 0 ) )
	if( strncmp( psrc + 3, "RMC,", 4 ) == 0 )
	{
		gps_sec_count++;
		ret = process_rmc( (uint8_t*)psrc );

		if( ret == 0 )                  /*�Ѷ�λ*/
		{
			process_hmi_15min_speed( ); /*���15�����ٶ�*/
			vdr_rx_gps( );              /*�г���¼�����ݴ���*/
			area_process( );            /*������·�澯*/
			calc_distance( );
		}else
		{
			adjust_mytime_now( );       /*����mytime_now*/
		}
		process_gps_report( );          /*����GPS�ϱ���Ϣ*/
	}


	/*���߿���·��� gps<
	   $GNTXT,01,01,01,ANTENNA OK*2B
	   $GNTXT,01,01,01,ANTENNA OPEN*3B
	 */
	if( strncmp( psrc + 3, "TXT", 3 ) == 0 )
	{
		if( strncmp( psrc, "GN", 2 ) == 0 )
		{
			gps_status.Position_Moule_Status = MODE_BDGPS;
		}
		else if( strncmp( psrc, "GP", 2 ) == 0 )
		{
			gps_status.Position_Moule_Status = MODE_GPS;
		}
		else if( strncmp( psrc, "BD", 2 ) == 0 )
		{
			gps_status.Position_Moule_Status = MODE_BD;
		}

		if( strncmp( psrc + 24, "OK", 2 ) == 0 )
		{
			gps_status.Antenna_Flag = 0;
			jt808_alarm				&= ~( BIT_ALARM_GPS_OPEN | BIT_ALARM_GPS_SHORT );
		}else if( strncmp( psrc + 24, "OPEN", 4 ) == 0 )
		{
			gps_status.Antenna_Flag = 1;
			jt808_alarm				|= BIT_ALARM_GPS_OPEN; /*bit5 ���߿�·*/
		}else if( strncmp( psrc + 24, "SHORT", 4 ) == 0 )
		{
			gps_status.Antenna_Flag = 1;
			jt808_alarm				|= BIT_ALARM_GPS_SHORT;
		}
	}
}

/*��ʼ��jt808 gps��صĴ���*/
void jt808_gps_init( void )
{
	jt808_vehicle_init( );
	area_init( );
	gps_pack_init( );
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
void gps_dump( uint8_t mode )
{
	gps_status.Raw_Output = ~gps_status.Raw_Output;
}

FINSH_FUNCTION_EXPORT( gps_dump, dump gps raw info );

/************************************** The End Of File **************************************/

#ifdef DEBUG_GPS
/**ģ�����gps�ٶ�*/
void gps_speed_add( uint8_t sp, uint32_t count )
{
	speed_add	= sp;
	speed_count = count;
}

FINSH_FUNCTION_EXPORT_ALIAS( gps_speed_add, gps_speed, debug gps speed );
#endif

/************************************** The End Of File **************************************/
