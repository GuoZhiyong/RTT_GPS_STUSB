#include "Menu_Include.h"

u8 BD_updata_screen=0;
u16 Updata_Counter=0,Updata_Flag=0,Updata_page=0;
static void show(void)
{
	lcd_fill(0);
	lcd_text12(24,3,"北斗模块升级",12,LCD_MODE_SET);
	lcd_text12(30,19,"请按确认键",10,LCD_MODE_SET);
	lcd_update_all();
}


static void keypress(unsigned int key)
{

	switch(KeyValue)
		{
		case KeyValueMenu:
			pMenuItem=&Menu_2_6_3_concuss;
			pMenuItem->show();
			CounterBack=0;

			BD_updata_screen=0;
			break;
		case KeyValueOk:
			if(BD_updata_screen==0)
				{
				BD_updata_screen=1;
				lcd_fill(0);
				if(bd_file_exist==1)
					{
					lcd_text12(6,3,"北斗模块升级进行中",18,LCD_MODE_SET);
					lcd_text12(30,19,"请耐心等候",10,LCD_MODE_SET);
					BD_updata_flag=1;
					Updata_Flag=1;
					}
				else
					{
					lcd_text12(12,10,"没有要升级的文件",16,LCD_MODE_SET);
					BD_updata_flag=0;
					}
				lcd_update_all();	
				}
			break;
		case KeyValueUP:
			break;
		case KeyValueDown:
			break;
		}
 KeyValue=0;
}


static void timetick(unsigned int systick)
{
if(Updata_Flag==1)
{
Updata_Counter++;//60ms
if(Updata_Counter<150)
       return;
else if(Updata_Counter==150)
      {
       Updata_page=1;
      }
else if(Updata_Counter%10==0)
      {
      if(Updata_page<FilePageBD_Sum)
	      	{
		Updata_page++;
		UpdataDisp[0]=Updata_page/100+'0';
		UpdataDisp[1]=Updata_page%100/10+'0';
		UpdataDisp[2]=Updata_page%10+'0';

		lcd_fill(0);
	    lcd_text12(18,3,"北斗模块升级中",14,LCD_MODE_SET);
		lcd_text12(30,20,(char*)UpdataDisp,sizeof(UpdataDisp),LCD_MODE_SET);
		lcd_update_all();
	      	}
	else
	  	{
	  	//GPIO_ResetBits(GPIOD,PIN_GPSPOWER); 
	  	lcd_fill(0);
	    lcd_text12(24,3,"模块升级完成",12,LCD_MODE_SET);
		lcd_update_all();
		pMenuItem=&Menu_2_6_4_version;
	       pMenuItem->show();

		Updata_Counter=0;
		Updata_Flag=0;
		Updata_page=0;
		//GPIO_SetBits(GPIOD,PIN_GPSPOWER); 
		
		BD_updata_screen=0;
	       BD_updata_flag=0;//升級完成
	  	}
       }
}

	CounterBack++;
	if(CounterBack!=MaxBankIdleTime*10)
		return;
	CounterBack=0;
	pMenuItem=&Menu_1_Idle;
	pMenuItem->show();


	
}


MENUITEM	Menu_2_6_1_BDupdata=
{
"北斗升级",
	8,
	&show,
	&keypress,
	&timetick,
	(void*)0
};

