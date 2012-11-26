#include "menu.h"
#include "stm32f4xx.h"

struct IMG_DEF test_dis_usb={12,12,test_00};

unsigned char in_out=1;
unsigned char DataOutInFlag=0;//  1:���뵼������  2:�����Ӳ˵�
unsigned char DynamicDisFlag=0,DynamicDisCoun=0;
unsigned char ZiKu_jindu[10]={"   0/   0"};

void NOUSB(void)
	{
	DisAddRead_ZK(30,10,"��",1,&test_dis_usb,0,0);
	lcd_text(45, 13,FONT_SEVEN_DOT,"USB");
	DisAddRead_ZK(69,10,"�豸",2,&test_dis_usb,0,0);
	}

void USB_shengji(void)
{
	if(USB_insertFlag==1)
		{
		lcd_fill(0x00);
		NOUSB();
		lcd_update_all();
		}
	else
		{
		//Write_AppFile=1;
		printf("\r\n���س�������");
		}
}
void DataInOutSe(unsigned char num)
{
	lcd_fill(0);
	lcd_text(0,11,FONT_TEN_DOT,"USB");
	lcd_text(40,3,FONT_TEN_DOT,"1.");
	lcd_text(40,21,FONT_TEN_DOT,"2.");
	if(num==1)
		{
		DisAddRead_ZK(60,3,"���ݵ���",4,&test_dis_usb,1,0);
		DisAddRead_ZK(60,19,"�̼�����",4,&test_dis_usb,0,0);
		}
	else
		{
		DisAddRead_ZK(60,3,"���ݵ���",4,&test_dis_usb,0,0);
		DisAddRead_ZK(60,19,"�̼�����",4,&test_dis_usb,1,0);
		}
	lcd_update_all();
}
void USB_OUTFileSe(unsigned char OutType)
{
	lcd_fill(0);
	DisAddRead_ZK(15,3,"ָ����",3,&test_dis_usb,0,0);
	if(OutType==1)
		{
		lcd_text(0,  5,FONT_SEVEN_DOT,"1.");
		DisAddRead_ZK(51,3,"ƽ���ٶ�",4,&test_dis_usb,1,0);		
        lcd_text(36,  21,FONT_SEVEN_DOT,"(        )");
		DisAddRead_ZK(42,19,"ÿ����",3,&test_dis_usb,0,0);	
		}
	else if(OutType==2)
		{
		lcd_text(0,  5,FONT_SEVEN_DOT,"2.");
		DisAddRead_ZK(51,3,"λ����Ϣ",4,&test_dis_usb,1,0);
		}
	else if(OutType==3)
		{
		lcd_text(0,  5,FONT_SEVEN_DOT,"3.");
		DisAddRead_ZK(51,3,"�¹��ɵ�",4,&test_dis_usb,1,0);
		}
	else if(OutType==4)
		{
		lcd_text(0,  5,FONT_SEVEN_DOT,"4.");
		DisAddRead_ZK(51,3,"ƣ�ͼ�ʻ",4,&test_dis_usb,1,0);
		}
	else if(OutType==5)
		{
		lcd_text(0,  5,FONT_SEVEN_DOT,"5.");
	    DisAddRead_ZK(51,3,"���ټ�ʻ",4,&test_dis_usb,1,0);
		}
	else if(OutType==6)
		{
		lcd_text(0,  5,FONT_SEVEN_DOT,"6.");
		DisAddRead_ZK(51,3,"��¼�˳�",4,&test_dis_usb,1,0);
		}
	else if(OutType==7)
		{
		lcd_text(0,  5,FONT_SEVEN_DOT,"7.");
		lcd_text(52,  5,FONT_SEVEN_DOT,"ACC");
		DisAddRead_ZK(75,3,"���",2,&test_dis_usb,1,0);
		}
	DisAddRead_ZK(99,3,"��¼",2,&test_dis_usb,0,0);
	lcd_update_all();
}


// 1:���ݵ�����   2:�������
void DataOUT(unsigned char i)
{
DisAddRead_ZK(10,10,"���ݵ���",4,&test_dis_usb,0,0);
if(i==1)
	lcd_text(58, 10,FONT_SEVEN_DOT," ... ...");
else if(i==2)
	DisAddRead_ZK(65,10,"���",2,&test_dis_usb,1,0);
}

