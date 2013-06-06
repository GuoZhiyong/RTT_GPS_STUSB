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
#include "msglist.h"
#include "jt808_sprintf.h"
#include "sst25.h"

#include "gsm.h"
#include "m66.h"

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
	int ( *func )( uint8_t linkno, uint8_t *pmsg );
}HANDLE_JT808_RX_MSG;

static struct rt_mailbox	mb_gprsdata;
#define MB_GPRSDATA_POOL_SIZE 32
static uint8_t				mb_gprsdata_pool[MB_GPRSDATA_POOL_SIZE];

static struct rt_mailbox	mb_gpsdata;
#define MB_GPSDATA_POOL_SIZE 32
static uint8_t				mb_gpsdata_pool[MB_GPSDATA_POOL_SIZE];

static uint16_t				tx_seq = 0; /*�������*/

static rt_device_t			pdev_gsm = RT_NULL;


/*������Ϣ�б�*/
MsgList* list_jt808_tx;

/*������Ϣ�б�*/
MsgList * list_jt808_rx;


/*
   ͬʱ׼���ÿ��õ��ĸ����ӣ�����Ҫ��ѡ����,����Ϊ
   ʵ���в�����ͬʱ�Զ�����ӽ�����ֻ�����η���������
   ��808������
   ����808������
   ��IC����Ȩ������
   ����IC����Ȩ������

 */
GSM_SOCKET	gsm_socket[MAX_GSM_SOCKET];

GSM_SOCKET	* psocket = RT_NULL;

JT808_PARAM jt808_param =
{
	0x13022200,         /*0x0000 �汾*/
	5,                  /*0x0001 �������ͼ��*/
	5,                  /*0x0002 TCPӦ��ʱʱ��*/
	3,                  /*0x0003 TCP��ʱ�ش�����*/
	3,                  /*0x0004 UDPӦ��ʱʱ��*/
	5,                  /*0x0005 UDP��ʱ�ش�����*/
	3,                  /*0x0006 SMS��ϢӦ��ʱʱ��*/
	5,                  /*0x0007 SMS��Ϣ�ش�����*/
	"CMNET",            /*0x0010 ��������APN*/
	"",                 /*0x0011 �û���*/
	"",                 /*0x0012 ����*/
	"60.28.50.210",     /*0x0013 ����������ַ*/
	"CMNET",            /*0x0014 ����APN*/
	"",                 /*0x0015 �����û���*/
	"",                 /*0x0016 ��������*/
	"www.google.com",   /*0x0017 ���ݷ�������ַ��ip������*/
	9131,               /*0x0018 TCP�˿�*/
	5678,               /*0x0019 UDP�˿�*/
	"",                 /*0x001A ic������������ַ��ip������*/
	0,                  /*0x001B ic��������TCP�˿�*/
	0,                  /*0x001C ic��������UDP�˿�*/
	"",                 /*0x001D ic�����ݷ�������ַ��ip������*/
	0,                  /*0x0020 λ�û㱨����*/
	1,                  /*0x0021 λ�û㱨����*/
	30,                 /*0x0022 ��ʻԱδ��¼�㱨ʱ����*/
	120,                /*0x0027 ����ʱ�㱨ʱ����*/
	5,                  /*0x0028 ��������ʱ�㱨ʱ����*/
	30,                 /*0x0029 ȱʡʱ��㱨���*/
	500,                /*0x002c ȱʡ����㱨���*/
	1000,               /*0x002d ��ʻԱδ��¼�㱨������*/
	1000,               /*0x002e ����ʱ����㱨���*/
	100,                /*0x002f ����ʱ����㱨���*/
	270,                /*0x0030 �յ㲹���Ƕ�*/
	500,                /*0x0031 ����Χ���뾶���Ƿ�λ����ֵ������λΪ��*/
	"1008611",          /*0x0040 ���ƽ̨�绰����*/
	"",                 /*0x0041 ��λ�绰����*/
	"",                 /*0x0042 �ָ��������õ绰����*/
	"",                 /*0x0043 ���ƽ̨SMS����*/
	"",                 /*0x0044 �����ն�SMS�ı���������*/
	5,                  /*0x0045 �ն˽����绰����*/
	3,                  /*0x0046 ÿ��ͨ��ʱ��*/
	3,                  /*0x0047 ����ͨ��ʱ��*/
	"",                 /*0x0048 �����绰����*/
	"",                 /*0x0049 ���ƽ̨��Ȩ���ź���*/
	5,                  /*0x0050 ����������*/
	3,                  /*0x0051 ���������ı�SMS����*/
	5,                  /*0x0052 �������տ���*/
	3,                  /*0x0053 ��������洢��־*/
	5,                  /*0x0054 �ؼ���־*/
	3,                  /*0x0055 ����ٶ�kmh*/
	5,                  /*0x0056 ���ٳ���ʱ��*/
	3,                  /*0x0057 ������ʻʱ������*/
	5,                  /*0x0058 �����ۼƼ�ʻʱ������*/
	3,                  /*0x0059 ��С��Ϣʱ��*/
	5,                  /*0x005A �ͣ��ʱ��*/
	900,                /*0x0005B ���ٱ���Ԥ����ֵ����λΪ 1/10Km/h */
	90,                 /*0x005C ƣ�ͼ�ʻԤ����ֵ����λΪ�루s����>0*/
	0x200a,             /*0x005D ��ײ������������:*/
	30,                 /*0x005E �෭�����������ã� �෭�Ƕȣ���λ 1 �ȣ�Ĭ��Ϊ 30 ��*/
	0,                  /*0x0064 ��ʱ���տ���*/
	0,                  /*0x0065 �������տ���*/
	3,                  /*0x0070 ͼ����Ƶ����(1-10)*/
	5,                  /*0x0071 ����*/
	3,                  /*0x0072 �Աȶ�*/
	5,                  /*0x0073 ���Ͷ�*/
	3,                  /*0x0074 ɫ��*/
	5,                  /*0x0080 ������̱����0.1km*/
	3,                  /*0x0081 ʡ��ID*/
	5,                  /*0x0082 ����ID*/
	"��O-00001",        /*0x0083 ����������*/
	1,                  /*0x0084 ������ɫ  1��ɫ 2��ɫ 3��ɫ 4��ɫ 9����*/
	0x0f,               /*0x0090 GNSS ��λģʽ*/
	0x01,               /*0x0091 GNSS ������*/
	0x01,               /*0x0092 GNSS ģ����ϸ��λ�������Ƶ��*/
	0x01,               /*0x0093    GNSS ģ����ϸ��λ���ݲɼ�Ƶ��*/
	0x01,               /*0x0094    GNSS ģ����ϸ��λ�����ϴ���ʽ*/
	0x01,               /*0x0095 GNSS ģ����ϸ��λ�����ϴ�����*/
	0,                  /*0x0100 CAN ����ͨ�� 1 �ɼ�ʱ����(ms)��0 ��ʾ���ɼ�*/
	0,                  /*0x0101 CAN ����ͨ�� 1 �ϴ�ʱ����(s)��0 ��ʾ���ϴ�*/
	0,                  /*0x0102 CAN ����ͨ�� 2 �ɼ�ʱ����(ms)��0 ��ʾ���ɼ�*/
	0,                  /*0x0103 CAN ����ͨ�� 2 �ϴ�ʱ����(s)��0 ��ʾ���ϴ�*/
	{ 0, 0 },           /*0x0110 CAN ���� ID �����ɼ�����*/
	{ 0, 0 },           /*0x0111 ����CAN ���� ID �����ɼ�����*/
	{ 0, 0 },           /*0x0112 ����CAN ���� ID �����ɼ�����*/
	{ 0, 0 },           /*0x0113 ����CAN ���� ID �����ɼ�����*/
	{ 0, 0 },           /*0x0114 ����CAN ���� ID �����ɼ�����*/
	{ 0, 0 },           /*0x0115 ����CAN ���� ID �����ɼ�����*/
	{ 0, 0 },           /*0x0116 ����CAN ���� ID �����ɼ�����*/
	{ 0, 0 },           /*0x0117 ����CAN ���� ID �����ɼ�����*/
	{ 0, 0 },           /*0x0118 ����CAN ���� ID �����ɼ�����*/
	{ 0, 0 },           /*0x0119 ����CAN ���� ID �����ɼ�����*/
};

