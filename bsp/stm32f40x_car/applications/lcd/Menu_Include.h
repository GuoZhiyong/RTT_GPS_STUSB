#ifndef _H_MENU_H_
#define _H_MENU_H_


#include <rtthread.h>
#include <stdio.h>
#include <rtdef.h>

#include "jt808.h"
#include "sed1520.h"



#define KeyValueMenu    1
#define KeyValueOk      2
#define KeyValueUP      3
#define KeyValueDown    4

//(1) 文本信息          开始Page 6800-6899   100 page
//(2) 事件设置          开始Page 6900-6949    50 page
//(3) 信息点播菜单设置  开始Page 6950-6999    50 page
#define InforStartPage_Text       6800
#define InforStartPage_Affair     6900
#define InforStartPage_Meun       6950


#define DECL_BMP(width,height,imgdata)	struct IMG_DEF BMP_##imgdata={width,height,imgdata}	

#define MaxBankIdleTime  1000//60s  LCD任务执行周期60ms,1min没有按键操作推出到idle状态


typedef void (*SHOW)(void );
typedef void (*KEYPRESS)(int);
typedef void (*TIMETICK)(int);
typedef void (*MSG)(void *p);


typedef  struct _menuitem{
	char *caption;			/*菜单项的文字信息*/
	unsigned char len;
	SHOW show;				/*显示时调用，初始化显示*/
	KEYPRESS keypress;		/*发生按键时调用*/
	TIMETICK timetick;		/*向其提供系统tick，比如返回待机画面*/
	MSG msg;				/*对外提供回调函数*/
	struct _menuitem *parent;	   
}MENUITEM; 

typedef struct _menuitem * PMENUITEM; 


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



extern unsigned char Dis_date[20];
extern unsigned char Dis_speDer[20];
extern unsigned char gps_bd_modeFLAG;//北斗1，gps2，双模3


extern unsigned char GPS_Flag,Gprs_Online_Flag;//记录gps gprs状态的标志
extern unsigned char DriverName[22],DriverCardNUM[20];//从IC卡里读出的驾驶员姓名和驾驶证号码
extern unsigned char speed_time_rec[15][6];

extern unsigned char ErrorRecord;
extern PilaoRecord PilaoJilu[12];
extern ChaosuRecord ChaosuJilu[20];

extern DispMailBoxInfor LCD_Post,GPStoLCD,OtherToLCD,PiLaoLCD,ChaoSuLCD;

extern unsigned char StartDisTiredExpspeed;//开始显示疲劳或者超速驾驶的记录,再判断提示时间信息错误时用
extern unsigned char tire_Flag,expsp_Flag;
extern unsigned char pilaoCounter,chaosuCounter;//记录返回疲劳驾驶和超速驾驶的条数
extern unsigned char pilaoCouAscii[2],chaosuCouAscii[2];

extern unsigned char ServiceNum[13];//设备的唯一性编码,IMSI号码的后12位
extern unsigned char DisComFlag;




ALIGN(RT_ALIGN_SIZE)
extern  MENUITEM *pMenuItem;

ALIGN(RT_ALIGN_SIZE)
extern MENUITEM	Menu_0_loggingin;
ALIGN(RT_ALIGN_SIZE) 
extern MENUITEM scr_logo; 
ALIGN(RT_ALIGN_SIZE) 
extern MENUITEM	Menu_SetTZXS;
ALIGN(RT_ALIGN_SIZE)
extern  MENUITEM	Menu_CenterTextInfor;
ALIGN(RT_ALIGN_SIZE)
extern  MENUITEM   Menu_1_Idle; 
//ALIGN(RT_ALIGN_SIZE)
//extern  MENUITEM	Menu_1_CarSet; 
ALIGN(RT_ALIGN_SIZE)
extern  MENUITEM	Menu_1_InforCheck;
ALIGN(RT_ALIGN_SIZE)
extern  MENUITEM	Menu_1_InforInteract;
ALIGN(RT_ALIGN_SIZE)
extern  MENUITEM Menu_1_InforTirExspd;
ALIGN(RT_ALIGN_SIZE)
extern  MENUITEM   Menu_1_usb;

ALIGN(RT_ALIGN_SIZE)
extern  MENUITEM	Menu_2_2_1_license;
ALIGN(RT_ALIGN_SIZE)
extern  MENUITEM	Menu_2_2_3_SpeedSet;
ALIGN(RT_ALIGN_SIZE)
extern MENUITEM	Menu_2_2_5_CarSim;
ALIGN(RT_ALIGN_SIZE)
extern MENUITEM	Menu_2_2_6_Carcol;

