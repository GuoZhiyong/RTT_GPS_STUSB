#include  <string.h>
#include "menu.h"
#include "Lcd_init.h"

#define  vin_width1  7
#define  vin_width2  8

struct IMG_DEF test_dis_VINset={12,12,test_00};

unsigned char VIN_Code[18];
unsigned char VIN_SetFlag=0,VIN_SetCounter=0;

unsigned char select_vin[]={0x0C,0x06,0xFF,0x06,0x0C};

DECL_BMP(8,5,select_vin);


void VIN_Set(unsigned char par,unsigned char type1_2)
{
	lcd_fill(0);
	lcd_text(0,3,FONT_SIX_DOT,(char *)VIN_Code);
	
	if(type1_2==1)
		{
		lcd_bitmap(par*vin_width1, 12, &BMP_select_vin, LCD_MODE_SET);
		lcd_text(0,19,FONT_SEVEN_DOT,"0123456789ABCDEFG");
		}
	else
		{
		if(par<4)
			lcd_bitmap(par*vin_width1, 12, &BMP_select_vin, LCD_MODE_SET);
		else if((par>=4)&&(par<8))
			lcd_bitmap(par*vin_width2, 12, &BMP_select_vin, LCD_MODE_SET);
		else if((par>=8)&&(par<=12))
			lcd_bitmap(par*vin_width1+4, 12, &BMP_select_vin, LCD_MODE_SET);
		else if(par>=13)
			lcd_bitmap(96+(par-13)*vin_width1+4, 12, &BMP_select_vin, LCD_MODE_SET);//13*7+5
		lcd_text(0,19,FONT_SEVEN_DOT,"H");
		lcd_text(7,19,FONT_SEVEN_DOT,"J");
		lcd_text(13,19,FONT_SEVEN_DOT,"KLMNPRSTUVWXY");
		lcd_text(115,19,FONT_SEVEN_DOT,"Z");
		}
	lcd_update_all();
}
static void show(void)
{
	VIN_Set(0,1);
	VIN_SetFlag=1;
	VIN_SetCounter=0;
}


static void keypress(unsigned int key)
{
unsigned char temp=0;
	switch(KeyValue)
		{
		case KeyValueMenu:
			pMenuItem=&Menu_1_CarSet;
			pMenuItem->show();
			CounterBack=0;

			VIN_SetFlag=0;
			VIN_SetCounter=0;
			memset(VIN_Code,0,sizeof(VIN_Code));
			break;
		case KeyValueOk:
			if((VIN_SetFlag>=1)&&(VIN_SetFlag<=17))
				{
				if(VIN_SetCounter<=9)
					VIN_Code[VIN_SetFlag-1]=VIN_SetCounter+'0';
				else
					{
					if(VIN_SetCounter<18)
						VIN_Code[VIN_SetFlag-1]=VIN_SetCounter+0x37;//-10+0x41
					else if((VIN_SetCounter>=18)&&(VIN_SetCounter<=22))
						VIN_Code[VIN_SetFlag-1]=VIN_SetCounter+0x38;
					else if(VIN_SetCounter==23)
						VIN_Code[VIN_SetFlag-1]=VIN_SetCounter+0x39;
					else if(VIN_SetCounter>=24)
						VIN_Code[VIN_SetFlag-1]=VIN_SetCounter+0x3a;
					}
				VIN_SetFlag++;
				
				VIN_SetCounter=0;
				VIN_Set(0,1);
				}		
			if(VIN_SetFlag==18)
				{
				VIN_SetFlag=19;
				lcd_fill(0);
				lcd_text(0,5,FONT_SIX_DOT,(char *)VIN_Code);
				lcd_text(25,19,FONT_NINE_DOT,"VIN");
				DisAddRead_ZK(54,19,"设置完成",4,&test_dis_VINset,1,0);
				lcd_update_all();		
				//写入车辆 VIN 号码
				//memcpy(Vechicle_Info.Vech_VIN,VIN_Code,17); 
				//Vehicleinfo_Write();
				}
			else if(VIN_SetFlag==19)
				{
				VIN_SetFlag=0;
				pMenuItem=&Menu_1_CarSet;
				pMenuItem->show();
				memset(VIN_Code,0,sizeof(VIN_Code));
				}
			/*else
				{
				VIN_Set(0);
				VIN_SetCounter=0;
				}*/

			break;
		case KeyValueUP:
			if(VIN_SetCounter==0)
				VIN_SetCounter=32;
			else
	            VIN_SetCounter--;
			if(VIN_SetCounter<=0)
				VIN_SetCounter=0;
			if(VIN_SetCounter<=16)
				VIN_Set(VIN_SetCounter,1);
			else
				{
				temp=VIN_SetCounter-17;
				VIN_Set(temp,2);
				}
			break;
		case KeyValueDown:
            VIN_SetCounter++;
			if(VIN_SetCounter>32)
				VIN_SetCounter=0;
			if(VIN_SetCounter<=16)
				VIN_Set(VIN_SetCounter,1);
			else
				{
				temp=VIN_SetCounter-17;
				VIN_Set(temp,2);
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
	
	VIN_SetFlag=0;
	VIN_SetCounter=0;
	memset(VIN_Code,0,sizeof(VIN_Code));
}


MENUITEM	Menu_2_2_4_VINset=
{
	&show,
	&keypress,
	&timetick,
	(void*)0
};


