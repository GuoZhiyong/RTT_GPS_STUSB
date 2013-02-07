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

#define DECL_PARAM_DWORD( id, value )	int parma_ ## id	= value
#define DECL_PARAM_STRING( id, value )	char* parma_ ## id	= value

DECL_PARAM_DWORD( 0x0000, 0x13020100 );  /*0x0000 �汾*/
DECL_PARAM_DWORD( 0x0001, 5 );                  /*0x0001 �������ͼ��*/
DECL_PARAM_DWORD( 0x0002, 3 );                  /*0x0002 TCPӦ��ʱʱ��*/
DECL_PARAM_DWORD( 0x0003, 15 );                 /*0x0003 TCP��ʱ�ش�����*/
DECL_PARAM_DWORD( 0x0004, 3 );                  /*0x0004	UDPӦ��ʱʱ��*/
DECL_PARAM_DWORD( 0x0005, 5 );                  /*0x0005 UDP��ʱ�ش�����*/
DECL_PARAM_DWORD( 0x0006, 3 );                  /*0x0006 SMS��ϢӦ��ʱʱ��*/
DECL_PARAM_DWORD( 0x0007, 5 );                  /*0x0007 SMS��Ϣ�ش�����*/
DECL_PARAM_STRING( 0x0010, (char*)"CMNET" );     /*0x0010 ��������APN*/
DECL_PARAM_STRING( 0x0011, "" );                 /*0x0011 �û���*/
DECL_PARAM_STRING( 0x0012, "" );                 /*0x0012 ����*/
DECL_PARAM_STRING( 0x0013, "" );                 /*0x0013 ����������ַ*/
DECL_PARAM_STRING( 0x0014, "" );                 /*0x0014 ����APN*/
DECL_PARAM_STRING( 0x0015, "" );                 /*0x0015 �����û���*/
DECL_PARAM_STRING( 0x0016, "" );                 /*0x0016 ��������*/
DECL_PARAM_STRING( 0x0017, "" );                 /*0x0017 ���ݷ�������ַ��ip������*/
DECL_PARAM_DWORD( 0x0018, 1234 );               /*0x0018 TCP�˿�*/
DECL_PARAM_DWORD( 0x0019, 5678 );               /*0x0019 UDP�˿�*/
DECL_PARAM_DWORD( 0x0020, 0 );                  /*0x0020 λ�û㱨����*/
DECL_PARAM_DWORD( 0x0021, 1 );                  /*0x0021 λ�û㱨����*/
DECL_PARAM_DWORD( 0x0022, 30 );                 /*0x0022 ��ʻԱδ��¼�㱨ʱ����*/
DECL_PARAM_DWORD( 0x0027, 120 );                /*0x0027 ����ʱ�㱨ʱ����*/
DECL_PARAM_DWORD( 0x0028, 5 );                  /*0x0028 ��������ʱ�㱨ʱ����*/
DECL_PARAM_DWORD( 0x0029, 30 );                 /*0x0029 ȱʡʱ��㱨���*/
DECL_PARAM_DWORD( 0x002C, 500 );                /*0x002c ȱʡ����㱨���*/
DECL_PARAM_DWORD( 0x002D, 1000 );               /*0x002d ��ʻԱδ��¼�㱨������*/
DECL_PARAM_DWORD( 0x002E, 1000 );               /*0x002e ����ʱ����㱨���*/
DECL_PARAM_DWORD( 0x002F, 100 );                /*0x002f ����ʱ����㱨���*/
DECL_PARAM_DWORD( 0x0030, 5 );                  /*0x0030 �յ㲹���Ƕ�*/
DECL_PARAM_STRING( 0x0040, "1008611" );          /*0x0040 ���ƽ̨�绰����*/
DECL_PARAM_STRING( 0x0041, "" );                 /*0x0041 ��λ�绰����*/
DECL_PARAM_STRING( 0x0042, "" );                 /*0x0042 �ָ��������õ绰����*/
DECL_PARAM_STRING( 0x0043, "" );                 /*0x0043 ���ƽ̨SMS����*/
DECL_PARAM_STRING( 0x0044, "" );                 /*0x0044 �����ն�SMS�ı���������*/
DECL_PARAM_DWORD( 0x0045, 5 );                  /*0x0045 �ն˽����绰����*/
DECL_PARAM_DWORD( 0x0046, 3 );                  /*0x0046 ÿ��ͨ��ʱ��*/
DECL_PARAM_DWORD( 0x0047, 3 );                  /*0x0047 ����ͨ��ʱ��*/
DECL_PARAM_STRING( 0x0048, "" );                 /*0x0048 �����绰����*/
DECL_PARAM_STRING( 0x0049, "" );                 /*0x0049 ���ƽ̨��Ȩ���ź���*/
DECL_PARAM_DWORD( 0x0050, 5 );                  /*0x0050 ����������*/
DECL_PARAM_DWORD( 0x0051, 3 );                  /*0x0051 ���������ı�SMS����*/
DECL_PARAM_DWORD( 0x0052, 5 );                  /*0x0052 �������տ���*/
DECL_PARAM_DWORD( 0x0053, 3 );                  /*0x0053 ��������洢��־*/
DECL_PARAM_DWORD( 0x0054, 5 );                  /*0x0054 �ؼ���־*/
DECL_PARAM_DWORD( 0x0055, 3 );                  /*0x0055 ����ٶ�kmh*/
DECL_PARAM_DWORD( 0x0056, 5 );                  /*0x0056 ���ٳ���ʱ��*/
DECL_PARAM_DWORD( 0x0057, 3 );                  /*0x0057 ������ʻʱ������*/
DECL_PARAM_DWORD( 0x0058, 5 );                  /*0x0058 �����ۼƼ�ʻʱ������*/
DECL_PARAM_DWORD( 0x0059, 3 );                  /*0x0059 ��С��Ϣʱ��*/
DECL_PARAM_DWORD( 0x005A, 5 );                  /*0x005A �ͣ��ʱ��*/
DECL_PARAM_DWORD( 0x0070, 3 );                  /*0x0070 ͼ����Ƶ����(1-10)*/
DECL_PARAM_DWORD( 0x0071, 5 );                  /*0x0071 ����*/
DECL_PARAM_DWORD( 0x0072, 3 );                  /*0x0072 �Աȶ�*/
DECL_PARAM_DWORD( 0x0073, 5 );                  /*0x0073 ���Ͷ�*/
DECL_PARAM_DWORD( 0x0074, 3 );                  /*0x0074 ɫ��*/
DECL_PARAM_DWORD( 0x0080, 5 );                  /*0x0080 ������̱����0.1km*/
DECL_PARAM_DWORD( 0x0081, 3 );                  /*0x0081 ʡ��ID*/
DECL_PARAM_DWORD( 0x0082, 5 );                  /*0x0082 ����ID*/
DECL_PARAM_STRING( 0x0083, "��O-00001" );        /*0x0083 ����������*/
DECL_PARAM_DWORD( 0x0084, 5 );                  /*0x0084 ������ɫ*/

