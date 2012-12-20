#include "Menu_Include.h"




u8 clear_screen=0;

static void show(void)
{
	lcd_fill(0);
	lcd_text12(30,10,"清空鉴权码",10,LCD_MODE_SET);
	lcd_update_all();
}


static void keypress(unsigned int key)
{
        u8 Reg_buf[22];  
		

	switch(KeyValue)
		{
		case KeyValueMenu:
			pMenuItem=&Menu_2_6_7_colstart; 
			pMenuItem->show();
			CounterBack=0;

			clear_screen=0;
			break;
		case KeyValueOk:
			if(clear_screen==0)
				{
				clear_screen=1;
				//---------------------------
				/*memset(Reg_buf,0,sizeof(Reg_buf));
				memcpy(Reg_buf,DevConfirmCode,strlen((const char*)DevConfirmCode)); 
				DevRegisterFlag=0;
				Reg_buf[20]=DevRegisterFlag;	  						
				DF_WriteFlashSector(DF_DevConfirmCode_Page,0,(u8*)Reg_buf,21);								 	 
				mDelaymS(50);  
				printf("\r\n		 清除注册--鉴权码为: ");  
				printf("%s\r\n",DevConfirmCode); */   
                //---------------------------
				lcd_fill(0);
				lcd_text12(24,10,"鉴权码已清空",12,LCD_MODE_SET);
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
	Cent_To_Disp();   
	CounterBack++;
	if(CounterBack!=MaxBankIdleTime*5)
		return;
	CounterBack=0;
	pMenuItem=&Menu_1_Idle;
	pMenuItem->show();

}


MENUITEM	Menu_2_6_6_clear=
{
"清空鉴权码",
	10,
	&show,
	&keypress,
	&timetick,
	(void*)0
};

