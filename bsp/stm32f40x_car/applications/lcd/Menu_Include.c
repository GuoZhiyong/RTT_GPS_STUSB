#include <stdio.h>

#include "Menu_Include.h"
#include <string.h>

#include "stm32f4xx.h"




unsigned int CounterBack=0;
unsigned char UpAndDown=1;//参数设置主菜单选择序号

unsigned char Dis_date[20]={"2000/00/00  00:00:00"};
unsigned char Dis_speDer[20]={"000 km/h    000 度"};
unsigned char gps_bd_modeFLAG=3;//北斗1，gps2，双模3

unsigned char GPS_Flag=0,Gprs_Online_Flag=0;//记录gps gprs状态的标志


unsigned char speed_time_rec[15][6];//年  月  日  时  分  速度
unsigned char DriverName[22],DriverCardNUM[20];//从IC卡里读出的驾驶员姓名和驾驶证号码
unsigned char ServiceNum[13];//设备的唯一性编码,IMSI号码的后12位

unsigned char KeyValue=0;
unsigned char KeyCheck_Flag[4]={0,0,0,0};             

unsigned char ErrorRecord=0;//疲劳超速记录   疲劳时间错误为1超速时间错误为2,按任意键清0
PilaoRecord PilaoJilu[12];
ChaosuRecord ChaosuJilu[20];

unsigned char StartDisTiredExpspeed=0;//开始显示疲劳或者超速驾驶的记录,再判断提示时间信息错误时用
unsigned char tire_Flag=0,expsp_Flag=0;//疲劳驾驶/超速驾驶  有记录为1(显示有几条记录)，无记录为2，查看记录变为3(显示按down逐条查看)
unsigned char pilaoCounter=0,chaosuCounter=0;//记录返回疲劳驾驶和超速驾驶的条数
unsigned char pilaoCouAscii[2],chaosuCouAscii[2];
DispMailBoxInfor LCD_Post,GPStoLCD,OtherToLCD,PiLaoLCD,ChaoSuLCD;

unsigned char SetVIN_NUM=1;// :设置车牌号码  2:设置VIN
unsigned char OK_Counter=0;//记录在快捷菜单下ok键按下的次数
unsigned char Screen_In=0,Screen_in0Z=0; //记录备选屏内选中的汉字

unsigned char OKorCancel=1,OKorCancel2=1,OKorCancelFlag=1;
unsigned char SetTZXSFlag=0,SetTZXSCounter=0;//SetTZXSFlag  1:校准车辆特征系数开始  2:校准车辆特征系数结束
                                                 //    1数据导出(... .../完成)  2:usb设备拔出
unsigned char OUT_DataCounter=0,DataOutStartFlag=0,DataOutOK=0;
unsigned char Rx_TZXS_Flag=0;

unsigned char battery_flag=0,tz_flag=0;
unsigned char USB_insertFlag=1;


unsigned char BuzzerFlag=0;//=1响1声  ＝11响2声

unsigned char DaYin=0;//待机按下打印键为101，2s后为1，开始打印(判断是否取到数据，没有提示错误，渠道数据打印)
unsigned char DaYinDelay=0;

unsigned char FileName_zk[11];

//==============读多个写入暂存,显示多个汉字使用========================
unsigned char test_idle[480];
//==============12*12========读字库中汉字的点阵==========
unsigned char test_00[24],Read_ZK[24];
unsigned char DisComFlag=0;
unsigned char ICcard_flag=0;



unsigned char a1[20];//申请出车    
unsigned char a2[20];//货己装齐准备启运
unsigned char a3[20];//平安到达一切顺利
unsigned char a4[20];//指定地点货未备齐
unsigned char a5[20];//指定地点无人接待
unsigned char a6[20];//货到无法联系货主
unsigned char a7[20];//货到因货损拒收
unsigned char a8[20];//有急事请速回话

unsigned char b1[20];//天气预报
unsigned char b2[20];//娱乐信息
unsigned char b3[20];//交通信息
unsigned char b4[20];//美食信息
unsigned char b5[20];//记录信息
unsigned char b6[20];//事件信息
unsigned char b7[20];//时尚信息
unsigned char b8[20];//美容信息



unsigned char DisInfor_Menu[8][20];
unsigned char DisInfor_Affair[8][20];

unsigned char UpAndDown_nm[4]={0xA1,0xFC,0xA1,0xFD};//↑ ↓

//========================================================================
unsigned char Set0_Comp_Flag=0;//首次使用设置车牌号、sim卡号、车辆颜色
unsigned char UpdataDisp[8]={"001/000"};//北斗升级进度
unsigned char BD_updata_flag=0;//北斗升级度u盘文件的标志
unsigned int  FilePageBD_Sum=0;//记录文件大小，读文件大小/514
unsigned int  bd_file_exist=0;//读出存在要升级的文件
unsigned char device_version[15]={"主机版本:V 1.00"};  
unsigned char bd_version[20]={"模块版本:V 00.00.000"};
unsigned char DomainNameStr_1[50]="jt1.yunnan.org.cn";  // 
unsigned char DomainNameStr_2[50]="jt2.yunnan.org.cn";  //    
unsigned char RemoteIP_1[4]={60,28,50,210}; //   {124,207,144,178 };//                 
unsigned int  RemotePort_1=6001; //   9016;//     
unsigned char RemoteIP_2[4]={124,207,144,179};    
unsigned int  RemotePort_2=9016; 

unsigned char ISP_Updata_Flag=0; //远程升级主机程序进度显示标志   1:开始升级  2:升级完成

ALIGN(RT_ALIGN_SIZE)  
MENUITEM *pMenuItem;   


//中心下发消息或者条件触发显示消息函数
void Cent_To_Disp(void)
{
}
void version_disp(void)
{
	lcd_fill(0);
	lcd_text12(0, 3,device_version,sizeof(device_version),LCD_MODE_SET);
	lcd_text12(0,19,bd_version,sizeof(bd_version),LCD_MODE_SET);
	lcd_update_all();
}