TERM_PARAM term_param =
{
	0x07,
	{ 0x11,0x22,	0x33, 0x44, 0x55, 0x66 },
	{ "TCBBD" },
	{ "TW701-BD" },
	{ 0x00,0x99,	0xaa, 0xbb, 0xcc, 0xdd, 0xee},
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
	ver32 = ( ver8[0] ) | ( ver8[1] << 8 ) | ( ver8[2] << 16 ) | ( ver8[3] << 24 );
	rt_kprintf( "param_load ver=%08x\r\n", ver32 );
	if( jt808_param.id_0x0000 != ver32 ) /*�����ǲ���δ��ʼ��*/
	{
		rt_kprintf( "%s(%d)param_save\r\n", __func__, __LINE__ );
		param_save( );
	}
	sst25_read( ADDR_PARAM, (uint8_t*)&jt808_param, sizeof( jt808_param ) );
}

#define TYPE_BYTE	0x01    /*�̶�Ϊ1�ֽ�,С�˶���*/
#define TYPE_WORD	0x02    /*�̶�Ϊ2�ֽ�,С�˶���*/
#define TYPE_DWORD	0x04    /*�̶�Ϊ4�ֽ�,С�˶���*/
#define TYPE_STR	0x80    /*�̶�Ϊ32�ֽ�,����˳��*/
#define TYPE_CAN_ID 0x48    /*�̶�Ϊ8�ֽ�,��ǰ�洢CAN_ID����*/

