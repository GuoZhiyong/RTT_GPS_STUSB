#include  <string.h>
#include "menu.h"
#include "Lcd_init.h"


struct IMG_DEF test_scr_Cancel={12,12,test_00};


static void show(void)
{
	lcd_fill(0);
	DisAddRead_ZK(0,10,"是否确认清除违章记录",10,&test_scr_Cancel,0,0);
	lcd_update_all();

}


static void keypress(unsigned int key)
{

	switch(KeyValue)
		{
		case KeyValueMenu:
			pMenuItem=&Menu_1_CarSet;
			pMenuItem->show();
			break;
		case KeyValueOk:

            //清除疲劳超速驾驶历史记录
			/*TIRED_DoorValue_Init();
            TIRED_DoorValue_Write(); 
            delay_ms(5); 
			ExpSpdRec_write=0;
			ExpSpdRec_write=0;
			DF_Write_RecordAdd(ExpSpdRec_write,ExpSpdRec_read,TYPE_ExpSpdAdd);  
			delay_ms(5); 
			TiredDrv_write=0;
			TiredDrv_read=0;
			DF_Write_RecordAdd(TiredDrv_write,TiredDrv_read,TYPE_TiredDrvAdd); 
			delay_ms(5);

			SST25V_SectorErase_4KByte((8*((u32)(TiredDrvStart_offset)/8))*PageSIZE);
			delay_ms(5);
			SST25V_SectorErase_4KByte((8*((u32)(ExpSpdStart_offset)/8))*PageSIZE);
			delay_ms(5);*/

			
					
			lcd_fill(0);
			DisAddRead_ZK(18,10,"违章记录已清除",7,&test_scr_Cancel,0,0);
			lcd_update_all();
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
	CounterBack=0;
	pMenuItem=&Menu_1_Idle;
	pMenuItem->show();

	
}


MENUITEM	Menu_2_2_5_Cancel=
{
	&show,
	&keypress,
	&timetick,
	(void*)0
};


