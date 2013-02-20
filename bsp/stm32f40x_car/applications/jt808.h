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
#ifndef _H_JT808_H_
#define _H_JT808_H_

#include <stm32f4xx.h>
#include <rtthread.h>
#define   MsgQ_Timeout 3

/*
�洢�������,���þ��Ե�ַ,��4K(0x1000)Ϊһ������
*/

#define ADDR_PARAM	0x000000000




/*�ֽ�˳��Ķ�������˳��*/
typedef struct
{
	uint32_t	latitude;       /*γ�� 1/10000�� */
	uint32_t	longitude;      /*���� 1/10000�� */
	uint16_t	altitude;       /*�߳� m*/
	uint16_t	speed;          /*�ٶ� 1/10KMH*/
	uint8_t		direction;      /*���� 0-178 �̶�Ϊ2��*/
	uint8_t		datetime[6];    /*YY-MM-DD hh-mm-ss BCD����*/
}T_GPSINFO;

//------- �ı���Ϣ --------
typedef struct _TEXT_INFO
{
	uint8_t TEXT_FLAG;          //  �ı���־
	uint8_t TEXT_SD_FLAG;       // ���ͱ�־λ
	uint8_t TEXT_Content[100];  // �ı�����
}TEXT_INFO;

//----- ��Ϣ ----
typedef struct _MSG_TEXT
{
	uint8_t TEXT_mOld;          //  ���µ�һ����Ϣ  дΪ1���������µ�һ����Ϣ
	uint8_t TEXT_TYPE;          //  ��Ϣ����   1-8  �еڼ���
	uint8_t TEXT_LEN;           //  ��Ϣ����
	uint8_t TEXT_STR[150];      //  ��Ϣ����
}MSG_TEXT;

//-----  ���� ------
typedef struct _CENTER_ASK
{
	uint8_t		ASK_SdFlag;     //  ��־λ           ���� TTS  1  ��   TTS ����  2
	uint16_t	ASK_floatID;    // ������ˮ��
	uint8_t		ASK_infolen;    // ��Ϣ����
	uint8_t		ASK_answerID;   // �ظ�ID
	uint8_t		ASK_info[30];   //  ��Ϣ����
	uint8_t		ASK_answer[30]; // ��ѡ��
}CENTRE_ASK;

//------ �¼�  -------
typedef struct _EVENT           //  name: event
{
	uint8_t Event_ID;           //  �¼�ID
	uint8_t Event_Len;          //  �¼�����
	uint8_t Event_Effective;    //  �¼��Ƿ���Ч��   1 ΪҪ��ʾ  0
	uint8_t Event_Str[20];      //  �¼�����
}EVENT;

//----- ��Ϣ ----
typedef struct _MSG_BROADCAST   // name: msg_broadcast
{
	uint8_t		INFO_TYPE;      //  ��Ϣ����
	uint16_t	INFO_LEN;       //  ��Ϣ����
	uint8_t		INFO_PlyCancel; // �㲥/ȡ����־      0 ȡ��  1  �㲥
	uint8_t		INFO_SDFlag;    //  ���ͱ�־λ
	uint8_t		INFO_Effective; //  ��ʾ�Ƿ���Ч   1 ��ʾ��Ч    0  ��ʾ��Ч
	uint8_t		INFO_STR[30];   //  ��Ϣ����
}MSG_BRODCAST;

//------ �绰�� -----
typedef struct _PHONE_BOOK      // name: phonebook
{
	uint8_t CALL_TYPE;          // ��������  1 ���� 2 ���� 3 ����/����
	uint8_t NumLen;             // ���볤��
	uint8_t UserLen;            // ��ϵ�˳���
	uint8_t Effective_Flag;     // ��Ч��־λ   ��Ч 0 ����Ч  1
	uint8_t NumberStr[20];      // �绰����
	uint8_t UserStr[10];        // ��ϵ������  GBK ����
}PHONE_BOOK;

