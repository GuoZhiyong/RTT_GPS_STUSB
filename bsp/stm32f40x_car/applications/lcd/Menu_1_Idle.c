#include  <stdlib.h>//êy×?×a??3é×?·?′?
#include  <stdio.h>
#include  <string.h>

#include "menu_include.h"

unsigned char dispstat=0;
unsigned char tickcount=0;
unsigned char RetrySet_flag=0;

unsigned char gsm_g[]={
0x1c,					/*[   ***  ]*/
0x22,					/*[  *   * ]*/
0x40,					/*[ *      ]*/
0x40,					/*[ *      ]*/
0x4e,					/*[ *  *** ]*/
0x42,					/*[ *    * ]*/
0x22,					/*[  *   * ]*/
0x1e,					/*[   **** ]*/
};

unsigned char gsm_0[]={
0x00,					/*[        ]*/	
0x00,					/*[        ]*/
0x00,					/*[        ]*/
0x00,					/*[        ]*/
0x00,					/*[        ]*/
0x00,					/*[        ]*/
0x80,					/*[*       ]*/
0x80,					/*[*       ]*/
};

unsigned char gsm_1[]={
0x00,					/*[        ]*/	
0x00,					/*[        ]*/
0x00,					/*[        ]*/
0x00,					/*[        ]*/
0x20,					/*[  *     ]*/
0x20,					/*[  *     ]*/
0xa0,					/*[* *     ]*/
0xa0,					/*[* *     ]*/
};

unsigned char gsm_2[]={
0x00,					/*[        ]*/	
0x00,					/*[        ]*/
0x08,					/*[    *   ]*/
0x08,					/*[    *   ]*/
0x28,					/*[  * *   ]*/
0x28,					/*[  * *   ]*/
0xa8,					/*[* * *   ]*/
0xa8,					/*[* * *   ]*/
};


unsigned char gsm_3[]={
0x02,					/*[      * ]*/	
0x02,					/*[      * ]*/
0x0a,					/*[    * * ]*/
0x0a,					/*[    * * ]*/
0x2a,					/*[  * * * ]*/
0x2a,					/*[  * * * ]*/
0xaa,					/*[* * * * ]*/
0xaa,					/*[* * * * ]*/
};

unsigned char link_on[]={
0x08,					/*[    *   ]*/	
0x04,					/*[     *  ]*/
0xfe,					/*[******* ]*/
0x00,					/*[        ]*/
0xfe,					/*[******* ]*/
0x40,					/*[ *      ]*/
0x20,					/*[  *     ]*/
0x00,					/*[        ]*/
};

unsigned char link_off[]={
0x10,					/*[   *    ]*/	
0x08,					/*[    *   ]*/
0xc6,					/*[**   ** ]*/
0x00,					/*[        ]*/
0xe6,					/*[***  ** ]*/
0x10,					/*[   *    ]*/
0x08,					/*[    *   ]*/
0x00,					/*[        ]*/
};
static unsigned char Battery[]={0x00,0xFC,0xFF,0xFF,0xFC,0x00};//8*6
static unsigned char NOBattery[]={0x04,0x0C,0x98,0xB0,0xE0,0xF8};//6*6
static unsigned char TriangleS[]={0x30,0x78,0xFC,0xFC,0x78,0x30};//6*6
static unsigned char TriangleK[]={0x30,0x48,0x84,0x84,0x48,0x30};//6*6


static unsigned char empty[]={0x84,0x84,0x84,0x84,0x84,0xFC}; /*空车*/
static unsigned char full_0[]={0x84,0x84,0x84,0xFC,0xFC,0xFC};/*半满*/
static unsigned char full_1[]={0xFC,0xFC,0xFC,0xFC,0xFC,0xFC};/*重车*/


//电池 是否校验特征系数的标志
DECL_BMP(8,6,Battery);  DECL_BMP(6,6,NOBattery);
DECL_BMP(6,6,TriangleS);DECL_BMP(6,6,TriangleK);
//信号强度标志
DECL_BMP(7,8,gsm_g);    DECL_BMP(7,8,gsm_0);
DECL_BMP(7,8,gsm_1);    DECL_BMP(7,8,gsm_2);
DECL_BMP(7,8,gsm_3);
//连接或者在线标志
DECL_BMP(7,8,link_on);  DECL_BMP(7,8,link_off);
//空车 半满 重车
DECL_BMP(6,6,empty);    DECL_BMP(6,6,full_0);  DECL_BMP(6,6,full_1);


