#include "menu.h"
#include "fonts.h"
#include "SED1520.h"

#include "Lcd_init.h"
#include "stm32f4xx.h"

unsigned char dispstat=0;
unsigned char tickcount=0;

struct IMG_DEF test_1_Idle={12,12,test_00};
static void show(void)
{
	lcd_fill(0);
	//DisAddRead_ZK(35,10,"�г���¼��",5,&test_1_Idle,0,0);
	lcd_text12(15,10,"�г���¼��12aA",14,LCD_MODE_SET);
	lcd_update_all();
}


static void keypress(unsigned int key)
{
	switch(KeyValue)
		{
		case 1:
			pMenuItem=&Menu_1_InforCheck;
			pMenuItem->show();
			break;		
		case 2:
			lcd_fill(0);
			//DisAddRead_ZK(35,10,"ȷ�ϼ�����",5,&test_1_Idle,1,0);
			lcd_text12(35,10,"ȷ�ϼ�����",10,LCD_MODE_INVERT);
			lcd_update_all();
			break;		
		case 3:
			lcd_fill(0);
			DisAddRead_ZK(35,10,"���ϼ�����",5,&test_1_Idle,1,0);
			lcd_update_all();
			break;		
		case 4:
			lcd_fill(0);
			DisAddRead_ZK(35,10,"���¼�����",5,&test_1_Idle,1,0);
			lcd_update_all();
			break;
		default:
			break;
		}
	KeyValue=0;
}

static void timetick(unsigned int systick)  
{
//ѭ����ʾ��������
/*tickcount++;
if(tickcount>=10) 
	{
	tickcount=0;
	
	switch(dispstat)
		{
		case 0:
			
			lcd_fill(0);
			lcd_text(30,3,FONT_TEN_DOT,"LCD TEST");
			lcd_text(30,18,FONT_TEN_DOT,"welcom 1");
			lcd_update_all();
			break;
		case 1:
			lcd_fill(0);
			lcd_text(30,3,FONT_TEN_DOT,"LCD TEST");
			lcd_text(30,18,FONT_TEN_DOT,"welcom 2");			
			lcd_update_all();
			break;
		} 
	dispstat++;
	if(dispstat>=2) dispstat=0;
	}
*/
}


MENUITEM	Menu_1_Idle=
{
	"�����궨",
	&show,
	&keypress,
	&timetick,
	(void*)0
};

