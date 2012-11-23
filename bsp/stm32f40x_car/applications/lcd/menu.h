#ifndef _H_MENU_H_
#define _H_MENU_H_

#include "fonts.h"
#include "sed1520.h"
#include <stdio.h>

#define KeyValueMenu    1
#define KeyValueOk      2
#define KeyValueUP      3
#define KeyValueDown    4

#define Dis_start_address_hz  0x08048430//0x08040000+8430  (15x94*24=0x8430)
#define Dis_start_address  0x08040000
#define ZIKU_94_24_hz     0x8D0   //����1�����ĵ����ֽ���



typedef void (*SHOW)(void);
typedef void (*KEYPRESS)(unsigned int);
//typedef void (*EXIT)(void);
typedef void (*TIMETICK)(unsigned int);
//typedef void (*MSG)(void *);


typedef struct _menuitem{
	SHOW show;				/*��ʾʱ���ã���ʼ����ʾ*/
	KEYPRESS keypress;		/*��������ʱ����*/
	//EXIT exit1(void);				/*�˳�ʱ����*/
	TIMETICK timetick;		/*�����ṩϵͳtick�����緵�ش�������*/
	//MSG		msg;
	struct _menuitem *parent;	
}MENUITEM;

typedef struct _menuitem * PMENUITEM;



//(1) �ı���Ϣ          ��ʼPage 6800-6899   100 page
//(2) �¼�����          ��ʼPage 6900-6949    50 page
//(3) ��Ϣ�㲥�˵�����  ��ʼPage 6950-6999    50 page
#define InforStartPage_Text       6800
#define InforStartPage_Affair     6900
#define InforStartPage_Meun       6950



#define DECL_BMP(width,height,imgdata)	struct IMG_DEF BMP_##imgdata={width,height,imgdata}	

#define MaxBankIdleTime  1000//60s  LCD����ִ������60ms,1minû�а��������Ƴ���idle״̬

extern unsigned int CounterBack;
extern unsigned char UpAndDown;
extern unsigned char KeyValue;
extern unsigned char KeyCheck_Flag[4];


typedef __packed struct
{
unsigned char Num;
unsigned char PCard[18];
unsigned char StartTime[6];
unsigned char EndTime[6];
}PilaoRecord;

typedef __packed struct
{
unsigned char Num;
unsigned char PCard[18];
unsigned char StartTime[6];
unsigned char EndTime[6];
unsigned char Speed;
}ChaosuRecord;

typedef __packed struct
{
unsigned char Head[3];
unsigned int Flag;
unsigned int AllLen;
unsigned char ZCFlag[2];
//unsigned char *PInfor;
unsigned char PInfor[200];
unsigned char CheckOut;
unsigned char End[3];
}DispMailBoxInfor;

typedef __packed struct
{
unsigned char SingleNum;
unsigned char SingleConfignre;
unsigned char SingleName;
}StatusSingle;

typedef __packed struct
{
unsigned char DriverTime[6];
unsigned char DriverCardNum[18];
unsigned char Type;  //01H����¼         02H���˳�          03H��������ʻ��
}DriverLandExit;

typedef __packed struct
{
unsigned char ACCTime[6];
unsigned char Type;  //1��ACC ON      2��ACC OFF
}ACCOnOffRecord;

extern unsigned char Dis_date[10];
extern unsigned char Dis_time[10];
extern unsigned char Dis_speed[10];
extern unsigned char Dis_direction[5];
extern unsigned char GPS_Flag,Gprs_Online_Flag;//��¼gps gprs״̬�ı�־
extern unsigned char DriverName[22],DriverCardNUM[20];//��IC��������ļ�ʻԱ�����ͼ�ʻ֤����
extern unsigned char speed_time_rec[15][6];

extern unsigned char ErrorRecord;
extern PilaoRecord PilaoJilu[12];
extern ChaosuRecord ChaosuJilu[20];
extern StatusSingle CarStatusInfor[8];
extern DriverLandExit DriverLandRecord[10];
extern ACCOnOffRecord ACConoffRecord[20];

extern DispMailBoxInfor LCD_Post,GPStoLCD,OtherToLCD,PiLaoLCD,ChaoSuLCD;

extern unsigned char StartDisTiredExpspeed;//��ʼ��ʾƣ�ͻ��߳��ټ�ʻ�ļ�¼,���ж���ʾʱ����Ϣ����ʱ��
extern unsigned char tire_Flag,expsp_Flag;
extern unsigned char pilaoCounter,chaosuCounter;//��¼����ƣ�ͼ�ʻ�ͳ��ټ�ʻ������
extern unsigned char pilaoCouAscii[2],chaosuCouAscii[2];

