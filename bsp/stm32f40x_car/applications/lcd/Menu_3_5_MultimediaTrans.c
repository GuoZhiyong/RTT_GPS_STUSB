#include "Menu_Include.h"
#include "sed1520.h"

unsigned char CarMulTrans_screen=0;


static void msg( void *p)
{
}
static void show(void)
{
	pMenuItem->tick=rt_tick_get();

	lcd_fill(0);
	lcd_text12(20,3,"��ý���¼��ϴ�",14,LCD_MODE_SET);
	lcd_update_all();
}


static void keypress(unsigned int key)
{
switch(key)
	{
	case KEY_MENU:
		pMenuItem=&Menu_3_InforInteract;
		pMenuItem->show();
		CounterBack=0;

		CarMulTrans_screen=0;
		break;
	case KEY_OK:
		if(CarMulTrans_screen==0)
			{
			CarMulTrans_screen=1;
			/*memset(send_data,0,sizeof(send_data));
			send_data[0]=0x08;
			send_data[1]=0x00;
			send_data[2]=0x00;
			send_data[3]=0x00;
			rt_mb_send(&mb_hmi, (rt_uint32_t)&send_data[0]);*/
			
			lcd_fill(0);
			lcd_text12(7,10,"��ý���¼��ϴ��ɹ�",18,LCD_MODE_INVERT);
			lcd_update_all();
            //���ϴ���ý����Ϣ�ı�־
			rt_kprintf("\r\n -----  ��ý���¼��ϴ��ɹ� \r\n");  
				#if NEED_TODO
			MediaObj.Media_Type=0; //ͼ��
		    MediaObj.Media_totalPacketNum=1;  // ͼƬ�ܰ���
		    MediaObj.Media_currentPacketNum=1;	// ͼƬ��ǰ����
		    MediaObj.Media_ID=1;   //  ��ý��ID
		    MediaObj.Media_Channel=Camera_Number;  // ͼƬ����ͷͨ����
		    MediaObj.SD_media_Flag=1; //��ý���¼���Ϣ�ϴ���־
		    Duomeiti_sdFlag=1; 
				#endif
			}
		break;
	case KEY_UP:
		break;
	case KEY_DOWN:
		break;
	}
}

	



MENUITEM	Menu_3_5_MultimediaTrans=
{
"��ý���¼��ϴ�",
	14,0,
	&show,
	&keypress,
	&timetick_default,
	&msg,
	(void*)0
};

