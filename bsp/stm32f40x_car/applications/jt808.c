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
#include "gsm.h"
#include "jt808.h"
#include "msglist.h"
#include "jt808_sprintf.h"
#include "sst25.h"

#include "pt.h"

#include "tree_config.h"

#define ByteSwap2( val )    \
    ( ( ( val & 0xff ) << 8 ) |   \
      ( ( val & 0xff00 ) >> 8 ) )

#define ByteSwap4( val )    \
    ( ( ( val & 0xff ) << 24 ) |   \
      ( ( val & 0xff00 ) << 8 ) |  \
      ( ( val & 0xff0000 ) >> 8 ) |  \
      ( ( val & 0xff000000 ) >> 24 ) )

typedef struct
{
	uint16_t id;
	int ( *func )( JT808_RX_MSG_NODEDATA* nodedata );
}HANDLE_JT808_RX_MSG;

static struct rt_mailbox	mb_gprsdata;
#define MB_GPRSDATA_POOL_SIZE 32
static uint8_t				mb_gprsdata_pool[MB_GPRSDATA_POOL_SIZE];

static struct rt_mailbox	mb_gpsdata;
#define MB_GPSDATA_POOL_SIZE 32
static uint8_t				mb_gpsdata_pool[MB_GPSDATA_POOL_SIZE];

uint32_t					jt808_alarm		= 0x0;
uint32_t					jt808_status	= 0x0;

static uint16_t				tx_seq = 0; /*�������*/

static rt_device_t			pdev_gsm = RT_NULL;

static struct pt			pt_jt808_socket;

/*������Ϣ�б�*/
MsgList* list_jt808_tx;

/*������Ϣ�б�*/
MsgList		* list_jt808_rx;

T_GPSINFO	gpsinfo;

GSM_SOCKET	gsm_socket[MAX_GSM_SOCKET];

GSM_SOCKET	curr_gsm_socket;

#define DECL_PARAM_DWORD( id, value ) { id, T_DWORD, (void*)value }
//#define DECL_PARAM_STRING( id )			{ id, T_STRING, NULL }
//#define DECL_PARAM_STRING( id )			{ id, T_NODEF }
//#define SET_PARAM_DWORD( id, value )	int parma_ ## id	= value
//#define SET_PARAM_STRING( id, value )	char* parma_ ## id	= value
#if 0
PARAM param[] =
{
	DECL_PARAM_DWORD( 0x0000, 0x13020200 ),     /*0x0000 �汾*/
	DECL_PARAM_DWORD( 0x0001, 5 ),              /*0x0001 �������ͼ��*/
	DECL_PARAM_DWORD( 0x0002, 3 ),              /*0x0002 TCPӦ��ʱʱ��*/
	DECL_PARAM_DWORD( 0x0003, 15 ),             /*0x0003 TCP��ʱ�ش�����*/
	DECL_PARAM_DWORD( 0x0004, 3 ),              /*0x0004	UDPӦ��ʱʱ��*/
	DECL_PARAM_DWORD( 0x0005, 5 ),              /*0x0005 UDP��ʱ�ش�����*/
	DECL_PARAM_DWORD( 0x0006, 3 ),              /*0x0006 SMS��ϢӦ��ʱʱ��*/
	DECL_PARAM_DWORD( 0x0007, 5 ),              /*0x0007 SMS��Ϣ�ش�����*/
	DECL_PARAM_DWORD( 0x0010, "CMNET" ),        /*0x0010 ��������APN*/
	DECL_PARAM_DWORD( 0x0011, "" ),             /*0x0011 �û���*/
	DECL_PARAM_DWORD( 0x0012, "" ),             /*0x0012 ����*/
	DECL_PARAM_DWORD( 0x0013, "" ),             /*0x0013 ����������ַ*/
	DECL_PARAM_DWORD( 0x0014, "" ),             /*0x0014 ����APN*/
	DECL_PARAM_DWORD( 0x0015, "" ),             /*0x0015 �����û���*/
	DECL_PARAM_DWORD( 0x0016, "" ),             /*0x0016 ��������*/
	DECL_PARAM_DWORD( 0x0017, "" ),             /*0x0017 ���ݷ�������ַ��ip������*/
	DECL_PARAM_DWORD( 0x0018, 1234 ),           /*0x0018 TCP�˿�*/
	DECL_PARAM_DWORD( 0x0019, 5678 ),           /*0x0019 UDP�˿�*/
	DECL_PARAM_DWORD( 0x0020, 0 ),              /*0x0020 λ�û㱨����*/
	DECL_PARAM_DWORD( 0x0021, 1 ),              /*0x0021 λ�û㱨����*/
	DECL_PARAM_DWORD( 0x0022, 30 ),             /*0x0022 ��ʻԱδ��¼�㱨ʱ����*/
	DECL_PARAM_DWORD( 0x0027, 120 ),            /*0x0027 ����ʱ�㱨ʱ����*/
	DECL_PARAM_DWORD( 0x0028, 5 ),              /*0x0028 ��������ʱ�㱨ʱ����*/
	DECL_PARAM_DWORD( 0x0029, 30 ),             /*0x0029 ȱʡʱ��㱨���*/
	DECL_PARAM_DWORD( 0x002C, 500 ),            /*0x002c ȱʡ����㱨���*/
	DECL_PARAM_DWORD( 0x002D, 1000 ),           /*0x002d ��ʻԱδ��¼�㱨������*/
	DECL_PARAM_DWORD( 0x002E, 1000 ),           /*0x002e ����ʱ����㱨���*/
	DECL_PARAM_DWORD( 0x002F, 100 ),            /*0x002f ����ʱ����㱨���*/
	DECL_PARAM_DWORD( 0x0030, 5 ),              /*0x0030 �յ㲹���Ƕ�*/
	DECL_PARAM_DWORD( 0x0040, "1008611" ),      /*0x0040 ���ƽ̨�绰����*/
	DECL_PARAM_DWORD( 0x0041, "" ),             /*0x0041 ��λ�绰����*/
	DECL_PARAM_DWORD( 0x0042, "" ),             /*0x0042 �ָ��������õ绰����*/
	DECL_PARAM_DWORD( 0x0043, "" ),             /*0x0043 ���ƽ̨SMS����*/
	DECL_PARAM_DWORD( 0x0044, "" ),             /*0x0044 �����ն�SMS�ı���������*/
	DECL_PARAM_DWORD( 0x0045, 5 ),              /*0x0045 �ն˽����绰����*/
	DECL_PARAM_DWORD( 0x0046, 3 ),              /*0x0046 ÿ��ͨ��ʱ��*/
	DECL_PARAM_DWORD( 0x0047, 3 ),              /*0x0047 ����ͨ��ʱ��*/
	DECL_PARAM_DWORD( 0x0048, "" ),             /*0x0048 �����绰����*/
	DECL_PARAM_DWORD( 0x0049, "" ),             /*0x0049 ���ƽ̨��Ȩ���ź���*/
	DECL_PARAM_DWORD( 0x0050, 5 ),              /*0x0050 ����������*/
	DECL_PARAM_DWORD( 0x0051, 3 ),              /*0x0051 ���������ı�SMS����*/
	DECL_PARAM_DWORD( 0x0052, 5 ),              /*0x0052 �������տ���*/
	DECL_PARAM_DWORD( 0x0053, 3 ),              /*0x0053 ��������洢��־*/
	DECL_PARAM_DWORD( 0x0054, 5 ),              /*0x0054 �ؼ���־*/
	DECL_PARAM_DWORD( 0x0055, 3 ),              /*0x0055 ����ٶ�kmh*/
	DECL_PARAM_DWORD( 0x0056, 5 ),              /*0x0056 ���ٳ���ʱ��*/
	DECL_PARAM_DWORD( 0x0057, 3 ),              /*0x0057 ������ʻʱ������*/
	DECL_PARAM_DWORD( 0x0058, 5 ),              /*0x0058 �����ۼƼ�ʻʱ������*/
	DECL_PARAM_DWORD( 0x0059, 3 ),              /*0x0059 ��С��Ϣʱ��*/
	DECL_PARAM_DWORD( 0x005A, 5 ),              /*0x005A �ͣ��ʱ��*/
	DECL_PARAM_DWORD( 0x0070, 3 ),              /*0x0070 ͼ����Ƶ����(1-10)*/
	DECL_PARAM_DWORD( 0x0071, 5 ),              /*0x0071 ����*/
	DECL_PARAM_DWORD( 0x0072, 3 ),              /*0x0072 �Աȶ�*/
	DECL_PARAM_DWORD( 0x0073, 5 ),              /*0x0073 ���Ͷ�*/
	DECL_PARAM_DWORD( 0x0074, 3 ),              /*0x0074 ɫ��*/
	DECL_PARAM_DWORD( 0x0080, 5 ),              /*0x0080 ������̱����0.1km*/
	DECL_PARAM_DWORD( 0x0081, 3 ),              /*0x0081 ʡ��ID*/
	DECL_PARAM_DWORD( 0x0082, 5 ),              /*0x0082 ����ID*/
	DECL_PARAM_DWORD( 0x0083, "��O-00001" ),    /*0x0083 ����������*/
	DECL_PARAM_DWORD( 0x0084, 5 ),              /*0x0084 ������ɫ*/
};

#endif

