#ifndef _H_SCR_
#define _H_SCR_

#include "stm32f4xx.h"
#include "sed1520.h"


typedef void (*SHOW)(void *parent);  /*�ϲ�˵�������,�޳�ʼ������(��֪��������)*/
typedef void (*KEYPRESS)(unsigned int);
typedef void (*TIMETICK)(unsigned int);
typedef void (*MSG)(void *pmsg);


#define LCD_MSG_ID_GPS	0x0001
#define LCD_MSG_ID_GSM	0x0002
#define LCD_MSG_ID_GPRS	0x0003
#define LCD_MSG_ID_MEMS	0x0004
#define LCD_MSG_ID_RTC	0x0005
#define LCD_MSG_ID_CAM	0x0006
#define LCD_MSG_ID_ICCARD	0x0007






typedef struct _lcd_msg
{
	uint32_t id;
	union
	{
		uint8_t payload[64];
		struct 
		{
			uint8_t gps_av;
			uint8_t year;
			uint8_t month;
			uint8_t day;
			uint8_t hour;
			uint8_t minitue;
			uint8_t sec;
		}gps_rmc;
		struct
		{
			uint8_t call;
		}gsm_call;
	}info;	
}LCD_MSG;



typedef  struct _scr{
	SHOW show;				/*��ʾʱ���ã���ʼ����ʾ*/
	KEYPRESS keypress;		/*��������ʱ����*/
	TIMETICK timetick;		/*�����ṩϵͳtick�����緵�ش�������*/
	MSG msg;				/*�����ṩ�ص�����*/
	struct _scr *parent;	/*���õĸ�����,�����ӡ��������Ϣ��*/   
}SCR; 

typedef struct _scr * PSCR; 



typedef struct 
{
	char *text;
	uint8_t len;
	PSCR scr;
}SCR_ITEM;


typedef struct _aux_io
{
	GPIO_TypeDef* port;
	uint16_t	pin;
	uint8_t		left;
	uint8_t		top;
}AUX_IO;


/*
���尴��״̬
*/

#define KEY_NONE		0x00

#define KEY_MENU_PRESS		0x08
#define KEY_OK_PRESS		0x04
#define KEY_UP_PRESS		0x02
#define KEY_DOWN_PRESS		0x01

#define KEY_MENU_REPEAT		0x80
#define KEY_OK_REPEAT		0x40
#define KEY_UP_REPEAT		0x20
#define KEY_DOWN_REPEAT		0x10





#define LCD_MODE_CLEAR     0
#define LCD_MODE_SET       1
#define LCD_MODE_XOR       2
#define LCD_MODE_INVERT		3




/*�õ���scr*/
extern PSCR pscr;
extern SCR scr_1_idle;
extern SCR scr_2_call;

extern uint32_t mems_alarm_tick;



#endif
