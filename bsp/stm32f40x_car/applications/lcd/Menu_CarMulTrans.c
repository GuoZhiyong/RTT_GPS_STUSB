#include "Menu_Include.h"
#include "jt808.h"

unsigned char CarMulTrans_screen=0;

static void show(void)
{
	lcd_fill(0);
	lcd_text12(20,3,"��ý���¼��ϴ�",14,LCD_MODE_SET);
	lcd_update_all();
}


static void keypress(unsigned int key)
{
switch(KeyValue)
	{
	case KeyValueMenu:
		pMenuItem=&Menu_Check_DnsIp;
		pMenuItem->show();
		CounterBack=0;

		CarMulTrans_screen=0;
		break;
	case KeyValueOk:
		if(CarMulTrans_screen==0)
			{
			CarMulTrans_screen=1;
			lcd_fill(0);
			lcd_text12(7,10,"��ý���¼��ϴ��ɹ�",18,LCD_MODE_INVERT);
			lcd_update_all();
            //���ϴ���ý����Ϣ�ı�־
			rt_kprintf("\r\n -----  ��ý���¼��ϴ��ɹ� \r\n");  
			MediaObj.Media_Type=0; //ͼ��
		    MediaObj.Media_totalPacketNum=1;  // ͼƬ�ܰ���
		    MediaObj.Media_currentPacketNum=1;	// ͼƬ��ǰ����
		    MediaObj.Media_ID=1;   //  ��ý��ID
		    MediaObj.Media_Channel=Camera_Number;  // ͼƬ����ͷͨ����
		    MediaObj.SD_media_Flag=1; //setflag 
		    Duomeiti_sdFlag=1; 
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

	CounterBack++;
	if(CounterBack!=MaxBankIdleTime)
		return;
	pMenuItem=&Menu_1_Idle;
	pMenuItem->show();
	CounterBack=0;					

	CarMulTrans_screen=0;

}


MENUITEM	Menu_DisMultimedia=
{
"��ý���ϴ�",
	10,
	&show,
	&keypress,
	&timetick,
	(void*)0
};

