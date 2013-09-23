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

#include "jt808_param.h"
#include "sst25.h"
#include "jt808.h"
#include <finsh.h>
#include "string.h"

#define TYPE_BYTE	0x01                /*�̶�Ϊ1�ֽ�,С�˶���*/
#define TYPE_WORD	0x02                /*�̶�Ϊ2�ֽ�,С�˶���*/
#define TYPE_DWORD	0x04                /*�̶�Ϊ4�ֽ�,С�˶���*/
#define TYPE_STR	0x80                /*�̶�Ϊ32�ֽ�,����˳��*/
#define TYPE_CAN	0x48                /*�̶�Ϊ8�ֽ�,��ǰ�洢CAN_ID����*/

JT808_PARAM jt808_param =
{
	0x13091305,                         /*0x0000 �汾*/
	50,                                 /*0x0001 �������ͼ��*/
	10,                                 /*0x0002 TCPӦ��ʱʱ��*/
	3,                                  /*0x0003 TCP��ʱ�ش�����*/
	10,                                 /*0x0004 UDPӦ��ʱʱ��*/
	5,                                  /*0x0005 UDP��ʱ�ش�����*/
	60,                                 /*0x0006 SMS��ϢӦ��ʱʱ��*/
	3,                                  /*0x0007 SMS��Ϣ�ش�����*/
	"CMNET",                            /*0x0010 ��������APN*/
	"",                                 /*0x0011 �û���*/
	"",                                 /*0x0012 ����*/
	"jt1.gghypt.net",                   /*0x0013 ����������ַ*/
	"CMNET",                            /*0x0014 ����APN*/
	"",                                 /*0x0015 �����û���*/
	"",                                 /*0x0016 ��������*/
	"jt2.gghypt.net",                   /*0x0017 ���ݷ�������ַ��ip������*/
	7008,                               /*0x0018 TCP�˿�*/
	5678,                               /*0x0019 UDP�˿�*/
	"www.google.com",                   /*0x001A ic������������ַ��ip������*/
	9901,                               /*0x001B ic��������TCP�˿�*/
	8875,                               /*0x001C ic��������UDP�˿�*/
	"www.google.com",                   /*0x001D ic�����ݷ�������ַ��ip������*/
	0,                                  /*0x0020 λ�û㱨����*/
	1,                                  /*0x0021 λ�û㱨����*/
	30,                                 /*0x0022 ��ʻԱδ��¼�㱨ʱ����*/
	120,                                /*0x0027 ����ʱ�㱨ʱ����*/
	5,                                  /*0x0028 ��������ʱ�㱨ʱ����*/
	30,                                 /*0x0029 ȱʡʱ��㱨���*/
	500,                                /*0x002c ȱʡ����㱨���*/
	1000,                               /*0x002d ��ʻԱδ��¼�㱨������*/
	1000,                               /*0x002e ����ʱ����㱨���*/
	100,                                /*0x002f ����ʱ����㱨���*/
	270,                                /*0x0030 �յ㲹���Ƕ�*/
	500,                                /*0x0031 ����Χ���뾶���Ƿ�λ����ֵ������λΪ��*/
	"1008611",                          /*0x0040 ���ƽ̨�绰����*/
	"",                                 /*0x0041 ��λ�绰����*/
	"",                                 /*0x0042 �ָ��������õ绰����*/
	"",                                 /*0x0043 ���ƽ̨SMS����*/
	"",                                 /*0x0044 �����ն�SMS�ı���������*/
	5,                                  /*0x0045 �ն˽����绰����*/
	3,                                  /*0x0046 ÿ��ͨ��ʱ��*/
	3,                                  /*0x0047 ����ͨ��ʱ��*/
	"",                                 /*0x0048 �����绰����*/
	"",                                 /*0x0049 ���ƽ̨��Ȩ���ź���*/
	5,                                  /*0x0050 ����������*/
	3,                                  /*0x0051 ���������ı�SMS����*/
	5,                                  /*0x0052 �������տ���*/
	3,                                  /*0x0053 ��������洢��־*/
	5,                                  /*0x0054 �ؼ���־*/
	90,                                 /*0x0055 ����ٶ�kmh*/
	5,                                  /*0x0056 ���ٳ���ʱ��*/
	4 * 60 * 60,                        /*0x0057 ������ʻʱ������*/
	8 * 60 * 60,                        /*0x0058 �����ۼƼ�ʻʱ������*/
	20 * 60,                            /*0x0059 ��С��Ϣʱ��*/
	12 * 60 * 60,                       /*0x005A �ͣ��ʱ��*/
	100,                                /*0x005B ���ٱ���Ԥ����ֵ����λΪ 1/10Km/h */
	90,                                 /*0x005C ƣ�ͼ�ʻԤ����ֵ����λΪ�루s����>0*/
	0x200a,                             /*0x005D ��ײ������������:*/
	30,                                 /*0x005E �෭�����������ã� �෭�Ƕȣ���λ 1 �ȣ�Ĭ��Ϊ 30 ��*/
	0,                                  /*0x0064 ��ʱ���տ���*/
	0,                                  /*0x0065 �������տ���*/
	3,                                  /*0x0070 ͼ����Ƶ����(1-10)*/
	5,                                  /*0x0071 ����*/
	3,                                  /*0x0072 �Աȶ�*/
	5,                                  /*0x0073 ���Ͷ�*/
	3,                                  /*0x0074 ɫ��*/
	5,                                  /*0x0080 ������̱����0.1km*/
	12,                                 /*0x0081 ʡ��ID*/
	101,                                /*0x0082 ����ID*/
	"��AP6834",                         /*0x0083 ����������*/
	2,                                  /*0x0084 ������ɫ	1��ɫ 2��ɫ 3��ɫ 4��ɫ 9����*/
	0x0f,                               /*0x0090 GNSS ��λģʽ*/
	0x01,                               /*0x0091 GNSS ������*/
	0x01,                               /*0x0092 GNSS ģ����ϸ��λ�������Ƶ��*/
	0x01,                               /*0x0093 GNSS ģ����ϸ��λ���ݲɼ�Ƶ��*/
	0x01,                               /*0x0094 GNSS ģ����ϸ��λ�����ϴ���ʽ*/
	0x01,                               /*0x0095 GNSS ģ����ϸ��λ�����ϴ�����*/
	0,                                  /*0x0100 CAN ����ͨ�� 1 �ɼ�ʱ����(ms)��0 ��ʾ���ɼ�*/
	0,                                  /*0x0101 CAN ����ͨ�� 1 �ϴ�ʱ����(s)��0 ��ʾ���ϴ�*/
	0,                                  /*0x0102 CAN ����ͨ�� 2 �ɼ�ʱ����(ms)��0 ��ʾ���ɼ�*/
	0,                                  /*0x0103 CAN ����ͨ�� 2 �ϴ�ʱ����(s)��0 ��ʾ���ϴ�*/
	{ 0, 0, 0, 0, 0, 0, 0, 0 },         /*0x0110 CAN ���� ID �����ɼ�����*/
	{ 0, 0, 0, 0, 0, 0, 0, 0 },         /*0x0111 ����CAN ���� ID �����ɼ�����*/
	{ 0, 0, 0, 0, 0, 0, 0, 0 },         /*0x0112 ����CAN ���� ID �����ɼ�����*/
	{ 0, 0, 0, 0, 0, 0, 0, 0 },         /*0x0113 ����CAN ���� ID �����ɼ�����*/
	{ 0, 0, 0, 0, 0, 0, 0, 0 },         /*0x0114 ����CAN ���� ID �����ɼ�����*/
	{ 0, 0, 0, 0, 0, 0, 0, 0 },         /*0x0115 ����CAN ���� ID �����ɼ�����*/
	{ 0, 0, 0, 0, 0, 0, 0, 0 },         /*0x0116 ����CAN ���� ID �����ɼ�����*/
	{ 0, 0, 0, 0, 0, 0, 0, 0 },         /*0x0117 ����CAN ���� ID �����ɼ�����*/
	{ 0, 0, 0, 0, 0, 0, 0, 0 },         /*0x0118 ����CAN ���� ID �����ɼ�����*/
	{ 0, 0, 0, 0, 0, 0, 0, 0 },         /*0x0119 ����CAN ���� ID �����ɼ�����*/

	"70420",                            /*0xF000 ������ID  70420*/
	"TW703",                            /*0xF001 �ն��ͺ�TW703-BD*/
	"0614100",                          /*0xF002 �ն�ID*/
	"12345",                            /*0xF003 ��Ȩ��*/
	0x07,                               /*0xF004 �ն�����*/
	"0000000000000000",                 /*0xF005 ����VIN*/
	"013920614100",                     /*0xF006 DeviceID*/
	"��ʻ֤����",                       /*0xF007 ��ʻ֤����*/
	"����",                             /*0xF008 ��ʻԱ����*/
	"120104197712015381",               /*0xF009 ��ʻ֤����*/
	"���ͻ���",                         /*0xF00A ��������*/
	"δ֪",                             /*0xF00B ��ҵ�ʸ�֤*/
	"δ֪      ",                       /*0xF00C ��֤����*/

	"1.00",                             /*0xF010 ����汾��*/
	"1.00",                             /*0xF011 Ӳ���汾��*/
	"TJ.GT",                            /*0xF012 ���ۿͻ�����*/
	0x3020,                             /*0xF013 ����ģ���ͺ�0,δȷ�� ,0x3020 Ĭ�� 0x3017*/

	0,                                  /*0xF020 �����*/
	0,                                  /*0xF021 ����״̬*/

	0x35DECC80,                         /*0xF030 ��¼�ǳ��ΰ�װʱ��,mytime��ʽ*/
	0,                                  /*id_0xF031;      ��ʼ���*/
	6250,                               /*id_0xF032;      ��������ϵ��*/

	6,                                  //line_space;               //�м��
	0,                                  //margin_left;				//��߽�
	0,                                  //margin_right;				//�ұ߽�
	1,                                  //step_delay;               //������ʱ,Ӱ���м��
	1,                                  //gray_level;               //�Ҷȵȼ�,����ʱ��
	5,                                  //heat_delay[0];			//������ʱ
	10,                                 //heat_delay[1];			//������ʱ
	15,                                 //heat_delay[2];			//������ʱ
	20,                                 //heat_delay[3];			//������ʱ
};