void DeviceCheck(void)
{
	lcd_fill(0x00);
	if(OUT_DataCounter==1)
		{
		DataOutStartFlag=1;
		if(USB_insertFlag==1)
			NOUSB();
		else
			DataOUT(1);
		}
	else if(OUT_DataCounter==2)
		{
		DataOutStartFlag=2;
		if(USB_insertFlag==1)
			NOUSB();
		else
			DataOUT(1);
		}
	else if(OUT_DataCounter==3)
		{
		DataOutStartFlag=3;
		if(USB_insertFlag==1)
			NOUSB();
		else
			DataOUT(1);
		}
	else if(OUT_DataCounter==4)
		{
		DataOutStartFlag=4;
		if(USB_insertFlag==1)
			NOUSB();
		else
			DataOUT(1);
		}
	else if(OUT_DataCounter==5)
		{
		DataOutStartFlag=5;
		if(USB_insertFlag==1)
			NOUSB();
		else
			DataOUT(1);
		}
	else if(OUT_DataCounter==6)
		{
		DataOutStartFlag=6;
		if(USB_insertFlag==1)
			NOUSB();
		else
			DataOUT(1);
		}
	else if(OUT_DataCounter==7)
		{
		DataOutStartFlag=7;
		if(USB_insertFlag==1)
			NOUSB();
		else
			DataOUT(1);
		}
	lcd_update_all();
}
void DisDataOUT(void)
{

	   if(DataOutStartFlag)
		{
		lcd_fill(00);
		if(DataOutStartFlag==1)
			{
			if(DynamicDisFlag==1)
				{
				DynamicDisFlag=0;
				DataOUT(1);
				}
			else
				{
				DynamicDisFlag=1;
				DataOUT(0);
				}
			}
		else if(DataOutStartFlag==2)
			{
			if(DynamicDisFlag==1)
				{
				DynamicDisFlag=0;
				DataOUT(1);
				}
			else
				{
				DynamicDisFlag=1;
				DataOUT(0);
				}
			}
		else if(DataOutStartFlag==3)
			{
			if(DynamicDisFlag==1)
				{
				DynamicDisFlag=0;
				DataOUT(1);
				}
			else
				{
				DynamicDisFlag=1;
				DataOUT(0);
				}
			}
		else if(DataOutStartFlag==4)
			{
			if(DynamicDisFlag==1)
				{
				DynamicDisFlag=0;
				DataOUT(1);
				}
			else
				{
				DynamicDisFlag=1;
				DataOUT(0);
				}
			}
		else if(DataOutStartFlag==5)
			{
			if(DynamicDisFlag==1)
				{
				DynamicDisFlag=0;
				DataOUT(1);
				}
			else
				{
				DynamicDisFlag=1;
				DataOUT(0);
				}
			}
		else if(DataOutStartFlag==6)
			{
			if(DynamicDisFlag==1)
				{
				DynamicDisFlag=0;
				DataOUT(1);
				}
			else
				{
				DynamicDisFlag=1;
				DataOUT(0);
				}
			}
		else if(DataOutStartFlag==7)
			{
			if(DynamicDisFlag==1)
				{
				DynamicDisFlag=0;
				DataOUT(1);
				}
			else
				{
				DynamicDisFlag=1;
				DataOUT(0);
				}
			}
		lcd_update_all();
		}

}
static void show(void)
{
    DataInOutSe(1);
	in_out=1;
	DataOutOK=0;
	OUT_DataCounter=1;
	DataOutInFlag=1;

}

	static void keypress(unsigned int key)
	{ 
		switch(KeyValue)
			{
			case KeyValueMenu:
				CounterBack=0;

				pMenuItem=&Menu_2_6_1_Ver;
				pMenuItem->show();
				//Task_SuspendResume(2);

				/*Write_OverTime=0;//��������ʱ��ͳ��
				Write_AppFile=0;//��������������==1,ֱ����������߳�ʱ
				FilePage=0;//��¼�ļ���С�����ļ���С/514
				FilePageCounter=0;//���������м�¼�ļ��������ȵĴ�С
				UpdataComplete=0;*/
				break;
			case KeyValueOk:
				if(DataOutInFlag==2)
					{
					DeviceCheck();
					DataOutInFlag=0;
					}
				if(DataOutInFlag==1)
					{
					if((in_out==1)||(in_out==2))
						{
						if(in_out==1)
							{
							DataOutInFlag=2;//������������ѡ���־
							USB_OUTFileSe(OUT_DataCounter);
							}
						else if(in_out==2)
							{
							USB_shengji();
							}
						}
					}
				break;
			case KeyValueUP:
				if(DataOutInFlag==1)
					{
					in_out=1;
					DataInOutSe(1);
					}
				if(DataOutInFlag==2)
					{
					OUT_DataCounter--;
					if(OUT_DataCounter<=1)
						OUT_DataCounter=1;
					USB_OUTFileSe(OUT_DataCounter);
					}
				break;
			case KeyValueDown:
				if(DataOutInFlag==1)
					{
					in_out=2;
					DataInOutSe(2);
					}
				if(DataOutInFlag==2)
					{
					OUT_DataCounter++;
					if(OUT_DataCounter>=7)
						OUT_DataCounter=7;
					USB_OUTFileSe(OUT_DataCounter);
					}
				break;
			}
	KeyValue=0;
	}
	