struct _tbl_id_lookup
{
	uint16_t	id;
	uint8_t		type;
	uint8_t		* val;
} tbl_id_lookup[] = {
	{ 0x0000, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0000 ) },    //uint32_t	id_0x0000;      /*0x0000 �汾*/
	{ 0x0001, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0001 ) },    //uint32_t	id_0x0001;      /*0x0001 �������ͼ��*/
	{ 0x0002, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0002 ) },    //uint32_t	id_0x0002;      /*0x0002 TCPӦ��ʱʱ��*/
	{ 0x0003, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0003 ) },    //uint32_t	id_0x0003;      /*0x0003 TCP��ʱ�ش�����*/
	{ 0x0004, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0004 ) },    //uint32_t	id_0x0004;      /*0x0004 UDPӦ��ʱʱ��*/
	{ 0x0005, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0005 ) },    //uint32_t	id_0x0005;      /*0x0005 UDP��ʱ�ش�����*/
	{ 0x0006, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0006 ) },    //uint32_t	id_0x0006;      /*0x0006 SMS��ϢӦ��ʱʱ��*/
	{ 0x0007, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0007 ) },    //uint32_t	id_0x0007;      /*0x0007 SMS��Ϣ�ش�����*/
	{ 0x0010, TYPE_STR,		 (uint8_t*)&( jt808_param.id_0x0010 ) },    //char		id_0x0010[32];  /*0x0010 ��������APN*/
	{ 0x0011, TYPE_STR,		 (uint8_t*)&( jt808_param.id_0x0011 ) },    //char		id_0x0011[32];  /*0x0011 �û���*/
	{ 0x0012, TYPE_STR,		 (uint8_t*)&( jt808_param.id_0x0012 ) },    //char		id_0x0012[32];  /*0x0012 ����*/
	{ 0x0013, TYPE_STR,		 (uint8_t*)&( jt808_param.id_0x0013 ) },    //char		id_0x0013[32];  /*0x0013 ����������ַ*/
	{ 0x0014, TYPE_STR,		 (uint8_t*)&( jt808_param.id_0x0014 ) },    //char		id_0x0014[32];  /*0x0014 ����APN*/
	{ 0x0015, TYPE_STR,		 (uint8_t*)&( jt808_param.id_0x0015 ) },    //char		id_0x0015[32];  /*0x0015 �����û���*/
	{ 0x0016, TYPE_STR,		 (uint8_t*)&( jt808_param.id_0x0016 ) },    //char		id_0x0016[32];  /*0x0016 ��������*/
	{ 0x0017, TYPE_STR,		 (uint8_t*)&( jt808_param.id_0x0017 ) },    //char		id_0x0017[32];  /*0x0017 ���ݷ�������ַ��ip������*/
	{ 0x0018, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0018 ) },    //uint32_t	id_0x0018;      /*0x0018 TCP�˿�*/
	{ 0x0019, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0019 ) },    //uint32_t	id_0x0019;      /*0x0019 UDP�˿�*/
	{ 0x001A, TYPE_STR,		 (uint8_t*)&( jt808_param.id_0x001A ) },    //char		id_0x001A[32];  /*0x001A ic������������ַ��ip������*/
	{ 0x001B, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x001B ) },    //uint32_t	id_0x001B;      /*0x001B ic��������TCP�˿�*/
	{ 0x001C, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x001C ) },    //uint32_t	id_0x001C;      /*0x001C ic��������UDP�˿�*/
	{ 0x001D, TYPE_STR,		 (uint8_t*)&( jt808_param.id_0x001D ) },    //char		id_0x001D[32];  /*0x001D ic�����ݷ�������ַ��ip������*/
	{ 0x0020, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0020 ) },    //uint32_t	id_0x0020;      /*0x0020 λ�û㱨����*/
	{ 0x0021, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0021 ) },    //uint32_t	id_0x0021;      /*0x0021 λ�û㱨����*/
	{ 0x0022, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0022 ) },    //uint32_t	id_0x0022;      /*0x0022 ��ʻԱδ��¼�㱨ʱ����*/
	{ 0x0027, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0027 ) },    //uint32_t	id_0x0027;      /*0x0027 ����ʱ�㱨ʱ����*/
	{ 0x0028, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0028 ) },    //uint32_t	id_0x0028;      /*0x0028 ��������ʱ�㱨ʱ����*/
	{ 0x0029, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0029 ) },    //uint32_t	id_0x0029;      /*0x0029 ȱʡʱ��㱨���*/
	{ 0x002C, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x002C ) },    //uint32_t	id_0x002C;      /*0x002c ȱʡ����㱨���*/
	{ 0x002D, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x002D ) },    //uint32_t	id_0x002D;      /*0x002d ��ʻԱδ��¼�㱨������*/
	{ 0x002E, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x002E ) },    //uint32_t	id_0x002E;      /*0x002e ����ʱ����㱨���*/
	{ 0x002F, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x002F ) },    //uint32_t	id_0x002F;      /*0x002f ����ʱ����㱨���*/
	{ 0x0030, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0030 ) },    //uint32_t	id_0x0030;      /*0x0030 �յ㲹���Ƕ�*/
	{ 0x0031, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0031 ) },    //uint16_t	id_0x0031;      /*0x0031 ����Χ���뾶���Ƿ�λ����ֵ������λΪ��*/
	{ 0x0040, TYPE_STR,		 (uint8_t*)&( jt808_param.id_0x0040 ) },    //char		id_0x0040[32];  /*0x0040 ���ƽ̨�绰����*/
	{ 0x0041, TYPE_STR,		 (uint8_t*)&( jt808_param.id_0x0041 ) },    //char		id_0x0041[32];  /*0x0041 ��λ�绰����*/
	{ 0x0042, TYPE_STR,		 (uint8_t*)&( jt808_param.id_0x0042 ) },    //char		id_0x0042[32];  /*0x0042 �ָ��������õ绰����*/
	{ 0x0043, TYPE_STR,		 (uint8_t*)&( jt808_param.id_0x0043 ) },    //char		id_0x0043[32];  /*0x0043 ���ƽ̨SMS����*/
	{ 0x0044, TYPE_STR,		 (uint8_t*)&( jt808_param.id_0x0044 ) },    //char		id_0x0044[32];  /*0x0044 �����ն�SMS�ı���������*/
	{ 0x0045, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0045 ) },    //uint32_t	id_0x0045;      /*0x0045 �ն˽����绰����*/
	{ 0x0046, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0046 ) },    //uint32_t	id_0x0046;      /*0x0046 ÿ��ͨ��ʱ��*/
	{ 0x0047, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0047 ) },    //uint32_t	id_0x0047;      /*0x0047 ����ͨ��ʱ��*/
	{ 0x0048, TYPE_STR,		 (uint8_t*)&( jt808_param.id_0x0048 ) },    //char		id_0x0048[32];  /*0x0048 �����绰����*/
	{ 0x0049, TYPE_STR,		 (uint8_t*)&( jt808_param.id_0x0049 ) },    //char		id_0x0049[32];  /*0x0049 ���ƽ̨��Ȩ���ź���*/
	{ 0x0050, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0050 ) },    //uint32_t	id_0x0050;      /*0x0050 ����������*/
	{ 0x0051, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0051 ) },    //uint32_t	id_0x0051;      /*0x0051 ���������ı�SMS����*/
	{ 0x0052, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0052 ) },    //uint32_t	id_0x0052;      /*0x0052 �������տ���*/
	{ 0x0053, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0053 ) },    //uint32_t	id_0x0053;      /*0x0053 ��������洢��־*/
	{ 0x0054, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0054 ) },    //uint32_t	id_0x0054;      /*0x0054 �ؼ���־*/
	{ 0x0055, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0055 ) },    //uint32_t	id_0x0055;      /*0x0055 ����ٶ�kmh*/
	{ 0x0056, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0056 ) },    //uint32_t	id_0x0056;      /*0x0056 ���ٳ���ʱ��*/
	{ 0x0057, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0057 ) },    //uint32_t	id_0x0057;      /*0x0057 ������ʻʱ������*/
	{ 0x0058, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0058 ) },    //uint32_t	id_0x0058;      /*0x0058 �����ۼƼ�ʻʱ������*/
	{ 0x0059, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0059 ) },    //uint32_t	id_0x0059;      /*0x0059 ��С��Ϣʱ��*/
	{ 0x005A, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x005A ) },    //uint32_t	id_0x005A;      /*0x005A �ͣ��ʱ��*/
	{ 0x005B, TYPE_WORD,	 (uint8_t*)&( jt808_param.id_0x005B ) },    //uint16_t	id_0x005B;      /*0x005B ���ٱ���Ԥ����ֵ����λΪ 1/10Km/h */
	{ 0x005C, TYPE_WORD,	 (uint8_t*)&( jt808_param.id_0x005C ) },    //uint16_t	id_0x005C;      /*0x005C ƣ�ͼ�ʻԤ����ֵ����λΪ�루s����>0*/
	{ 0x005D, TYPE_WORD,	 (uint8_t*)&( jt808_param.id_0x005D ) },    //uint16_t	id_0x005D;      /*0x005D ��ײ������������:b7..0����ײʱ��(4ms) b15..8����ײ���ٶ�(0.1g) 0-79 ֮�䣬Ĭ��Ϊ10 */
	{ 0x005E, TYPE_WORD,	 (uint8_t*)&( jt808_param.id_0x005E ) },    //uint16_t	id_0x005E;      /*0x005E �෭�����������ã� �෭�Ƕȣ���λ 1 �ȣ�Ĭ��Ϊ 30 ��*/
	{ 0x0064, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0064 ) },    //uint32_t	id_0x0064;      /*0x0064 ��ʱ���տ���*/
	{ 0x0065, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0065 ) },    //uint32_t	id_0x0065;      /*0x0065 �������տ���*/
	{ 0x0070, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0070 ) },    //uint32_t	id_0x0070;      /*0x0070 ͼ����Ƶ����(1-10)*/
	{ 0x0071, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0071 ) },    //uint32_t	id_0x0071;      /*0x0071 ����*/
	{ 0x0072, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0072 ) },    //uint32_t	id_0x0072;      /*0x0072 �Աȶ�*/
	{ 0x0073, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0073 ) },    //uint32_t	id_0x0073;      /*0x0073 ���Ͷ�*/
	{ 0x0074, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0074 ) },    //uint32_t	id_0x0074;      /*0x0074 ɫ��*/
	{ 0x0080, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0080 ) },    //uint32_t	id_0x0080;      /*0x0080 ������̱����0.1km*/
	{ 0x0081, TYPE_WORD,	 (uint8_t*)&( jt808_param.id_0x0081 ) },    //uint16_t	id_0x0081;      /*0x0081 ʡ��ID*/
	{ 0x0082, TYPE_WORD,	 (uint8_t*)&( jt808_param.id_0x0082 ) },    //uint16_t	id_0x0082;      /*0x0082 ����ID*/
	{ 0x0083, TYPE_STR,		 (uint8_t*)&( jt808_param.id_0x0083 ) },    //char		id_0x0083[32];  /*0x0083 ����������*/
	{ 0x0084, TYPE_BYTE,	 (uint8_t*)&( jt808_param.id_0x0084 ) },    //uint8_t		id_0x0084;      /*0x0084 ������ɫ  1��ɫ 2��ɫ 3��ɫ 4��ɫ 9����*/
	{ 0x0090, TYPE_BYTE,	 (uint8_t*)&( jt808_param.id_0x0090 ) },    //uint8_t		id_0x0090;      /*0x0090 GNSS ��λģʽ*/
	{ 0x0091, TYPE_BYTE,	 (uint8_t*)&( jt808_param.id_0x0091 ) },    //uint8_t		id_0x0091;      /*0x0091 GNSS ������*/
	{ 0x0092, TYPE_BYTE,	 (uint8_t*)&( jt808_param.id_0x0092 ) },    //uint8_t		id_0x0092;      /*0x0092 GNSS ģ����ϸ��λ�������Ƶ��*/
	{ 0x0093, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0093 ) },    //uint32_t	id_0x0093;      /*0x0093 GNSS ģ����ϸ��λ���ݲɼ�Ƶ��*/
	{ 0x0094, TYPE_BYTE,	 (uint8_t*)&( jt808_param.id_0x0094 ) },    //uint8_t		id_0x0094;      /*0x0094 GNSS ģ����ϸ��λ�����ϴ���ʽ*/
	{ 0x0095, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0095 ) },    //uint32_t	id_0x0095;      /*0x0095 GNSS ģ����ϸ��λ�����ϴ�����*/
	{ 0x0100, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0100 ) },    //uint32_t	id_0x0100;      /*0x0100 CAN ����ͨ�� 1 �ɼ�ʱ����(ms)��0 ��ʾ���ɼ�*/
	{ 0x0101, TYPE_WORD,	 (uint8_t*)&( jt808_param.id_0x0101 ) },    //uint16_t	id_0x0101;      /*0x0101 CAN ����ͨ�� 1 �ϴ�ʱ����(s)��0 ��ʾ���ϴ�*/
	{ 0x0102, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0102 ) },    //uint32_t	id_0x0102;      /*0x0102 CAN ����ͨ�� 2 �ɼ�ʱ����(ms)��0 ��ʾ���ɼ�*/
	{ 0x0103, TYPE_WORD,	 (uint8_t*)&( jt808_param.id_0x0103 ) },    //uint16_t	id_0x0103;      /*0x0103 CAN ����ͨ�� 2 �ϴ�ʱ����(s)��0 ��ʾ���ϴ�*/
	{ 0x0110, TYPE_BYTE | 8, (uint8_t*)&( jt808_param.id_0x0110 ) },    //uint8_t		id_0x0110[8];   /*0x0110 CAN ���� ID �����ɼ�����*/
	{ 0x0111, TYPE_BYTE | 8, (uint8_t*)&( jt808_param.id_0x0111 ) },    //uint8_t		id_0x0111[8];   /*0x0111 ����CAN ���� ID �����ɼ�����*/
	{ 0x0112, TYPE_BYTE | 8, (uint8_t*)&( jt808_param.id_0x0112 ) },    //uint8_t		id_0x0112[8];   /*0x0112 ����CAN ���� ID �����ɼ�����*/
	{ 0x0113, TYPE_BYTE | 8, (uint8_t*)&( jt808_param.id_0x0113 ) },    //uint8_t		id_0x0113[8];   /*0x0113 ����CAN ���� ID �����ɼ�����*/
	{ 0x0114, TYPE_BYTE | 8, (uint8_t*)&( jt808_param.id_0x0114 ) },    //uint8_t		id_0x0114[8];   /*0x0114 ����CAN ���� ID �����ɼ�����*/
	{ 0x0115, TYPE_BYTE | 8, (uint8_t*)&( jt808_param.id_0x0115 ) },    //uint8_t		id_0x0115[8];   /*0x0115 ����CAN ���� ID �����ɼ�����*/
	{ 0x0116, TYPE_BYTE | 8, (uint8_t*)&( jt808_param.id_0x0116 ) },    //uint8_t		id_0x0116[8];   /*0x0116 ����CAN ���� ID �����ɼ�����*/
	{ 0x0117, TYPE_BYTE | 8, (uint8_t*)&( jt808_param.id_0x0117 ) },    //uint8_t		id_0x0117[8];   /*0x0117 ����CAN ���� ID �����ɼ�����*/
	{ 0x0118, TYPE_BYTE | 8, (uint8_t*)&( jt808_param.id_0x0118 ) },    //uint8_t		id_0x0118[8];   /*0x0118 ����CAN ���� ID �����ɼ�����*/
	{ 0x0119, TYPE_BYTE | 8, (uint8_t*)&( jt808_param.id_0x0119 ) } //uint8_t		id_0x0119[8];   /*0x0119 ����CAN ���� ID �����ɼ�����*/
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
uint8_t param_put( uint16_t id, uint8_t len, uint8_t* value )
{
	int		i, j;
	uint8_t *psrc, *pdst;

	for( i = 0; i < sizeof( tbl_id_lookup ) / sizeof( struct _tbl_id_lookup ); i++ )
	{
		if( id == tbl_id_lookup[i].id )
		{
			if( ( tbl_id_lookup[i].type == TYPE_DWORD ) && ( len == 4 ) )
			{
				psrc = value;
				//rt_kprintf("psrc=%02x %02x %02x %02x \r\n",*(psrc+3),*(psrc+2),*(psrc+1),*(psrc+0));
				pdst	= tbl_id_lookup[i].val;
				*pdst++ = *( psrc + 3 );
				*pdst++ = *( psrc + 2 );
				*pdst++ = *( psrc + 1 );
				*pdst	= *( psrc + 0 );
				return 0;
			}
			if( ( tbl_id_lookup[i].type == TYPE_WORD ) && ( len == 2 ) )
			{
				psrc	= value;
				pdst	= tbl_id_lookup[i].val;
				*pdst++ = *( psrc + 1 );
				*pdst	= *psrc;
				return 0;
			}
			if( ( tbl_id_lookup[i].type == TYPE_BYTE ) && ( len == 1 ) )
			{
				psrc	= value;
				pdst	= tbl_id_lookup[i].val;
				*pdst	= *psrc;
				return 0;
			}
			if( tbl_id_lookup[i].type == TYPE_CAN_ID )
			{
				psrc	= value;
				pdst	= tbl_id_lookup[i].val;
				*pdst++ = *( psrc + 3 );
				*pdst++ = *( psrc + 2 );
				*pdst++ = *( psrc + 1 );
				*pdst++ = *psrc;
				*pdst++ = *( psrc + 7 );
				*pdst++ = *( psrc + 6 );
				*pdst++ = *( psrc + 5 );
				*pdst	= *( psrc + 4 );
				return 0;
			}
			if( tbl_id_lookup[i].type == TYPE_STR )
			{
				psrc = tbl_id_lookup[i].val;
				strncpy( psrc, value, 32 );
				*( psrc + 31 ) = 0;
				return 0;
			}
		}
	}
	return 1;
}

