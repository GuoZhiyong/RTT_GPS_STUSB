#include  <string.h>
#include "Menu_Include.h"

u8 comfirmation_flag=0;
u8 col_screen=0;
u8 CarBrandCol_Cou=1;

unsigned char car_col[13]={"������ɫ:��ɫ"}; 

void car_col_fun(u8 par)
{                                                      
                                           //������ɫ�����
if(par==1)
	memcpy(Menu_VecLogoColor,"��ɫ",4);     //   1
else if(par==2)
	memcpy(Menu_VecLogoColor,"��ɫ",4);    //   2
else if(par==3)
	memcpy(Menu_VecLogoColor,"��ɫ",4);     //   3
else if(par==4)
	memcpy(Menu_VecLogoColor,"��ɫ",4);    //   4
else if(par==5)
   {	memcpy(Menu_VecLogoColor,"����",4);  par=9; } //   9
   
Menu_color_num=par; 

memcpy(car_col+9,Menu_VecLogoColor,4);  
lcd_fill(0);
lcd_text12(20,10,(char *)car_col,13,LCD_MODE_SET);
lcd_update_all();
}
static void msg( void *p)
{

}
static void show(void)
{
CounterBack=0;
col_screen=1;
car_col_fun(1);
}


static void keypress(unsigned int key)
{
	switch(KeyValue)
		{
		case KeyValueMenu:
			if(comfirmation_flag==4)
				{
				pMenuItem=&Menu_1_Idle;
				pMenuItem->show();
				}
			else
				{
				pMenuItem=&Menu_0_loggingin;
				pMenuItem->show();
				}
			col_screen=0;
			CarBrandCol_Cou=1;
			comfirmation_flag=0;
			break;
		case KeyValueOk:
            if(col_screen==1)
				{
				col_screen=2;
				CarSet_0_counter=1;//
				menu_color_flag=1;//������ɫ�������
                lcd_fill(0);
				lcd_text12(20,3,(char *)car_col,13,LCD_MODE_SET);
				lcd_text12(12,18,"��ȷ�ϼ��鿴��Ϣ",16,LCD_MODE_SET);
				lcd_update_all();
				}
			else if(col_screen==2)
				{
				menu_color_flag=0;
				
				col_screen=3;
				comfirmation_flag=1;//����������Ϣ��־
				lcd_fill(0);
				lcd_text12(0,0,Menu_Car_license,8,LCD_MODE_SET);
				lcd_text12(54,0,Menu_VechileType,6,LCD_MODE_SET);
				lcd_text12(96,0,(char *)Menu_VecLogoColor,4,LCD_MODE_SET);
				
				lcd_text12(0,11,"VIN",3,LCD_MODE_SET);
				lcd_text12(19,11,(char *)Menu_Vin_Code,17,LCD_MODE_SET);
				lcd_text12(24,22,"ȷ��",4,LCD_MODE_INVERT);
				lcd_text12(72,22,"ȡ��",4,LCD_MODE_SET);
				lcd_update_all();
				}
			else if(comfirmation_flag==1)
				{
				col_screen=0;
				comfirmation_flag=4;
				//�������õ���Ϣ
				lcd_fill(0);
				lcd_text12(18,3,"������������Ϣ",14,LCD_MODE_SET);
				lcd_text12(0,18,"���˵��������������",20,LCD_MODE_SET);
				lcd_update_all();

                //���ƺ�
				memset(JT808Conf_struct.Vechicle_Info.Vech_Num,0,sizeof(JT808Conf_struct.Vechicle_Info.Vech_Num));
				memcpy(JT808Conf_struct.Vechicle_Info.Vech_Num,Menu_Car_license,strlen(Menu_Car_license));
				//��������
				memset(JT808Conf_struct.Vechicle_Info.Vech_Type,0,sizeof(JT808Conf_struct.Vechicle_Info.Vech_Type));
				memcpy(JT808Conf_struct.Vechicle_Info.Vech_Type,Menu_VechileType,strlen(Menu_VechileType));
                //����VIN
                memset(JT808Conf_struct.Vechicle_Info.Vech_VIN,0,sizeof(JT808Conf_struct.Vechicle_Info.Vech_VIN));
                memcpy(JT808Conf_struct.Vechicle_Info.Vech_VIN,Menu_Vin_Code,17);          

				// ������ɫ
				JT808Conf_struct.Vechicle_Info.Dev_Color=Menu_color_num;
				//�����������
				JT808Conf_struct.password_flag=1; 
				//  �洢
				Api_Config_Recwrite_Large(jt808,0,(u8*)&JT808Conf_struct,sizeof(JT808Conf_struct));
				}
			else if(comfirmation_flag==2)
				{
				col_screen=0;
				comfirmation_flag=3;
				lcd_fill(0);
				lcd_text12(6, 3,"��ȷ���Ƿ���������",18,LCD_MODE_SET);
				lcd_text12(12,18,"��ȷ�ϼ���������",16,LCD_MODE_SET);
				lcd_update_all();
				}
			else if(comfirmation_flag==3)
				{
				col_screen=0;
				comfirmation_flag=0;
				//��������
				pMenuItem=&Menu_0_loggingin;
				pMenuItem->show();
				
				comfirmation_flag=0;
				col_screen=0;
				CarBrandCol_Cou=1;
				}

			break;
		case KeyValueUP:
			if(col_screen==1)
				{
				CarBrandCol_Cou--;
				if(CarBrandCol_Cou<1)
					CarBrandCol_Cou=5;
				car_col_fun(CarBrandCol_Cou);
				}
			else if(col_screen==3)
				{
				comfirmation_flag=1;
				lcd_fill(0);
			    	lcd_text12(0,0,Menu_Car_license,8,LCD_MODE_SET);
				lcd_text12(54,0,Menu_VechileType,6,LCD_MODE_SET);
				lcd_text12(96,0,(char *)Menu_VecLogoColor,4,LCD_MODE_SET);
				
				lcd_text12(0,11,"VIN",3,LCD_MODE_SET);
				lcd_text12(19,11,(char *)Menu_Vin_Code,17,LCD_MODE_SET);
				lcd_text12(24,22,"ȷ��",4,LCD_MODE_INVERT);
				lcd_text12(72,22,"ȡ��",4,LCD_MODE_SET);
				lcd_update_all();
				}

			break;
		case KeyValueDown:
			if(col_screen==1)
				{
				CarBrandCol_Cou++;
				if(CarBrandCol_Cou>5)
					CarBrandCol_Cou=1;
				car_col_fun(CarBrandCol_Cou);
				}
			else if(col_screen==3)
				{
				comfirmation_flag=2;
				lcd_fill(0);
				lcd_text12(0,0,Menu_Car_license,8,LCD_MODE_SET);
				lcd_text12(54,0,Menu_VechileType,6,LCD_MODE_SET);
				lcd_text12(96,0,(char *)Menu_VecLogoColor,4,LCD_MODE_SET);
				
				lcd_text12(0,11,"VIN",3,LCD_MODE_SET);
				lcd_text12(19,11,(char *)Menu_Vin_Code,17,LCD_MODE_SET);
				lcd_text12(24,22,"ȷ��",4,LCD_MODE_SET);
				lcd_text12(72,22,"ȡ��",4,LCD_MODE_INVERT);
				lcd_update_all();
				}

			break;
		}
	KeyValue=0;
}


static void timetick(unsigned int systick)
{
/*
	CounterBack++;
	if(CounterBack!=MaxBankIdleTime*5)
		return;
	CounterBack=0;
	pMenuItem=&Menu_0_loggingin;
	pMenuItem->show();


	col_screen=0;
	CarBrandCol_Cou=1;
	comfirmation_flag=0;*/
}


MENUITEM	Menu_0_4_Colour=
{
"������ɫ����",
	12,
	&show,
	&keypress,
	&timetick,
	&msg,
	(void*)0
};


