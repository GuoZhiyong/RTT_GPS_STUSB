#include  <string.h>
#include "Menu_Include.h"
#include "Lcd.h"

unsigned char vech_num[16]={"车牌号:津000001"};
static unsigned char updown_flag=0;

//驾驶员代码
void Display_driver(uint8_t drivercar)
{
switch(drivercar)
	{
	case 1:
		lcd_fill(0);
		//车牌号Vechicle_Info.Vech_Num
		lcd_text12(10,3,vech_num,15,LCD_MODE_SET);
        //车辆颜色
		lcd_text12(10,19,"车辆颜色:蓝色",13,LCD_MODE_SET);
		lcd_update_all();
		break;

	case 2://车辆ID Vechicle_Info.DevicePhone
		lcd_fill(0);
		lcd_text12(0,3,"车辆ID:012345678912",19,LCD_MODE_SET);
		//读取设备速度取得是GPS速度还是速度线速度
		/*DF_ReadFlash(DF_Speed_GetType_Page,0,(u8*)&Speed_GetType,1);
		if(Speed_GetType==0)*/
			lcd_text12(0,19,"设备速度:GPS速度",16,LCD_MODE_SET);
		/*else if(Speed_GetType==1)
			lcd_text12(0,5,"设备速度:传感器速度",19,LCD_MODE_SET);*/
		lcd_update_all();
		break;
	default:
		break;
	}

}


static void show(void)
{
	lcd_fill(0);
	lcd_text12(24, 3,"车辆信息查看",12,LCD_MODE_SET);
	lcd_text12(24,19,"查看请按选择",12,LCD_MODE_SET);
	lcd_update_all();

}


static void keypress(unsigned int key)
{
	switch(KeyValue)
		{
		case KeyValueMenu:
			pMenuItem=&Menu_1_InforCheck;
			pMenuItem->show();
			CounterBack=0;

			updown_flag=0;
			break;
		case KeyValueOk:
			updown_flag=1;
			Display_driver(1);
			break;
		case KeyValueUP:
            if(updown_flag==1)
				Display_driver(1);
			break;
		case KeyValueDown:
			if(updown_flag==1)
				Display_driver(2);
			break;
		}
	KeyValue=0;
}


static void timetick(unsigned int systick)
{
    Cent_To_Disp();
	CounterBack++;
	if(CounterBack!=MaxBankIdleTime)
		return;
	CounterBack=0;
	pMenuItem=&Menu_1_Idle;
	pMenuItem->show();
}


ALIGN(RT_ALIGN_SIZE)
MENUITEM	Menu_2_3_4_carinfor=
{
	"车辆信息查看",
	12,
	&show,
	&keypress,
	&timetick,
	(void*)0
};