JT808_PARAM jt808_param =
{
	0x13021600,                                 /*0x0000 �汾*/
	5,                                          /*0x0001 �������ͼ��*/
	5,                                          /*0x0002 TCPӦ��ʱʱ��*/
	3,                                          /*0x0003 TCP��ʱ�ش�����*/
	3,                                          /*0x0004 UDPӦ��ʱʱ��*/
	5,                                          /*0x0005 UDP��ʱ�ش�����*/
	3,                                          /*0x0006 SMS��ϢӦ��ʱʱ��*/
	5,                                          /*0x0007 SMS��Ϣ�ش�����*/
	"CMNET",                                    /*0x0010 ��������APN*/
	"",                                         /*0x0011 �û���*/
	"",                                         /*0x0012 ����*/
	"60.28.50.210",                             /*0x0013 ����������ַ*/
	"CMNET",                                    /*0x0014 ����APN*/
	"",                                         /*0x0015 �����û���*/
	"",                                         /*0x0016 ��������*/
	"www.google.com",                           /*0x0017 ���ݷ�������ַ��ip������*/
	9131,                                       /*0x0018 TCP�˿�*/
	5678,                                       /*0x0019 UDP�˿�*/
	0,                                          /*0x0020 λ�û㱨����*/
	1,                                          /*0x0021 λ�û㱨����*/
	30,                                         /*0x0022 ��ʻԱδ��¼�㱨ʱ����*/
	120,                                        /*0x0027 ����ʱ�㱨ʱ����*/
	5,                                          /*0x0028 ��������ʱ�㱨ʱ����*/
	30,                                         /*0x0029 ȱʡʱ��㱨���*/
	500,                                        /*0x002c ȱʡ����㱨���*/
	1000,                                       /*0x002d ��ʻԱδ��¼�㱨������*/
	1000,                                       /*0x002e ����ʱ����㱨���*/
	100,                                        /*0x002f ����ʱ����㱨���*/
	5,                                          /*0x0030 �յ㲹���Ƕ�*/
	"1008611",                                  /*0x0040 ���ƽ̨�绰����*/
	"",                                         /*0x0041 ��λ�绰����*/
	"",                                         /*0x0042 �ָ��������õ绰����*/
	"",                                         /*0x0043 ���ƽ̨SMS����*/
	"",                                         /*0x0044 �����ն�SMS�ı���������*/
	5,                                          /*0x0045 �ն˽����绰����*/
	3,                                          /*0x0046 ÿ��ͨ��ʱ��*/
	3,                                          /*0x0047 ����ͨ��ʱ��*/
	"",                                         /*0x0048 �����绰����*/
	"",                                         /*0x0049 ���ƽ̨��Ȩ���ź���*/
	5,                                          /*0x0050 ����������*/
	3,                                          /*0x0051 ���������ı�SMS����*/
	5,                                          /*0x0052 �������տ���*/
	3,                                          /*0x0053 ��������洢��־*/
	5,                                          /*0x0054 �ؼ���־*/
	3,                                          /*0x0055 ����ٶ�kmh*/
	5,                                          /*0x0056 ���ٳ���ʱ��*/
	3,                                          /*0x0057 ������ʻʱ������*/
	5,                                          /*0x0058 �����ۼƼ�ʻʱ������*/
	3,                                          /*0x0059 ��С��Ϣʱ��*/
	5,                                          /*0x005A �ͣ��ʱ��*/
	3,                                          /*0x0070 ͼ����Ƶ����(1-10)*/
	5,                                          /*0x0071 ����*/
	3,                                          /*0x0072 �Աȶ�*/
	5,                                          /*0x0073 ���Ͷ�*/
	3,                                          /*0x0074 ɫ��*/
	5,                                          /*0x0080 ������̱����0.1km*/
	3,                                          /*0x0081 ʡ��ID*/
	5,                                          /*0x0082 ����ID*/
	"��O-00001",                                /*0x0083 ����������*/
	1,                                          /*0x0084 ������ɫ*/
};