#define FLAG_DISABLE_REPORT_INVALID 1   /*�豸�Ƿ�*/

#define FLAG_DISABLE_REPORT_AREA 2      /*�����ڽ�ֹ�ϱ�*/

//static uint32_t flag_disable_report = 0;    /*��ֹ�ϱ��ı�־λ*/

/*���������serialflash*/
void param_save( void )
{
	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 5 );
	sst25_write_back( ADDR_PARAM, (uint8_t*)&jt808_param, sizeof( jt808_param ) );
	rt_kprintf( "parma_save size=%d\n", sizeof( jt808_param ) );
	rt_sem_release( &sem_dataflash );
}

FINSH_FUNCTION_EXPORT( param_save, save param );


/*
   ���ز�����serialflash
   ���ʱ����Բ���sem_dataflash
   ��Ϊû������ʹ��

 */
void param_load( void )
{
	/*Ԥ��һ��������*/
	uint8_t		ver8[4];
	uint32_t	ver32;
	//rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 5 );
	sst25_read( ADDR_PARAM, ver8, 4 );
	ver32 = ( ver8[0] ) | ( ver8[1] << 8 ) | ( ver8[2] << 16 ) | ( ver8[3] << 24 );
	if( jt808_param.id_0x0000 != ver32 ) /*�����ǲ���δ��ʼ��*/
	{
		sst25_write_back( ADDR_PARAM, (uint8_t*)&jt808_param, sizeof( jt808_param ) );
	}
	sst25_read( ADDR_PARAM, (uint8_t*)&jt808_param, sizeof( jt808_param ) );
	//rt_sem_release( &sem_dataflash );
	rt_kprintf( "\nparma ver=%x size=%d\n", BYTESWAP4( jt808_param.id_0x0000 ), sizeof( jt808_param ) );
}

