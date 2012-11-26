#include  <string.h>
#include "menu.h"
#include "Lcd_init.h"



struct IMG_DEF test_scr_CarType={12,12,test_00};

unsigned char CarType_counter=0;
unsigned char CarType_Type=0;


void CarType(unsigned char type_Sle,unsigned char sel)
{
switch(type_Sle)
	{
	case 1:
		lcd_fill(0);
		if(sel==0)
			{
			DisAddRead_ZK(24,3,"��������ѡ��",6,&test_scr_CarType,0,0);
	        lcd_text(0,19,FONT_NINE_DOT,"1."); 
		    DisAddRead_ZK(24,19,"���ͳ�",3,&test_scr_CarType,0,0);
			}
		else
			{
			DisAddRead_ZK(12,10,"��������",4,&test_scr_CarType,0,0);
	        lcd_text(60,10,FONT_NINE_DOT,":"); 
		    DisAddRead_ZK(70,10,"���ͳ�",3,&test_scr_CarType,0,0);
			}
		lcd_update_all();
		break;
	case 2:
		lcd_fill(0);
		if(sel==0)
			{
			DisAddRead_ZK(24,3,"��������ѡ��",6,&test_scr_CarType,0,0);
	        lcd_text(0,19,FONT_NINE_DOT,"2."); 
		    DisAddRead_ZK(24,19,"���ͳ�",3,&test_scr_CarType,0,0);
			}
		else
			{
			DisAddRead_ZK(12,10,"��������",4,&test_scr_CarType,0,0);
	        lcd_text(60,10,FONT_NINE_DOT,":"); 
		    DisAddRead_ZK(70,10,"���ͳ�",3,&test_scr_CarType,0,0);
			}
		lcd_update_all();
		break;
	case 3:
		lcd_fill(0);
		if(sel==0)
			{
			DisAddRead_ZK(24,3,"��������ѡ��",6,&test_scr_CarType,0,0);
	        lcd_text(0,19,FONT_NINE_DOT,"3."); 
		    DisAddRead_ZK(24,19,"С�ͳ�",3,&test_scr_CarType,0,0);
			}
		else
			{
			DisAddRead_ZK(12,10,"��������",4,&test_scr_CarType,0,0);
	        lcd_text(60,10,FONT_NINE_DOT,":"); 
		    DisAddRead_ZK(70,10,"С�ͳ�",3,&test_scr_CarType,0,0);
			}
		lcd_update_all();
		break;
	case 4:
		lcd_fill(0);
		if(sel==0)
			{
			DisAddRead_ZK(24,3,"��������ѡ��",6,&test_scr_CarType,0,0);
	        lcd_text(0,19,FONT_NINE_DOT,"4."); 
		    DisAddRead_ZK(24,19,"΢�ͳ�",3,&test_scr_CarType,0,0);
			}
		else
			{
			DisAddRead_ZK(12,10,"��������",4,&test_scr_CarType,0,0);
	        lcd_text(60,10,FONT_NINE_DOT,":"); 
		    DisAddRead_ZK(70,10,"΢�ͳ�",3,&test_scr_CarType,0,0);
			}
		lcd_update_all();
		break;
	case 5:
		lcd_fill(0);
		if(sel==0)
			{
			DisAddRead_ZK(24,3,"��������ѡ��",6,&test_scr_CarType,0,0);
	        lcd_text(0,19,FONT_NINE_DOT,"5."); 
		    DisAddRead_ZK(24,19,"���˳�",3,&test_scr_CarType,0,0);
			}
		else
			{
			DisAddRead_ZK(12,10,"��������",4,&test_scr_CarType,0,0);
	        lcd_text(60,10,FONT_NINE_DOT,":"); 
		    DisAddRead_ZK(70,10,"���˳�",3,&test_scr_CarType,0,0);
			}
		lcd_update_all();
		break;
	case 6:
		lcd_fill(0);
		if(sel==0)
			{
			DisAddRead_ZK(24,3,"��������ѡ��",6,&test_scr_CarType,0,0);
	        lcd_text(0,19,FONT_NINE_DOT,"6."); 
		    DisAddRead_ZK(24,19,"���˳�",3,&test_scr_CarType,0,0);
			}
		else
			{
			DisAddRead_ZK(12,10,"��������",4,&test_scr_CarType,0,0);
	        lcd_text(60,10,FONT_NINE_DOT,":"); 
		    DisAddRead_ZK(70,10,"���˳�",3,&test_scr_CarType,0,0);
			}
		lcd_update_all();
		break;
	case 7:
		lcd_fill(0);
		if(sel==0)
			{
			DisAddRead_ZK(24,3,"��������ѡ��",6,&test_scr_CarType,0,0);
	        lcd_text(0,19,FONT_NINE_DOT,"7."); 
		    DisAddRead_ZK(24,19,"ΣƷ��",3,&test_scr_CarType,0,0);
			}
		else
			{
			DisAddRead_ZK(12,10,"��������",4,&test_scr_CarType,0,0);
	        lcd_text(60,10,FONT_NINE_DOT,":"); 
		    DisAddRead_ZK(70,10,"ΣƷ��",3,&test_scr_CarType,0,0);
			}
		lcd_update_all();
		break;
	case 8:
		lcd_fill(0);
		if(sel==0)
			{
			DisAddRead_ZK(24,3,"��������ѡ��",6,&test_scr_CarType,0,0);
	        lcd_text(0,19,FONT_NINE_DOT,"8."); 
		    DisAddRead_ZK(24,19,"���⳵",3,&test_scr_CarType,0,0);
			}
		else
			{
			DisAddRead_ZK(12,10,"��������",4,&test_scr_CarType,0,0);
	        lcd_text(60,10,FONT_NINE_DOT,":"); 
		    DisAddRead_ZK(70,10,"���⳵",3,&test_scr_CarType,0,0);
			}
		lcd_update_all();
		break;
	}
}


