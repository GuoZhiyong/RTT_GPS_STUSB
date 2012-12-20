#include  <string.h>
#include "Menu_Include.h"
#include "Lcd.h"

unsigned char vech_num[16]={"���ƺ�:��000001"};
static unsigned char updown_flag=0;

//��ʻԱ����
void Display_driver(uint8_t drivercar)
{
switch(drivercar)
	{
	case 1:
		lcd_fill(0);
		//���ƺ�Vechicle_Info.Vech_Num
		lcd_text12(10,3,vech_num,15,LCD_MODE_SET);
        //������ɫ
		lcd_text12(10,19,"������ɫ:��ɫ",13,LCD_MODE_SET);
		lcd_update_all();
		break;

	case 2://����ID Vechicle_Info.DevicePhone
		lcd_fill(0);
		lcd_text12(0,3,"����ID:012345678912",19,LCD_MODE_SET);
		//��ȡ�豸�ٶ�ȡ����GPS�ٶȻ����ٶ����ٶ�
		/*DF_ReadFlash(DF_Speed_GetType_Page,0,(u8*)&Speed_GetType,1);
		if(Speed_GetType==0)*/
			lcd_text12(0,19,"�豸�ٶ�:GPS�ٶ�",16,LCD_MODE_SET);
		/*else if(Speed_GetType==1)
			lcd_text12(0,5,"�豸�ٶ�:�������ٶ�",19,LCD_MODE_SET);*/
		lcd_update_all();
		break;
	default:
		break;
	}

}


static void show(void)
{
	lcd_fill(0);
	lcd_text12(24, 3,"������Ϣ�鿴",12,LCD_MODE_SET);
	lcd_text12(24,19,"�鿴�밴ѡ��",12,LCD_MODE_SET);
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
	"������Ϣ�鿴",
	12,
	&show,
	&keypress,
	&timetick,
	(void*)0
};


