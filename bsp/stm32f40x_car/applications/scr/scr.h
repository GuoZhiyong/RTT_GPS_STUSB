#ifndef _H_SCR_
#define _H_SCR_

#include "stm32f4xx.h"





typedef struct IMG_DEF 
 { unsigned char width;      /* Image width */
   unsigned char height;     /* Image height*/
   const unsigned char *pdata;    /* Image table start address in memory  */
 } IMG_DEF;




#define DECL_BMP(width,height,imgdata)	struct IMG_DEF BMP_##imgdata={width,height,imgdata}	


typedef void (*SHOW)(unsigned int );
typedef void (*KEYPRESS)(unsigned int);
typedef void (*TIMETICK)(unsigned int);
typedef void (*MSG)(void *p);


typedef  struct _scr{
	SHOW show;				/*��ʾʱ���ã���ʼ����ʾ*/
	KEYPRESS keypress;		/*��������ʱ����*/
	TIMETICK timetick;		/*�����ṩϵͳtick�����緵�ش�������*/
	MSG msg;				/*�����ṩ�ص�����*/
	struct _scr *parent;	/*���õĸ�����,�����ӡ��������Ϣ��*/   
}SCR; 

typedef struct _scr * PSCR; 


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




/*���峣�õ���Դ arrow_up arrow_dn ���ֵ�*/





#endif