FINSH_FUNCTION_EXPORT( param_load, load param );

#define ID_LOOKUP( id, type ) { id, type, (uint8_t*)&( jt808_param.id_ ## id ) }

struct _tbl_id_lookup
{
	uint16_t	id;
	uint8_t		type;
	uint8_t		* val;
} tbl_id_lookup[] = {
	ID_LOOKUP( 0x0000, TYPE_DWORD ),    //uint32_t  id_0x0000;   /*0x0000 �汾*/
	ID_LOOKUP( 0x0001, TYPE_DWORD ),    //uint32_t  id_0x0001;   /*0x0001 �������ͼ��*/
	ID_LOOKUP( 0x0002, TYPE_DWORD ),    //uint32_t  id_0x0002;   /*0x0002 TCPӦ��ʱʱ��*/
	ID_LOOKUP( 0x0003, TYPE_DWORD ),    //uint32_t  id_0x0003;   /*0x0003 TCP��ʱ�ش�����*/
	ID_LOOKUP( 0x0004, TYPE_DWORD ),    //uint32_t  id_0x0004;   /*0x0004 UDPӦ��ʱʱ��*/
	ID_LOOKUP( 0x0005, TYPE_DWORD ),    //uint32_t  id_0x0005;   /*0x0005 UDP��ʱ�ش�����*/
	ID_LOOKUP( 0x0006, TYPE_DWORD ),    //uint32_t  id_0x0006;   /*0x0006 SMS��ϢӦ��ʱʱ��*/
	ID_LOOKUP( 0x0007, TYPE_DWORD ),    //uint32_t  id_0x0007;   /*0x0007 SMS��Ϣ�ش�����*/
	ID_LOOKUP( 0x0010, TYPE_STR ),      //char   id_0x0010[32];  /*0x0010 ��������APN*/
	ID_LOOKUP( 0x0011, TYPE_STR ),      //char   id_0x0011[32];  /*0x0011 �û���*/
	ID_LOOKUP( 0x0012, TYPE_STR ),      //char   id_0x0012[32];  /*0x0012 ����*/
	ID_LOOKUP( 0x0013, TYPE_STR ),      //char   id_0x0013[32];  /*0x0013 ����������ַ*/
	ID_LOOKUP( 0x0014, TYPE_STR ),      //char   id_0x0014[32];  /*0x0014 ����APN*/
	ID_LOOKUP( 0x0015, TYPE_STR ),      //char   id_0x0015[32];  /*0x0015 �����û���*/
	ID_LOOKUP( 0x0016, TYPE_STR ),      //char   id_0x0016[32];  /*0x0016 ��������*/
	ID_LOOKUP( 0x0017, TYPE_STR ),      //char   id_0x0017[32];  /*0x0017 ���ݷ�������ַ��ip������*/
	ID_LOOKUP( 0x0018, TYPE_DWORD ),    //uint32_t  id_0x0018;   /*0x0018 TCP�˿�*/
	ID_LOOKUP( 0x0019, TYPE_DWORD ),    //uint32_t  id_0x0019;   /*0x0019 UDP�˿�*/
	ID_LOOKUP( 0x001A, TYPE_STR ),      //char   id_0x001A[32];  /*0x001A ic������������ַ��ip������*/
	ID_LOOKUP( 0x001B, TYPE_DWORD ),    //uint32_t  id_0x001B;   /*0x001B ic��������TCP�˿�*/
	ID_LOOKUP( 0x001C, TYPE_DWORD ),    //uint32_t  id_0x001C;   /*0x001C ic��������UDP�˿�*/
	ID_LOOKUP( 0x001D, TYPE_STR ),      //char   id_0x001D[32];  /*0x001D ic�����ݷ�������ַ��ip������*/
	ID_LOOKUP( 0x0020, TYPE_DWORD ),    //uint32_t  id_0x0020;   /*0x0020 λ�û㱨����*/
	ID_LOOKUP( 0x0021, TYPE_DWORD ),    //uint32_t  id_0x0021;   /*0x0021 λ�û㱨����*/
	ID_LOOKUP( 0x0022, TYPE_DWORD ),    //uint32_t  id_0x0022;   /*0x0022 ��ʻԱδ��¼�㱨ʱ����*/
	ID_LOOKUP( 0x0027, TYPE_DWORD ),    //uint32_t  id_0x0027;   /*0x0027 ����ʱ�㱨ʱ����*/
	ID_LOOKUP( 0x0028, TYPE_DWORD ),    //uint32_t  id_0x0028;   /*0x0028 ��������ʱ�㱨ʱ����*/
	ID_LOOKUP( 0x0029, TYPE_DWORD ),    //uint32_t  id_0x0029;   /*0x0029 ȱʡʱ��㱨���*/
	ID_LOOKUP( 0x002C, TYPE_DWORD ),    //uint32_t  id_0x002C;   /*0x002c ȱʡ����㱨���*/
	ID_LOOKUP( 0x002D, TYPE_DWORD ),    //uint32_t  id_0x002D;   /*0x002d ��ʻԱδ��¼�㱨������*/
	ID_LOOKUP( 0x002E, TYPE_DWORD ),    //uint32_t  id_0x002E;   /*0x002e ����ʱ����㱨���*/
	ID_LOOKUP( 0x002F, TYPE_DWORD ),    //uint32_t  id_0x002F;   /*0x002f ����ʱ����㱨���*/
	ID_LOOKUP( 0x0030, TYPE_DWORD ),    //uint32_t  id_0x0030;   /*0x0030 �յ㲹���Ƕ�*/
	ID_LOOKUP( 0x0031, TYPE_DWORD ),    //uint16_t  id_0x0031;   /*0x0031 ����Χ���뾶���Ƿ�λ����ֵ������λΪ��*/
	ID_LOOKUP( 0x0040, TYPE_STR ),      //char   id_0x0040[32];  /*0x0040 ���ƽ̨�绰����*/
	ID_LOOKUP( 0x0041, TYPE_STR ),      //char   id_0x0041[32];  /*0x0041 ��λ�绰����*/
	ID_LOOKUP( 0x0042, TYPE_STR ),      //char   id_0x0042[32];  /*0x0042 �ָ��������õ绰����*/
	ID_LOOKUP( 0x0043, TYPE_STR ),      //char   id_0x0043[32];  /*0x0043 ���ƽ̨SMS����*/
	ID_LOOKUP( 0x0044, TYPE_STR ),      //char   id_0x0044[32];  /*0x0044 �����ն�SMS�ı���������*/
	ID_LOOKUP( 0x0045, TYPE_DWORD ),    //uint32_t  id_0x0045;   /*0x0045 �ն˽����绰����*/
	ID_LOOKUP( 0x0046, TYPE_DWORD ),    //uint32_t  id_0x0046;   /*0x0046 ÿ��ͨ��ʱ��*/
	ID_LOOKUP( 0x0047, TYPE_DWORD ),    //uint32_t  id_0x0047;   /*0x0047 ����ͨ��ʱ��*/
	ID_LOOKUP( 0x0048, TYPE_STR ),      //char   id_0x0048[32];  /*0x0048 �����绰����*/
	ID_LOOKUP( 0x0049, TYPE_STR ),      //char   id_0x0049[32];  /*0x0049 ���ƽ̨��Ȩ���ź���*/
	ID_LOOKUP( 0x0050, TYPE_DWORD ),    //uint32_t  id_0x0050;   /*0x0050 ����������*/
	ID_LOOKUP( 0x0051, TYPE_DWORD ),    //uint32_t  id_0x0051;   /*0x0051 ���������ı�SMS����*/
	ID_LOOKUP( 0x0052, TYPE_DWORD ),    //uint32_t  id_0x0052;   /*0x0052 �������տ���*/
	ID_LOOKUP( 0x0053, TYPE_DWORD ),    //uint32_t  id_0x0053;   /*0x0053 ��������洢��־*/
	ID_LOOKUP( 0x0054, TYPE_DWORD ),    //uint32_t  id_0x0054;   /*0x0054 �ؼ���־*/
	ID_LOOKUP( 0x0055, TYPE_DWORD ),    //uint32_t  id_0x0055;   /*0x0055 ����ٶ�kmh*/
	ID_LOOKUP( 0x0056, TYPE_DWORD ),    //uint32_t  id_0x0056;   /*0x0056 ���ٳ���ʱ��*/
	ID_LOOKUP( 0x0057, TYPE_DWORD ),    //uint32_t  id_0x0057;   /*0x0057 ������ʻʱ������*/
	ID_LOOKUP( 0x0058, TYPE_DWORD ),    //uint32_t  id_0x0058;   /*0x0058 �����ۼƼ�ʻʱ������*/
	ID_LOOKUP( 0x0059, TYPE_DWORD ),    //uint32_t  id_0x0059;   /*0x0059 ��С��Ϣʱ��*/
	ID_LOOKUP( 0x005A, TYPE_DWORD ),    //uint32_t  id_0x005A;   /*0x005A �ͣ��ʱ��*/
	ID_LOOKUP( 0x005B, TYPE_WORD ),     //uint16_t  id_0x005B;   /*0x005B ���ٱ���Ԥ����ֵ����λΪ 1/10Km/h */
	ID_LOOKUP( 0x005C, TYPE_WORD ),     //uint16_t  id_0x005C;   /*0x005C ƣ�ͼ�ʻԤ����ֵ����λΪ�루s����>0*/
	ID_LOOKUP( 0x005D, TYPE_WORD ),     //uint16_t  id_0x005D;   /*0x005D ��ײ������������:b7..0����ײʱ��(4ms) b15..8����ײ���ٶ�(0.1g) 0-79 ֮�䣬Ĭ��Ϊ10 */
	ID_LOOKUP( 0x005E, TYPE_WORD ),     //uint16_t  id_0x005E;   /*0x005E �෭�����������ã� �෭�Ƕȣ���λ 1 �ȣ�Ĭ��Ϊ 30 ��*/
	ID_LOOKUP( 0x0064, TYPE_DWORD ),    //uint32_t  id_0x0064;   /*0x0064 ��ʱ���տ���*/
	ID_LOOKUP( 0x0065, TYPE_DWORD ),    //uint32_t  id_0x0065;   /*0x0065 �������տ���*/
	ID_LOOKUP( 0x0070, TYPE_DWORD ),    //uint32_t  id_0x0070;   /*0x0070 ͼ����Ƶ����(1-10)*/
	ID_LOOKUP( 0x0071, TYPE_DWORD ),    //uint32_t  id_0x0071;   /*0x0071 ����*/
	ID_LOOKUP( 0x0072, TYPE_DWORD ),    //uint32_t  id_0x0072;   /*0x0072 �Աȶ�*/
	ID_LOOKUP( 0x0073, TYPE_DWORD ),    //uint32_t  id_0x0073;   /*0x0073 ���Ͷ�*/
	ID_LOOKUP( 0x0074, TYPE_DWORD ),    //uint32_t  id_0x0074;   /*0x0074 ɫ��*/
	ID_LOOKUP( 0x0080, TYPE_DWORD ),    //uint32_t  id_0x0080;   /*0x0080 ������̱����0.1km*/
	ID_LOOKUP( 0x0081, TYPE_WORD ),     //uint16_t  id_0x0081;   /*0x0081 ʡ��ID*/
	ID_LOOKUP( 0x0082, TYPE_WORD ),     //uint16_t  id_0x0082;   /*0x0082 ����ID*/
	ID_LOOKUP( 0x0083, TYPE_STR ),      //char   id_0x0083[32];  /*0x0083 ����������*/
	ID_LOOKUP( 0x0084, TYPE_BYTE ),     //uint8_t		 id_0x0084;      /*0x0084 ������ɫ	1��ɫ 2��ɫ 3��ɫ 4��ɫ 9����*/
	ID_LOOKUP( 0x0090, TYPE_BYTE ),     //uint8_t		 id_0x0090;      /*0x0090 GNSS ��λģʽ*/
	ID_LOOKUP( 0x0091, TYPE_BYTE ),     //uint8_t		 id_0x0091;      /*0x0091 GNSS ������*/
	ID_LOOKUP( 0x0092, TYPE_BYTE ),     //uint8_t		 id_0x0092;      /*0x0092 GNSS ģ����ϸ��λ�������Ƶ��*/
	ID_LOOKUP( 0x0093, TYPE_DWORD ),    //uint32_t  id_0x0093;   /*0x0093 GNSS ģ����ϸ��λ���ݲɼ�Ƶ��*/
	ID_LOOKUP( 0x0094, TYPE_BYTE ),     //uint8_t		 id_0x0094;      /*0x0094 GNSS ģ����ϸ��λ�����ϴ���ʽ*/
	ID_LOOKUP( 0x0095, TYPE_DWORD ),    //uint32_t  id_0x0095;   /*0x0095 GNSS ģ����ϸ��λ�����ϴ�����*/
	ID_LOOKUP( 0x0100, TYPE_DWORD ),    //uint32_t  id_0x0100;   /*0x0100 CAN ����ͨ�� 1 �ɼ�ʱ����(ms)��0 ��ʾ���ɼ�*/
	ID_LOOKUP( 0x0101, TYPE_WORD ),     //uint16_t  id_0x0101;   /*0x0101 CAN ����ͨ�� 1 �ϴ�ʱ����(s)��0 ��ʾ���ϴ�*/
	ID_LOOKUP( 0x0102, TYPE_DWORD ),    //uint32_t  id_0x0102;   /*0x0102 CAN ����ͨ�� 2 �ɼ�ʱ����(ms)��0 ��ʾ���ɼ�*/
	ID_LOOKUP( 0x0103, TYPE_WORD ),     //uint16_t  id_0x0103;   /*0x0103 CAN ����ͨ�� 2 �ϴ�ʱ����(s)��0 ��ʾ���ϴ�*/
	ID_LOOKUP( 0x0110, TYPE_BYTE | 8 ), //uint8_t		 id_0x0110[8];	 /*0x0110 CAN ���� ID �����ɼ�����*/
	ID_LOOKUP( 0x0111, TYPE_BYTE | 8 ), //uint8_t		 id_0x0111[8];	 /*0x0111 ����CAN ���� ID �����ɼ�����*/
	ID_LOOKUP( 0x0112, TYPE_BYTE | 8 ), //uint8_t		 id_0x0112[8];	 /*0x0112 ����CAN ���� ID �����ɼ�����*/
	ID_LOOKUP( 0x0113, TYPE_BYTE | 8 ), //uint8_t		 id_0x0113[8];	 /*0x0113 ����CAN ���� ID �����ɼ�����*/
	ID_LOOKUP( 0x0114, TYPE_BYTE | 8 ), //uint8_t		 id_0x0114[8];	 /*0x0114 ����CAN ���� ID �����ɼ�����*/
	ID_LOOKUP( 0x0115, TYPE_BYTE | 8 ), //uint8_t		 id_0x0115[8];	 /*0x0115 ����CAN ���� ID �����ɼ�����*/
	ID_LOOKUP( 0x0116, TYPE_BYTE | 8 ), //uint8_t		 id_0x0116[8];	 /*0x0116 ����CAN ���� ID �����ɼ�����*/
	ID_LOOKUP( 0x0117, TYPE_BYTE | 8 ), //uint8_t		 id_0x0117[8];	 /*0x0117 ����CAN ���� ID �����ɼ�����*/
	ID_LOOKUP( 0x0118, TYPE_BYTE | 8 ), //uint8_t		 id_0x0118[8];	 /*0x0118 ����CAN ���� ID �����ɼ�����*/
	ID_LOOKUP( 0x0119, TYPE_BYTE | 8 ), //uint8_t		 id_0x0119[8];	 /*0x0119 ����CAN ���� ID �����ɼ�����*/

	ID_LOOKUP( 0xF000, TYPE_STR ),      //uint8_t		 id_0x0119[8];	 /*0x0119 ����CAN ���� ID �����ɼ�����*/
	ID_LOOKUP( 0xF001, TYPE_STR ),      /*0xF001 �ն��ͺ� 20byte*/
	ID_LOOKUP( 0xF002, TYPE_STR ),      /*0xF002 �ն�ID 7byte*/
	ID_LOOKUP( 0xF003, TYPE_STR ),      /*0xF003 ��Ȩ��*/
	ID_LOOKUP( 0xF004, TYPE_BYTE ),     /*0xF004 �ն�����*/
	ID_LOOKUP( 0xF005, TYPE_STR ),      /*0xF005 ������ʶ,VIN*/
	ID_LOOKUP( 0xF006, TYPE_STR ),      /*0xF006 ������ʶ,MOBILE*/
	ID_LOOKUP( 0xF008, TYPE_STR ),      /*0xF008 ��ʻԱ����*/
	ID_LOOKUP( 0xF009, TYPE_STR ),      /*0xF009 ��ʻ֤����*/
	ID_LOOKUP( 0xF00A, TYPE_STR ),      /*0xF00A ��������*/
	ID_LOOKUP( 0xF00B, TYPE_STR ),      /*0xF00B ��ҵ�ʸ�֤*/
	ID_LOOKUP( 0xF00C, TYPE_STR ),      /*0xF00C ��֤����*/

	ID_LOOKUP( 0xF010, TYPE_STR ),      /*0xF010 ����汾��*/
	ID_LOOKUP( 0xF011, TYPE_STR ),      /*0xF011 Ӳ���汾��*/

	ID_LOOKUP( 0xF013, TYPE_DWORD ),    /*0xF013 Ӳ���汾��*/

	ID_LOOKUP( 0x0020, TYPE_WORD ),     /*0xF020 �����*/
	ID_LOOKUP( 0x0021, TYPE_WORD ),     /*0xF021 ����״̬*/

	ID_LOOKUP( 0xF040, TYPE_BYTE ),     //line_space;               //�м��
	ID_LOOKUP( 0xF041, TYPE_BYTE ),     //margin_left;				//��߽�
	ID_LOOKUP( 0xF042, TYPE_BYTE ),     //margin_right;				//�ұ߽�
	ID_LOOKUP( 0xF043, TYPE_BYTE ),     //step_delay;               //������ʱ,Ӱ���м��
	ID_LOOKUP( 0xF044, TYPE_BYTE ),     //gray_level;               //�Ҷȵȼ�,����ʱ��
	ID_LOOKUP( 0xF045, TYPE_BYTE ),     //heat_delay[0];			//������ʱ
	ID_LOOKUP( 0xF046, TYPE_BYTE ),     //heat_delay[1];			//������ʱ
	ID_LOOKUP( 0xF047, TYPE_BYTE ),     //heat_delay[2];			//������ʱ
	ID_LOOKUP( 0xF048, TYPE_BYTE ),     //heat_delay[3];			//������ʱ
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
	int		i;
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
			if( tbl_id_lookup[i].type == TYPE_CAN )
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
				strncpy( (char*)psrc, (char*)value, 32 );
				*( psrc + 31 ) = 0;
				return 0;
			}
		}
	}
	return 1;
}