typedef struct _MULTIMEDIA
{
	u32 Media_ID;               //   ��ý������ID
	u8	Media_Type;             //   0:   ͼ��    1 : ��Ƶ    2:  ��Ƶ
	u8	Media_CodeType;         //   �����ʽ  0 : JPEG  1:TIF  2:MP3  3:WAV  4: WMV
	u8	Event_Code;             //   �¼�����  0: ƽ̨�·�ָ��  1: ��ʱ����  2 : ���ٱ������� 3: ��ײ�෭�������� ��������
	u8	Media_Channel;          //   ͨ��ID
	//----------------------
	u8	SD_Eventstate;          // �����¼���Ϣ�ϴ�״̬    0 ��ʾ����   1  ��ʾ���ڷ���״̬
	u8	SD_media_Flag;          // ����û���¼���Ϣ��־λ
	u8	SD_Data_Flag;           // �������ݱ�־λ
	u8	SD_timer;               // ���Ͷ�ʱ��
	u8	MaxSd_counter;          // ����ʹ���
	u8	Media_transmittingFlag; // ��ý�崫������״̬  1: ��ý�崫��ǰ����1����λ��Ϣ    2 :��ý�����ݴ�����  0:  δ���ж�ý�����ݴ���
	u16 Media_totalPacketNum;   // ��ý���ܰ���
	u16 Media_currentPacketNum; // ��ý�嵱ǰ����
	//----------------------
	u8	RSD_State;              //  �ش�״̬   0 : �ش�û������   1 :  �ش���ʼ    2  : ��ʾ˳���굫�ǻ�û�յ����ĵ��ش�����
	u8	RSD_Timer;              //  ��״̬�µļ�����
	u8	RSD_Reader;             //  �ش���������ǰ��ֵ
	u8	RSD_total;              //  �ش�ѡ����Ŀ

	u8 Media_ReSdList[10];      //  ��ý���ش���Ϣ�б�
}MULTIMEDIA;

//--------------ÿ���ӵ�ƽ���ٶ�
typedef  struct AvrgMintSpeed
{
	uint8_t datetime[6];        //current
	uint8_t datetime_Bak[6];    //Bak
	uint8_t avgrspd[60];
	uint8_t saveFlag;
}Avrg_MintSpeed;

extern uint32_t		jt808_alarm;
extern uint32_t		jt808_status;

extern TEXT_INFO	TextInfo;
//-------�ı���Ϣ-------
extern MSG_TEXT		TEXT_Obj;
extern MSG_TEXT		TEXT_Obj_8[8], TEXT_Obj_8bak[8];

//------ ����  --------
extern CENTRE_ASK ASK_Centre;                       // ��������

//------- �¼� ----
extern EVENT	EventObj;                           // �¼�
extern EVENT	EventObj_8[8];                      // �¼�

//------  ��Ϣ�㲥  ---
extern MSG_BRODCAST MSG_BroadCast_Obj;              // ��Ϣ�㲥
extern MSG_BRODCAST MSG_Obj_8[8];                   // ��Ϣ�㲥

//------  �绰��  -----
extern PHONE_BOOK		PhoneBook, Rx_PhoneBOOK;    //  �绰��
extern PHONE_BOOK		PhoneBook_8[8];

extern MULTIMEDIA		MediaObj;                   // ��ý����Ϣ

extern uint8_t			CarLoadState_Flag;          //ѡ�г���״̬�ı�־   1:�ճ�   2:���   3:�س�
extern uint8_t			Warn_Status[4];

extern u16				ISP_total_packnum;          // ISP  �ܰ���
extern u16				ISP_current_packnum;        // ISP  ��ǰ����

extern u8				APN_String[30];

extern u8				Camera_Number;
extern u8				Duomeiti_sdFlag;

extern Avrg_MintSpeed	Avrgspd_Mint;
extern u8				avgspd_Mint_Wr; // ��дÿ����ƽ���ٶȼ�¼�±�

/*for new use*/

typedef struct
{
	int		id;
	short	attr;
	int		latitute;   /*�Զ�λ��λ��γ��ֵ����10��6�η�����ȷ�������֮һ��*/
	int		longitute;
	int		radius;     /*��λΪ��m��·��Ϊ�ùյ㵽��һ�յ�*/
	char	start[6];
	char	end[6];
	short	speed;
	char	interval;   /*����ʱ��,��*/
}GPS_AREA_CIRCLE;

typedef enum
{
	T_NODEF = 1,
	T_BYTE,
	T_WORD,
	T_DWORD,
	T_STRING,
}PARAM_TYPE;

/*�ն˲�������*/
typedef  struct
{
	uint8_t		id;
	PARAM_TYPE	type;
	void		* pvalue;
}PARAM;

/*�ն˲�������*/
typedef __packed struct
{
	PARAM_TYPE	type;
	void		* pvalue;
}PARAM_BODY;

#if 0
typedef struct
{
	uint32_t ver;    /*�汾��Ϣ�ĸ��ֽ�yy_mm_dd_build,�Ƚϴ�С*/
/*����ע����Ϣ*/
	uint16_t	id0100_1_w;
	uint16_t	id0100_2_w;
	uint8_t		id0100_3_s[5];
	uint8_t		id0100_4_s[8];
	uint8_t		id0100_5_s[7];
	uint8_t		id0100_6_b;
	uint8_t		id0100_7_s[12];

/*�����й�*/
	char	apn[32];
	char	user[32];
	char	psw[32];
	char	mobile[6];
/*�������*/
	uint32_t	timeout_udp;    /*udp���䳬ʱʱ��*/
	uint32_t	retry_udp;      /*udp�����ش�����*/
	uint32_t	timeout_tcp;    /*udp���䳬ʱʱ��*/
	uint32_t	retry_tcp;      /*udp�����ش�����*/
}JT808_PARAM;

