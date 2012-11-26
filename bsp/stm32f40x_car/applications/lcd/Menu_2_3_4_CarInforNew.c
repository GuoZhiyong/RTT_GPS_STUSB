#include  <string.h>
#include "menu.h"
#include "Lcd_init.h"


struct IMG_DEF test_dis_Driver2={12,12,test_00};

//static unsigned char updown_flag=0;
//static unsigned char updown_counter=0;
static unsigned char Speed_GetType=0;


typedef struct _DIS_CAR_INFOR
{
unsigned char DIS_TEXT_screen;
unsigned char DIS_TEXT_screen_counter;
}DIS_CARI_NFOR;

DIS_CARI_NFOR DIS_CAR_inform_temp;


//��ʻԱ����
void Display_driver(unsigned char drivercar)
{
switch(drivercar)
	{
	case 1:
		lcd_fill(0);
		//���ƺ�
		DisAddRead_ZK(10,3,"���ƺ�",3,&test_dis_Driver2,0,0);
		//DisAddRead_ZK(53,3,(char *)Vechicle_Info.Vech_Num,1,&test_dis_Driver2,0,0);
		//lcd_text(65,3,FONT_NINE_DOT,(char *)Vechicle_Info.Vech_Num+2);
        //��������
		DisAddRead_ZK(10,19,"��������",4,&test_dis_Driver2,0,0);
		//DisAddRead_ZK(65,19,(char *)Vechicle_Info.Vech_Type,3,&test_dis_Driver2,0,0);
		lcd_update_all();
		break;

	case 2:
		lcd_fill(0);
		DisAddRead_ZK(0,5,"����",2,&test_dis_Driver2,0,0);
		lcd_text(25,8,FONT_SEVEN_DOT,"VIN:");
		//lcd_text(0,20,FONT_SIX_DOT,(char *)Vechicle_Info.Vech_VIN);
		lcd_update_all();
		break;
	case 3:
		lcd_fill(0);
        //��ȡ�豸�ٶ�ȡ����GPS�ٶȻ����ٶ����ٶ�
		//DF_ReadFlash(DF_Speed_GetType_Page,0,(u8*)&Speed_GetType,1);
		if(Speed_GetType==0)
			{
			DisAddRead_ZK(0,5,"�豸�ٶ�",4,&test_dis_Driver2,0,0);
			lcd_text(48,8,FONT_SEVEN_DOT,":GPS");
			DisAddRead_ZK(76,5,"�ٶ�",2,&test_dis_Driver2,0,0); 
			}
		else if(Speed_GetType==1)
			{
			DisAddRead_ZK(0,5,"�豸�ٶ�",4,&test_dis_Driver2,0,0);
			lcd_text(48,8,FONT_NINE_DOT,":");
			DisAddRead_ZK(57,5,"�������ٶ�",5,&test_dis_Driver2,0,0); 
			}
		lcd_update_all();
		break;
	default:
		break;
	}

}


static void show(void)
{
	DIS_CAR_inform_temp.DIS_TEXT_screen=1;
	Display_driver(1);
}


static void keypress(unsigned int key)
{
	switch(KeyValue)
		{
		case KeyValueMenu:
			pMenuItem=&Menu_1_InforCheck;
			pMenuItem->show();
			
			CounterBack=0;
 			memset(&DIS_CAR_inform_temp,0,sizeof(DIS_CAR_inform_temp));
			break;
		case KeyValueOk:
			Display_driver(1);
			break;
		case KeyValueUP:
            if(DIS_CAR_inform_temp.DIS_TEXT_screen==1)
            	{
            	DIS_CAR_inform_temp.DIS_TEXT_screen_counter--;
				if(DIS_CAR_inform_temp.DIS_TEXT_screen_counter<=1)
					DIS_CAR_inform_temp.DIS_TEXT_screen_counter=1;
				Display_driver(DIS_CAR_inform_temp.DIS_TEXT_screen_counter);
            	}
			break;
		case KeyValueDown:
			if(DIS_CAR_inform_temp.DIS_TEXT_screen==1)
				{
				DIS_CAR_inform_temp.DIS_TEXT_screen_counter++;
				if(DIS_CAR_inform_temp.DIS_TEXT_screen_counter>=3)
					DIS_CAR_inform_temp.DIS_TEXT_screen_counter=3;
				Display_driver(DIS_CAR_inform_temp.DIS_TEXT_screen_counter);
				}
			break;
		}
	KeyValue=0;
}


static void timetick(unsigned int systick)
{
	CounterBack++;
	if(CounterBack!=MaxBankIdleTime)
		return;
	
	pMenuItem=&Menu_1_Idle;
	pMenuItem->show();

	CounterBack=0;
	memset(&DIS_CAR_inform_temp,0,sizeof(DIS_CAR_inform_temp));
}


MENUITEM	Menu_2_3_4_carinfor=
{
	"������Ϣ�鿴",
	&show,
	&keypress,
	&timetick,
	(void*)0
};


