/************************************************************
 * Copyright (C), 2008-2012,
 * FileName:		// �ļ���
 * Author:			// ����
 * Date:			// ����
 * Description:		// ģ������
 * Version:			// �汾��Ϣ
 * Function List:	// ��Ҫ�������书��
 *     1. -------
 * History:			// ��ʷ�޸ļ�¼
 *     <author>  <time>   <version >   <desc>
 *     David    96/10/12     1.0     build this moudle
 ***********************************************************/
#include "Menu_Include.h"
#include "sed1520.h"


/*��ʾ��Ϣ*/
static void msg( void *p )
{
}

/**/
static void show( void )
{
}

/**/
static void keypress( unsigned int key )
{
	switch( key )
	{
		case KEY_MENU:
			break;
		case KEY_OK:
			break;
		case KEY_UP:
			break;
		case KEY_DOWN:
			break;
	}
}

/**/
static void timetick( unsigned int systick )
{
}

MENUITEM Menu_3_6_Record =
{
	"��Ϣ",
	4,
	&show,
	&keypress,
	&timetick,
	&msg,
	(void*)0
};

/************************************** The End Of File **************************************/
