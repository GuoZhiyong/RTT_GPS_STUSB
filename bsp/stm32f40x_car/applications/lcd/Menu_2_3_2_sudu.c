#include "menu.h"
#include "Lcd_init.h"

struct IMG_DEF test_dis_15min={12,12,test_00};
unsigned char Speed_15minFlag=0;//ȡ�ٶ���ȷ���ߴ��� 0:��ȷ 1:����

static unsigned char CheckSpeedFlag=0;
static unsigned char index_speednum;
static unsigned char SpeedNumScreen=0;
static unsigned char ReadSpeedFlag=0;//�����ȡһ�Σ��ٴΰ���ȷ�ϼ�����

unsigned char Fetch_15minSpeed(unsigned char Num15)
{

}

static void drawspeed(unsigned char num)
{
	unsigned char t[32];
	
	lcd_fill(0);	
	sprintf((char*)t,"%02d  %02d:%02d %02d",num,speed_time_rec[num-1][3],speed_time_rec[num-1][4],speed_time_rec[num-1][5]);
	lcd_text(10,0,FONT_NINE_DOT,(char*)t);
	sprintf((char*)t,"%02d  %02d:%02d %02d",num+1,speed_time_rec[num][3],speed_time_rec[num][4],speed_time_rec[num][5]);
	lcd_text(10,11,FONT_NINE_DOT,(char*)t);
	sprintf((char*)t,"%02d  %02d:%02d %02d",num+2,speed_time_rec[num+1][3],speed_time_rec[num+1][4],speed_time_rec[num+1][5]);
	lcd_text(10,22,FONT_NINE_DOT,(char*)t);	
	if(index_speednum<=9)
		index_speednum+=3;
	lcd_update_all();
}

static void show(void)
{
	index_speednum=1;
	SpeedNumScreen=0;//��ʾ�ٶȵڼ��������
	ReadSpeedFlag=1;	
	lcd_fill(0);
	DisAddRead_ZK(10,3,"ǰ",1,&test_dis_15min,0,0);
	lcd_text(22,3,FONT_TEN_DOT,"15");
	DisAddRead_ZK(38,3,"���ӳ��ٲ�ѯ",6,&test_dis_15min,0,0);
    DisAddRead_ZK(24,19,"��ȷ�ϼ��鿴",6,&test_dis_15min,0,0);
	lcd_update_all();

}

static void keypress(unsigned int key)
{
	switch(KeyValue)
		{
		case KeyValueMenu:
				Speed_15minFlag=0;
				CounterBack=0;
				CheckSpeedFlag=0;
				pMenuItem=&Menu_1_InforCheck;
				pMenuItem->show();
				ReadSpeedFlag=0;
				break;
		case KeyValueOk:
			if(ReadSpeedFlag==1)
				{
				ReadSpeedFlag=0;
				Speed_15minFlag=Fetch_15minSpeed(15);//ȡ���15���ӵ��ٶ���Ϣ
				printf("\r\n�����15���ӵ��ٶ���Ϣ���= %d 0:right 1:error\r\n",Speed_15minFlag);
				drawspeed(1);//��ʾ��1��
				SpeedNumScreen=0;
				CheckSpeedFlag=1;
				}
			break;
		case KeyValueUP:
			if((SpeedNumScreen>=1) &&(CheckSpeedFlag==1))
				{ 
				SpeedNumScreen--;
				drawspeed(SpeedNumScreen*3+1);
				}
			break;
		case KeyValueDown:
			if((SpeedNumScreen<=3) &&(CheckSpeedFlag==1))
				{ 
				SpeedNumScreen++;							    
				drawspeed(SpeedNumScreen*3+1);
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
	pMenuItem=&Menu_1_Idle;
	pMenuItem->show();
    CounterBack=0;

	CheckSpeedFlag=0;
	Speed_15minFlag=0;
	ReadSpeedFlag=0;
}


MENUITEM	Menu_2_3_2_sudu=
{
	"���15�����ٶ�",
	&show,
	&keypress,
	&timetick,
	(void*)0
};	