#if 0

PARAM param_list[] =
{
	{ 0x0000, T_DWORD,	(void*)0x13020100 },    /*0x0000 �汾*/
	{ 0x0001, T_DWORD,	5				  },    /*0x0001 �������ͼ��*/
	{ 0x0002, T_DWORD,	3				  },    /*0x0002 TCPӦ��ʱʱ��*/
	{ 0x0003, T_DWORD,	15				  },    /*0x0003 TCP��ʱ�ش�����*/
	{ 0x0004, T_DWORD,	3				  },    /*0x0004	UDPӦ��ʱʱ��*/
	{ 0x0005, T_DWORD,	5				  },    /*0x0005 UDP��ʱ�ش�����*/
	{ 0x0006, T_DWORD,	3				  },    /*0x0006 SMS��ϢӦ��ʱʱ��*/
	{ 0x0007, T_DWORD,	5				  },    /*0x0007 SMS��Ϣ�ش�����*/
	{ 0x0010, T_STRING, (char*)"CMNET"	  },    /*0x0010 ��������APN*/
	{ 0x0011, T_STRING, ""				  },    /*0x0011 �û���*/
	{ 0x0012, T_STRING, ""				  },    /*0x0012 ����*/
	{ 0x0013, T_STRING, ""				  },    /*0x0013 ����������ַ*/
	{ 0x0014, T_STRING, ""				  },    /*0x0014 ����APN*/
	{ 0x0015, T_STRING, ""				  },    /*0x0015 �����û���*/
	{ 0x0016, T_STRING, ""				  },    /*0x0016 ��������*/
	{ 0x0017, T_STRING, ""				  },    /*0x0017 ���ݷ�������ַ��ip������*/
	{ 0x0018, T_DWORD,	1234			  },    /*0x0018 TCP�˿�*/
	{ 0x0019, T_DWORD,	5678			  },    /*0x0019 UDP�˿�*/
	{ 0x0020, T_DWORD,	0				  },    /*0x0020 λ�û㱨����*/
	{ 0x0021, T_DWORD,	1				  },    /*0x0021 λ�û㱨����*/
	{ 0x0022, T_DWORD,	30				  },    /*0x0022 ��ʻԱδ��¼�㱨ʱ����*/
	{ 0x0027, T_DWORD,	120				  },    /*0x0027 ����ʱ�㱨ʱ����*/
	{ 0x0028, T_DWORD,	5				  },    /*0x0028 ��������ʱ�㱨ʱ����*/
	{ 0x0029, T_DWORD,	30				  },    /*0x0029 ȱʡʱ��㱨���*/
	{ 0x002C, T_DWORD,	500				  },    /*0x002c ȱʡ����㱨���*/
	{ 0x002D, T_DWORD,	1000			  },    /*0x002d ��ʻԱδ��¼�㱨������*/
	{ 0x002E, T_DWORD,	1000			  },    /*0x002e ����ʱ����㱨���*/
	{ 0x002F, T_DWORD,	100				  },    /*0x002f ����ʱ����㱨���*/
	{ 0x0030, T_DWORD,	5				  },    /*0x0030 �յ㲹���Ƕ�*/
	{ 0x0040, T_STRING, "1008611"		  },    /*0x0040 ���ƽ̨�绰����*/
	{ 0x0041, T_STRING, ""				  },    /*0x0041 ��λ�绰����*/
	{ 0x0042, T_STRING, ""				  },    /*0x0042 �ָ��������õ绰����*/
	{ 0x0043, T_STRING, ""				  },    /*0x0043 ���ƽ̨SMS����*/
	{ 0x0044, T_STRING, ""				  },    /*0x0044 �����ն�SMS�ı���������*/
	{ 0x0045, T_DWORD,	5				  },    /*0x0045 �ն˽����绰����*/
	{ 0x0046, T_DWORD,	3				  },    /*0x0046 ÿ��ͨ��ʱ��*/
	{ 0x0047, T_DWORD,	3				  },    /*0x0047 ����ͨ��ʱ��*/
	{ 0x0048, T_STRING, ""				  },    /*0x0048 �����绰����*/
	{ 0x0049, T_STRING, ""				  },    /*0x0049 ���ƽ̨��Ȩ���ź���*/
	{ 0x0050, T_DWORD,	5				  },    /*0x0050 ����������*/
	{ 0x0051, T_DWORD,	3				  },    /*0x0051 ���������ı�SMS����*/
	{ 0x0052, T_DWORD,	5				  },    /*0x0052 �������տ���*/
	{ 0x0053, T_DWORD,	3				  },    /*0x0053 ��������洢��־*/
	{ 0x0054, T_DWORD,	5				  },    /*0x0054 �ؼ���־*/
	{ 0x0055, T_DWORD,	3				  },    /*0x0055 ����ٶ�kmh*/
	{ 0x0056, T_DWORD,	5				  },    /*0x0056 ���ٳ���ʱ��*/
	{ 0x0057, T_DWORD,	3				  },    /*0x0057 ������ʻʱ������*/
	{ 0x0058, T_DWORD,	5				  },    /*0x0058 �����ۼƼ�ʻʱ������*/
	{ 0x0059, T_DWORD,	3				  },    /*0x0059 ��С��Ϣʱ��*/
	{ 0x005A, T_DWORD,	5				  },    /*0x005A �ͣ��ʱ��*/
	{ 0x0070, T_DWORD,	3				  },    /*0x0070 ͼ����Ƶ����(1-10)*/
	{ 0x0071, T_DWORD,	5				  },    /*0x0071 ����*/
	{ 0x0072, T_DWORD,	3				  },    /*0x0072 �Աȶ�*/
	{ 0x0073, T_DWORD,	5				  },    /*0x0073 ���Ͷ�*/
	{ 0x0074, T_DWORD,	3				  },    /*0x0074 ɫ��*/
	{ 0x0080, T_DWORD,	5				  },    /*0x0080 ������̱����0.1km*/
	{ 0x0081, T_DWORD,	3				  },    /*0x0081 ʡ��ID*/
	{ 0x0082, T_DWORD,	5				  },    /*0x0082 ����ID*/
	{ 0x0083, T_STRING, "��O-00001"		  },    /*0x0083 ����������*/
	{ 0x0084, T_DWORD,	5				  },    /*0x0084 ������ɫ*/
};
#endif

/************************************** The End Of File **************************************/