/*���ò���*/
void param_put_int( uint16_t id, uint32_t value )
{
	uint32_t	i, j;
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

FINSH_FUNCTION_EXPORT( param_put_int, modify param );

/*д���ַ���*/
static void param_put_str( uint16_t id, uint8_t* value )
{
	int		i;
	uint8_t *p;

	for( i = 0; i < sizeof( tbl_id_lookup ) / sizeof( struct _tbl_id_lookup ); i++ )
	{
		if( id == tbl_id_lookup[i].id )
		{
			p = tbl_id_lookup[i].val;
			strncpy( (char*)p, (char*)value, 32 );
			break;
		}
	}
}

FINSH_FUNCTION_EXPORT( param_put_str, modify param );
#if 0
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
			if( tbl_id_lookup[i].type == TYPE_CAN )
			{
				p = tbl_id_lookup[i].val;
				memcpy( value, p, 8 );
				return 8;
			}
		}
	}
	return 0;
}

#endif

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
			break;
		}
	}
	return val;
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
	int			i, id;
	uint8_t		*p;
	uint32_t	val = 0;

	for( i = 0; i < sizeof( tbl_id_lookup ) / sizeof( struct _tbl_id_lookup ); i++ )
	{
		id	= tbl_id_lookup[i].id;
		p	= tbl_id_lookup[i].val;
		val = 0;
		switch( tbl_id_lookup[i].type )
		{
			case TYPE_DWORD:    /*�ֽڶ��뷽ʽ little_endian*/
				val |= ( *p++ );
				val |= ( ( *p++ ) << 8 );
				val |= ( ( *p++ ) << 16 );
				val |= ( ( *p ) << 24 );
				rt_kprintf( "\nid=%04x value=%08x", id, val );
				break;
			case TYPE_CAN:      /*8���ֽ�*/
				val |= ( *p++ );
				val |= ( ( *p++ ) << 8 );
				val |= ( ( *p++ ) << 16 );
				val |= ( ( *p++ ) << 24 );
				rt_kprintf( "\nid=%04x value=%08x", id, val );
				val = 0;
				val |= ( *p++ );
				val |= ( ( *p++ ) << 8 );
				val |= ( ( *p++ ) << 16 );
				val |= ( ( *p ) << 24 );
				rt_kprintf( " %08x", val );
				break;
			case TYPE_STR:
				rt_kprintf( "\nid=%04x value=%s", id, p );
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
		rt_kprintf( "%s\n", printbuf );
		len -= count;
	}
}

