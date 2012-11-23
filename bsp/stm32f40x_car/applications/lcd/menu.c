#include <stdio.h>

#include "menu.h"
#include "fonts.h"
#include <string.h>

#include "stm32f4xx.h"


struct IMG_DEF test_dis_pub={12,12,test_00};

MENUITEM *pMenuItem;

unsigned int CounterBack=0;
unsigned char UpAndDown=1;//参数设置主菜单选择序号

unsigned char Dis_date[10]={"00/00/00"};
unsigned char Dis_time[10]={"00:00:00"};
unsigned char Dis_speed[10]={"000 km/h"};
unsigned char Dis_direction[5]={"000"};

unsigned char GPS_Flag=0,Gprs_Online_Flag=0;//记录gps gprs状态的标志


unsigned char speed_time_rec[15][6];//年  月  日  时  分  速度
unsigned char DriverName[22],DriverCardNUM[20];//从IC卡里读出的驾驶员姓名和驾驶证号码
unsigned char ServiceNum[13];//设备的唯一性编码,IMSI号码的后12位

unsigned char KeyValue=0;
unsigned char KeyCheck_Flag[4]={0,0,0,0};             

unsigned char ErrorRecord=0;//疲劳超速记录   疲劳时间错误为1超速时间错误为2,按任意键清0
PilaoRecord PilaoJilu[12];
ChaosuRecord ChaosuJilu[20];
StatusSingle CarStatusInfor[8];
DriverLandExit DriverLandRecord[10];
ACCOnOffRecord ACConoffRecord[20];

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
                                                  //1数据导出(... .../完成)  2:usb设备拔出
unsigned char OUT_DataCounter=0,DataOutStartFlag=0,DataOutOK=0;
unsigned char Rx_TZXS_Flag=0;

unsigned char battery_flag=0,tz_flag=0;
unsigned char USB_insertFlag=1;

unsigned char XinhaoStatus[10]={":00000000"};

unsigned char dayin_ErrorStatus=0;//打印机错误状态 1:过压 2:压轴开 3:缺纸 4:过热 5:没有取到有效数据
unsigned char BuzzerFlag=0;//=1响1声  ＝11响2声

unsigned char DaYin=0;//待机按下打印键为101，2s后为1，开始打印(判断是否取到数据，没有提示错误，渠道数据打印)
unsigned char DaYinDelay=0;

unsigned char FileName_zk[11];

//==============读多个写入暂存,显示多个汉字使用========================
unsigned char test_idle[480];
//==============12*12========读字库中汉字的点阵==========
unsigned char test_00[24],Read_ZK[24];

unsigned char ANTENNA_CHECK=0;

unsigned char DisComFlag=0;



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




//================================================================
//   查汉字库显示矩阵
//================================================================
void  Check_Hz_Show(unsigned int i,unsigned int j)
{
unsigned char m=0;
unsigned long int hz_address=0, hz_94_add_in=0,hz_94_add=0;
unsigned long int addr=0,value=0;

if((i>=0xb0)&&(i<=0xf7))	//汉字区域
	hz_94_add=(i-0xB0)*ZIKU_94_24_hz+Dis_start_address_hz;
else
	{
	if((i>=0xa1)&&(i<=0xa9))	//符号
	hz_94_add=(i-0xA1)*ZIKU_94_24_hz+Dis_start_address;
	}

hz_94_add_in=(j-0xA1)*24;
hz_address=hz_94_add+hz_94_add_in;

addr = hz_address;
for(m=0;m<6;m++)
	{
	value=*(__IO uint32_t*)addr;
	Read_ZK[m*4+0]=(unsigned char)(value&0xff);
	Read_ZK[m*4+1]=(unsigned char)(value>>8);
	Read_ZK[m*4+2]=(unsigned char)(value>>16);
	Read_ZK[m*4+3]=(unsigned char)(value>>24);
	addr+=4;
	//rt_kprintf("\n%x,%x,%x,%x\n",Read_ZK[m*4],Read_ZK[m*4+1],Read_ZK[m*4+2],Read_ZK[m*4+3]);
	}
}



void DisAddRead_ZK(char Left0ffset,char top ,char *p,char len,const struct IMG_DEF *img_ptr,char inver1,char inver2)
{
char j=0;
for(j=0;j<len;j++)
		{			
		  Check_Hz_Show(p[j*2],p[j*2+1]);
		  memcpy(&test_idle[j*24],Read_ZK,24);   
		}
for(j=0;j<len;j++)
	{		
	if(j<10)
		{
		memcpy(test_00,&test_idle[j*24],24);
		if(inver1==0)
			lcd_bitmap((j*12)+Left0ffset,top,img_ptr,LCD_MODE_SET);
		else if(inver1==1)
			lcd_bitmap((j*12)+Left0ffset,top,img_ptr,LCD_MODE_INVERT);
		}
	else
		{
		memcpy(test_00,&test_idle[j*24],24);
		if(inver2==0)
			lcd_bitmap(((j-10)*12)+Left0ffset,top+16,img_ptr,LCD_MODE_SET);
		else if(inver2==1)
			lcd_bitmap(((j-10)*12)+Left0ffset,top+16,img_ptr,LCD_MODE_INVERT);
		}
	}
}

