#include  <string.h>
#include "Menu_Include.h"




u8 col_screen=0;
u8 CarBrandCol_Cou=1;

unsigned char car_col[13]={"车牌颜色:蓝色"};

void car_col_fun(u8 par)
{                                                      
                                           //车牌颜色编码表
if(par==1)
	memcpy(car_col+9,"蓝色",4);     //   1
else if(par==2)
	memcpy(car_col+9,"黄色",4);    //   2
else if(par==3)
	memcpy(car_col+9,"黑色",4);     //   3
else if(par==4)
	memcpy(car_col+9,"白色",4);    //   4
else if(par==5)
	memcpy(car_col+9,"其他",4);    //   9

lcd_fill(0);
lcd_text12(20,10,car_col,13,LCD_MODE_SET);
lcd_update_all();
}

static void show(void)
{
car_col_fun(1);
}


static void keypress(unsigned int key)
{
	switch(KeyValue)
		{
		case KeyValueMenu:
			pMenuItem=&Menu_1_Idle;
			pMenuItem->show();
                 	col_screen=0;
			CarBrandCol_Cou=1;
			break;
		case KeyValueOk:
			if(col_screen==0)
				{
				col_screen=1;
				lcd_fill(0);
				lcd_text12(20,10,car_col,13,LCD_MODE_SET);
				lcd_update_all();
				}
			else if(col_screen==1)
				{
				col_screen=0;
				pMenuItem=&Menu_0_loggingin;
			    pMenuItem->show();
				   
				col_screen=0;
				CarBrandCol_Cou=1;
				}
			
			if(Set0_Comp_Flag==2)
				Set0_Comp_Flag=3;       
			break;
		case KeyValueUP:
			/*if(col_screen==0)
				{
				CarBrandCol_Cou--;
				if(CarBrandCol_Cou<1)
					CarBrandCol_Cou=5;
				car_col_fun(CarBrandCol_Cou);
				}*/

			break;
		case KeyValueDown:
			/*if(col_screen==0)
				{
				CarBrandCol_Cou++;
				if(CarBrandCol_Cou>5)
					CarBrandCol_Cou=1;
				car_col_fun(CarBrandCol_Cou);
				}*/

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

	col_screen=0;
	CarBrandCol_Cou=1;
}


MENUITEM	Menu_2_2_6_Carcol=
{
"车辆颜色设置",
	12,
	&show,
	&keypress,
	&timetick,
	(void*)0
};