void GPSGPRS_Status(void)
{
if(gps_bd_modeFLAG==1)
	lcd_text12(19,0,"BD",2,LCD_MODE_SET);
else if(gps_bd_modeFLAG==2)
	lcd_text12(19,0,"GPS",3,LCD_MODE_SET);
else if(gps_bd_modeFLAG==3)
	lcd_text12(19,0,"G/B",3,LCD_MODE_SET);
if(GPS_Flag==0)
	lcd_bitmap(37,2,&BMP_link_off, LCD_MODE_SET);
else
	lcd_bitmap(37,2,&BMP_link_on, LCD_MODE_SET);

lcd_text12(48,0,"GPRS",4,LCD_MODE_SET);
if(Gprs_Online_Flag==0)	
	lcd_bitmap(72,2,&BMP_link_off, LCD_MODE_SET);
else
	lcd_bitmap(72,2,&BMP_link_on, LCD_MODE_SET);

//车辆载重标志
if(CarLoadState_Flag==1)
	lcd_bitmap(95,2,&BMP_empty, LCD_MODE_SET);
else if(CarLoadState_Flag==2)
	lcd_bitmap(95,2,&BMP_full_0, LCD_MODE_SET);
else if(CarLoadState_Flag==3)
	lcd_bitmap(95,2,&BMP_full_1, LCD_MODE_SET);

//电源标志
if(battery_flag==1)
	lcd_bitmap(105,2,&BMP_Battery, LCD_MODE_SET);
else
	lcd_bitmap(105,2,&BMP_NOBattery, LCD_MODE_SET);

//是否校验特征系数
if(tz_flag==1)
	lcd_bitmap(115,2,&BMP_TriangleS, LCD_MODE_SET);
else
	lcd_bitmap(115,2,&BMP_TriangleK, LCD_MODE_SET);
}
void  Disp_Idle(void)
{
    lcd_fill(0);	
	lcd_text12(0, 10,(char *)Dis_date,20,LCD_MODE_SET);
	lcd_text12(0,20,(char *)Dis_speDer,18,LCD_MODE_SET);
	lcd_bitmap(0,3,&BMP_gsm_g, LCD_MODE_SET);
	lcd_bitmap(8,3,&BMP_gsm_3, LCD_MODE_SET);
	GPSGPRS_Status();
	
	lcd_update_all();
}
static void show(void)
{
	Disp_Idle();
}


static void keypress(unsigned int key)
{
	switch(KeyValue)
		{
		case KeyValueMenu:
			    SetVIN_NUM=1;
				OK_Counter=0;
				
				CounterBack=0;
				UpAndDown=1;//
				
				RetrySet_flag=0;//采集特征系数仅在待机界面有效

				pMenuItem=&Menu_1_InforCheck;
				pMenuItem->show();

			break;
		case KeyValueOk:
			RetrySet_flag=1;
							
			break;
		case KeyValueUP:
			if(RetrySet_flag==1)
				RetrySet_flag=2;

			break;
		case KeyValueDown:

			if(RetrySet_flag==2)
				{
				RetrySet_flag=0;
				pMenuItem=&Menu_0_loggingin;//&Menu_SetTZXS;  // 以前是自检---现在是设置界面
				pMenuItem->show();
				// ----清除鉴权码 --需要重新注册重新鉴权
				/*memset(Reg_buf,0,sizeof(Reg_buf));
				DevRegisterFlag=0;
				Reg_buf[20]=DevRegisterFlag;	  						
				DF_WriteFlashSector(DF_DevConfirmCode_Page,0,(u8*)Reg_buf,21);	
				Vechicle_Info.DevoceEffectFlag=0;
				AT_Stage(AT_Idle);
				start_dial_stateFLAG=0;
				dialing_counter=0;*/   	 
				}

			break;
		}
	KeyValue=0;	
}

static void timetick(unsigned int systick)  
{
	  u8 Reg_buf[22];   
//Cent_To_Disp();

		
//主电源掉电
	if(Warn_Status[1]&0x01)  
		{
		BuzzerFlag=11;
		lcd_fill(0);
		lcd_text12(30,10,"主电源掉电",10,LCD_MODE_SET); 
		lcd_update_all();
		}

//循环显示待机界面
tickcount++;
if(tickcount>=25) 
	{
	tickcount=0;
    Disp_Idle();
	}
}


MENUITEM	Menu_1_Idle=
{
    "待机界面",
	8,
	&show,
	&keypress,
	&timetick,
	(void*)0
};

