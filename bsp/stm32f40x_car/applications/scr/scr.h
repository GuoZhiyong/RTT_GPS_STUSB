#ifndef _H_SCR_
#define _H_SCR_

#include "stm32f4xx.h"





typedef struct _IMG_DEF 
 { unsigned char width;      /* Image width */
   unsigned char height;     /* Image height*/
   const unsigned char *pdata;    /* Image table start address in memory  */
 } IMG_DEF;




#define DECL_BMP(width,height,imgdata)	IMG_DEF BMP_##imgdata={width,height,imgdata}	


typedef void (*SHOW)(void *parent);  /*�ϲ�˵�������,�޳�ʼ������(��֪��������)*/
typedef void (*KEYPRESS)(unsigned int);
typedef void (*TIMETICK)(unsigned int);
typedef void (*MSG)(void *thiz,void *p);


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




/*
���尴��״̬
*/

#define KEY_NONE		0x00

#define KEY_MENU_PRESS		0x01
#define KEY_UP_PRESS		0x02
#define KEY_DOWN_PRESS		0x04
#define KEY_OK_PRESS		0x08

#define KEY_MENU_REPEAT		0x10
#define KEY_UP_REPEAT		0x20
#define KEY_DOWN_REPEAT		0x40
#define KEY_OK_REPEAT		0x80




#define LCD_MODE_CLEAR     0
#define LCD_MODE_SET       1
#define LCD_MODE_XOR       2
#define LCD_MODE_INVERT		3


/*���峣�õ���Դ arrow_up arrow_dn ���ֵ�*/





extern const unsigned char asc_0507[][8];

extern const unsigned char asc_0608[][6];

extern IMG_DEF BMP_res_arrow_none;
extern IMG_DEF BMP_res_arrow_dn;
extern IMG_DEF BMP_res_arrow_up;
extern IMG_DEF BMP_select_set; 
extern IMG_DEF BMP_noselect_set; 



void lcd_init( void );
void lcd_fill( unsigned char pattern );
void lcd_fill_rect( int left, int top, int right, int bottom, unsigned char pattern );
void lcd_asc0608( char left, char top, char *p, char len, const char mode );
void lcd_text12( char left, char top, char *p, char len, const char mode );
void lcd_bitmap( const uint8_t left, const uint8_t top, const IMG_DEF *img_ptr, const uint8_t mode );
void lcd_update( const unsigned char top, const unsigned char bottom );
void lcd_update_all( void );

/*�õ���scr*/
extern PSCR pscr;

extern SCR scr_1_signal;
extern SCR scr_2_driver;
extern SCR scr_3_geoinfo;
extern SCR scr_4_sms;
extern SCR scr_4_1_sms_send;
extern SCR scr_4_2_sms_inbox;
extern SCR scr_4_3_sms_outbox;

extern SCR scr_5_phonecall;
extern SCR scr_6_advance;
extern SCR scr_7_mileage;
extern SCR scr_8_record;
extern SCR scr_9_tired;
extern SCR scr_a_vehicle;
extern SCR scr_b_network;
extern SCR scr_c_multimedia;













extern SCR scr_login;
extern SCR scr_idle;
extern SCR scr_main;
extern SCR scr_selfcheck;
extern SCR scr_export_usb;



#endif