FINSH_FUNCTION_EXPORT( param_dump, dump param );

/*�ֶ�����apn*/
void apn( uint8_t *s )
{
	param_put_str( 0x0010, s );
}

FINSH_FUNCTION_EXPORT( apn, set apn );

/*������ip port*/
void ipport1( uint8_t *ip, uint16_t port )
{
	param_put_str( 0x0013, ip );
	param_put_int( 0x0018, port );
	param_save( );
}

FINSH_FUNCTION_EXPORT( ipport1, set ipport );

/*��ȡ������mobile�ն��ֻ��� 6�ֽ�,����12λ������0*/
void deviceid( uint8_t *s )
{
	uint8_t len, i;
	char	buf[13];
	len = strlen( s );
	memset( buf, 0, 13 );
	if( len >= 12 )
	{
		strncpy( buf, s, 12 );
	}else
	{
		strcpy( buf + 12 - len, s );
	}
	buf[12] = 0;
	param_put_str( 0xF006, s );
	param_save( );
}

FINSH_FUNCTION_EXPORT( deviceid, set deviceid );

static uint16_t id_get = 1; /*���浱ǰ���͵�id*/


/*
   ����������
 */
uint16_t get_param_and_fill_buf( uint8_t* pbuf )
{
	uint16_t	i;
	uint8_t		*p;
	uint16_t	count = 0;

	for( i = id_get; i < sizeof( tbl_id_lookup ) / sizeof( struct _tbl_id_lookup ); i++ )
	{
		if( tbl_id_lookup[i].id >= 0xF000 )
		{
			continue;
		}
		*pbuf++ = ( tbl_id_lookup[i].id ) >> 24;
		*pbuf++ = ( tbl_id_lookup[i].id ) >> 16;
		*pbuf++ = ( tbl_id_lookup[i].id ) >> 8;
		*pbuf++ = ( tbl_id_lookup[i].id ) & 0xFF;
		count	+= 4;

		if( tbl_id_lookup[i].type == TYPE_DWORD )
		{
			p		= tbl_id_lookup[i].val;
			*pbuf++ = 4;
			*pbuf++ = p[3];
			*pbuf++ = p[2];
			*pbuf++ = p[1];
			*pbuf++ = p[0];
			count	+= 5;
		}

		if( tbl_id_lookup[i].type == TYPE_WORD )
		{
			p		= tbl_id_lookup[i].val;
			*pbuf++ = 2;
			*pbuf++ = p[1];
			*pbuf++ = p[0];
			count	+= 3;
		}

		if( tbl_id_lookup[i].type == TYPE_BYTE )
		{
			p		= tbl_id_lookup[i].val;
			*pbuf++ = 1;
			*pbuf++ = *p++;
			count	+= 2;
		}
		if( tbl_id_lookup[i].type == TYPE_STR )
		{
			p		= tbl_id_lookup[i].val;
			*pbuf++ = strlen( (char*)p );
			memcpy( pbuf, p, strlen( (char*)p ) );
			count	+= ( strlen( (char*)p ) + 1 );
			pbuf	+= strlen( (char*)p );
		}
		if( tbl_id_lookup[i].type == TYPE_CAN )
		{
			*pbuf++ = 8;
			p		= tbl_id_lookup[i].val;
			memcpy( pbuf, p, 8 );
			count	+= 9;
			pbuf	+= 8;
		}
		if( count > 512 )
		{
			break;
		}
	}
	id_get = i;
	return count;
}

