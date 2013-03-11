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
			lcd_text12(0,0,"��γ��",6,LCD_MODE_INVERT);
			lcd_asc0608(121-6*12,12,"E:----------",12,LCD_MODE_INVERT);
			lcd_asc0608(121-6*12,20,"N:----------",12,LCD_MODE_INVERT);
			lcd_bitmap(121-5,28,&BMP_res_arrow_dn,LCD_MODE_SET);
			break;
		case 1:/*һ����ʾ [��λ �ٶ� ���� �߶�]   8*2+3=19 19*6=114,ע���Ű�����*/
			lcd_text12(1,0,"��λ",4,LCD_MODE_INVERT); /*��һ��*/
			lcd_text12(33,0,"�ٶ�",4,LCD_MODE_SET);/*��8��*/
			lcd_text12(65,0,"����",4,LCD_MODE_SET);
			lcd_text12(97,0,"�߶�",4,LCD_MODE_SET);
			lcd_asc0608(25-6*2,0,"--",2,LCD_MODE_SET); /*��һ��*/
			lcd_asc0608(57-6*2,0,"--",2,LCD_MODE_SET);/*��8��*/
			lcd_asc0608(89-6*2,0,"--",2,LCD_MODE_SET);
			lcd_asc0608(121-6*2,0,"--",2,LCD_MODE_SET);
			lcd_bitmap(121-5*2,28,&BMP_res_arrow_dn,LCD_MODE_SET);
			lcd_bitmap(121-5,28,&BMP_res_arrow_up,LCD_MODE_SET);
			break;
		case 2:
			lcd_text12(0,0,"��λԴ",6,LCD_MODE_INVERT);
			lcd_text12((121-6*13)/2,16,"BD2 B1/GPS L1",13,LCD_MODE_SET);
			lcd_bitmap(121-5*2,28,&BMP_res_arrow_dn,LCD_MODE_SET);
			lcd_bitmap(121-5,28,&BMP_res_arrow_up,LCD_MODE_SET);
		case 3:
			lcd_text12(0,0,"ʱ��",4,LCD_MODE_INVERT);
			lcd_asc0608(100-6*10,10,"2013-03-11",10,LCD_MODE_SET);
			lcd_asc0608(100-6*8,18,"09:07:14",8,LCD_MODE_SET);
			lcd_bitmap(121-5,28,&BMP_res_arrow_up,LCD_MODE_SET);

			break;
	}		
	lcd_update_all();
}

static void show(void* parent)
{
	scr_3_geoinfo.parent=(PSCR)parent;
	showpage();

}


/*��������*/
static void keypress(unsigned int key)
{
	switch(key)
	{
		case KEY_MENU_PRESS:   /*�����ϼ��˵�*/
			if((void*)0==scr_3_geoinfo.parent) break;
			pscr=scr_3_geoinfo.parent;
			pscr->show(0);
			break;
		case KEY_OK_PRESS:				
			break;
		case KEY_UP_PRESS:
			if(pageindex==0) pageindex=4;
			pageindex--;
			showpage();
			break;	
		case KEY_DOWN_PRESS:
			pageindex++;
			pageindex%=4;
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



SCR scr_3_geoinfo=
{
	&show,
	&keypress,
	&timetick,
	(void*)0
};