#endif


typedef struct _jt808_param
	{
		uint32_t	id_0x0000;		/*0x0000 �汾*/
		uint32_t	id_0x0001;		/*0x0001 �������ͼ��*/
		uint32_t	id_0x0002;		/*0x0002 TCPӦ��ʱʱ��*/
		uint32_t	id_0x0003;		/*0x0003 TCP��ʱ�ش�����*/
		uint32_t	id_0x0004;		/*0x0004 UDPӦ��ʱʱ��*/
		uint32_t	id_0x0005;		/*0x0005 UDP��ʱ�ش�����*/
		uint32_t	id_0x0006;		/*0x0006 SMS��ϢӦ��ʱʱ��*/
		uint32_t	id_0x0007;		/*0x0007 SMS��Ϣ�ش�����*/
		char		id_0x0010[32];	/*0x0010 ��������APN*/
		char		id_0x0011[32];	/*0x0011 �û���*/
		char		id_0x0012[32];	/*0x0012 ����*/
		char		id_0x0013[32];	/*0x0013 ����������ַ*/
		char		id_0x0014[32];	/*0x0014 ����APN*/
		char		id_0x0015[32];	/*0x0015 �����û���*/
		char		id_0x0016[32];	/*0x0016 ��������*/
		char		id_0x0017[32];	/*0x0017 ���ݷ�������ַ��ip������*/
		uint32_t	id_0x0018;		/*0x0018 TCP�˿�*/
		uint32_t	id_0x0019;		/*0x0019 UDP�˿�*/
		uint32_t	id_0x0020;		/*0x0020 λ�û㱨����*/
		uint32_t	id_0x0021;		/*0x0021 λ�û㱨����*/
		uint32_t	id_0x0022;		/*0x0022 ��ʻԱδ��¼�㱨ʱ����*/
		uint32_t	id_0x0027;		/*0x0027 ����ʱ�㱨ʱ����*/
		uint32_t	id_0x0028;		/*0x0028 ��������ʱ�㱨ʱ����*/
		uint32_t	id_0x0029;		/*0x0029 ȱʡʱ��㱨���*/
		uint32_t	id_0x002C;		/*0x002c ȱʡ����㱨���*/
		uint32_t	id_0x002D;		/*0x002d ��ʻԱδ��¼�㱨������*/
		uint32_t	id_0x002E;		/*0x002e ����ʱ����㱨���*/
		uint32_t	id_0x002F;		/*0x002f ����ʱ����㱨���*/
		uint32_t	id_0x0030;		/*0x0030 �յ㲹���Ƕ�*/
		char		id_0x0040[16];	/*0x0040 ���ƽ̨�绰����*/
		char		id_0x0041[16];	/*0x0041 ��λ�绰����*/
		char		id_0x0042[16];	/*0x0042 �ָ��������õ绰����*/
		char		id_0x0043[16];	/*0x0043 ���ƽ̨SMS����*/
		char		id_0x0044[16];	/*0x0044 �����ն�SMS�ı���������*/
		uint32_t	id_0x0045;		/*0x0045 �ն˽����绰����*/
		uint32_t	id_0x0046;		/*0x0046 ÿ��ͨ��ʱ��*/
		uint32_t	id_0x0047;		/*0x0047 ����ͨ��ʱ��*/
		char		id_0x0048[16];	/*0x0048 �����绰����*/
		char		id_0x0049[16];	/*0x0049 ���ƽ̨��Ȩ���ź���*/
		uint32_t	id_0x0050;		/*0x0050 ����������*/
		uint32_t	id_0x0051;		/*0x0051 ���������ı�SMS����*/
		uint32_t	id_0x0052;		/*0x0052 �������տ���*/
		uint32_t	id_0x0053;		/*0x0053 ��������洢��־*/
		uint32_t	id_0x0054;		/*0x0054 �ؼ���־*/
		uint32_t	id_0x0055;		/*0x0055 ����ٶ�kmh*/
		uint32_t	id_0x0056;		/*0x0056 ���ٳ���ʱ��*/
		uint32_t	id_0x0057;		/*0x0057 ������ʻʱ������*/
		uint32_t	id_0x0058;		/*0x0058 �����ۼƼ�ʻʱ������*/
		uint32_t	id_0x0059;		/*0x0059 ��С��Ϣʱ��*/
		uint32_t	id_0x005A;		/*0x005A �ͣ��ʱ��*/
		uint32_t	id_0x0070;		/*0x0070 ͼ����Ƶ����(1-10)*/
		uint32_t	id_0x0071;		/*0x0071 ����*/
		uint32_t	id_0x0072;		/*0x0072 �Աȶ�*/
		uint32_t	id_0x0073;		/*0x0073 ���Ͷ�*/
		uint32_t	id_0x0074;		/*0x0074 ɫ��*/
		uint32_t	id_0x0080;		/*0x0080 ������̱����0.1km*/
		uint32_t	id_0x0081;		/*0x0081 ʡ��ID*/
		uint32_t	id_0x0082;		/*0x0082 ����ID*/
		char		id_0x0083[16];	/*0x0083 ����������*/
		uint32_t	id_0x0084;		/*0x0084 ������ɫ  1��ɫ 2��ɫ 3��ɫ 4��ɫ 9����*/
}JT808_PARAM;


