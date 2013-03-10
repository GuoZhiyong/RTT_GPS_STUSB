#include "scr.h"




static uint8_t pageindex=0;

/*
ֻ����ʾ��Ϣ����û���Ӳ˵�
��ʾ3ҳ����Ϣ ��γ�� ��λ ʱ��
*/
static void showpage(void)
{
	lcd_fill(0);
	switch(pageindex){
		case 0:
			lcd_text12(0,0,"��������",8,LCD_MODE_INVERT);
			lcd_asc0608(6,14,"IP",2,LCD_MODE_INVERT);
			lcd_asc0608(26,14,"122.224.088.034",15,LCD_MODE_SET);

			lcd_asc0608(0,23,"Port",4,LCD_MODE_INVERT);
			lcd_asc0608(26,23,"1234,7000",9,LCD_MODE_SET);    /*�Ҷ���*/
			
			lcd_bitmap(116,28,&BMP_res_arrow_dn,LCD_MODE_SET);
			break;
		case 1:
			lcd_text12(0,0,"���÷�����",10,LCD_MODE_INVERT);
			lcd_asc0608(6,14,"IP",2,LCD_MODE_INVERT);
			lcd_asc0608(26,14,"122.224.088.034",15,LCD_MODE_SET);

			lcd_asc0608(0,23,"Port",4,LCD_MODE_INVERT);
			lcd_asc0608(26,23,"1234,7000",9,LCD_MODE_SET);    /*�Ҷ���*/
			
			lcd_bitmap(116,28,&BMP_res_arrow_dn,LCD_MODE_SET);

			break;
		case 2:
			lcd_text12(6,0,"�����",6,LCD_MODE_INVERT);
			lcd_text12(80,0,"CMNET",5,LCD_MODE_INVERT);
			lcd_text12(0,16,"�ϴ����",8,LCD_MODE_INVERT);
			lcd_text12(80,0,"10",2,LCD_MODE_SET);
			lcd_bitmap(116,28,&BMP_res_arrow_up,LCD_MODE_SET);
			break;
		case 3:
			lcd_text12(0,0,"��������",8,LCD_MODE_INVERT);
			lcd_text12(40,16,"13606549494",11,LCD_MODE_SET);
			lcd_bitmap(116,28,&BMP_res_arrow_up,LCD_MODE_SET);
			break;	
		case 4:
			lcd_text12(0,0,"��Ȩ��",6,LCD_MODE_INVERT);
			lcd_asc0608(0,13,"User",4,LCD_MODE_INVERT);
			
			lcd_asc0608(4,22,"Key",4,LCD_MODE_INVERT);
			lcd_bitmap(116,28,&BMP_res_arrow_up,LCD_MODE_SET);
			break;		
		case 5:
			lcd_text12(0,0,"PPP��Ȩ",6,LCD_MODE_INVERT);
			lcd_bitmap(116,28,&BMP_res_arrow_up,LCD_MODE_SET);
			break;			
	}		
	lcd_update_all();
}

static void show(void* parent)
{
	 scr_3_5_network.parent=(PSCR)parent;
	pageindex=0;
	showpage();

}


/*��������*/
static void keypress(unsigned int key)
{
	switch(key)
	{
		case KEY_MENU_PRESS:
		case KEY_OK_PRESS:				/*�����ϼ��˵�*/
			break;
		case KEY_UP_PRESS:
			pageindex--;
			if(pageindex==0) pageindex=2;
			showpage();
			break;	
		case KEY_DOWN_PRESS:
			pageindex++;
			pageindex%=3;
			showpage();
			break;
	}
}

/*ϵͳʱ��*/
static void timetick(unsigned int systick)
{

}

/*�����Լ�״̬����Ϣ*/
static void msg(void *thiz,void *p)
{


}



SCR scr_3_5_network=
{
	&show,
	&keypress,
	&timetick,
	(void*)0
};