TERM_PARAM term_param =
{
	{ 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 },
	{ 'T',	'C',  'B',	'B',  'D' },
	{ 'T',	'W',  '7',	'0',  '1',	'-', 'B', 'D', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{ 0x00, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee},
};

#define FLAG_DISABLE_REPORT_INVALID 1       /*�豸�Ƿ�*/
#define FLAG_DISABLE_REPORT_AREA	2       /*�����ڽ�ֹ�ϱ�*/

static uint32_t flag_disable_report = 0;    /*��ֹ�ϱ��ı�־λ*/

/*���������serialflash*/
void param_save( void )
{
	rt_kprintf( "parma_save size=%d\r\n", sizeof( jt808_param ) );
	sst25_write_back( ADDR_PARAM, (uint8_t*)&jt808_param, sizeof( jt808_param ) );
}

/*���ز�����serialflash*/
void param_load( void )
{
	/*Ԥ��һ��������*/
	uint8_t		ver8[4];
	uint32_t	ver32;
	sst25_read( ADDR_PARAM, ver8, 4 );
	ver32 = ( ver8[0] << 24 ) | ( ver8[1] << 16 ) | ( ver8[2] << 8 ) | ( ver8[3] );
	rt_kprintf( "param_load ver=%08x\r\n", ver32 );
	if( jt808_param.id_0x0000 != ver32 ) /*�����ǲ���δ��ʼ��*/
	{
		rt_kprintf( "%s(%d)\r\n", __func__, __LINE__ );
		param_save( );
	}
	sst25_read( ADDR_PARAM, (uint8_t*)&jt808_param, sizeof( jt808_param ) );
}

/*��ӡ������Ϣ*/
void param_print( void )
{
	uint8_t tbl[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
	int		i, count = 0;
	uint8_t c;
	uint8_t *p = (uint8_t*)&jt808_param;
	uint8_t printbuf[70];
	int32_t len = sizeof( jt808_param );

	while( len > 0 )
	{
		count = ( len < 16 ) ? len : 16;
		memset( printbuf, 0x20, 70 );
		for( i = 0; i < count; i++ )
		{
			c					= *p;
			printbuf[i * 3]		= tbl[c >> 4];
			printbuf[i * 3 + 1] = tbl[c & 0x0f];
			if( c < 0x20 )
			{
				c = '.';
			}
			if( c > 0x7f )
			{
				c = '.';
			}
			printbuf[50 + i] = c;
			p++;
		}
		printbuf[69] = 0;
		rt_kprintf( "%s\r\n", printbuf );
		len -= count;
	}
}

FINSH_FUNCTION_EXPORT( param_print, print param );


/*
   $GNRMC,074001.00,A,3905.291037,N,11733.138255,E,0.1,,171212,,,A*655220.9*3F0E
   $GNTXT,01,01,01,ANTENNA OK*2B7,N,11733.138255,E,0.1,,171212,,,A*655220.9*3F0E
   $GNGGA,074002.00,3905.291085,N,11733.138264,E,1,11,0.9,8.2,M,-1.6,M,,,1.4*68E
   $GNGLL,3905.291085,N,11733.138264,E,074002.00,A,0*02.9,8.2,M,-1.6,M,,,1.4*68E
   $GPGSA,A,3,18,05,08,02,26,29,15,,,,,,,,,,,,,,,,,,,,,,,,,,1.6,0.9,1.4,0.9*3F8E
   $BDGSA,A,3,04,03,01,07,,,,,,,,,,,,,,,,,,,,,,,,,,,,,1.6,0.9,1.4,0.9*220.9*3F8E
   $GPGSV,2,1,7,18,10,278,29,05,51,063,32,08,21,052,24,02,24,140,45*4C220.9*3F8E
   $GPGSV,2,2,7,26,72,055,24,29,35,244,37,15,66,224,37*76,24,140,45*4C220.9*3F8E
   $BDGSV,1,1,4,04,27,124,38,03,42,190,34,01,38,146,37,07,34,173,35*55220.9*3F8E

   ���ش�����ֶ����������ȷ�Ļ�
 */
uint8_t process_rmc( uint8_t *pinfo )
{
	//�������������,ִ������ת��
	uint8_t		year = 0, mon = 0, day = 0, hour = 0, min = 0, sec = 0, fDateModify = 0;
	uint32_t	degrees, minutes;
	uint8_t		count;

	uint8_t		gps_time[10];
	uint8_t		gps_av	= 0;
	uint8_t		gps_ns	= 0;
	uint8_t		gps_ew	= 0;
	uint8_t		gps_latitude[16];
	uint8_t		gps_longitude[16];
	uint8_t		gps_speed[8];
	uint8_t		gps_direct[8];
	uint8_t		gps_date[8];

	uint8_t		*psrc = pinfo + 7; //ָ��ʼλ��
/*ʱ�䴦�� */
	count = 0;
	while( ( *psrc != ',' ) && ( count < 10 ) )
	{
		gps_time[count++]	= *psrc;
		gps_time[count]		= 0;
		psrc++;
	}
	if( ( count == 0 ) || ( count == 10 ) )
	{
		return 0;
	}
	hour	= ( gps_time[0] - 0x30 ) * 10 + ( gps_time[1] - 0x30 ) + 8;
	min		= ( gps_time[2] - 0x30 ) * 10 + ( gps_time[3] - 0x30 );
	sec		= ( gps_time[4] - 0x30 ) * 10 + ( gps_time[5] - 0x30 );
	if( hour > 23 )
	{
		fDateModify = 1;
		hour		-= 24;
	}
	gpsinfo.datetime[3] = ( ( hour / 10 ) << 4 ) | ( hour % 10 );
	gpsinfo.datetime[4] = ( ( min / 10 ) << 4 ) | ( min % 10 );
	gpsinfo.datetime[5] = ( ( sec / 10 ) << 4 ) | ( sec % 10 );
/*A_V����*/
	psrc++;
	if( ( *psrc == 'A' ) || ( *psrc == 'V' ) )
	{
		gps_av = *psrc;
	} else
	{
		return 1;
	}
	if( gps_av == 'A' )
	{
		jt808_status &= ~0x01;
	} else
	{
		jt808_status |= 0x01;
	}
/*γ�ȴ���ddmm.mmmmmm*/
	psrc++;
	count = 0;
	while( ( *psrc != ',' ) && ( count < 11 ) )
	{
		gps_latitude[count++]	= *psrc;
		gps_latitude[count]		= 0;
		psrc++;
	}
	if( count == 0 )
	{
		return 2;
	}
	degrees = ( ( gps_latitude[0] - 0x30 ) * 10 + ( gps_latitude[1] - 0x30 ) ) * 60 * 100000;
	minutes = ( gps_latitude[2] - 0x30 ) * 1000000 +
	          ( gps_latitude[3] - 0x30 ) * 100000 +
	          ( gps_latitude[5] - 0x30 ) * 10000 +
	          ( gps_latitude[6] - 0x30 ) * 1000 +
	          ( gps_latitude[7] - 0x30 ) * 100 +
	          ( gps_latitude[8] - 0x30 ) * 10 +
	          ( gps_latitude[9] - 0x30 );

	gpsinfo.latitude = ByteSwap4( degrees + minutes );

/*N_S����*/
	psrc++;
	if( ( *psrc == 'N' ) || ( *psrc == 'S' ) )
	{
		gps_ns = *psrc;
	} else
	{
		return 3;
	}
	if( gps_ns == 'N' )
	{
		jt808_status &= ~0x02;
	} else
	{
		jt808_status |= 0x02;
	}

/*���ȴ���*/
	psrc++;
	count = 0;
	while( ( *psrc != ',' ) && ( count < 12 ) )
	{
		gps_longitude[count++]	= *psrc;
		gps_longitude[count]	= 0;
		psrc++;
	}
	if( count == 0 )
	{
		return 4;
	}
	degrees = ( ( gps_latitude[0] - 0x30 ) * 100 + ( gps_latitude[1] - 0x30 ) * 10 + ( gps_latitude[2] - 0x30 ) ) * 60 * 100000;
	minutes = ( gps_latitude[3] - 0x30 ) * 1000000 +
	          ( gps_latitude[4] - 0x30 ) * 100000 +
	          ( gps_latitude[6] - 0x30 ) * 10000 +
	          ( gps_latitude[7] - 0x30 ) * 1000 +
	          ( gps_latitude[8] - 0x30 ) * 100 +
	          ( gps_latitude[9] - 0x30 ) * 10 +
	          ( gps_latitude[10] - 0x30 );
	gpsinfo.longitude = ByteSwap4( degrees + minutes );
/*N_S����*/
	psrc++;
	if( ( *psrc == 'E' ) || ( *psrc == 'W' ) )
	{
		gps_ew = *psrc;
	} else
	{
		return 5;
	}
	if( gps_ew == 'E' )
	{
		jt808_status &= ~0x04;
	} else
	{
		jt808_status |= 0x04;
	}

/*�ٶȴ���*/
	psrc++;
	count = 0;
	while( ( *psrc != ',' ) && ( count < 7 ) )
	{
		gps_speed[count++]	= *psrc;
		gps_speed[count]	= 0;
		psrc++;
	}
	if( count == 0 )
	{
		return 6;
	}

/*������*/
	psrc++;
	count = 0;
	while( ( *psrc != ',' ) && ( count < 7 ) )
	{
		gps_direct[count++] = *psrc;
		gps_direct[count]	= 0;
		psrc++;
	}
	if( count == 0 )
	{
		return 7;
	}

/*���ڴ���*/
	psrc++;
	count = 0;
	while( ( *psrc != ',' ) && ( count < 7 ) )
	{
		gps_date[count++]	= *psrc;
		gps_date[count]		= 0;
		psrc++;
	}
	if( count == 0 )
	{
		return 8;
	}
	day		= ( ( gps_date[0] - 0x30 ) * 10 ) + ( gps_date[1] - 0x30 );
	mon		= ( ( gps_date[2] - 0x30 ) * 10 ) + ( gps_date[3] - 0x30 );
	year	= ( ( gps_date[4] - 0x30 ) * 10 ) + ( gps_date[5] - 0x30 );

	if( fDateModify )
	{
		day++;
		if( mon == 2 )
		{
			if( ( year % 4 ) == 0 )
			{
				if( day == 30 )
				{
					day = 1; mon++;
				}
			}else if( day == 29 )
			{
				day = 1; mon++;
			}
		}else if( ( mon == 4 ) || ( mon == 6 ) || ( mon == 9 ) || ( mon == 11 ) )
		{
			if( day == 31 )
			{
				mon++; day = 1;
			}
		}else
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
	gpsinfo.datetime[0] = ( ( year / 10 ) << 4 ) | ( year % 10 );
	gpsinfo.datetime[1] = ( ( mon / 10 ) << 4 ) | ( mon % 10 );
	gpsinfo.datetime[2] = ( ( day / 10 ) << 4 ) | ( day % 10 );
	return 0;
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
void process_gga( uint8_t *pinfo )
{
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
void gps_analy( uint8_t * pinfo )
{
	uint16_t	len;
	uint8_t		*psrc;
	len		= ( pinfo[0] << 8 ) | pinfo[1];
	psrc	= pinfo + 2;
	if( ( strncmp( psrc, "$GPRMC,", 7 ) == 0 ) || ( strncmp( psrc, "$BDRMC,", 7 ) == 0 ) || ( strncmp( psrc, "$GNRMC,", 7 ) == 0 ) )
	{
		process_rmc( psrc );
	}
	if( ( strncmp( psrc, "$GPGGA,", 7 ) == 0 ) || ( strncmp( psrc, "$BDGGA,", 7 ) == 0 ) || ( strncmp( psrc, "$GNGGA,", 7 ) == 0 ) )
	{
		process_gga( psrc );
	}
}

/*
   jt808��ʽ���ݽ����ж�
   <��ʶ0x7e><��Ϣͷ><��Ϣ��><У����><��ʶ0x7e>

   ������Ч�����ݳ���,Ϊ0 �����д�

 */
static uint16_t jt808_decode_fcs( uint8_t *pinfo, uint16_t length )
{
	uint8_t		*psrc, *pdst;
	uint16_t	count, len;
	uint8_t		fstuff	= 0; /*�Ƿ��ֽ����*/
	uint8_t		fcs		= 0;

	if( length < 5 )
	{
		return 0;
	}
	if( *pinfo != 0x7e )
	{
		return 0;
	}
	if( *( pinfo + length - 1 ) != 0x7e )
	{
		return 0;
	}
	psrc	= pinfo + 1;    /*1byte��ʶ��Ϊ��ʽ��Ϣ*/
	pdst	= pinfo;
	count	= 0;            /*ת���ĳ���*/
	len		= length - 2;   /*ȥ����ʶλ�����ݳ���*/

	while( len )
	{
		if( fstuff )
		{
			*pdst	= *psrc + 0x7c;
			fstuff	= 0;
			count++;
			fcs ^= *pdst;
		}else
		{
			if( *psrc == 0x7d )
			{
				fstuff = 1;
			} else
			{
				*pdst	= *psrc;
				fcs		^= *pdst;
				count++;
			}
		}
		psrc++;
		pdst++;
		len--;
	}
	if( fcs != 0 )
	{
		rt_kprintf( "%s>fcs error\r\n", __func__ );
		return 0;
	}
	rt_kprintf( "count=%d\r\n", count );
	return count;
}

/*
   jt808�ն˷�����Ϣ
   ���������Ϣע�ᵽ������Ϣ�Ĵ����߳���
   ��Ҫ������ϢID,����Ϣ�壬��jt808_send�߳����
    ��Ϣ�����
    ���ͺ��ط�����
    ��ˮ��
    �ѷ���Ϣ�Ļ���free
   ���ݽ����ĸ�ʽ
   <msgid 2bytes><msg_len 2bytes><msgbody nbytes>

 */
static void jt808_send( void* parameter )
{
}

/*���ͺ��յ�Ӧ����*/
void jt808_tx_response( JT808_RX_MSG_NODEDATA* nodedata )
{
	uint8_t		* msg = nodedata->pmsg;
	uint16_t	id;
	uint16_t	seq;
	uint8_t		res;

	seq = ( *msg << 8 ) | *( msg + 1 );
	id	= ( *( msg + 2 ) << 8 ) | *( msg + 3 );
	res = *( msg + 4 );

	switch( id )        // �ж϶�Ӧ�ն���Ϣ��ID�����ִ���
	{
		case 0x0200:    //	��Ӧλ����Ϣ��Ӧ��
			rt_kprintf( "\r\nCentre ACK!\r\n" );
			break;
		case 0x0002:    //	��������Ӧ��
			rt_kprintf( "\r\n  Centre  Heart ACK!\r\n" );
			break;
		case 0x0101:    //	�ն�ע��Ӧ��
			break;
		case 0x0102:    //	�ն˼�Ȩ
			break;
		case 0x0800:    // ��ý���¼���Ϣ�ϴ�
			break;
		case 0x0702:
			rt_kprintf( "\r\n  ��ʻԱ��Ϣ�ϱ�---����Ӧ��!  \r\n" );
			break;
		case 0x0701:
			rt_kprintf( "\r\n	�����˵��ϱ�---����Ӧ��!  \r\n");
			break;
		default:
			rt_kprintf( "\r\nunknown id=%04x\r\n", id );
			break;
	}
}

/*
   ��Ϣ���ͳ�ʱ
 */
static rt_err_t jt808_tx_timeout( JT808_TX_MSG_NODEDATA* nodedata )
{
	rt_kprintf( "tx timeout\r\n" );
}

/*ͨ�ñ����䷢����Ϣ*/
static void handle_jt808_tx( const char *fmt, ... )
{
	uint8_t					*pdata;
	JT808_TX_MSG_NODEDATA	*pnodedata;
	uint8_t					buf[512];
	uint8_t					*p;
	uint8_t					encode_len	= 0; /*�����ĳ���*/
	uint8_t					fcs			= 0;

	va_list					args;
	rt_size_t				length;

	pnodedata = rt_malloc( sizeof( JT808_TX_MSG_NODEDATA ) );
	if( pnodedata == NULL )
	{
		return;
	}

	pnodedata->type		= TERMINAL_ACK; /*���ͼ�ɾ����������������*/
	pnodedata->state	= IDLE;

	va_start( args, fmt );
	length = vsnprintf( buf, sizeof( buf ), fmt, args );
	va_end( args );

	tx_seq++;
}

/*
   �ն�ͨ��Ӧ��
 */
static void handle_jt808_tx_0x0001( uint16_t seq, uint16_t id, uint8_t res )
{
	uint8_t					*pdata;
	JT808_TX_MSG_NODEDATA	*pnodedata;
	uint8_t					buf[64];
	uint8_t					*p;
	uint16_t				len;

	pnodedata = rt_malloc( sizeof( JT808_TX_MSG_NODEDATA ) );
	if( pnodedata == NULL )
	{
		return;
	}

	pnodedata->type		= TERMINAL_ACK; /*���ͼ�ɾ����������������*/
	pnodedata->state	= IDLE;

	len		= jt808_pack( buf, "\x00\x01\x00\x05%6s%w%w%w%b", term_param.mobile, tx_seq, seq, id, res );
	pdata	= rt_malloc( len );
	if( pdata == NULL )
	{
		rt_free( pnodedata );
		return;
	}
	memcpy( pdata, buf, len );
	pnodedata->msg_len	= len;
	pnodedata->pmsg		= pdata;
	msglist_append( list_jt808_tx, pnodedata );
	tx_seq++;
}

/*ƽ̨ͨ��Ӧ��,�յ���Ϣ��ֹͣ����*/
static int handle_jt808_rx_0x8001( JT808_RX_MSG_NODEDATA* nodedata )
{
	MsgListNode				* iter;
	JT808_TX_MSG_NODEDATA	*iterdata;
	MsgListNode				* iter_tmp;

	uint8_t					* msg = nodedata->pmsg;
	uint16_t				id;
	uint16_t				seq;
	uint8_t					res;

	seq = ( *msg << 8 ) | *( msg + 1 );
	id	= ( *( msg + 2 ) << 8 ) | *( msg + 3 );
	res = *( msg + 4 );

	/*��������*/
	iter = list_jt808_tx->first;
	if( ( iterdata->head_id == id ) && ( iterdata->head_sn == seq ) )
	{
		iterdata->cb_tx_response( nodedata );
		iterdata->state = ACK_OK;
	}
}

/* ������Ķ��ն�ע����Ϣ��Ӧ��*/
static int handle_jt808_rx_0x8100( JT808_RX_MSG_NODEDATA* nodedata )
{
	MsgListNode				* iter;
	JT808_TX_MSG_NODEDATA	*iterdata;

	uint16_t				ack_seq;
	uint8_t					res;
	uint8_t					* msg;

	msg		= nodedata->pmsg;
	ack_seq = ( *msg << 8 ) | *( msg + 1 );
	res		= *( msg + 2 );

	iter		= list_jt808_tx->first;
	iterdata	= iter->data;
	if( ( iterdata->head_id == 0x0100 ) && ( iterdata->head_sn == ack_seq ) )
	{
		rt_kprintf( "\r\n%s(%d)>res=%d\r\n", __func__, __LINE__, res );
		if( res == 0 )
		{
			strncpy( term_param.register_code, msg + 3, nodedata->msg_len );
			iterdata->state = ACK_OK;
		}
	}
	return 1;
}

#if 0


/*������Ӧ�Ĳ��Һ�����
   ռ�ÿռ� 0x120*2=288*2=576�ֽ�

   ����Ϊ���λ��Ϊ1
   bit 15 14 13..0
     0  0 xxxx     DWORD
     0  1 xxxx     BYTE
     1  0 xxxx     WORD
     1  1 xxxx     STRING
 */

const uint16_t tbl_id_index[] =
{
/*-����      0        1       2        3       4       5       6       7        8       9       a       b       c       d       e       f   */
/*0x0000*/ 0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD,
/*0x0010*/ 0xC008, 0xC009, 0xC00A, 0xC00B, 0xC00C, 0xC00D, 0xC00E, 0xC00F, 0x0010, 0x0011, 0xC012, 0x0013, 0x0014, 0xC015, 0xDEAD, 0xDEAD,
/*0x0020*/ 0x0016, 0x0017, 0x0018, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0x0019, 0x001A, 0x001B, 0xDEAD, 0xDEAD, 0x001C, 0x001D, 0x001E, 0x001F,
/*0x0030*/ 0x0020, 0x8021, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD,
/*0x0040*/ 0xC022, 0xC023, 0xC024, 0xC025, 0xC026, 0x0027, 0x0028, 0x0029, 0xC02A, 0xC02B, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD,
/*0x0050*/ 0x002C, 0x002D, 0x002E, 0x002F, 0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x8037, 0x8038, 0x8039, 0x803A, 0xDEAD,
/*0x0060*/ 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0x003B, 0x003C, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD,
/*0x0070*/ 0x003D, 0x003E, 0x003F, 0x0040, 0x0041, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD,
/*0x0080*/ 0x0042, 0x8043, 0x8044, 0xC045, 0x4046, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD,
/*0x0090*/ 0x4047, 0x4048, 0x4049, 0x404A, 0x404B, 0x404C, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD,
/*0x00a0*/ 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD,
/*0x00b0*/ 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD,
/*0x00c0*/ 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD,
/*0x00d0*/ 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD,
/*0x00e0*/ 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD,
/*0x00f0*/ 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD,
/*0x0100*/ 0x004D, 0x804E, 0x004F, 0xC050, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD, 0xDEAD,
/*0x0110*/ 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005A, 0x005B, 0x005C, 0x005D, 0x005E, 0x005F, 0x0060,
};

/*����id�����������е�λ��*/
static uint16_t param_id_to_index( uint16_t id )
{
	return tbl_id_index[id] & 0x7FF;
}

#endif


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static void jt808_param_save_int( uint16_t id, uint32_t val )
{
}

/*
   #define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
   #define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})
   #define container(ptr, type, member) (type *)( (char *)ptr - offsetof(type,member) )
 */

__packed struct _param {
/*0x0000*/
	uint32_t	ver;
	uint32_t	heartbeat;                  /*0x0001 �������ͼ��*/
	uint32_t	tcp_ack_timeout;            /*0x0002 TCPӦ��ʱʱ��*/
	uint32_t	tcp_retry;                  /*0x0003 TCP��ʱ�ش�����*/
	uint32_t	udp_ack_timeout;            /*0x0004  UDPӦ��ʱʱ��*/
	uint32_t	udp_retry;                  /*0x0005 UDP��ʱ�ش�����*/
	uint32_t	sms_ack_timeout;            /*0x0006 SMS��ϢӦ��ʱʱ��*/
	uint32_t	sms_retry;                  /*0x0007 SMS��Ϣ�ش�����*/
/*0x0010*/	
	char		main_apn[32];               /*0x0010 ��������APN*/
	char		main_user[32];              /*0x0011 �û���*/
	char		main_psw[32];               /*0x0012 ����*/
	char		main_ip_domain[32];         /*0x0013 ����������ַ*/
	char		backup_apn[32];             /*0x0014 ����APN*/
	char		backup_user[32];            /*0x0015 �����û���*/
	char		backup_psw[32];             /*0x0016 ��������*/
	char		backup_ip_domain[32];       /*0x0017 ���ݷ�������ַ��ip������*/
	uint32_t	tcp_port;                   /*0x0018 TCP�˿�*/
	uint32_t	udp_port;                   /*0x0019 UDP�˿�*/
	char		ic_ip_domain[32];
	uint32_t	ic_tcp_port;
	uint32_t	ic_udp_port;
	char		ic_backup_ip_domain[32];
/*0x0020*/	
	uint32_t	report_strategy;            /*0x0020 λ�û㱨����*/
	uint32_t	report_scheme;              /*0x0021 λ�û㱨����*/
	uint32_t	report_intervel_logout;     /*0x0022 ��ʻԱδ��¼�㱨ʱ����*/
	uint32_t	report_intervel_sleep;      /*0x0027 ����ʱ�㱨ʱ����*/
	uint32_t	report_intervel_emergency;  /*0x0028 ��������ʱ�㱨ʱ����*/
	uint32_t	report_intervel_default;    /*0x0029 ȱʡʱ��㱨���*/
	uint32_t	report_distance_default;    /*0x002c ȱʡ����㱨���*/
	uint32_t	report_distance_logout;     /*0x002d ��ʻԱδ��¼�㱨������*/
	uint32_t	report_distance_sleep;      /*0x002e ����ʱ����㱨���*/
	uint32_t	report_distance_emergency;  /*0x002f ����ʱ����㱨���*/
	uint32_t	knee_point_angle;           /*0x0030 �յ㲹���Ƕ�*/
	uint16_t	elect_rail_radius;
/*0x0040*/	
	char		*telnumber_monitor;         /*0x0040 ���ƽ̨�绰����*/
	char		*telnumber_reset;           /*0x0041 ��λ�绰����*/
	char		*telnumber_restore;         /*0x0042 �ָ��������õ绰����*/
	char		*telnumber_monitor_sms;     /*0x0043 ���ƽ̨SMS����*/
	char		*telnumber_receiver_sms;    /*0x0044 �����ն�SMS�ı���������*/
	uint32_t	dialin_strategy;            /*0x0045 �ն˽����绰����*/
	uint32_t	call_duration;              /*0x0046 ÿ��ͨ��ʱ��*/
	uint32_t	month_call_duration;        /*0x0047 ����ͨ��ʱ��*/
	char		*telnumber_admin;           /*0x0048 �����绰����*/
	char		*smsnumber_admin;           /*0x0049 ���ƽ̨��Ȩ���ź���*/
/*0x0050*/
	uint32_t	alarm_mask;                 /*0x0050 ����������*/
	uint32_t	alarm_sms_mask;             /*0x0051 ���������ı�SMS����*/
	uint32_t	alarm_cam_mask;             /*0x0052 �������տ���*/
	uint32_t	alarm_storage_mask;         /*0x0053 ��������洢��־*/
	uint32_t	alarm_key_mask;             /*0x0054 �ؼ���־*/
	uint32_t	speed_limit_high;           /*0x0055 ����ٶ�kmh*/
	uint32_t	speed_duration;             /*0x0056 ���ٳ���ʱ��*/
	uint32_t	continue_drive_limit;       /*0x0057 ������ʻʱ������*/
	uint32_t	day_drive_limit;            /*0x0058 �����ۼƼ�ʻʱ������*/
	uint32_t	rest_duration_min;          /*0x0059 ��С��Ϣʱ��*/
	uint32_t	park_duration_max;          /*0x005A �ͣ��ʱ��*/
	uint16_t	overspeed;
	uint16_t	tired_drive;
	uint16_t	collision_param;
	uint16_t	tilt_param;
	__packed union {
		uint32_t data32;
		__packed struct bit_def {
			char		cam1_photo : 1;
			char		cam2_photo : 1;
			char		cam3_photo : 1;
			char		cam4_photo : 1;
			char		cam5_photo : 1;
			char		reserved1 : 3;
			char		cam1_save : 1;
			char		cam2_save : 1;
			char		cam3_save : 1;
			char		cam4_save : 1;
			char		cam5_save : 1;
			char		reserved2 : 3;
			char		time_unit : 1;
			uint16_t	time_interval : 15;
		} bit;
	}			camera_time_control;
	uint32_t	camera_distance_control;
/*0x0070*/	
	uint32_t	camera_quality;     /*0x0070 ͼ����Ƶ����(1-10)*/
	uint32_t	camera_brightness;  /*0x0071 ����*/
	uint32_t	camera_contrast;    /*0x0072 �Աȶ�*/
	uint32_t	camera_saturation;  /*0x0073 ���Ͷ�*/
	uint32_t	camera_colourity;   /*0x0074 ɫ��*/
/*0x0080*/	
	uint32_t	odometer;           /*0x0080 ������̱����0.1km*/
	uint16_t	id_province;        /*0x0081 ʡ��ID*/
	uint16_t	id_city;            /*0x0082 ����ID*/
	char		* vehicle_number;   /*0x0083 ����������*/
	uint8_t		vehicle_color;      /*0x0084 ������ɫ*/
/*0x0090*/	
	uint8_t		gnss_mode;
	uint8_t		gnss_baud;
	uint8_t		gnss_freq_out;
	uint32_t	gnss_freq_sampler;
	uint8_t		gnss_detail_report_mode;
	uint32_t	gnss_detail_report_unit;
/*0x0100*/
	uint32_t	can_1_sample_interval;
	uint16_t	can_1_report_interval;
	uint32_t	can_2_sample_interval;
	uint16_t	can_2_report_interval;
/*0x0110*/
	uint8_t		can_id_setup[128 * 8];
} param = {
/*0x0000*/
	0x13022200,30, 5, 3, 15, 3, 30, 3,
/*0x0010*/	
	"CMNET",		"",	  "",	"60.28.50.210",
	"CMNET",		"",	  "",	"www.google.com",
	9131,			5678,
	"60.28.50.210", 9131, 5678, "www.ic.ip",
/*0x0020*/	
	0,		0, 30,
	180,	5,	   30,
	100,	200,   1000,	100,
	270,	100,
/*0x0040*/
	"10086","10086","10086","10086","10086",
	0,300,6000,
	"10086","10086",
/*0x0050*/	
	0xffffffff, 0xffffffff, 0xffffffff,	 0xffffffff,   0xffffffff,
	90,			120,		4 * 60 * 60, 12 * 60 * 60, 20 * 60,	  4 * 60 * 60,
	50,			30,	
	0x2040,		15,
	0x0,		0x0,
/*0x0070*/
	5,			128,		64,			 64,		   128,
/*0x0080*/	
	0, 0x02, 0x03, "��O-00001", 0,
/*0x0090*/	
	0x0f, 0x01, 0x01, 1, 0x01, 30,
/*0x0100*/
	200, 10, 200, 10
/*0x0110*/

};

/*������1*/
__packed struct _param_block_0x0001
{
	uint32_t	heartbeat;          /*0x0001 �������ͼ��*/
	uint32_t	tcp_ack_timeout;    /*0x0002 TCPӦ��ʱʱ��*/
	uint32_t	tcp_retry;          /*0x0003 TCP��ʱ�ش�����*/
	uint32_t	udp_ack_timeout;    /*0x0004  UDPӦ��ʱʱ��*/
	uint32_t	udp_retry;          /*0x0005 UDP��ʱ�ش�����*/
	uint32_t	sms_ack_timeout;    /*0x0006 SMS��ϢӦ��ʱʱ��*/
	uint32_t	sms_retry;          /*0x0007 SMS��Ϣ�ش�����*/
} param_block_1 = { 30, 5, 3, 15, 3, 30, 3 };


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
uint32_t param_block_1_get_int( uint16_t id )
{
	switch( id )
	{
		case 0: return param_block_1.heartbeat;
		case 1: return param_block_1.tcp_ack_timeout;
		case 2: return param_block_1.tcp_retry;
		case 3: return param_block_1.udp_ack_timeout;
		case 4: return param_block_1.udp_retry;
		case 5: return param_block_1.sms_ack_timeout;
		case 6: return param_block_1.sms_retry;
		default:
			return 0;
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
void param_block_1_put_int( uint16_t id, uint32_t val )
{
	switch( id )
	{
		case 0: param_block_1.heartbeat			= val; break;
		case 1: param_block_1.tcp_ack_timeout	= val; break;
		case 2: param_block_1.tcp_retry			= val; break;
		case 3: param_block_1.udp_ack_timeout	= val; break;
		case 4: param_block_1.udp_retry			= val; break;
		case 5: param_block_1.sms_ack_timeout	= val; break;
		case 6: param_block_1.sms_retry			= val; break;
		default: break;
	}
}

__packed struct _param_block_0x0010
{
	char		main_apn[32];           /*0x0010 ��������APN*/
	char		main_user[32];          /*0x0011 �û���*/
	char		main_psw[32];           /*0x0012 ����*/
	char		main_ip_domain[32];     /*0x0013 ����������ַ*/
	char		backup_apn[32];         /*0x0014 ����APN*/
	char		backup_user[32];        /*0x0015 �����û���*/
	char		backup_psw[32];         /*0x0016 ��������*/
	char		backup_ip_domain[32];   /*0x0017 ���ݷ�������ַ��ip������*/
	uint32_t	tcp_port;               /*0x0018 TCP�˿�*/
	uint32_t	udp_port;               /*0x0019 UDP�˿�*/
	char		ic_ip_domain[32];
	uint32_t	ic_tcp_port;
	uint32_t	ic_udp_port;
	char		ic_backup_ip_domain[32];
} param_block_2 =
{
	"CMNET",		"",	  "",	"60.28.50.210",
	"CMNET",		"",	  "",	"www.google.com",
	9131,			5678,
	"60.28.50.210", 9131, 5678, "www.ic.ip"
};


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
uint32_t param_block_2_get_int( uint16_t id )
{
	switch( id )
	{
		case 0x0018: return param_block_2.tcp_port;
		case 0x0019: return param_block_2.udp_port;
		case 0x001b: return param_block_2.ic_tcp_port;
		case 0x001c: return param_block_2.ic_udp_port;
		default:
			return 0;
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
void param_block_2_put_int( uint16_t id, uint32_t val )
{
	switch( id )
	{
		case 0x0018: param_block_2.tcp_port		= val; break;
		case 0x0019: param_block_2.udp_port		= val; break;
		case 0x001b: param_block_2.ic_tcp_port	= val; break;
		case 0x001c: param_block_2.ic_udp_port	= val; break;
		default: break;
	}
}

__packed struct _param_block_0x0020
{
	uint32_t	report_strategy;            /*0x0020 λ�û㱨����*/
	uint32_t	report_scheme;              /*0x0021 λ�û㱨����*/
	uint32_t	report_intervel_logout;     /*0x0022 ��ʻԱδ��¼�㱨ʱ����*/
	uint32_t	reserved_0x0023;
	uint32_t	reserved_0x0024;
	uint32_t	reserved_0x0025;
	uint32_t	reserved_0x0026;
	uint32_t	report_intervel_sleep;      /*0x0027 ����ʱ�㱨ʱ����*/
	uint32_t	report_intervel_emergency;  /*0x0028 ��������ʱ�㱨ʱ����*/
	uint32_t	report_intervel_default;    /*0x0029 ȱʡʱ��㱨���*/
	uint32_t	reserved_0x002A;
	uint32_t	reserved_0x002B;
	uint32_t	report_distance_default;    /*0x002c ȱʡ����㱨���*/
	uint32_t	report_distance_logout;     /*0x002d ��ʻԱδ��¼�㱨������*/
	uint32_t	report_distance_sleep;      /*0x002e ����ʱ����㱨���*/
	uint32_t	report_distance_emergency;  /*0x002f ����ʱ����㱨���*/
	uint32_t	knee_point_angle;           /*0x0030 �յ㲹���Ƕ�*/
	uint16_t	elect_rail_radius;
} param_block_3 =
{
	0,		0,
	30,
	0xdead, 0xdead,0xdead,	0xdead,
	180,	5,	   30,
	0xdead, 0xdead,
	100,	200,   1000,	100,
	270,
	100,
};

__packed struct _param_block_0x0040
{
	char		*telnumber_monitor;         /*0x0040 ���ƽ̨�绰����*/
	char		*telnumber_reset;           /*0x0041 ��λ�绰����*/
	char		*telnumber_restore;         /*0x0042 �ָ��������õ绰����*/
	char		*telnumber_monitor_sms;     /*0x0043 ���ƽ̨SMS����*/
	char		*telnumber_receiver_sms;    /*0x0044 �����ն�SMS�ı���������*/
	uint32_t	dialin_strategy;            /*0x0045 �ն˽����绰����*/
	uint32_t	call_duration;              /*0x0046 ÿ��ͨ��ʱ��*/
	uint32_t	month_call_duration;        /*0x0047 ����ͨ��ʱ��*/
	char		*telnumber_admin;           /*0x0048 �����绰����*/
	char		*smsnumber_admin;           /*0x0049 ���ƽ̨��Ȩ���ź���*/
} param_block_4 =
{
	"10086",
	"10086",
	"10086",
	"10086",
	"10086",
	0,		300,6000,
	"10086",
	"10086",
};

__packed struct _param_block_0x0050
{
	uint32_t	alarm_mask;             /*0x0050 ����������*/
	uint32_t	alarm_sms_mask;         /*0x0051 ���������ı�SMS����*/
	uint32_t	alarm_cam_mask;         /*0x0052 �������տ���*/
	uint32_t	alarm_storage_mask;     /*0x0053 ��������洢��־*/
	uint32_t	alarm_key_mask;         /*0x0054 �ؼ���־*/
	uint32_t	speed_limit_high;       /*0x0055 ����ٶ�kmh*/
	uint32_t	speed_duration;         /*0x0056 ���ٳ���ʱ��*/
	uint32_t	continue_drive_limit;   /*0x0057 ������ʻʱ������*/
	uint32_t	day_drive_limit;        /*0x0058 �����ۼƼ�ʻʱ������*/
	uint32_t	rest_duration_min;      /*0x0059 ��С��Ϣʱ��*/
	uint32_t	park_duration_max;      /*0x005A �ͣ��ʱ��*/
	uint16_t	overspeed;
	uint16_t	tired_drive;
	uint16_t	collision_param;
	uint16_t	tilt_param;
	uint32_t	reserved_0x005F;
	uint32_t	reserved_0x0060;
	uint32_t	reserved_0x0061;
	uint32_t	reserved_0x0062;
	uint32_t	reserved_0x0063;
	__packed union {
		uint32_t data32;
		__packed struct  {
			char		cam1_photo : 1;
			char		cam2_photo : 1;
			char		cam3_photo : 1;
			char		cam4_photo : 1;
			char		cam5_photo : 1;
			char		reserved1 : 3;
			char		cam1_save : 1;
			char		cam2_save : 1;
			char		cam3_save : 1;
			char		cam4_save : 1;
			char		cam5_save : 1;
			char		reserved2 : 3;
			char		time_unit : 1;
			uint16_t	time_interval : 15;
		} bit;
	}			camera_time_control;
	uint32_t	camera_distance_control;
	uint32_t	reserved_0x0066;
	uint32_t	reserved_0x0067;
	uint32_t	reserved_0x0068;
	uint32_t	reserved_0x0069;
	uint32_t	reserved_0x006A;
	uint32_t	reserved_0x006B;
	uint32_t	reserved_0x006C;
	uint32_t	reserved_0x006D;
	uint32_t	reserved_0x006E;
	uint32_t	reserved_0x006F;
	uint32_t	camera_quality;     /*0x0070 ͼ����Ƶ����(1-10)*/
	uint32_t	camera_brightness;  /*0x0071 ����*/
	uint32_t	camera_contrast;    /*0x0072 �Աȶ�*/
	uint32_t	camera_saturation;  /*0x0073 ���Ͷ�*/
	uint32_t	camera_colourity;   /*0x0074 ɫ��*/
} param_block_5 =
{
	0xffffffff, 0xffffffff, 0xffffffff,	 0xffffffff,   0xffffffff,
	90,			120,		4 * 60 * 60, 12 * 60 * 60, 20 * 60,	  4 * 60 * 60,
	50,			30,
	0x2040,		15,
	0xdead,		0xdead,		0xdead,		 0xdead,	   0xdead,
	0x0,		0x0,
	0xdead,		0xdead,		0xdead,		 0xdead,	   0xdead,	  0xdead, 0xdead,0xdead, 0xdead, 0xdead,
	5,			128,		64,			 64,		   128
};

__packed struct _param_block_0x0080
{
	uint32_t	odometer;           /*0x0080 ������̱����0.1km*/
	uint16_t	id_province;        /*0x0081 ʡ��ID*/
	uint16_t	id_city;            /*0x0082 ����ID*/
	char		* vehicle_number;   /*0x0083 ����������*/
	uint8_t		vehicle_color;      /*0x0084 ������ɫ*/
} param_block_6 =
{
	0, 0x02, 0x03, "��O-00001", 0
};

__packed struct _param_block_0x0090
{
	uint8_t		gnss_mode;
	uint8_t		gnss_baud;
	uint8_t		gnss_freq_out;
	uint32_t	gnss_freq_sampler;
	uint8_t		gnss_detail_report_mode;
	uint32_t	gnss_detail_report_unit;
} param_block_7 =
{
	0x0f, 0x01, 0x01, 1, 0x01, 30
};

__packed struct _param_block_0x0100
{
	uint32_t	can_1_sample_interval;
	uint16_t	can_1_report_interval;
	uint32_t	can_2_sample_interval;
	uint16_t	can_2_report_interval;
	uint32_t	reserved_0x0104;
	uint32_t	reserved_0x0105;
	uint32_t	reserved_0x0106;
	uint32_t	reserved_0x0107;
	uint32_t	reserved_0x0108;
	uint32_t	reserved_0x0109;
	uint32_t	reserved_0x010A;
	uint32_t	reserved_0x010B;
	uint32_t	reserved_0x010C;
	uint32_t	reserved_0x010D;
	uint32_t	reserved_0x010E;
	uint32_t	reserved_0x010F;
	uint8_t		can_id_setup[128 * 8];
} param_block_8 =
{
	200, 10, 200, 10
};


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
uint32_t param_get_int( uint16_t id )
{
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
char* param_get_string( uint16_t id )
{
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
rt_err_t param_get_array( uint16_t id, uint8_t buf, uint8_t count )
{
}

/*�����ն˲���*/
static int handle_jt808_rx_0x8103( JT808_RX_MSG_NODEDATA* nodedata )
{
	uint8_t		* msg;
	uint8_t		* p;

	uint8_t		param_count;

	uint16_t	msg_len, count = 0;

	uint32_t	id;
	uint8_t		len;

	msg			= nodedata->pmsg;
	msg_len		= nodedata->msg_len - 1;
	param_count = *msg;
	p			= msg + 1;
/*ʹ�����ݳ���,�ж������Ƿ������û��ʹ�ò�������*/
	while( count < msg_len )
	{
		id		= ( ( *p++ ) << 24 ) | ( ( *p++ ) << 16 ) | ( ( *p++ ) << 8 ) | ( *p++ );
		len		= *p++;
		count	+= ( 5 + len );
		if( id > 0 && id < 8 )
		{
		}else if( id > 0x0f && id < 0x1e )
		{
		}
	}
	return 1;
}

/*��ѯ�ն˲���*/
static int handle_jt808_rx_0x8104( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/*�ն˿���*/
static int handle_jt808_rx_0x8105( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/**/
static int handle_jt808_rx_0x8201( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/**/
static int handle_jt808_rx_0x8202( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/**/
static int handle_jt808_rx_0x8300( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/**/
static int handle_jt808_rx_0x8301( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/**/
static int handle_jt808_rx_0x8302( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/**/
static int handle_jt808_rx_0x8303( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/**/
static int handle_jt808_rx_0x8304( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/**/
static int handle_jt808_rx_0x8400( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/**/
static int handle_jt808_rx_0x8401( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/**/
static int handle_jt808_rx_0x8500( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/**/
static int handle_jt808_rx_0x8600( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/**/
static int handle_jt808_rx_0x8601( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/**/
static int handle_jt808_rx_0x8602( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/**/
static int handle_jt808_rx_0x8603( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/**/
static int handle_jt808_rx_0x8604( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/**/
static int handle_jt808_rx_0x8605( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/**/
static int handle_jt808_rx_0x8606( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/**/
static int handle_jt808_rx_0x8607( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/**/
static int handle_jt808_rx_0x8700( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/**/
static int handle_jt808_rx_0x8701( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/**/
static int handle_jt808_rx_0x8800( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/**/
static int handle_jt808_rx_0x8801( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/**/
static int handle_jt808_rx_0x8802( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/**/
static int handle_jt808_rx_0x8803( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/**/
static int handle_jt808_rx_0x8804( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/**/
static int handle_jt808_rx_0x8805( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/**/
static int handle_jt808_rx_0x8900( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/**/
static int handle_jt808_rx_0x8A00( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
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
static int handle_jt808_rx_default( JT808_RX_MSG_NODEDATA* nodedata )
{
	rt_kprintf( "\r\nunknown!\r\n" );
	return 1;
}

#define DECL_JT808_RX_HANDLE( a )	{ a, handle_jt808_rx_ ## a }
#define DECL_JT808_TX_HANDLE( a )	{ a, handle_jt808_tx_ ## a }

HANDLE_JT808_RX_MSG handle_jt808_rx_msg[] =
{
	DECL_JT808_RX_HANDLE( 0x8001 ), //	ͨ��Ӧ��
	DECL_JT808_RX_HANDLE( 0x8100 ), //  ������Ķ��ն�ע����Ϣ��Ӧ��
	DECL_JT808_RX_HANDLE( 0x8103 ), //	�����ն˲���
	DECL_JT808_RX_HANDLE( 0x8104 ), //	��ѯ�ն˲���
	DECL_JT808_RX_HANDLE( 0x8105 ), // �ն˿���
	DECL_JT808_RX_HANDLE( 0x8201 ), // λ����Ϣ��ѯ    λ����Ϣ��ѯ��Ϣ��Ϊ��
	DECL_JT808_RX_HANDLE( 0x8202 ), // ��ʱλ�ø��ٿ���
	DECL_JT808_RX_HANDLE( 0x8300 ), //	�ı���Ϣ�·�
	DECL_JT808_RX_HANDLE( 0x8301 ), //	�¼�����
	DECL_JT808_RX_HANDLE( 0x8302 ), // �����·�
	DECL_JT808_RX_HANDLE( 0x8303 ), //	��Ϣ�㲥�˵�����
	DECL_JT808_RX_HANDLE( 0x8304 ), //	��Ϣ����
	DECL_JT808_RX_HANDLE( 0x8400 ), //	�绰�ز�
	DECL_JT808_RX_HANDLE( 0x8401 ), //	���õ绰��
	DECL_JT808_RX_HANDLE( 0x8500 ), //	��������
	DECL_JT808_RX_HANDLE( 0x8600 ), //	����Բ������
	DECL_JT808_RX_HANDLE( 0x8601 ), //	ɾ��Բ������
	DECL_JT808_RX_HANDLE( 0x8602 ), //	���þ�������
	DECL_JT808_RX_HANDLE( 0x8603 ), //	ɾ����������
	DECL_JT808_RX_HANDLE( 0x8604 ), //	���������
	DECL_JT808_RX_HANDLE( 0x8605 ), //	ɾ���������
	DECL_JT808_RX_HANDLE( 0x8606 ), //	����·��
	DECL_JT808_RX_HANDLE( 0x8607 ), //	ɾ��·��
	DECL_JT808_RX_HANDLE( 0x8700 ), //	�г���¼�����ݲɼ�����
	DECL_JT808_RX_HANDLE( 0x8701 ), //	��ʻ��¼�ǲ����´�����
	DECL_JT808_RX_HANDLE( 0x8800 ), //	��ý�������ϴ�Ӧ��
	DECL_JT808_RX_HANDLE( 0x8801 ), //	����ͷ��������
	DECL_JT808_RX_HANDLE( 0x8802 ), //	�洢��ý�����ݼ���
	DECL_JT808_RX_HANDLE( 0x8803 ), //	�洢��ý�������ϴ�����
	DECL_JT808_RX_HANDLE( 0x8804 ), //	¼����ʼ����
	DECL_JT808_RX_HANDLE( 0x8805 ), //	�����洢��ý�����ݼ����ϴ����� ---- ����Э��Ҫ��
	DECL_JT808_RX_HANDLE( 0x8900 ), //	��������͸��
	DECL_JT808_RX_HANDLE( 0x8A00 ), //	ƽ̨RSA��Կ
};


/*
   ���մ���
   ����jt808��ʽ������
   <linkno><����2byte><��ʶ0x7e><��Ϣͷ><��Ϣ��><У����><��ʶ0x7e>

 */
uint16_t jt808_rx_proc( uint8_t * pinfo )
{
	uint8_t					*psrc;
	uint16_t				len;
	uint8_t					linkno;
	uint16_t				i;
	uint8_t					flag_find = 0;

	uint16_t				ret;

	MsgListNode				* node;
	JT808_RX_MSG_NODEDATA	* nodedata;

	MsgListNode				* iter;
	JT808_RX_MSG_NODEDATA	* iterdata;

	MsgListNode				* iter_tmp;
	JT808_RX_MSG_NODEDATA	* iterdata_tmp;

	linkno	= pinfo[0];
	len		= ( pinfo[1] << 8 ) | pinfo[2];
	rt_kprintf( ">dump start len=%d\r\n", len );
	psrc = pinfo + 3;
	for( i = 0; i < len; i++ )
	{
		rt_kprintf( "%02x ", *psrc++ );
	}
	rt_kprintf( "\r\n>dump end\r\n" );

	len = jt808_decode_fcs( pinfo + 3, len );
	if( len == 0 )                            /*��ʽ����ȷ*/
	{
		rt_kprintf( ">len=0\r\n" );
		rt_free( pinfo );
		return 1;
	}

/*���յ�����Ϣ���н���,��ʱ�Խ���ת�嵽pinfo+3*/
	nodedata = rt_malloc( sizeof( JT808_RX_MSG_NODEDATA ) );
	if( nodedata == RT_NULL )                       /*�޷��������Ϣ*/
	{
		rt_free( pinfo );
		return 1;
	}

	psrc				= pinfo;                    /*ע�⿪ʼ��linkno len*/
	nodedata->linkno	= linkno;
	nodedata->id		= ( *( psrc + 3 ) << 8 ) | *( psrc + 4 );
	nodedata->attr		= ( *( psrc + 5 ) << 8 ) | *( psrc + 6 );
	memcpy( nodedata->mobileno, psrc + 7, 6 );
	nodedata->seq		= ( *( psrc + 13 ) << 8 ) | *( psrc + 14 );
	nodedata->msg_len	= nodedata->attr & 0x3ff;   /*��Ч����Ϣ������attr�ֶ�ָʾ*/
	nodedata->tick		= rt_tick_get( );           /*�յ���ʱ��*/

/* �������ݴ���,����Ҫ����MsgNode */
	if( ( nodedata->attr & 0x2000 ) == 0 )
	{
		nodedata->pmsg = psrc + 15;                 /*��Ϣ�忪ʼλ��*/
		for( i = 0; i < sizeof( handle_jt808_rx_msg ) / sizeof( HANDLE_JT808_RX_MSG ); i++ )
		{
			if( nodedata->id == handle_jt808_rx_msg[i].id )
			{
				handle_jt808_rx_msg[i].func( nodedata );
				flag_find = 1;
			}
		}
		if( !flag_find )
		{
			handle_jt808_rx_default( nodedata );
		}
		rt_free( pinfo );
		rt_free( nodedata );
	}


/*����Ƿ��г�ʱû�д������Ϣ����Ҫ�Ƕ����Ϣ
   �յ���Ϣ�Żᴦ�������ʱ��û���յ���ռ���ڴ�
 */

	iter		= list_jt808_rx->first;
	flag_find	= 0;
	while( iter != NULL )
	{
		iterdata = iter->data;
		if( rt_tick_get( ) - iterdata->tick > RT_TICK_PER_SECOND * 10 ) /*����10��û�����ݰ�*/
		{
			/*����һ�£�׼��ɾ���ýڵ㴮*/
			if( iter->prev == NULL )                                    /*����*/
			{
				list_jt808_rx->first = iter->next;
			}else
			{
				iter->prev->next = iter->next;
				if( iter->next != RT_NULL )
				{
					iter->next->prev = iter->prev;
				}
			}

			while( iter != NULL )
			{
				iter_tmp	= iter->next;
				iterdata	= iter->data;
				rt_free( iterdata->pmsg );
				rt_free( iterdata );
				rt_free( iter );
				iter = iter_tmp;
			}
		}
	}
/*���Ƕ��,����*/
	if( ( nodedata->attr & 0x2000 ) == 0 )
	{
		return 0;
	}

/*�ְ�����,�����µĽڵ�*/
	node = msglist_node_create( (void*)nodedata );
	if( node == RT_NULL )
	{
		rt_free( nodedata );
		rt_free( pinfo );
		return 1;
	}
	nodedata->packetcount	= ( *( psrc + 12 ) << 8 ) | *( psrc + 13 );
	nodedata->packetno		= ( *( psrc + 14 ) << 8 ) | *( psrc + 15 );
/*���ǲ��ǵ�һ���ְ�*/
	flag_find	= 0;
	iter		= list_jt808_rx->first;
	while( iter != NULL )
	{
		iterdata = (JT808_RX_MSG_NODEDATA*)( iter->data );
		if( iterdata->id == nodedata->id ) /*�жϵ���ϢIDһ��,�������зְ�����ID��һ�£�Ҳ��Ϊ���°�*/
		{
			flag_find = 1;
			break;
		}
		iter = iter->next;
	}
/*�ҵ������ǵ�һ���ְ���Ҫ�����еķְ�����*/
	if( flag_find )                                     /*�ҵ��� iter����ʼ����sibling������*/
	{
		if( iterdata->packetno < nodedata->packetno )   /*��������,�ı������ϵĽڵ�*/
		{
			node->prev			= iter->prev;
			node->next			= iter->next;           /*�滻ԭ����λ��*/
			node->sibling_dn	= iter;
			iter->sibling_up	= node;
		}else /*�ڶ����Ϣ�ķ�֧��*/
		{
			flag_find = 0;
			while( iter->sibling_dn != NULL )
			{
				iter		= iter->sibling_dn;
				iterdata	= iter->data;
				if( iterdata->packetno < nodedata->packetno )
				{
					node->sibling_up	= iter->sibling_up;
					node->sibling_dn	= iter;
					iter->sibling_up	= node;
					flag_find			= 1;
					break;
				}
			}
			if( flag_find == 0 ) /*��βҲû���ҵ�*/
			{
				node->sibling_up	= iter;
				iter->sibling_dn	= node;
			}
		}
	}else /*û�ҵ���û��ʹ��msglist_append(list_jt808_rx,nodedata);*/
	{
		if( list_jt808_rx->first == NULL )  /*�ǵ�һ���ڵ�*/
		{
			list_jt808_rx->first->data = nodedata;
		}else /*���нڵ㣬��ӵ����*/
		{
			iter		= iter->prev;       /*��ʱiterΪNULL,Ӧָ��ǰһ����Чnode*/
			iter->next	= node;
			node->prev	= iter;
		}
	}
}

/*
   ����ÿ��Ҫ������Ϣ��״̬
   ���������д�����?
 */

static MsgListRet jt808_tx_proc( MsgListNode* node )
{
	MsgListNode				* pnode		= (MsgListNode*)node;
	JT808_TX_MSG_NODEDATA	* pnodedata = (JT808_TX_MSG_NODEDATA*)( pnode->data );
	int						i;

	if( node == RT_NULL )
	{
		return MSGLIST_RET_OK;
	}

	if( pnodedata->state == IDLE )                      /*���У�������Ϣ��ʱ��û������*/
	{
		if( pnodedata->retry >= jt808_param.id_0x0003 ) /*�Ѿ��ﵽ���Դ���*/
		{
			/*��ʾ����ʧ��*/
			pnodedata->cb_tx_timeout( pnodedata );      /*���÷���ʧ�ܴ�����*/
			return MSGLIST_RET_DELETE_NODE;
		}else
		{
			//rt_device_write( pdev_gsm, 0, pnodedata->pmsg, pnodedata->msg_len );
			gsm_ipsend( pnodedata->pmsg, pnodedata->msg_len, jt808_param.id_0x0002 * RT_TICK_PER_SECOND );
			pnodedata->tick = rt_tick_get( );
			pnodedata->retry++;
			pnodedata->timeout	= pnodedata->retry * jt808_param.id_0x0002 * RT_TICK_PER_SECOND;
			pnodedata->state	= WAIT_ACK;
			rt_kprintf( "send retry=%d,timeout=%d\r\n", pnodedata->retry, pnodedata->timeout * 10 );
		}
		return MSGLIST_RET_OK;
	}

	if( pnodedata->state == WAIT_ACK )
	{
		if( rt_tick_get( ) - pnodedata->tick > pnodedata->timeout )
		{
			pnodedata->state = IDLE;
		}
	}

	if( pnodedata->state == ACK_OK )  /*�յ�ACK���ڽ�������λ*/
	{
		//pnodedata->cb_tx_response( pnodedata );
		return MSGLIST_RET_DELETE_NODE;
	}

	return MSGLIST_RET_OK;
}

/*
   �ж�һ���ַ����ǲ��Ǳ�ʾip��str
   ����ɡ[[0..9|.] ���

   '.' 0x2e
   '/' 0x2f
   '0' 0x30
   '9' 0x39
   ��һ�¡����ж� '/'
 */
static uint8_t is_ipstr( char * str )
{
	char *p = str;
	while( *p != NULL )
	{
		if( ( *p > '9' ) || ( *p < '.' ) )
		{
			return 0;
		}
		p++;
	}
	return 1;
}

/*jt808��socket����

   ά����·�����в�ͬ��ԭ��
   �ϱ�״̬��ά��
   1.��δ����
   2.�������ӣ�DNS,��ʱ��Ӧ��
   3.��ֹ�ϱ����ر�ģ�������
   4.��ǰ���ڽ��п��и��£���ý���ϱ��Ȳ���Ҫ��ϵĹ���

 */
static void jt808_socket_proc( void )
{
	volatile uint32_t	state;
	static rt_tick_t	lasttick;
	static uint8_t		flag_connect = 0;

	if( flag_disable_report )
	{
		return;
	}
/*���GSM״̬*/
	state = config_gsmstate( 0 );
	if( state == GSM_IDLE )
	{
		if( flag_connect == 0 ) /*������server*/
		{
			curr_gsm_socket.apn		= jt808_param.id_0x0010;
			curr_gsm_socket.user	= jt808_param.id_0x0011;
			curr_gsm_socket.psw		= jt808_param.id_0x0012;
			curr_gsm_socket.type	= 't';
			if( is_ipstr( jt808_param.id_0x0013 ) )
			{
				strcpy( curr_gsm_socket.ip_str, jt808_param.id_0x0013 );
			} else
			{
				curr_gsm_socket.ip_domain = jt808_param.id_0x0013;
			}
			curr_gsm_socket.port = jt808_param.id_0x0018;
			rt_kprintf( "\r\napn=%s\r\n", curr_gsm_socket.apn );
		}else
		{
			curr_gsm_socket.apn		= jt808_param.id_0x0014;
			curr_gsm_socket.user	= jt808_param.id_0x0015;
			curr_gsm_socket.psw		= jt808_param.id_0x0016;
			curr_gsm_socket.type	= 't';
			if( is_ipstr( jt808_param.id_0x0017 ) )
			{
				strcpy( curr_gsm_socket.ip_str, jt808_param.id_0x0017 );
			} else
			{
				curr_gsm_socket.ip_domain = jt808_param.id_0x0017;
			}

			curr_gsm_socket.port = jt808_param.id_0x0018;
		}
		curr_gsm_socket.state = SOCKET_IDLE;
		config_gsmstate( GSM_POWERON );
		return;
	}
	if( state == GSM_POWERON ) /*��鳬ʱ*/
	{
	}
	if( state != GSM_AT )
	{
		return;
	}

/*���socket״̬,�ж��ò���DNS*/
	switch( curr_gsm_socket.state )
	{
		case SOCKET_IDLE:
			if( flag_connect == 0 )
			{
				if( is_ipstr( jt808_param.id_0x0013 ) )
				{
					curr_gsm_socket.state = SOCKET_CONNECT;
				} else
				{
					curr_gsm_socket.state = SOCKET_DNS;
				}
			}else
			{
				if( is_ipstr( jt808_param.id_0x0017 ) )
				{
					curr_gsm_socket.state = SOCKET_CONNECT;
				} else
				{
					curr_gsm_socket.state = SOCKET_DNS;
				}
			}
			break;
		case SOCKET_CONNECT_ERR:
		case SOCKET_DNS_ERR:
			flag_connect++;
			if( flag_connect == 2 )             /*�Ƿ�Ҫ�ر��豸��������������*/
			{
				flag_connect = 0;
			}
			config_gsmstate( GSM_POWEROFF );    /*�ر�gsm�ȴ�����*/
			break;
	}
	if( state != SOCKET_READY )
	{
		return;
	}
/*�Ƿ�������*/
}

#define offsetof( TYPE, MEMBER ) ( (size_t)&( (TYPE*)0 )->MEMBER )

typedef __packed struct _test
{
	uint8_t id_0001[2];
	uint8_t id_0000[1];
	uint8_t id_0002[4];
	char	id_0003[32];
//	uint8_t id_0004[4];
//	uint8_t id_0005[2];
//	uint8_t id_0006[1];
}STU_TEST;

STU_TEST stu_test = {
	0x1234, 0, 0x87654321,
	"test",
//0x12345678,0x4321,0xff
};


/*
   ����״̬ά��
   jt808Э�鴦��

 */
ALIGN( RT_ALIGN_SIZE )
static char thread_jt808_stack[4096];
struct rt_thread thread_jt808;

/***/
static void rt_thread_entry_jt808( void* parameter )
{
	rt_err_t				ret;
	int						i;
	uint8_t					*pstr;

	MsgListNode				* iter;
	MsgListNode				* iter_next;
	JT808_TX_MSG_NODEDATA	* pnodedata;

//	for(i=0;i<MAX_GSM_SOCKET;i++) gsm_socket[i].msglist_tx = msglist_create();

	rt_kprintf( "\r\nstu_test.id_0000=%08x", *(char*)( &stu_test + offsetof( STU_TEST, id_0000 ) ) );
	rt_kprintf( "\r\nstu_test.id_0001=%08x", *(uint16_t*)( &stu_test + offsetof( STU_TEST, id_0001 ) ) );
	rt_kprintf( "\r\nstu_test.id_0002=%08x", *(uint32_t*)( &stu_test + offsetof( STU_TEST, id_0002 ) ) );
	rt_kprintf( "\r\nstu_test.id_0003=%s\r\n", (char*)( &stu_test + offsetof( STU_TEST, id_0003 ) ) );
//	rt_kprintf("\r\nstu_test.id_0004=%08x",stu_test.id_0004);
//	rt_kprintf("\r\nstu_test.id_0005=%04x",stu_test.id_0005);
//	rt_kprintf("\r\nstu_test.id_0006=%02x",stu_test.id_0006);

	PT_INIT( &pt_jt808_socket );

/*��ȡ������������*/
	param_load( );

	param_print( );

	list_jt808_tx	= msglist_create( );
	list_jt808_rx	= msglist_create( );

	pdev_gsm = rt_device_find( "gsm" ); /*û�г�����,δ�ҵ���ô��*/

	while( 1 )
	{
/*����gps��Ϣ*/
		ret = rt_mb_recv( &mb_gpsdata, (rt_uint32_t*)&pstr, 5 );
		if( ret == RT_EOK )
		{
			gps_analy( pstr );
			rt_free( pstr );
		}
/*����gprs��Ϣ*/
		ret = rt_mb_recv( &mb_gprsdata, (rt_uint32_t*)&pstr, 5 );
		if( ret == RT_EOK )
		{
			jt808_rx_proc( pstr );
		}
/*jt808 socket����*/
		jt808_socket_proc( );
/*��������*/
		iter = list_jt808_tx->first;
		if( jt808_tx_proc( iter ) == MSGLIST_RET_DELETE_NODE )  /*ɾ���ýڵ�*/
		{
			pnodedata = (JT808_TX_MSG_NODEDATA*)( iter->data );
			rt_free( pnodedata->pmsg );                         /*ɾ���û�����*/
			rt_free( pnodedata );                               /*ɾ���ڵ�����*/
			list_jt808_tx->first = iter->next;                  /*ָ����һ��*/
			rt_free( iter );
		}

		rt_thread_delay( RT_TICK_PER_SECOND / 20 );
	}

	msglist_destroy( list_jt808_tx );
}

/*jt808�����̳߳�ʼ��*/
void jt808_init( void )
{
	rt_mb_init( &mb_gprsdata, "gprsdata", &mb_gprsdata_pool, MB_GPRSDATA_POOL_SIZE / 4, RT_IPC_FLAG_FIFO );
	rt_mb_init( &mb_gpsdata, "gpsdata", &mb_gpsdata_pool, MB_GPSDATA_POOL_SIZE / 4, RT_IPC_FLAG_FIFO );

	rt_thread_init( &thread_jt808,
	                "jt808",
	                rt_thread_entry_jt808,
	                RT_NULL,
	                &thread_jt808_stack[0],
	                sizeof( thread_jt808_stack ), 10, 5 );
	rt_thread_startup( &thread_jt808 );
}

/*gps���մ���*/
void gps_rx( uint8_t *pinfo, uint16_t length )
{
	uint8_t *pmsg;
	pmsg = rt_malloc( length + 2 );
	if( pmsg != RT_NULL )
	{
		pmsg[0] = length >> 8;
		pmsg[1] = length & 0xff;
		memcpy( pmsg + 2, pinfo, length );
		rt_mb_send( &mb_gpsdata, (rt_uint32_t)pmsg );
	}
}

/*gprs���մ���,�յ�����Ҫ���촦��*/
rt_err_t gprs_rx( uint8_t linkno, uint8_t * pinfo, uint16_t length )
{
	uint8_t *pmsg;
	pmsg = rt_malloc( length + 3 ); /*����������Ϣ*/
	if( pmsg != RT_NULL )
	{
		pmsg[0] = linkno;
		pmsg[1] = length >> 8;
		pmsg[2] = length & 0xff;
		memcpy( pmsg + 3, pinfo, length );
		rt_mb_send( &mb_gprsdata, (rt_uint32_t)pmsg );
		return 0;
	}
	return 1;
}

FINSH_FUNCTION_EXPORT( gprs_rx, simlute gprs rx );


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
rt_err_t gprs_tx( uint8_t linkno, uint8_t * pinfo, uint16_t length )
{
	uint8_t *pmsg;
	pmsg = rt_malloc( length + 3 ); /*����������Ϣ*/
	if( pmsg != RT_NULL )
	{
		pmsg[0] = linkno;
		pmsg[1] = length >> 8;
		pmsg[2] = length & 0xff;
		memcpy( pmsg + 3, pinfo, length );
		rt_mb_send( &mb_gprsdata, (rt_uint32_t)pmsg );
		return 0;
	}
	return 1;
}

FINSH_FUNCTION_EXPORT( gprs_tx, simlute gprs tx );


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
rt_err_t server( uint8_t cmd, char* apn, char* name, char* psw )
{
	if( cmd == 0 ) /*��ӡ����*/
	{
		rt_kprintf( "\r\napn=%s user=%s psw=%s ip_domain=%s tcp_port=%d udp_port=%d",
		            jt808_param.id_0x0010,
		            jt808_param.id_0x0011,
		            jt808_param.id_0x0012,
		            jt808_param.id_0x0013,
		            jt808_param.id_0x0018,
		            jt808_param.id_0x0019
		            );
		rt_kprintf( "\r\napn=%s user=%s psw=%s ip_domain=%s tcp_port=%d udp_port=%d\r\n",
		            jt808_param.id_0x0014,
		            jt808_param.id_0x0015,
		            jt808_param.id_0x0016,
		            jt808_param.id_0x0017,
		            jt808_param.id_0x0018,
		            jt808_param.id_0x0019
		            );
	}
	if( cmd == 1 )  /*������server*/
	{
	}
	if( cmd == 2 )  /*���ñ���server*/
	{
	}
	return RT_EOK;
}

FINSH_FUNCTION_EXPORT( server, config server );


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static uint16_t jt808_pack_byte( uint8_t *buf, uint8_t *fcs, uint8_t data )
{
	uint8_t *p = buf;
	*fcs ^= data;
	if( ( data == 0x7d ) || ( data == 0x7e ) )
	{
		*p++	= 0x7d;
		*p		= ( data - 0x7c );
		return 2;
	}else
	{
		*p = data;
		return 1;
	}
}

/*
   static uint16_t jt808_pack_word(uint8_t *buf,uint8_t *fcs,uint16_t data)
   {
   uint16_t count=0;
   count+=jt808_pack_byte(buf+count,fcs,(data>>8));
   count+=jt808_pack_byte(buf+count,fcs,(data&0xff));
   return count;
   }

   static uint16_t jt808_pack_dword(uint8_t *buf,uint8_t *fcs,uint32_t data)
   {
   uint16_t count=0;
   count+=jt808_pack_byte(buf+count,fcs,(data>>24));
   count+=jt808_pack_byte(buf+count,fcs,(data>>16));
   count+=jt808_pack_byte(buf+count,fcs,(data>>8));
   count+=jt808_pack_byte(buf+count,fcs,(data&0xff));
   return count;
   }
 */

/*���ݽ����ȣ����ڼ���*/
static uint16_t jt808_pack_int( uint8_t *buf, uint8_t *fcs, uint32_t data, uint8_t width )
{
	uint16_t count = 0;
	switch( width )
	{
		case 1:
			count += jt808_pack_byte( buf + count, fcs, ( data & 0xff ) );
			break;
		case 2:
			count	+= jt808_pack_byte( buf + count, fcs, ( data >> 8 ) );
			count	+= jt808_pack_byte( buf + count, fcs, ( data & 0xff ) );
			break;
		case 4:
			count	+= jt808_pack_byte( buf + count, fcs, ( data >> 24 ) );
			count	+= jt808_pack_byte( buf + count, fcs, ( data >> 16 ) );
			count	+= jt808_pack_byte( buf + count, fcs, ( data >> 8 ) );
			count	+= jt808_pack_byte( buf + count, fcs, ( data & 0xff ) );
			break;
	}
	return count;
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
static uint16_t jt808_pack_string( uint8_t *buf, uint8_t *fcs, char *str )
{
	uint16_t	count	= 0;
	char		*p		= str;
	while( *p )
	{
		count += jt808_pack_byte( buf + count, fcs, *p++ );
	}
	return count;
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
static uint16_t jt808_pack_array( uint8_t *buf, uint8_t *fcs, uint8_t *src, uint16_t len )
{
	uint16_t	count = 0;
	int			i;
	char		*p = src;
	for( i = 0; i < len; i++ )
	{
		count += jt808_pack_byte( buf + count, fcs, *p++ );
	}
	return count;
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
static rt_err_t jt808_tx( void )
{
	uint8_t					*pdata;
	JT808_TX_MSG_NODEDATA	*pnodedata;
	uint8_t					buf[256];
	uint8_t					*p;
	uint16_t				len;
	uint8_t					fcs = 0;
	int						i;

	pnodedata = rt_malloc( sizeof( JT808_TX_MSG_NODEDATA ) );
	if( pnodedata == NULL )
	{
		return -1;
	}
	pnodedata->type				= TERMINAL_CMD; /*���ͼ�ɾ����������������*/
	pnodedata->state			= IDLE;
	pnodedata->retry			= 0;
	pnodedata->cb_tx_timeout	= jt808_tx_timeout;
	pnodedata->cb_tx_response	= jt808_tx_response;

	len = 1;
	len += jt808_pack_int( buf + len, &fcs, 0x0100, 2 );
	len += jt808_pack_int( buf + len, &fcs, 37 + strlen( jt808_param.id_0x0083 ), 2 );
	len += jt808_pack_array( buf + len, &fcs, term_param.mobile, 6 );
	len += jt808_pack_int( buf + len, &fcs, tx_seq, 2 );

	len			+= jt808_pack_int( buf + len, &fcs, jt808_param.id_0x0081, 2 );
	len			+= jt808_pack_int( buf + len, &fcs, jt808_param.id_0x0082, 2 );
	len			+= jt808_pack_array( buf + len, &fcs, term_param.producer_id, 5 );
	len			+= jt808_pack_array( buf + len, &fcs, term_param.model, 20 );
	len			+= jt808_pack_array( buf + len, &fcs, term_param.terminal_id, 7 );
	len			+= jt808_pack_int( buf + len, &fcs, jt808_param.id_0x0084, 1 );
	len			+= jt808_pack_string( buf + len, &fcs, jt808_param.id_0x0083 );
	len			+= jt808_pack_byte( buf + len, &fcs, fcs );
	buf[0]		= 0x7e;
	buf[len]	= 0x7e;
	pdata		= rt_malloc( len + 1 );
	if( pdata == NULL )
	{
		rt_free( pnodedata );
		return;
	}
	rt_kprintf( "\r\n--------------------\r\n" );
	for( i = 0; i < len + 1; i++ )
	{
		rt_kprintf( "%02x ", buf[i] );
	}
	rt_kprintf( "\r\n--------------------\r\n" );
	memcpy( pdata, buf, len + 1 );
	pnodedata->msg_len	= len + 1;
	pnodedata->pmsg		= pdata;
	pnodedata->head_sn	= tx_seq;
	pnodedata->head_id	= 0x0100;
	msglist_append( list_jt808_tx, pnodedata );
	tx_seq++;
}

FINSH_FUNCTION_EXPORT( jt808_tx, jt808_tx test );

/************************************** The End Of File **************************************/