ALIGN(RT_ALIGN_SIZE)
extern  MENUITEM Menu_2_3_1_Sing8;
ALIGN(RT_ALIGN_SIZE)
extern  MENUITEM Menu_2_3_2_sudu;
ALIGN(RT_ALIGN_SIZE)
extern  MENUITEM	Menu_2_3_3_TextInforStor;
ALIGN(RT_ALIGN_SIZE)
extern  MENUITEM	Menu_2_3_4_carinfor;
ALIGN(RT_ALIGN_SIZE)
extern  MENUITEM Menu_2_3_5_jiayuan;
ALIGN(RT_ALIGN_SIZE)
extern  MENUITEM Menu_2_3_6_Mileage;
ALIGN(RT_ALIGN_SIZE)
extern  MENUITEM Menu_2_3_7_CenterInforMeun;
ALIGN(RT_ALIGN_SIZE)
extern  MENUITEM	Menu_2_3_8_Ver;
ALIGN(RT_ALIGN_SIZE)
extern  MENUITEM Menu_2_4_1_CenterQuestion;
ALIGN(RT_ALIGN_SIZE)
extern  MENUITEM	Menu_2_4_2_CarStatus;
ALIGN(RT_ALIGN_SIZE)
extern  MENUITEM	Menu_2_4_3_CarEleInfor;
ALIGN(RT_ALIGN_SIZE)
extern  MENUITEM	Menu_2_4_4_Multimedia;
ALIGN(RT_ALIGN_SIZE)
extern  MENUITEM	Menu_2_4_5_Record;
ALIGN(RT_ALIGN_SIZE)
extern  MENUITEM Menu_2_4_6_CenterAffairSet;
ALIGN(RT_ALIGN_SIZE)
extern  MENUITEM Menu_2_4_7_LogOut;
ALIGN(RT_ALIGN_SIZE)
extern  MENUITEM Menu_2_5_1_pilao;
ALIGN(RT_ALIGN_SIZE)
extern  MENUITEM Menu_2_5_2_chaosu;

ALIGN(RT_ALIGN_SIZE)
extern MENUITEM Menu_2_6_1_BDupdata;
ALIGN(RT_ALIGN_SIZE)
extern MENUITEM Menu_2_6_2_can;
ALIGN(RT_ALIGN_SIZE)
extern MENUITEM Menu_2_6_3_concuss;
ALIGN(RT_ALIGN_SIZE)
extern MENUITEM Menu_2_6_4_version;
ALIGN(RT_ALIGN_SIZE)
extern MENUITEM Menu_2_6_5_tel;
ALIGN(RT_ALIGN_SIZE)
extern MENUITEM Menu_2_6_6_clear;
ALIGN(RT_ALIGN_SIZE)
extern MENUITEM Menu_2_6_7_colstart;

ALIGN(RT_ALIGN_SIZE)
extern MENUITEM	Menu_DisTelText;
ALIGN(RT_ALIGN_SIZE)
extern MENUITEM	Menu_DisMultimedia;

ALIGN(RT_ALIGN_SIZE)
extern MENUITEM	Menu_Check_DnsIp;

extern MENUITEM	Menu_1_bdupgrade;


extern unsigned char SetVIN_NUM;//   1:设置车牌号码  2:设置VIN
extern unsigned char OK_Counter;//记录在快捷菜单下ok键按下的次数
extern unsigned char Screen_In,Screen_in0Z; //记录备选屏内选中的汉字

extern unsigned char OKorCancel,OKorCancel2,OKorCancelFlag;
extern unsigned char SetTZXSFlag,SetTZXSCounter;//SetTZXSFlag  1:校准车辆特征系数开始  2:校准车辆特征系数结束

extern unsigned char OUT_DataCounter,DataOutStartFlag,DataOutOK;
extern unsigned char Rx_TZXS_Flag;
extern unsigned char battery_flag,tz_flag;
extern unsigned char USB_insertFlag;

extern unsigned char BuzzerFlag;
extern unsigned char DaYin;
extern unsigned char DaYinDelay;

extern unsigned char FileName_zk[11];

//==============读多个写入暂存,显示多个汉字使用========================
extern unsigned char test_idle[480];
//==============12*12========读字库中汉字的点阵==========
extern unsigned char test_00[24],Read_ZK[24];


//↑ ↓
extern unsigned char UpAndDown_nm[4];
extern unsigned char ICcard_flag;

extern unsigned char a1[20];//申请出车    
extern unsigned char a2[20];//货己装齐准备启运
extern unsigned char a3[20];//平安到达一切顺利
extern unsigned char a4[20];//指定地点货未备齐
extern unsigned char a5[20];//指定地点无人接待
extern unsigned char a6[20];//货到无法联系货主
extern unsigned char a7[20];//货到因货损拒收
extern unsigned char a8[20];//有急事请速回话

extern unsigned char b1[20];//天气预报
extern unsigned char b2[20];//娱乐信息
extern unsigned char b3[20];//交通信息
extern unsigned char b4[20];//美食信息
extern unsigned char b5[20];//记录信息
extern unsigned char b6[20];//事件信息
extern unsigned char b7[20];//时尚信息
extern unsigned char b8[20];//美容信息


extern unsigned char DisInfor_Menu[8][20];
extern unsigned char DisInfor_Affair[8][20];


//========================================================================
extern unsigned char Set0_Comp_Flag;//首次使用设置车牌号、sim卡号、车辆颜色
extern unsigned char UpdataDisp[8];//北斗升级进度
extern unsigned char BD_updata_flag;//北斗升级度u盘文件的标志
extern unsigned int  FilePageBD_Sum;//记录文件大小，读文件大小/514
extern unsigned int  bd_file_exist;//读出存在要升级的文件
extern unsigned char device_version[15];  
extern unsigned char bd_version[20];
extern unsigned char DomainNameStr_1[50]; // 
extern unsigned char DomainNameStr_2[50]; //
extern unsigned char RemoteIP_1[4];//   {124,207,144,178 };//                 
extern unsigned int  RemotePort_1; //   9016;//     
extern unsigned char RemoteIP_2[4];    
extern unsigned int  RemotePort_2;

extern unsigned char ISP_Updata_Flag; //远程升级主机程序进度显示标志   1:开始升级  2:升级完成


extern void Cent_To_Disp(void);
extern void version_disp(void);


#endif

