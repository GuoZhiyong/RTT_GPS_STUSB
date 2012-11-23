#include <stdio.h>

#include "menu.h"
#include "fonts.h"
#include <string.h>

#include "stm32f4xx.h"


struct IMG_DEF test_dis_pub={12,12,test_00};

MENUITEM *pMenuItem;

unsigned int CounterBack=0;
unsigned char UpAndDown=1;//�����������˵�ѡ�����

unsigned char Dis_date[10]={"00/00/00"};
unsigned char Dis_time[10]={"00:00:00"};
unsigned char Dis_speed[10]={"000 km/h"};
unsigned char Dis_direction[5]={"000"};

unsigned char GPS_Flag=0,Gprs_Online_Flag=0;//��¼gps gprs״̬�ı�־


unsigned char speed_time_rec[15][6];//��  ��  ��  ʱ  ��  �ٶ�
unsigned char DriverName[22],DriverCardNUM[20];//��IC��������ļ�ʻԱ�����ͼ�ʻ֤����
unsigned char ServiceNum[13];//�豸��Ψһ�Ա���,IMSI����ĺ�12λ

unsigned char KeyValue=0;
unsigned char KeyCheck_Flag[4]={0,0,0,0};             

unsigned char ErrorRecord=0;//ƣ�ͳ��ټ�¼   ƣ��ʱ�����Ϊ1����ʱ�����Ϊ2,���������0
PilaoRecord PilaoJilu[12];
ChaosuRecord ChaosuJilu[20];
StatusSingle CarStatusInfor[8];
DriverLandExit DriverLandRecord[10];
ACCOnOffRecord ACConoffRecord[20];

unsigned char StartDisTiredExpspeed=0;//��ʼ��ʾƣ�ͻ��߳��ټ�ʻ�ļ�¼,���ж���ʾʱ����Ϣ����ʱ��
unsigned char tire_Flag=0,expsp_Flag=0;//ƣ�ͼ�ʻ/���ټ�ʻ  �м�¼Ϊ1(��ʾ�м�����¼)���޼�¼Ϊ2���鿴��¼��Ϊ3(��ʾ��down�����鿴)
unsigned char pilaoCounter=0,chaosuCounter=0;//��¼����ƣ�ͼ�ʻ�ͳ��ټ�ʻ������
unsigned char pilaoCouAscii[2],chaosuCouAscii[2];
DispMailBoxInfor LCD_Post,GPStoLCD,OtherToLCD,PiLaoLCD,ChaoSuLCD;

unsigned char SetVIN_NUM=1;// :���ó��ƺ���  2:����VIN
unsigned char OK_Counter=0;//��¼�ڿ�ݲ˵���ok�����µĴ���
unsigned char Screen_In=0,Screen_in0Z=0; //��¼��ѡ����ѡ�еĺ���

unsigned char OKorCancel=1,OKorCancel2=1,OKorCancelFlag=1;
unsigned char SetTZXSFlag=0,SetTZXSCounter=0;//SetTZXSFlag  1:У׼��������ϵ����ʼ  2:У׼��������ϵ������
                                                  //1���ݵ���(... .../���)  2:usb�豸�γ�
unsigned char OUT_DataCounter=0,DataOutStartFlag=0,DataOutOK=0;
unsigned char Rx_TZXS_Flag=0;

unsigned char battery_flag=0,tz_flag=0;
unsigned char USB_insertFlag=1;

unsigned char XinhaoStatus[10]={":00000000"};

unsigned char dayin_ErrorStatus=0;//��ӡ������״̬ 1:��ѹ 2:ѹ�Ὺ 3:ȱֽ 4:���� 5:û��ȡ����Ч����
unsigned char BuzzerFlag=0;//=1��1��  ��11��2��

unsigned char DaYin=0;//�������´�ӡ��Ϊ101��2s��Ϊ1����ʼ��ӡ(�ж��Ƿ�ȡ�����ݣ�û����ʾ�����������ݴ�ӡ)
unsigned char DaYinDelay=0;

unsigned char FileName_zk[11];

//==============�����д���ݴ�,��ʾ�������ʹ��========================
unsigned char test_idle[480];
//==============12*12========���ֿ��к��ֵĵ���==========
unsigned char test_00[24],Read_ZK[24];

unsigned char ANTENNA_CHECK=0;

unsigned char DisComFlag=0;



unsigned char a1[20];//�������    
unsigned char a2[20];//����װ��׼������
unsigned char a3[20];//ƽ������һ��˳��
unsigned char a4[20];//ָ���ص��δ����
unsigned char a5[20];//ָ���ص����˽Ӵ�
unsigned char a6[20];//�����޷���ϵ����
unsigned char a7[20];//������������
unsigned char a8[20];//�м������ٻػ�

unsigned char b1[20];//����Ԥ��
unsigned char b2[20];//������Ϣ
unsigned char b3[20];//��ͨ��Ϣ
unsigned char b4[20];//��ʳ��Ϣ
unsigned char b5[20];//��¼��Ϣ
unsigned char b6[20];//�¼���Ϣ
unsigned char b7[20];//ʱ����Ϣ
unsigned char b8[20];//������Ϣ



unsigned char DisInfor_Menu[8][20];
unsigned char DisInfor_Affair[8][20];

unsigned char UpAndDown_nm[4]={0xA1,0xFC,0xA1,0xFD};//�� ��




//================================================================
//   �麺�ֿ���ʾ����
//================================================================
void  Check_Hz_Show(unsigned int i,unsigned int j)
{
unsigned char m=0;
unsigned long int hz_address=0, hz_94_add_in=0,hz_94_add=0;
unsigned long int addr=0,value=0;

if((i>=0xb0)&&(i<=0xf7))	//��������
	hz_94_add=(i-0xB0)*ZIKU_94_24_hz+Dis_start_address_hz;
else
	{
	if((i>=0xa1)&&(i<=0xa9))	//����
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