static void timetick(unsigned int systick)
{

	if(USB_insertFlag==0)
		{
		DynamicDisCoun++;
	    if(DynamicDisCoun>=2)
	    	{
	    	DynamicDisCoun=0;
			DisDataOUT();
	    	}
		}
/*	if(DisFileName_flag==1)
		{
		DisFileName_flag=0;
		lcd_fill(0);
		DisAddRead_ZK(36,3,"�̼�����",4,&test_dis_usb,0,0);
		lcd_text(10,19,FONT_TEN_DOT,(char *)FileName_zk);		
		lcd_update_all();
		} */
    if(DataOutOK==1)
    	{
    	DataOutOK=2;
    	lcd_fill(0x00);
		DataOUT(2);
		lcd_update_all();
    	}
	if((DataOutOK==2)&&(USB_insertFlag==1))
	  	{
		DataOutOK=0;
		lcd_fill(0x00);
		lcd_text(25, 13,FONT_SEVEN_DOT,"USB ");
		DisAddRead_ZK(49,10,"�豸�γ�",4,&test_dis_usb,0,0);
		lcd_update_all();
	  	}
/*	if(UpdataComplete==1)
		{
		UpdataComplete=0;
		lcd_fill(0x00);
		DisAddRead_ZK(24,10,"�̼��������",6,&test_dis_usb,0,0);
		lcd_update_all();
		}
	else if((UpdataComplete==2)||(UpdataComplete==3))
		{
		lcd_fill(0x00);
		if(UpdataComplete==2)
			DisAddRead_ZK(24,3,"�̼�������ʱ",6,&test_dis_usb,0,0);
		else if(UpdataComplete==3)
			DisAddRead_ZK(24,3,"��д��������",6,&test_dis_usb,0,0);
		DisAddRead_ZK(30,19,"�����¿�ʼ",5,&test_dis_usb,0,0);
		UpdataComplete=0;
		lcd_update_all();
		}
	else if(UpdataComplete==4)
		{
		UpdataComplete=0;
		lcd_fill(0x00);
		//lcd_text(40, 3,FONT_NINE_DOT,(char *)Device_ID);
		DisAddRead_ZK(8,19,"���ʹ��󣬲������",9,&test_dis_usb,0,0);
		lcd_update_all();
		}
	else if(UpdataComplete==5)
		{
		UpdataComplete=0;
		lcd_fill(0x00);
		DisAddRead_ZK(36,3,"���ش���",4,&test_dis_usb,0,0);
		DisAddRead_ZK(0,19,"�����Ƿ��������ļ�",10,&test_dis_usb,0,0);
		lcd_update_all();
		}
	else if(UpdataComplete==6)
		{
		UpdataComplete=0;
		lcd_fill(0x00);
		DisAddRead_ZK(0,10,"У���������������",10,&test_dis_usb,0,0);
		lcd_update_all();
		}  */
   
	CounterBack++;
	if(CounterBack!=MaxBankIdleTime*2)
		return;
	pMenuItem=&Menu_1_Idle;
	pMenuItem->show();
    CounterBack=0;
}


MENUITEM	Menu_1_usb=
{
	"",
	&show,
	&keypress,
	&timetick,
	(void*)0
};

