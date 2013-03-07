#ifndef _H_SCR_
#define _H_SCR_

#include "stm32f4xx.h"





typedef struct IMG_DEF 
 { unsigned char width;      /* Image width */
   unsigned char height;     /* Image height*/
   const unsigned char *pdata;    /* Image table start address in memory  */
 } IMG_DEF;




#define DECL_BMP(width,height,imgdata)	struct IMG_DEF BMP_##imgdata={width,height,imgdata}	


typedef void (*SHOW)(void);  /*上层菜单调用者,无初始化数据(不知道调用者)*/
typedef void (*KEYPRESS)(void *thiz,unsigned int);
typedef void (*TIMETICK)(void *thiz,unsigned int);
typedef void (*MSG)(void *thiz,void *p);


typedef  struct _scr{
	SHOW show;				/*显示时调用，初始化显示*/
	KEYPRESS keypress;		/*发生按键时调用*/
	TIMETICK timetick;		/*向其提供系统tick，比如返回待机画面*/
	MSG msg;				/*对外提供回调函数*/
	struct _scr *parent;	/*调用的父界面,比如打印，弹出信息等*/   
}SCR; 

typedef struct _scr * PSCR; 


/*
定义按键状态
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


/*定义常用的资源 arrow_up arrow_dn 数字等*/




#endif