/*�������*/
void jt808_0x8104_fill_data( JT808_TX_NODEDATA *pnodedata )
{
	uint8_t		buf[600];
	uint16_t	count;

	id_get++;
	count = get_param_and_fill_buf( buf );              /*�ֽ�������*/
	rt_kprintf( "\ncount=%d id_get=%d\n", count, id_get );

	pnodedata->packet_no++;
	if( pnodedata->packet_no == pnodedata->packet_num ) /*�ﵽ���һ��*/
	{
		pnodedata->timeout = RT_TICK_PER_SECOND * 10;
	}
	memcpy( pnodedata->tag_data + 16, buf, count );
	pnodedata->retry		= 0;
	pnodedata->msg_len		= count + 16;
	pnodedata->tag_data[2]	= 0x20 + ( count >> 8 );
	pnodedata->tag_data[3]	= count & 0xFF;
	pnodedata->tag_data[12] = pnodedata->packet_num >> 8;
	pnodedata->tag_data[13] = pnodedata->packet_num & 0xFF;
	pnodedata->tag_data[14] = pnodedata->packet_no >> 8;
	pnodedata->tag_data[15] = pnodedata->packet_no & 0xFF;
	pnodedata->state		= IDLE;
}

/*Ӧ��
   ������Ļ��յ�Ӧ��
 */
