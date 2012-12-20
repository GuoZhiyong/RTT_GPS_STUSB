#include <stdio.h>

#include "Menu_Include.h"
#include <string.h>

#include "stm32f4xx.h"




unsigned int CounterBack=0;
unsigned char UpAndDown=1;//�����������˵�ѡ�����

unsigned char Dis_date[20]={"2000/00/00  00:00:00"};
unsigned char Dis_speDer[20]={"000 km/h    000 ��"};
unsigned char gps_bd_modeFLAG=3;//����1��gps2��˫ģ3

unsigned char GPS_Flag=0,Gprs_Online_Flag=0;//��¼gps gprs״̬�ı�־


unsigned char speed_time_rec[15][6];//��  ��  ��  ʱ  ��  �ٶ�
unsigned char DriverName[22],DriverCardNUM[20];//��IC��������ļ�ʻԱ�����ͼ�ʻ֤����
unsigned char ServiceNum[13];//�豸��Ψһ�Ա���,IMSI����ĺ�12λ

unsigned char KeyValue=0;
unsigned char KeyCheck_Flag[4]={0,0,0,0};             

unsigned char ErrorRecord=0;//ƣ�ͳ��ټ�¼   ƣ��ʱ�����Ϊ1����ʱ�����Ϊ2,���������0
PilaoRecord PilaoJilu[12];
ChaosuRecord ChaosuJilu[20];

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
                                                 //    1���ݵ���(... .../���)  2:usb�豸�γ�
unsigned char OUT_DataCounter=0,DataOutStartFlag=0,DataOutOK=0;
unsigned char Rx_TZXS_Flag=0;

unsigned char battery_flag=0,tz_flag=0;
unsigned char USB_insertFlag=1;


unsigned char BuzzerFlag=0;//=1��1��  ��11��2��

unsigned char DaYin=0;//�������´�ӡ��Ϊ101��2s��Ϊ1����ʼ��ӡ(�ж��Ƿ�ȡ�����ݣ�û����ʾ�����������ݴ�ӡ)
unsigned char DaYinDelay=0;

unsigned char FileName_zk[11];

//==============�����д���ݴ�,��ʾ�������ʹ��========================
unsigned char test_idle[480];
//==============12*12========���ֿ��к��ֵĵ���==========
unsigned char test_00[24],Read_ZK[24];
unsigned char DisComFlag=0;
unsigned char ICcard_flag=0;



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

//========================================================================
unsigned char Set0_Comp_Flag=0;//�״�ʹ�����ó��ƺš�sim���š�������ɫ
unsigned char UpdataDisp[8]={"001/000"};//������������
unsigned char BD_updata_flag=0;//����������u���ļ��ı�־
unsigned int  FilePageBD_Sum=0;//��¼�ļ���С�����ļ���С/514
unsigned int  bd_file_exist=0;//��������Ҫ�������ļ�
unsigned char device_version[15]={"�����汾:V 1.00"};  
unsigned char bd_version[20]={"ģ��汾:V 00.00.000"};
unsigned char DomainNameStr_1[50]="jt1.yunnan.org.cn";  // 
unsigned char DomainNameStr_2[50]="jt2.yunnan.org.cn";  //    
unsigned char RemoteIP_1[4]={60,28,50,210}; //   {124,207,144,178 };//                 
unsigned int  RemotePort_1=6001; //   9016;//     
unsigned char RemoteIP_2[4]={124,207,144,179};    
unsigned int  RemotePort_2=9016; 

unsigned char ISP_Updata_Flag=0; //Զ�������������������ʾ��־   1:��ʼ����  2:�������

ALIGN(RT_ALIGN_SIZE)  
MENUITEM *pMenuItem;   


//�����·���Ϣ��������������ʾ��Ϣ����
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