static void show(void)
{
	lcd_fill(0);
	DisAddRead_ZK(24,3,"��������ѡ��",6,&test_scr_CarType,0,0);
	DisAddRead_ZK(0,19,"��ȷ�ϼ�ѡ��������",10,&test_scr_CarType,0,0);
	lcd_update_all();
	
	CarType_counter=1;
	CarType_Type=1;

	CarType(CarType_counter,0);
	//--printf("\r\n��������ѡ�� = %d",CarType_counter);
}


static void keypress(unsigned int key)
{
	switch(KeyValue)
		{
		case KeyValueMenu:
			pMenuItem=&Menu_1_CarSet;
			pMenuItem->show();

			break;
		case KeyValueOk:
			if(CarType_Type==1)
				{
				CarType_Type=2;
				CarType(CarType_counter,1);
				//printf("\r\nCarType_Type = %d",CarType_Type);
				}
			else if(CarType_Type==2)
				{
				CarType_Type=3;
				lcd_fill(0);
				DisAddRead_ZK(12,10,"��������ѡ�����",8,&test_scr_CarType,0,0);
				lcd_update_all();
				
				//д�복������
			/*	if((CarType_counter>=1)&&(CarType_counter<=8))
					memset(Vechicle_Info.Vech_Type,0,sizeof(Vechicle_Info.Vech_Type));
				if(CarType_counter==1)
					memcpy(Vechicle_Info.Vech_Type,"���ͳ�",6); 
				else if(CarType_counter==2)
					memcpy(Vechicle_Info.Vech_Type,"���ͳ�",6); 
				else if(CarType_counter==3)
					memcpy(Vechicle_Info.Vech_Type,"С�ͳ�",6); 
				else if(CarType_counter==4)
					memcpy(Vechicle_Info.Vech_Type,"΢�ͳ�",6); 
				else if(CarType_counter==5)
					memcpy(Vechicle_Info.Vech_Type,"���˳�",6); 
				else if(CarType_counter==6)
					memcpy(Vechicle_Info.Vech_Type,"���˳�",6); 
				else if(CarType_counter==7)
					memcpy(Vechicle_Info.Vech_Type,"ΣƷ��",6);
				else if(CarType_counter==8)
					memcpy(Vechicle_Info.Vech_Type,"���⳵",6); 
				Vehicleinfo_Write();   */
				} 
			else if(CarType_Type==3)
				{
				pMenuItem=&Menu_1_CarSet;
				pMenuItem->show();
				
				CarType_counter=0;
				CarType_Type=0;
				}
			break;
		case KeyValueUP:
			if(	CarType_Type==1)
				{
				CarType_counter--;
				if(CarType_counter<=1)
					CarType_counter=1;
				//printf("\r\n  up  ��������ѡ�� = %d",CarType_counter);
				CarType(CarType_counter,0);
				}
			break;
		case KeyValueDown:
			if(	CarType_Type==1)
				{
				CarType_counter++;
				if(CarType_counter>=8)
					CarType_counter=8;
				//printf("\r\n down ��������ѡ�� = %d",CarType_counter);
				CarType(CarType_counter,0);
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
	CounterBack=0;
	pMenuItem=&Menu_1_Idle;
	pMenuItem->show();

	
}


MENUITEM	Menu_2_2_2_CarType=
{
	"��������ѡ��",
	&show,
	&keypress,
	&timetick,
	(void*)0
};