static JT808_MSG_STATE jt808_0x8104_response( JT808_TX_NODEDATA * pnodedata, uint8_t *pmsg )
{
	if( pnodedata->packet_num == pnodedata->packet_no ) /*�Ѿ����������а�*/
	{
		rt_kprintf( "0x8104_response_delete\n" );
		pnodedata->state = ACK_OK;
	}
	rt_kprintf( "0x8104_response_idle\n" );
	jt808_0x8104_fill_data( pnodedata );
	return IDLE;
}

/*��ʱ��Ĵ�����*/
static JT808_MSG_STATE jt808_0x8104_timeout( JT808_TX_NODEDATA * pnodedata )
{
	if( pnodedata->packet_num == pnodedata->packet_no ) /*�Ѿ����������а�*/
	{
		rt_kprintf( "0x8104_timeout_delete\n" );
		pnodedata->state = ACK_OK;
	}
	rt_kprintf( "0x8104_timeout_idle\n" );
	jt808_0x8104_fill_data( pnodedata );
	return IDLE;
}

/*�ϱ������ն˲���*/
void jt808_param_0x8104( uint8_t *pmsg )
{
	JT808_TX_NODEDATA * pnodedata;
//	uint8_t				* pdata;
//	uint16_t			id;
	uint8_t		buf[600];
//	uint8_t				*p;
	uint16_t	param_size	= 0;
	uint16_t	param_count = 0;
	uint16_t	i, count;

	pnodedata = node_begin( 1, MULTI_CMD, 0x0104, -1, 600 );
	if( pnodedata == RT_NULL )
	{
		return;
	}

	memset( buf, 0, sizeof( buf ) );
	/*�����������ܴ�С����ͳ��0x0000��0xFxxx��*/

	for( i = 1; i < sizeof( tbl_id_lookup ) / sizeof( struct _tbl_id_lookup ) - 1; i++ )
	{
		if( tbl_id_lookup[i].id >= 0xF000 )
		{
			continue;
		}
		param_count++;
		switch( tbl_id_lookup[i].type )
		{
			case TYPE_DWORD:
				param_size += 9;
				break;
			case TYPE_WORD:
				param_size += 7;
				break;
			case  TYPE_BYTE:
				param_size += 6;
				break;
			case TYPE_STR:
				param_size += ( strlen( (char*)( tbl_id_lookup[i].val ) ) + 5 );
				break;
			case TYPE_CAN:
				param_size += 13;
				break;
		}
	}
	rt_kprintf( "\ntotal param_count=%d size=%d\n", param_count, param_size );
	pnodedata->packet_num	= ( param_size + 511 ) / 512;   /*Ĭ��512�ְ�*/
	pnodedata->packet_no	= 1;
	rt_kprintf( "\npacket_num=%d \n", pnodedata->packet_num );

	id_get	= 1;
	count	= get_param_and_fill_buf( buf + 3 );            /*�ճ������ֽڣ���дӦ����ˮ�Ų�������*/
	rt_kprintf( "\ncount=%d id_get=%d\n", count, id_get );

	buf[0]	= pmsg[10];
	buf[1]	= pmsg[11];
	buf[2]	= param_count;
	node_data( pnodedata, buf, count + 3 );
	pnodedata->tag_data[12] = pnodedata->packet_num >> 8;
	pnodedata->tag_data[13] = pnodedata->packet_num & 0xFF;
	pnodedata->tag_data[14] = pnodedata->packet_no >> 8;
	pnodedata->tag_data[15] = pnodedata->packet_no & 0xFF;
	node_end(SINGLE_ACK, pnodedata, jt808_0x8104_timeout, jt808_0x8104_response, RT_NULL );
}