extern unsigned char ServiceNum[13];//�豸��Ψһ�Ա���,IMSI����ĺ�12λ
extern unsigned char DisComFlag;


extern MENUITEM scr_logo; 

extern MENUITEM	Menu_SetTZXS;
extern MENUITEM	Menu_CenterTextInfor;

extern MENUITEM Menu_1_Idle;
extern MENUITEM	Menu_1_CarSet;
extern MENUITEM	Menu_1_InforCheck;
extern MENUITEM	Menu_1_InforInteract;
extern MENUITEM	Menu_1_InforTirExspd;
extern MENUITEM Menu_1_usb;

extern MENUITEM	Menu_2_2_1_license;
extern MENUITEM Menu_2_2_2_CarType;
extern MENUITEM	Menu_2_2_3_SpeedSet;
extern MENUITEM	Menu_2_2_4_VINset;
extern MENUITEM	Menu_2_2_5_Cancel;
extern MENUITEM	Menu_2_2_6_oil;

extern MENUITEM Menu_2_3_1_Sing8;
extern MENUITEM Menu_2_3_2_sudu;
extern MENUITEM	Menu_2_3_3_TextInforStor;
extern MENUITEM	Menu_2_3_4_carinfor;
extern MENUITEM Menu_2_3_5_jiayuan;
extern MENUITEM Menu_2_3_6_Mileage;
extern MENUITEM Menu_2_3_7_CenterInforMeun;

extern MENUITEM Menu_2_4_1_CenterQuestion;
extern MENUITEM	Menu_2_4_2_CarStatus;
extern MENUITEM	Menu_2_4_3_CarEleInfor;
extern MENUITEM	Menu_2_4_4_Multimedia;
extern MENUITEM	Menu_2_4_5_Record;
extern MENUITEM Menu_2_4_6_CenterAffairSet;
extern MENUITEM Menu_2_4_7_LogOut;

extern MENUITEM Menu_2_5_1_pilao;
extern MENUITEM Menu_2_5_2_chaosu;

extern MENUITEM	Menu_2_6_1_Ver;

extern MENUITEM *pMenuItem;



extern unsigned char SetVIN_NUM;//1:���ó��ƺ���  2:����VIN
extern unsigned char OK_Counter;//��¼�ڿ�ݲ˵���ok�����µĴ���
extern unsigned char Screen_In,Screen_in0Z; //��¼��ѡ����ѡ�еĺ���

extern unsigned char OKorCancel,OKorCancel2,OKorCancelFlag;
extern unsigned char SetTZXSFlag,SetTZXSCounter;//SetTZXSFlag  1:У׼��������ϵ����ʼ  2:У׼��������ϵ������

extern unsigned char OUT_DataCounter,DataOutStartFlag,DataOutOK;
extern unsigned char Rx_TZXS_Flag;
extern unsigned char battery_flag,tz_flag;
extern unsigned char USB_insertFlag;
extern unsigned char XinhaoStatus[10];

extern unsigned char dayin_ErrorStatus;
extern unsigned char BuzzerFlag;
extern unsigned char DaYin;
extern unsigned char DaYinDelay;

extern unsigned char FileName_zk[11];

//==============�����д���ݴ�,��ʾ�������ʹ��========================
extern unsigned char test_idle[480];
//==============12*12========���ֿ��к��ֵĵ���==========
extern unsigned char test_00[24],Read_ZK[24];


//�� ��
extern unsigned char UpAndDown_nm[4];

extern unsigned char a1[20];//�������    
extern unsigned char a2[20];//����װ��׼������
extern unsigned char a3[20];//ƽ������һ��˳��
extern unsigned char a4[20];//ָ���ص��δ����
extern unsigned char a5[20];//ָ���ص����˽Ӵ�
extern unsigned char a6[20];//�����޷���ϵ����
extern unsigned char a7[20];//������������
extern unsigned char a8[20];//�м������ٻػ�

extern unsigned char b1[20];//����Ԥ��
extern unsigned char b2[20];//������Ϣ
extern unsigned char b3[20];//��ͨ��Ϣ
extern unsigned char b4[20];//��ʳ��Ϣ
extern unsigned char b5[20];//��¼��Ϣ
extern unsigned char b6[20];//�¼���Ϣ
extern unsigned char b7[20];//ʱ����Ϣ
extern unsigned char b8[20];//������Ϣ


extern unsigned char DisInfor_Menu[8][20];
extern unsigned char DisInfor_Affair[8][20];
extern unsigned char ANTENNA_CHECK;


extern void DisAddRead_ZK(char Left0ffset,char top ,char *p,char len,const struct IMG_DEF *img_ptr,char inver1,char inver2);


#endif

