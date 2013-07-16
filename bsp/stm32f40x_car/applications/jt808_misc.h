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
#ifndef _H_JT808_MISC_
#define _H_JT808_MISC_

#include "stm32f4xx.h"
#include "jt808.h"

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

extern TEXT_INFO	TextInfo;
//-------�ı���Ϣ-------
extern MSG_TEXT		TEXT_Obj;
extern MSG_TEXT		TEXT_Obj_8[8], TEXT_Obj_8bak[8];

//------ ����  --------
extern CENTRE_ASK ASK_Centre;                       // ��������

//------- �¼� ----
//extern EVENT	EventObj;                           // �¼�
//extern EVENT	EventObj_8[8];                      // �¼�

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

typedef __packed struct
{
	uint32_t	id;                     /*�������*/
	MYTIME		datetime;               /*�յ���ʱ��*/
	uint8_t		flag;					/*�Ѷ���δ��*/
	uint8_t		len;                    /*���ȣ���Ϊ�ض̵����256-9*/
	uint8_t		body[256 - 10];          /*�ض̺��յ�����Ϣ*/
}TEXTMSG;



typedef __packed struct
{
	uint32_t	id;                     /*�������*/
	MYTIME		datetime;               /*�յ���ʱ��*/
	uint8_t     flag;					/*�ѻش�δ�ش�*/
	uint8_t		len;                    /*���ȣ���Ϊ�ض̵����256-9*/
	uint8_t		body[256 - 10];          /*�ض̺��յ�����Ϣ*/
}CENTER_ASK;




/*
   �¼�:������4k��buffer��
 */
typedef __packed struct
{
	uint8_t flag;                       /*��־*/
	uint8_t id;                         /*�¼�ID*/
	uint8_t len;                        /*�¼�����*/
	uint8_t body[64-3];                   /*�¼�����*/
}EVENT;





extern uint8_t textmsg_count;
extern uint8_t center_ask_count;

void jt808_misc_0x8300( uint8_t *pmsg );
void jt808_misc_0x8301( uint8_t *pmsg );
void jt808_misc_0x8302( uint8_t *pmsg );
void jt808_misc_0x8303( uint8_t *pmsg );
void jt808_misc_0x8304( uint8_t *pmsg );
void jt808_misc_0x8400( uint8_t *pmsg );
void jt808_misc_0x8401( uint8_t *pmsg );
void jt808_misc_0x8500( uint8_t *pmsg );


void jt808_textmsg_get( uint8_t index, TEXTMSG* pout );

uint8_t jt808_event_get(void);

extern uint8_t* event_buf;




void jt808_misc_init( void );


#endif
/************************************** The End Of File **************************************/