FINSH_FUNCTION_EXPORT_ALIAS( jt808_param_0x8104, param, desc );

/*�����ѯ��ֻӦ�𵥰�*/
void jt808_param_0x8106( uint8_t *pmsg )
{
	JT808_TX_NODEDATA	* pnodedata;
	uint8_t				* pdata;
	uint32_t			id;
	uint8_t				buf[600];
	uint8_t				* pbuf;
	uint8_t				*p;
	uint16_t			param_size	= 0;
	uint16_t			param_count = 0;
	uint16_t			i, count;

	memset( buf, 0, sizeof( buf ) );
	/*�����������ܴ�С����ͳ��0x0000��0xFxxx��*/
	pdata	= pmsg + 13;    /*ָ��������*/
	count	= 0;
	pbuf	= buf + 3;      /*�ճ������ֽڣ���дӦ����ˮ�Ų�������*/
	for( param_count = 0; param_count < pmsg[12]; param_count++ )
	{
		id	= *pdata++ << 24;
		id	|= *pdata++ << 16;
		id	|= *pdata++ << 8;
		id	|= *pdata++ & 0xFF;

		for( i = 1; i < sizeof( tbl_id_lookup ) / sizeof( struct _tbl_id_lookup ) - 1; i++ )
		{
			if( tbl_id_lookup[i].id == id )
			{
				*pbuf++ = id >> 24;
				*pbuf++ = id >> 16;
				*pbuf++ = id >> 8;
				*pbuf++ = id & 0xFF;
				count	+= 4;

				if( tbl_id_lookup[i].type == TYPE_DWORD )
				{
					p		= tbl_id_lookup[i].val;
					*pbuf++ = 4;
					*pbuf++ = p[3];
					*pbuf++ = p[2];
					*pbuf++ = p[1];
					*pbuf++ = p[0];
					count	+= 5;
				}

				if( tbl_id_lookup[i].type == TYPE_WORD )
				{
					p		= tbl_id_lookup[i].val;
					*pbuf++ = 2;
					*pbuf++ = p[1];
					*pbuf++ = p[0];
					count	+= 3;
				}

				if( tbl_id_lookup[i].type == TYPE_BYTE )
				{
					p		= tbl_id_lookup[i].val;
					*pbuf++ = 1;
					*pbuf++ = *p++;
					count	+= 2;
				}
				if( tbl_id_lookup[i].type == TYPE_STR )
				{
					p		= tbl_id_lookup[i].val;
					*pbuf++ = strlen( (char*)p );
					memcpy( pbuf, p, strlen( (char*)p ) );
					count	+= ( strlen( (char*)p ) + 1 );
					pbuf	+= strlen( (char*)p );
				}
				if( tbl_id_lookup[i].type == TYPE_CAN )
				{
					*pbuf++ = 8;
					p		= tbl_id_lookup[i].val;
					memcpy( pbuf, p, 8 );
					count	+= 9;
					pbuf	+= 8;
				}
				if( count > 512 )
				{
					break;
				}
			}
		}
	}
	rt_kprintf( "\ntotal count=%d size=%d\n", param_count, param_size );

	pnodedata = node_begin( 1, SINGLE_ACK, 0x0104, -1, 600 );
	if( pnodedata == RT_NULL )
	{
		return;
	}

	buf[0]				= pmsg[10];
	buf[1]				= pmsg[11];
	buf[2]				= param_count;
	pnodedata->timeout	= RT_TICK_PER_SECOND * 5;
	node_data( pnodedata, buf, count + 3 );
	node_end( SINGLE_ACK,pnodedata, jt808_0x8104_timeout, jt808_0x8104_response, RT_NULL );
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
void jt808_param_0x8103( uint8_t *pmsg )
{
	uint8_t		* p;
	uint8_t		res = 0;
	uint32_t	param_id;
	uint8_t		param_len;
	uint8_t		param_count;
	uint16_t	offset;
	uint16_t	seq, id;

	id	= ( pmsg[0] << 8 ) | pmsg[1];
	seq = ( pmsg[10] << 8 ) | pmsg[11];

	if( *( pmsg + 2 ) >= 0x20 ) /*����Ƕ�������ò���*/
	{
		rt_kprintf( "\n>%s multi packet no support!", __func__ );
		jt808_tx_0x0001( seq, id, 1 );
	}else
	{
		param_count = pmsg[12];
		offset		= 13;
		for( param_count = 0; param_count < pmsg[12]; param_count++ )
		{
			p			= pmsg + offset;
			param_id	= ( p[0] << 24 ) | ( p[1] << 16 ) | ( p[2] << 8 ) | ( p[3] );
			param_len	= p[4];
			res			|= param_put( param_id, param_len, &p[5] );
			offset		+= ( param_len + 5 );
			//rt_kprintf( "\n0x8103>id=%x res=%d \n", param_id, res );
		}

		if( res ) /*�д���*/
		{
			jt808_tx_0x0001( seq, id, 1 );
		}else
		{
			jt808_tx_0x0001( seq, id, 0 );
			param_save( );
		}
	}
}

/************************************** The End Of File **************************************/
