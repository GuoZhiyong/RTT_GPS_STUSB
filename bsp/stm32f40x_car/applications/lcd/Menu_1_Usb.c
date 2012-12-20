#include "Menu_Include.h"
#include "stm32f4xx.h"


unsigned char in_out=1;
unsigned char DataOutInFlag=0;//  1:导入导出界面  2:导出子菜单
unsigned char DynamicDisFlag=0,DynamicDisCoun=0;
unsigned char ZiKu_jindu[10]={"   0/   0"};


void USB_shengji(void)
{
	if(USB_insertFlag==1)
		{
		lcd_fill(0x00);
		lcd_text12(30,10,"无USB设备",9,LCD_MODE_SET);
		lcd_update_all();
		}
	else
		{
		//Write_AppFile=1;
		rt_kprintf("\r\n本地程序升级");
		}
}
void DataInOutSe(unsigned char num)
{
	lcd_fill(0);
	lcd_text12(0,11,"USB",3,LCD_MODE_SET);
	if(num==1)
		{
		lcd_text12(60, 3,"1.数据导出",8,LCD_MODE_INVERT);
		lcd_text12(60,19,"2.固件升级",8,LCD_MODE_SET);
		}
	else
		{
		lcd_text12(60, 3,"1.数据导出",8,LCD_MODE_SET);
		lcd_text12(60,19,"2.固件升级",8,LCD_MODE_INVERT);
		}
	lcd_update_all();
}
void USB_OUTFileSe(unsigned char OutType)
{
	lcd_fill(0);
	if(OutType==1)
		lcd_text12(0,3,"1.指定的事故疑点记录",20,LCD_MODE_SET);
	else if(OutType==2)
		lcd_text12(0,3,"2.指定的疲劳驾驶记录",20,LCD_MODE_SET);
	else if(OutType==3)
	    lcd_text12(0,3,"3.指定的超速驾驶记录",20,LCD_MODE_SET);
	lcd_update_all();
}


// 1:数据到出中   2:导出完成
void DataOUT(unsigned char i)
{
if(i==1)
	lcd_text12(58, 10,"数据导出... ...",15,LCD_MODE_SET);
else if(i==2)
	lcd_text12(65,10,"数据导出完成",12,LCD_MODE_SET);
}

void DeviceCheck(void)
{
	lcd_fill(0x00);
	if(OUT_DataCounter==1)
		{
		DataOutStartFlag=1;
		if(USB_insertFlag==1)
			lcd_text12(30,10,"无USB设备",9,LCD_MODE_SET);
		else
			DataOUT(1);
		}
	else if(OUT_DataCounter==2)
		{
		DataOutStartFlag=2;
		if(USB_insertFlag==1)
			lcd_text12(30,10,"无USB设备",9,LCD_MODE_SET);
		else
			DataOUT(1);
		}
	else if(OUT_DataCounter==3)
		{
		DataOutStartFlag=3;
		if(USB_insertFlag==1)
			lcd_text12(30,10,"无USB设备",9,LCD_MODE_SET);
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

				pMenuItem=&Menu_2_3_8_Ver;
				pMenuItem->show();
				//Task_SuspendResume(2);

				/*Write_OverTime=0;//本地升级时间统计
				Write_AppFile=0;//程序升级过程中==1,直到升级完或者超时
				FilePage=0;//记录文件大小，读文件大小/514
				FilePageCounter=0;//升级过程中记录文件升级进度的大小
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
							DataOutInFlag=2;//导出数据类型选择标志
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
					if(OUT_DataCounter>=3)
						OUT_DataCounter=3;
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
		lcd_text12(36,3,"固件升级",8,LCD_MODE_SET);
		lcd_text12(10,19,(char *)FileName_zk,sizeof(FileName_zk),LCD_MODE_SET);		
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
		lcd_text12(25,10,"USB设备拔出",11,LCD_MODE_SET);
		lcd_update_all();
	  	}
/*	if(UpdataComplete==1)
		{
		UpdataComplete=0;
		lcd_fill(0x00);
		lcd_text12(24,10,"固件升级完成"12,LCD_MODE_SET);
		lcd_update_all();
		}
	else if((UpdataComplete==2)||(UpdataComplete==3))
		{
		lcd_fill(0x00);
		if(UpdataComplete==2)
			lcd_text12(24,3,"固件升级超时",12,LCD_MODE_SET);
		else if(UpdataComplete==3)
			lcd_text12(24,3,"重写次数过多",12,LCD_MODE_SET);
		lcd_text12(30,19,"请重新开始",10,LCD_MODE_SET);
		UpdataComplete=0;
		lcd_update_all();
		}
	else if(UpdataComplete==4)
		{
		UpdataComplete=0;
		lcd_fill(0x00);
		//lcd_text12(40, 3,(char *)Device_ID,sizeof(Device_ID),LCD_MODE_SET);
		lcd_text12(8,19,"类型错误，不予更新",18,LCD_MODE_SET);
		lcd_update_all();
		}
	else if(UpdataComplete==5)
		{
		UpdataComplete=0;
		lcd_fill(0x00);
		lcd_text12(36,3,"下载错误",8,LCD_MODE_SET);
		lcd_text12(0,19,"请检查是否有下载文件",20,LCD_MODE_SET);
		lcd_update_all();
		}
	else if(UpdataComplete==6)
		{
		UpdataComplete=0;
		lcd_fill(0x00);
		lcd_text12(0,10,"校验错误，请重新升级",20,LCD_MODE_SET);
		lcd_update_all();
		}  */
   
	CounterBack++;
	if(CounterBack!=MaxBankIdleTime*2)
		return;
	pMenuItem=&Menu_1_Idle;
	pMenuItem->show();
    CounterBack=0;
}

ALIGN(RT_ALIGN_SIZE)
MENUITEM	Menu_1_usb=
{
	"USB数据",
	7,
	&show,
	&keypress,
	&timetick,
	(void*)0
};