typedef struct
{
	char mobile[6];		/*�ն˺���*/
	uint8_t producer_id[5];
	uint8_t model[20];
	uint8_t terminal_id[7];
}TERM_PARAM;



typedef enum
{
	IDLE = 1,                   /*���еȴ�����*/
	WAIT_ACK,                   /*�ȴ�ACK��*/
	ACK_OK,                     /*���յ�ACKӦ��*/
} JT808_MSG_STATE;

typedef enum
{
	TERMINAL_CMD = 1,
	TERMINAL_ACK,
	CENTER_CMD,
	CENTER_ACK
}JT808_MSG_TYPE;

typedef __packed struct
{
	uint16_t	id;
	uint16_t	attr;
	uint8_t		mobile[6];
	uint16_t	seq;
}JT808_MSG_HEAD;

typedef __packed struct
{
	uint32_t	tick;           /*�յ���ʱ��,������յĳ�ʱ�ж�*/
	uint8_t		linkno;         /*ʹ�õ�linkno*/
	uint16_t	id;             /*��ϢID*/
	uint16_t	attr;           /*��Ϣ������*/
	uint8_t		mobileno[6];    /*�ն��ֻ���*/
	uint16_t	seq;            /*��Ϣ��ˮ��*/
	uint16_t	packetcount;    /*������ܰ����������*/
	uint16_t	packetno;       /*��ǰ����ţ���1��ʼ�������*/
	uint16_t	msg_len;        /*��Ϣ����*/
	uint8_t		*pmsg;          /*�յ���Ϣ��*/
}JT808_RX_MSG_NODEDATA;

typedef __packed struct _jt808_tx_msg_nodedata
{
/*���ͻ������*/
//	uint8_t			linkno;     /*����ʹ�õ�link,������Э���Զ��socket*/
	JT808_MSG_TYPE	type;
	JT808_MSG_STATE state;      /*����״̬*/
	uint32_t		retry;      /*�ش�����,�������ݼ��Ҳ���*/
	uint32_t		max_retry;  /*����ش�����*/
	uint32_t		timeout;    /*��ʱʱ��*/
	uint32_t		tick;       /*����ʱ��*/
/*���յĴ����ж����*/
	void ( *cb_tx_timeout )( struct _jt808_tx_msg_nodedata *pnodedata );
	void ( *cb_tx_response )( JT808_RX_MSG_NODEDATA* pnodedata );
	uint16_t	head_id;        /*��ϢID*/
	uint16_t	head_sn;        /*��Ϣ��ˮ��*/
/*��ʵ�ķ�������*/
	uint16_t	msg_len;        /*��Ϣ����*/
	uint8_t		*pmsg;          /*������Ϣ��,��ʵ��Ҫ���͵����ݸ�ʽ������ת���FCS���<7e>Ϊ��־*/
}JT808_TX_MSG_NODEDATA;



typedef enum
{
	SOCKET_IDLE=1,	/*��������*/
//	SOCKET_INIT,
	SOCKET_DNS,	/*DNS��ѯ��*/
	SOCKET_DNS_ERR,
	SOCKET_CONNECT, /*������*/
	SOCKET_CONNECT_ERR, /*���Ӵ��󣬶Է���Ӧ��*/
	SOCKET_READY,	/*����ɣ����Խ�������*/
}T_SOCKET_STATE;


/*���֧��4������*/
#define MAX_GSM_SOCKET 4

typedef struct 
{
	T_SOCKET_STATE	state; 		     /*����״̬*/
	char			type;			/*�������� 'u':udp client 't':TCP client  'U' udp server*/
	char*			apn;
	char*			user;
	char*			psw;
	char*			ip_domain;    /*����*/
	char			ip_str[16];     /*dns���IP xxx.xxx.xxx.xxx*/
	uint16_t		port;           /*�˿�*/
//	MsgList*		msglist_tx;
//	MsgList*		msglist_rx;
}GSM_SOCKET;



extern JT808_PARAM jt808_param;

extern GSM_SOCKET curr_gsm_socket;

void gps_rx( uint8_t *pinfo, uint16_t length );
rt_err_t gprs_rx( uint8_t linkno, uint8_t *pinfo, uint16_t length );


#endif

/************************************** The End Of File **************************************/