/*���ò���*/
static void param_put_int( uint16_t id, uint32_t value )
{
	uint32_t	i, j;
	int			len;
	uint8_t		*p;

	for( i = 0; i < sizeof( tbl_id_lookup ) / sizeof( struct _tbl_id_lookup ); i++ )
	{
		if( id == tbl_id_lookup[i].id )
		{
			p		= tbl_id_lookup[i].val;
			j		= value;
			*p++	= ( j & 0xff );
			*p++	= j >> 8;
			*p++	= j >> 16;
			*p		= j >> 24;
			break;
		}
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
static void param_put_str( uint16_t id, uint8_t* value )
{
	int		i, j;
	int		len;
	uint8_t *p;

	for( i = 0; i < sizeof( tbl_id_lookup ) / sizeof( struct _tbl_id_lookup ); i++ )
	{
		if( id == tbl_id_lookup[i].id )
		{
			p = tbl_id_lookup[i].val;
			strncpy( p, (char*)value, 32 );
			break;
		}
	}
}

/*��ȡ����,���ز������Ͳ���*/
uint8_t param_get( uint16_t id, uint8_t* value )
{
	int			i;
	uint8_t		*p;
	uint32_t	val;
	for( i = 0; i < sizeof( tbl_id_lookup ) / sizeof( struct _tbl_id_lookup ); i++ )
	{
		if( id == tbl_id_lookup[i].id )
		{
			if( tbl_id_lookup[i].type == TYPE_DWORD )
			{
				p = tbl_id_lookup[i].val;
				memcpy( value, p, 4 );
				return 4;
			}

			if( tbl_id_lookup[i].type == TYPE_WORD )
			{
				p = tbl_id_lookup[i].val;
				memcpy( value, p, 2 );
				return 2;
			}

			if( tbl_id_lookup[i].type == TYPE_BYTE )
			{
				p		= tbl_id_lookup[i].val;
				*value	= *p;
				return 1;
			}
			if( tbl_id_lookup[i].type == TYPE_STR )
			{
				p = tbl_id_lookup[i].val;
				memcpy( value, p, strlen( p ) );
				return strlen( p );
			}
			if( tbl_id_lookup[i].type == TYPE_CAN_ID )
			{
				p = tbl_id_lookup[i].val;
				memcpy( value, p, 8 );
				return 8;
			}
		}
	}
	return 0;
}

/*��ȡ����*/
uint32_t param_get_int( uint16_t id )
{
	int			i;
	uint8_t		*p;
	uint32_t	val = 0;
	for( i = 0; i < sizeof( tbl_id_lookup ) / sizeof( struct _tbl_id_lookup ); i++ )
	{
		if( id == tbl_id_lookup[i].id )
		{
			p	= tbl_id_lookup[i].val;
			val |= ( *p++ );
			val |= ( ( *p++ ) << 8 );
			val |= ( ( *p++ ) << 16 );
			val |= ( ( *p ) << 24 );
			return val;
		}
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
void param_print( void )
{
	int			i, j, id;
	int			type;
	uint8_t		*p;
	uint32_t	val = 0;

	for( i = 0; i < sizeof( tbl_id_lookup ) / sizeof( struct _tbl_id_lookup ); i++ )
	{
		id	= tbl_id_lookup[i].id;
		p	= tbl_id_lookup[i].val;
		val = 0;
		switch( tbl_id_lookup[i].type )
		{
			case TYPE_DWORD: /*�ֽڶ��뷽ʽ little_endian*/
				val |= ( *p++ );
				val |= ( ( *p++ ) << 8 );
				val |= ( ( *p++ ) << 16 );
				val |= ( ( *p ) << 24 );
				rt_kprintf( "\r\nid=%04x value=%08x\r\n", id, val );
				break;
			case TYPE_CAN_ID:
				val |= ( *p++ );
				val |= ( ( *p++ ) << 8 );
				val |= ( ( *p++ ) << 16 );
				val |= ( ( *p++ ) << 24 );
				rt_kprintf( "\r\nid=%04x value=%08x", id, val );
				val = 0;
				val |= ( *p++ );
				val |= ( ( *p++ ) << 8 );
				val |= ( ( *p++ ) << 16 );
				val |= ( ( *p ) << 24 );
				rt_kprintf( " %08x\r\n", val );
				break;

				break;
			case TYPE_STR:
				rt_kprintf( "\r\nid=%04x value=%s\r\n", id, p );
				break;
		}
	}
}

FINSH_FUNCTION_EXPORT( param_print, print param );

/*��ӡ������Ϣ*/
void param_dump( void )
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

FINSH_FUNCTION_EXPORT( param_dump, dump param );


/*
   jt808��ʽ���ݽ����ж�
   <��ʶ0x7e><��Ϣͷ><��Ϣ��><У����><��ʶ0x7e>

   ������Ч�����ݳ���,Ϊ0 �����д�

 */
static uint16_t jt808_decode_fcs( uint8_t * pinfo, uint16_t length )
{
	uint8_t		* psrc, * pdst;
	uint16_t	count, len;
	uint8_t		fstuff	= 0;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        /*�Ƿ��ֽ����*/
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
	psrc	= pinfo + 1;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            /*1byte��ʶ��Ϊ��ʽ��Ϣ*/
	pdst	= pinfo;
	count	= 0;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    /*ת���ĳ���*/
	len		= length - 2;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           /*ȥ����ʶλ�����ݳ���*/

	while( len )
	{
		if( fstuff )
		{
			*pdst	= *psrc + 0x7c;
			fstuff	= 0;
			count++;
			fcs ^= *pdst;
		} else
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

/**���һ���ֽ�**/
static uint16_t jt808_pack_byte( uint8_t * buf, uint8_t * fcs, uint8_t data )
{
	uint8_t * p = buf;
	*fcs ^= data;
	if( ( data == 0x7d ) || ( data == 0x7e ) )
	{
		*p++	= 0x7d;
		*p		= ( data - 0x7c );
		return 2;
	} else
	{
		*p = data;
		return 1;
	}
}

/*���ݽ����ȣ����ڼ���*/
static uint16_t jt808_pack_int( uint8_t * buf, uint8_t * fcs, uint32_t data, uint8_t width )
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

/*����ַ���***/
static uint16_t jt808_pack_string( uint8_t * buf, uint8_t * fcs, char * str )
{
	uint16_t	count	= 0;
	char		* p		= str;
	while( *p )
	{
		count += jt808_pack_byte( buf + count, fcs, *p++ );
	}
	return count;
}

/**�������**/
static uint16_t jt808_pack_array( uint8_t * buf, uint8_t * fcs, uint8_t * src, uint16_t len )
{
	uint16_t	count = 0;
	int			i;
	char		* p = src;
	for( i = 0; i < len; i++ )
	{
		count += jt808_pack_byte( buf + count, fcs, *p++ );
	}
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
static void jt808_send( void * parameter )
{
}

/*���ͺ��յ�Ӧ����*/
void jt808_tx_response( uint8_t linkno, uint8_t *pmsg )
{
	uint8_t		* msg = pmsg + 12;
	uint16_t	id;
	uint16_t	seq;
	uint8_t		res;

	seq = ( *msg << 8 ) | *( msg + 1 );
	id	= ( *( msg + 2 ) << 8 ) | *( msg + 3 );
	res = *( msg + 4 );

	switch( id )                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                // �ж϶�Ӧ�ն���Ϣ��ID�����ִ���
	{
		case 0x0200:                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            //	��Ӧλ����Ϣ��Ӧ��
			rt_kprintf( "\r\nCentre ACK!\r\n" );
			break;
		case 0x0002:                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            //	��������Ӧ��
			rt_kprintf( "\r\n  Centre  Heart ACK!\r\n" );
			break;
		case 0x0101:                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            //	�ն�ע��Ӧ��
			break;
		case 0x0102:                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            //	�ն˼�Ȩ
			break;
		case 0x0800:                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            // ��ý���¼���Ϣ�ϴ�
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
static rt_err_t jt808_tx_timeout( JT808_TX_MSG_NODEDATA * nodedata )
{
	rt_kprintf( "tx timeout\r\n" );
}

/*
   ���һ����Ϣ�������б���
 */
static rt_err_t jt808_add_tx_data( uint8_t linkno, JT808_MSG_TYPE type, uint16_t id, uint8_t *pinfo, uint16_t len )
{
	uint8_t					* pdata;
	JT808_TX_MSG_NODEDATA	* pnodedata;

	pnodedata = rt_malloc( sizeof( JT808_TX_MSG_NODEDATA ) );
	if( pnodedata == NULL )
	{
		return -RT_ERROR;
	}
	pnodedata->type				= type;
	pnodedata->state			= IDLE;
	pnodedata->retry			= 0;
	pnodedata->cb_tx_timeout	= jt808_tx_timeout;
	pnodedata->cb_tx_response	= jt808_tx_response;
//�ڴ˿��Դ洢���ϱ�
	pdata = rt_malloc( len );
	if( pdata == NULL )
	{
		rt_free( pnodedata );
		return -RT_ERROR;
	}
	memcpy( pdata, pinfo, len );
	pnodedata->msg_len	= len;
	pnodedata->pmsg		= pdata;
	pnodedata->head_sn	= tx_seq;
	pnodedata->head_id	= id;
	msglist_append( list_jt808_tx, pnodedata );
	tx_seq++;
}

/*
   �ն�ͨ��Ӧ��
 */
static rt_err_t jt808_tx_0x0001( uint8_t linkno, uint16_t seq, uint16_t id, uint8_t res )
{
	uint8_t					* pdata;
	JT808_TX_MSG_NODEDATA	* pnodedata;
	uint8_t					buf [256];
	uint8_t					* p;
	uint16_t				len;

	pnodedata = rt_malloc( sizeof( JT808_TX_MSG_NODEDATA ) );
	if( pnodedata == NULL )
	{
		return -RT_ERROR;
	}
	pnodedata->type				= TERMINAL_ACK;
	pnodedata->state			= IDLE;
	pnodedata->retry			= 0;
	pnodedata->cb_tx_timeout	= jt808_tx_timeout;
	pnodedata->cb_tx_response	= jt808_tx_response;

	len		= jt808_pack( buf, "%w%6s%w%w%w%b", 0x0001, term_param.mobile, tx_seq, seq, id, res );
	pdata	= rt_malloc( len );
	if( pdata == NULL )
	{
		rt_free( pnodedata );
		return -RT_ERROR;
	}
	memcpy( pdata, buf, len );
	pnodedata->msg_len	= len;
	pnodedata->pmsg		= pdata;
	msglist_append( list_jt808_tx, pnodedata );
	tx_seq++;
}

/*ƽ̨ͨ��Ӧ��,�յ���Ϣ��ֹͣ����*/
static int handle_rx_0x8001( uint8_t linkno, uint8_t *pmsg )
{
	MsgListNode				* iter;
	JT808_TX_MSG_NODEDATA	* iterdata;

	uint16_t				id;
	uint16_t				seq;
	uint8_t					res;
/*������Ϣͷ12byte*/
	seq = ( *( pmsg + 12 ) << 8 ) | *( pmsg + 13 );
	id	= ( *( pmsg + 14 ) << 8 ) | *( pmsg + 15 );
	res = *( pmsg + 16 );

	/*��������*/
	iter		= list_jt808_tx->first;
	iterdata	= (JT808_TX_MSG_NODEDATA*)iter->data;
	if( ( iterdata->head_id == id ) && ( iterdata->head_sn == seq ) )
	{
		iterdata->cb_tx_response( linkno, pmsg ); /*Ӧ������*/
		iterdata->state = ACK_OK;
	}
}

/*�����ְ�����*/
static int handle_rx_0x8003( uint8_t linkno, uint8_t *pmsg )
{
}

/* ������Ķ��ն�ע����Ϣ��Ӧ��*/
static int handle_rx_0x8100( uint8_t linkno, uint8_t *pmsg )
{
	MsgListNode				* iter;
	JT808_TX_MSG_NODEDATA	* iterdata;

	uint16_t				body_len; /*��Ϣ�峤��*/
	uint16_t				ack_seq;
	uint8_t					res;
	uint8_t					* msg;

	body_len	= ( ( *( pmsg + 2 ) << 8 ) | ( *( pmsg + 3 ) ) ) & 0x3FF;
	msg			= pmsg + 12;

	ack_seq = ( *msg << 8 ) | *( msg + 1 );
	res		= *( msg + 2 );

	iter		= list_jt808_tx->first;
	iterdata	= iter->data;
	if( ( iterdata->head_id == 0x0100 ) && ( iterdata->head_sn == ack_seq ) )
	{
		if( res == 0 )
		{
			strncpy( term_param.register_code, msg + 3, body_len - 3 );
			iterdata->state = ACK_OK;
		}
	}
	return 1;
}

/*�����ն˲���*/
static int handle_rx_0x8103( uint8_t linkno, uint8_t *pmsg )
{
	uint8_t		* p;
	uint8_t		res;

	uint16_t	msg_len, count = 0;
	uint32_t	param_id;
	uint8_t		param_len;

	uint16_t	seq, id;

	if( *( pmsg + 2 ) >= 0x20 ) /*����Ƕ�������ò���*/
	{
		rt_kprintf( "\r\n>%s multi packet no support!", __func__ );
		return 1;
	}

	id	= ( pmsg[0] << 8 ) | pmsg[1];
	seq = ( pmsg[10] << 8 ) | pmsg[11];

	msg_len = ( ( pmsg[2] << 8 ) | pmsg[3] ) & 0x3FF - 1;
	p		= pmsg + 13;

	/*ʹ�����ݳ���,�ж������Ƿ������û��ʹ�ò�������*/
	while( count < msg_len )
	{
		param_id	= ( ( *p++ ) << 24 ) | ( ( *p++ ) << 16 ) | ( ( *p++ ) << 8 ) | ( *p++ );
		param_len	= *p++;
		count		+= ( 5 + param_len );
		res			|= param_put( param_id, param_len, p );
		if( res )
		{
			rt_kprintf( "\r\n%s>res=%d\r\n", __func__, __LINE__ );
			break;
		}
	}
	/*����ͨ��Ӧ��*/
	jt808_tx_0x0001( linkno, seq, id, res );
	return 1;
}

/*��ѯȫ���ն˲������п��ܻᳬ����������ֽ�*/
static int handle_rx_0x8104( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/*�ն˿���*/
static int handle_rx_0x8105( uint8_t linkno, uint8_t *pmsg )
{
	uint8_t cmd;
	uint8_t * cmd_arg;

	cmd = *( pmsg + 12 );
	switch( cmd )
	{
		case 1: /*��������*/
			break;
		case 2: /*�ն˿�������ָ��������*/
			break;
		case 3: /*�ն˹ػ�*/
			break;
		case 4: /*�ն˸�λ*/
			break;
		case 5: /*�ָ���������*/
			break;
		case 6: /*�ر�����ͨѶ*/
			break;
		case 7: /*�ر���������ͨѶ*/
			break;
	}
	return 1;
}

/*��ѯָ���ն˲���,����Ӧ��0x0104*/
static int handle_rx_0x8106( uint8_t linkno, uint8_t *pmsg )
{
	int			i;
	uint8_t		*p;
	uint8_t		fcs = 0;
	uint8_t		value[8];
	uint32_t	id;
	uint16_t	len;
	uint16_t	pos;
	uint16_t	info_len	= 0;
	uint16_t	head_len	= 0;
	uint8_t		param_count, return_param_count;

	uint8_t		buf[1500];

	pos					= 100;              /*�ȿճ�100byte*/
	param_count			= *( pmsg + 12 );   /*�ܵĲ�������*/
	return_param_count	= 0;
	p					= pmsg + 13;
	/*���Ҫ������Ϣ�����ݣ�����¼����*/
	for( i = 0; i < param_count; i++ )      /*�����δ֪��id��ô�죬����,�������������͸ı���*/
	{
		id	= *p++;
		id	|= ( *p++ ) << 8;
		id	|= ( *p++ ) << 16;
		id	|= ( *p++ ) << 24;
		len = param_get( id, value );       /*�õ������ĳ��ȣ�δת��*/
		if( len )
		{
			return_param_count++;           /*�ҵ���Ч��id*/
			pos += jt808_pack_int( buf + pos, &fcs, id, 2 );
			pos + jt808_pack_int( buf + pos, &fcs, len, 1 );
			pos			+= jt808_pack_array( buf + pos, &fcs, value, len );
			info_len	+= ( len + 3 );     /*id+����+����*/
		}
	}

	head_len	= 1;                        /*�ճ���ʼ��0x7e*/
	head_len	+= jt808_pack_int( buf + head_len, &fcs, 0x0104, 2 );
	head_len	+= jt808_pack_int( buf + head_len, &fcs, info_len + 3, 2 );
	head_len	+= jt808_pack_array( buf + head_len, &fcs, pmsg + 4, 6 );
	head_len	+= jt808_pack_int( buf + head_len, &fcs, tx_seq, 2 );

	head_len	+= jt808_pack_array( buf + head_len, &fcs, pmsg + 10, 2 );
	head_len	+= jt808_pack_int( buf + head_len, &fcs, return_param_count, 1 );

	memcpy( buf + head_len, buf + 100, pos - 100 ); /*ƴ������*/
	len = head_len + pos - 100;                     /*��ǰ����0x7e,<head><msg>*/

	len			+= jt808_pack_byte( buf + len, &fcs, fcs );
	buf [0]		= 0x7e;
	buf [len]	= 0x7e;

	jt808_add_tx_data( linkno, TERMINAL_ACK, 0x0104, buf, len + 1 );
	return 1;
}

/*��ѯ�ն�����,Ӧ�� 0x0107*/
static int handle_rx_0x8107( uint8_t linkno, uint8_t *pmsg )
{
	uint8_t		buf[100];
	uint8_t		fcs			= 0;
	uint16_t	len			= 1;
	uint16_t	info_len	= 0;
	uint16_t	head_len	= 1;

	len += jt808_pack_int( buf + len, &fcs, 0x0107, 2 );
	len += jt808_pack_int( buf + len, &fcs, 0x0107, 2 );
	len += jt808_pack_int( buf + len, &fcs, 0x0107, 2 );

	jt808_add_tx_data( linkno, TERMINAL_ACK, 0x0107, buf, len + 1 );
	return 1;
}

/**/
static int handle_rx_0x8201( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8202( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8300( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8301( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8302( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8303( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8304( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8400( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8401( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8500( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8600( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8601( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8602( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8603( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8604( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8605( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8606( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8607( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/*��ʻ��¼�����ݲɼ�*/
static int handle_rx_0x8700( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/*��ʻ��¼�ǲ����´�*/
static int handle_rx_0x8701( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8800( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/*����ͷ������������*/
static int handle_rx_0x8801( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8802( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8803( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8804( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8805( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8900( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8A00( uint8_t linkno, uint8_t *pmsg )
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
static int handle_rx_default( uint8_t linkno, uint8_t *pmsg )
{
	rt_kprintf( "\r\nunknown!\r\n" );
	return 1;
}

#define DECL_JT808_RX_HANDLE( a )	{ a, handle_rx_ ## a }
#define DECL_JT808_TX_HANDLE( a )	{ a, handle_jt808_tx_ ## a }

HANDLE_JT808_RX_MSG handle_rx_msg[] =
{
	DECL_JT808_RX_HANDLE( 0x8001 ), //	ͨ��Ӧ��
	DECL_JT808_RX_HANDLE( 0x8003 ), //	�����ְ�����
	DECL_JT808_RX_HANDLE( 0x8100 ), //  ������Ķ��ն�ע����Ϣ��Ӧ��
	DECL_JT808_RX_HANDLE( 0x8103 ), //	�����ն˲���
	DECL_JT808_RX_HANDLE( 0x8104 ), //	��ѯ�ն˲���
	DECL_JT808_RX_HANDLE( 0x8105 ), // �ն˿���
	DECL_JT808_RX_HANDLE( 0x8106 ), // ��ѯָ���ն˲���
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
	uint8_t		* psrc;
	uint16_t	len;
	uint8_t		linkno;
	uint16_t	i, id;
	uint8_t		flag_find	= 0;
	uint8_t		fcs			= 0;
	uint16_t	ret;

	linkno	= pinfo [0];
	len		= ( pinfo [1] << 8 ) | pinfo [2];
	rt_kprintf( ">dump start len=%d\r\n", len );

/*ȥת�壬����ֱ����pinfo�ϲ���*/
	len = jt808_decode_fcs( pinfo + 3, len );
	if( len == 0 )
	{
		rt_kprintf( ">len=0\r\n" );
		return 1;
	}
/*��ʾ��������Ϣ*/
	rt_kprintf( "\r\n>dump start" );
	psrc = pinfo + 3;
	for( i = 0; i < len; i++ )
	{
		if( i % 16 == 0 )
		{
			rt_kprintf( "\r\n" );
		}
		rt_kprintf( "%02x ", *psrc++ );
	}
	rt_kprintf( "\r\n>dump end\r\n" );
/*fcs����*/
	psrc = pinfo + 3;
	for( i = 0; i < len - 1; i++ )
	{
		fcs ^= *psrc++;
	}
	if( fcs != *( psrc + len - 1 ) )
	{
		rt_kprintf( "\r\n%d>%s fcs error!", rt_tick_get( ), __func__ );
		return 1;
	}

/*ֱ�Ӵ����յ�����Ϣ������ID�ַ���ֱ�ӷַ���Ϣ*/

	psrc	= pinfo + 3;
	id		= ( *psrc << 8 ) | *( psrc + 1 );

	for( i = 0; i < sizeof( handle_rx_msg ) / sizeof( HANDLE_JT808_RX_MSG ); i++ )
	{
		if( id == handle_rx_msg [i].id )
		{
			handle_rx_msg [i].func( linkno, psrc );
			flag_find = 1;
		}
	}
	if( !flag_find )
	{
		handle_rx_default( linkno, psrc );
	}
}

/*
   ����ÿ��Ҫ������Ϣ��״̬
   ���������д�����?
 */

static MsgListRet jt808_tx_proc( MsgListNode * node )
{
	MsgListNode				* pnode		= ( MsgListNode* )node;
	JT808_TX_MSG_NODEDATA	* pnodedata = ( JT808_TX_MSG_NODEDATA* )( pnode->data );
	int						i;

	if( node == RT_NULL )
	{
		return MSGLIST_RET_OK;
	}

	if( pnodedata->state == IDLE )                      /*���У�������Ϣ��ʱ��û������*/
	{
		if( pnodedata->retry >= jt808_param.id_0x0003 ) /*����������ش�����*/                                                                     /*�Ѿ��ﵽ���Դ���*/
		{
			/*��ʾ����ʧ��*/
			pnodedata->cb_tx_timeout( pnodedata );      /*���÷���ʧ�ܴ�����*/
			return MSGLIST_RET_DELETE_NODE;
		} else
		{
			socket_write( pnodedata->linkno, pnodedata->pmsg, pnodedata->msg_len, jt808_param.id_0x0002 * RT_TICK_PER_SECOND );
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
		return MSGLIST_RET_OK;
	}

	if( pnodedata->state == ACK_OK )
	{
		return MSGLIST_RET_DELETE_NODE;
	}

	return MSGLIST_RET_OK;
}

/*jt808��socket����

   ά����·�����в�ͬ��ԭ��
   �ϱ�״̬��ά��
   1.��δ����
   2.�������ӣ�DNS,��ʱ��Ӧ��
   3.��ֹ�ϱ����ر�ģ�������
   4.��ǰ���ڽ��п��и��£���ý���ϱ��Ȳ���Ҫ��ϵĹ���

 */

typedef enum
{
	CONNECT_IDLE = 0,
	CONNECTING,
	CONNECTED
}CONN_STATE;

struct
{
	uint8_t		disable_connect; /*��ֹ���ӱ�־��Э����� 0:��������*/
	CONN_STATE	server_state;
	uint8_t		server_index;
	CONN_STATE	auth_state;
	uint8_t		auth_index;
} connect_state =
{ 0, CONNECT_IDLE, 0, CONNECT_IDLE, 0 };


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static void jt808_socket_proc( void )
{
	T_SOCKET_STATE	state;
	rt_tick_t		lasttick;

	if( flag_disable_report )
	{
		return;
	}
/*���GSM״̬*/
	state = gsmstate( 0 );
	if( state == GSM_IDLE )
	{
		if( connect_state.disable_connect == 0 )
		{
			gsmstate( GSM_POWERON ); /*��������*/
		}
		return;
	}
	if( state != GSM_AT )
	{
		return;
	}

/*��GSM_AT������£����μ�����ӵ�״̬*/
/*���808������������״̬*/
	if( connect_state.server_state == CONNECT_IDLE )                                    /*û������*/
	{
		psocket						= &gsm_socket[connect_state.server_index % 2];      /*�������ӱ���,˫����������*/
		psocket->state				= SOCKET_INIT;
		connect_state.server_state	= CONNECTING;
	}

	if( connect_state.server_state == CONNECTED )                                       /*808������������,����IC��������*/
	{
		if( connect_state.auth_state == CONNECT_IDLE )                                  /*û������*/
		{
			psocket						= &gsm_socket[connect_state.auth_index % 2];    /*�������ӱ���,˫����������*/
			psocket->state				= SOCKET_INIT;
			connect_state.auth_state	= CONNECTING;
		}
		if( connect_state.auth_state == CONNECTING )
		{
		}
	}

/*���socket״̬,�ж��ò���DNS*/
	switch( psocket->state )
	{
		case SOCKET_IDLE:
			break;
		case SOCKET_CONNECT_ERR:
		case SOCKET_DNS_ERR:
			gsmstate( GSM_POWEROFF );   /*�ر�gsm�ȴ�����*/
			break;
	}
}

/*
   ����״̬ά��
   jt808Э�鴦��

 */
ALIGN( RT_ALIGN_SIZE )
static char thread_jt808_stack [4096];
struct rt_thread thread_jt808;

/***/
static void rt_thread_entry_jt808( void * parameter )
{
	rt_err_t				ret;
	int						i;
	uint8_t					* pstr;

	MsgListNode				* iter;
	MsgListNode				* iter_next;
	JT808_TX_MSG_NODEDATA	* pnodedata;

	int						j = 0xaabbccdd;

	rt_kprintf( "\r\n1.id0=%08x\r\n", param_get_int( 0x0000 ) );

	param_put_int( 0x000, j );
	rt_kprintf( "\r\nwrite j=%08x read=%08x\r\n", j, param_get_int( 0x0000 ) );

	param_put( 0x000, 4, (uint8_t*)&j );
	rt_kprintf( "\r\nid0=%08x\r\n", param_get_int( 0x0000 ) );


/*��ȡ������������*/
	param_load( );

	list_jt808_tx	= msglist_create( );
	list_jt808_rx	= msglist_create( );

	pdev_gsm = rt_device_find( "gsm" );

	while( 1 )
	{
/*����gps��Ϣ*/
		ret = rt_mb_recv( &mb_gpsdata, ( rt_uint32_t* )&pstr, 5 );
		if( ret == RT_EOK )
		{
			gps_analy( pstr );
			rt_free( pstr );
		}
/*����gprs��Ϣ*/
		ret = rt_mb_recv( &mb_gprsdata, ( rt_uint32_t* )&pstr, 5 );
		if( ret == RT_EOK )
		{
			jt808_rx_proc( pstr );
			rt_free( pstr );
		}
/*jt808 socket����*/
		jt808_socket_proc( );

/*������Ϣ��������*/
		iter = list_jt808_tx->first;

		if( jt808_tx_proc( iter ) == MSGLIST_RET_DELETE_NODE )  /*ɾ���ýڵ�*/
		{
			pnodedata = ( JT808_TX_MSG_NODEDATA* )( iter->data );
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
	                &thread_jt808_stack [0],
	                sizeof( thread_jt808_stack ), 10, 5 );
	rt_thread_startup( &thread_jt808 );
}

/*gps���մ���*/
void gps_rx( uint8_t * pinfo, uint16_t length )
{
	uint8_t * pmsg;
	pmsg = rt_malloc( length + 2 );
	if( pmsg != RT_NULL )
	{
		pmsg [0]	= length >> 8;
		pmsg [1]	= length & 0xff;
		memcpy( pmsg + 2, pinfo, length );
		rt_mb_send( &mb_gpsdata, ( rt_uint32_t )pmsg );
	}
}

/*gprs���մ���,�յ�����Ҫ���촦��*/
rt_err_t gprs_rx( uint8_t linkno, uint8_t * pinfo, uint16_t length )
{
	uint8_t * pmsg;
	pmsg = rt_malloc( length + 3 );                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           /*����������Ϣ*/
	if( pmsg != RT_NULL )
	{
		pmsg [0]	= linkno;
		pmsg [1]	= length >> 8;
		pmsg [2]	= length & 0xff;
		memcpy( pmsg + 3, pinfo, length );
		rt_mb_send( &mb_gprsdata, ( rt_uint32_t )pmsg );
		return 0;
	}
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
rt_err_t server( uint8_t cmd, char * apn, char * name, char * psw )
{
	if( cmd == 0 )
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
	if( cmd == 1 )                                                                                                                                          /*������server*/
	{
	}
	if( cmd == 2 )                                                                                                                                          /*���ñ���server*/
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
static rt_err_t jt808_tx( void )
{
	uint8_t					* pdata;
	JT808_TX_MSG_NODEDATA	* pnodedata;
	uint8_t					buf [256];
	uint8_t					* p;
	uint16_t				len;
	uint8_t					fcs = 0;
	int						i;

	pnodedata = rt_malloc( sizeof( JT808_TX_MSG_NODEDATA ) );
	if( pnodedata == NULL )
	{
		return -1;
	}
	pnodedata->type				= TERMINAL_CMD;
	pnodedata->state			= IDLE;
	pnodedata->retry			= 0;
	pnodedata->cb_tx_timeout	= jt808_tx_timeout;
	pnodedata->cb_tx_response	= jt808_tx_response;

	len = jt808_pack( buf, "%w%w%6s%w%w%w%5s%20s%7s%b%s",
	                  0x0100,
	                  37 + strlen( jt808_param.id_0x0083 ),
	                  term_param.mobile,
	                  tx_seq,
	                  jt808_param.id_0x0081,
	                  jt808_param.id_0x0082,
	                  term_param.producer_id,
	                  term_param.model,
	                  term_param.terminal_id,
	                  jt808_param.id_0x0084,
	                  jt808_param.id_0x0083 );

	rt_kprintf( "\r\n*********************\r\n" );
	for( i = 0; i < len; i++ )
	{
		rt_kprintf( "%02x ", buf [i] );
	}
	rt_kprintf( "\r\n*********************\r\n" );

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
	buf [0]		= 0x7e;
	buf [len]	= 0x7e;
	pdata		= rt_malloc( len + 1 );
	if( pdata == NULL )
	{
		rt_free( pnodedata );
		return;
	}
	rt_kprintf( "\r\n--------------------\r\n" );
	for( i = 0; i < len + 1; i++ )
	{
		rt_kprintf( "%02x ", buf [i] );
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

