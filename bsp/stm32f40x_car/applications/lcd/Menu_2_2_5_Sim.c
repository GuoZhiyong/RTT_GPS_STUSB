#include  <string.h>
#include "Menu_Include.h"

#define  Sim_width1  6


u8 Sim_Code[12];
u8 Sim_SetFlag=1,Sim_SetCounter=0;


unsigned char select_Sim[]={0x0C,0x06,0xFF,0x06,0x0C};

DECL_BMP(8,5,select_Sim);


void Sim_Set(u8 par,u8 type1_2)
{
	lcd_fill(0);
	lcd_text12(0,3,(char *)Sim_Code,Sim_SetFlag-1,LCD_MODE_SET);
	
	if(type1_2==1)
		{
		lcd_bitmap(par*Sim_width1, 12, &BMP_select_Sim, LCD_MODE_SET);
		lcd_text12(0,19,"0123456789",10,LCD_MODE_SET);
		}
	lcd_update_all();
}


static void show(void)
{
Sim_Set(Sim_SetCounter*Sim_width1,1);
}


static void keypress(unsigned int key)
{
	switch(KeyValue)
		{
		case KeyValueMenu:
			pMenuItem=&Menu_0_loggingin;
			pMenuItem->show();
			memset(Sim_Code,0,sizeof(Sim_Code));
			Sim_SetFlag=1;
			Sim_SetCounter=0;
			break;
		case KeyValueOk:
			if((Sim_SetFlag>=1)&&(Sim_SetFlag<=12))
				{
				if(Sim_SetCounter<=9)
					Sim_Code[Sim_SetFlag-1]=Sim_SetCounter+'0';
				
				Sim_SetFlag++;	
				Sim_SetCounter=0;
				Sim_Set(0,1);
				}		
			if(Sim_SetFlag==13)
				{
				lcd_fill(0);
				Sim_SetFlag=14;
				lcd_text12(0,5,(char *)Sim_Code,12,LCD_MODE_SET);
				lcd_text12(25,19,"ID设置完成",10,LCD_MODE_SET);
				lcd_update_all();		
				//写入车辆 ID 号码
				/*printf("\r\n 输入手机号: %s \r\n",Sim_Code);
				memcpy(Vechicle_Info.DevicePhone,Sim_Code,12);   
				Vechicle_Info.DevoceEffectFlag=1;
				Vehicleinfo_Write();
				SimPhone_ini();  	// 读取初始化    
                */
				if(Set0_Comp_Flag==1)
					Set0_Comp_Flag=2;
				}
			else if(Sim_SetFlag==14)
				{
				Sim_SetFlag=1;
				pMenuItem=&Menu_0_loggingin;
				pMenuItem->show();
				memset(Sim_Code,0,sizeof(Sim_Code));
				}
			
			break;
		case KeyValueUP:
			if((Sim_SetFlag>=1)&&(Sim_SetFlag<=12))
				{
				if(Sim_SetCounter==0)
					Sim_SetCounter=9;
				else if(Sim_SetCounter>=1)
					Sim_SetCounter--;
				Sim_Set(Sim_SetCounter,1);
				}
			break;
		case KeyValueDown:
			if((Sim_SetFlag>=1)&&(Sim_SetFlag<=12))
				{
				Sim_SetCounter++;
				if(Sim_SetCounter>9)
					Sim_SetCounter=0;
				Sim_Set(Sim_SetCounter,1);	
				}
			break;
		}
	KeyValue=0;
}


static void timetick(unsigned int systick)
{
}


MENUITEM	Menu_2_2_5_CarSim=
{
"SIM卡卡号设置",
	13,
	&show,
	&keypress,
	&timetick,
	(void*)0
};


