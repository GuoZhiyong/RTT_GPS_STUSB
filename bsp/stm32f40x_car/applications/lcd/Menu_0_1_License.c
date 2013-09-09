/************************************************************
 * Copyright (C), 2008-2012,
 * FileName:		// 文件名
 * Author:			// 作者
 * Date:			// 日期
 * Description:		// 模块描述
 * Version:			// 版本信息
 * Function List:	// 主要函数及其功能
 *     1. -------
 * History:			// 历史修改记录
 *     <author>  <time>   <version >   <desc>
 *     David    96/10/12     1.0     build this moudle
 ***********************************************************/
#include  <string.h>
#include "Menu_Include.h"
#include "sed1520.h"

#if 1

/*输入位置 0 汉字 1-6 字母或数字*/
static uint8_t	input_pos;
static int8_t	pos;
/*当前的选择*/

/*车牌号码*/
static char	chepai[9];

static char		* hz = { "京津冀晋蒙辽吉黑沪苏浙皖闽赣鲁豫鄂湘粤新桂琼渝川贵云藏陕甘青宁新港澳" };
static char		* asc = { "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ" };


/**/
static uint8_t min( uint8_t a, uint8_t b )
{
	if( a > b )
	{
		return b;
	} else
	{
		return a;
	}
}

/**/
static void display( void )
{
	uint8_t page, index;
	lcd_fill( 0 );

	lcd_text12( 0, 4, "车牌号:", 7, LCD_MODE_SET );
	lcd_text12( 50, 4, chepai, strlen( chepai ), LCD_MODE_SET );
	if( input_pos == 0 ) /*输入中文*/
	{
		if( pos < 0 )
		{
			pos = (strlen( hz ) >> 1)-1;
		}else
		{
			pos %= ( strlen( hz ) >> 1 );
		}
		page	= pos / 10;
		index	= pos % 10;
		lcd_text12( 0, 20, hz + page * 20, min( 20, strlen( hz ) - page * 20 ), LCD_MODE_SET );
		lcd_text12( index * 12, 20, hz + page * 20 + index * 2, 2, LCD_MODE_INVERT );
	}else if( input_pos > 8 )
	{
		pMenuItem = &Menu_0_5_DeviceID;
		pMenuItem->show( );
		
	}else /*输入字母数字*/
	{
		if( pos < 0 )
		{
			pos = strlen( asc )-1;
		}else
		{
			pos %= strlen( asc );
		}
		page	= pos / 20;
		index	= pos % 20;
		lcd_text12( 0, 20, asc + page * 20, min( 20, strlen( asc ) - page * 20 ), LCD_MODE_SET );
		lcd_text12( index * 6, 20, asc + page * 20 + index, 1, LCD_MODE_INVERT );
	}
	lcd_update_all( );
}

/**/
static void msg( void *p )
{
}

/**/
static void show( void )
{
	input_pos	= 0;
	pos			= 0;
	memset( chepai, 0, 9 );
	display( );
}

/**/
static void keypress( unsigned int key )
{
	switch( key )
	{
		case KEY_MENU:
			pMenuItem = &Menu_0_loggingin;
			pMenuItem->show( );

			break;
		case KEY_OK:
			if( input_pos == 0 )
			{
				memcpy( chepai, hz+pos*2, 2 );
				input_pos += 2;
			}else
			{
				chepai[input_pos]=*(asc+pos);
				input_pos++;
			}
			pos=0;
			display( );
			break;
		case KEY_UP:
		case KEY_UP_REPEAT:
			pos--;
			display( );
			break;
		case KEY_DOWN:
		case KEY_DOWN_REPEAT:
			pos++;
			display( );
			break;
	}
}

/**/
static void timetick( unsigned int systick )
{
}

MENUITEM Menu_0_1_license =
{
	"车牌号输入",
	10,
	0,
	&show,
	&keypress,
	&timetick,
	&msg,
	(void*)0
};

#else

#define width_hz	12
#define width_zf	6
#define top_line	14

unsigned char	screen_flag		= 0; //==1开始输入汉字界面
unsigned char	screen_in_sel	= 1, screen_in_sel2 = 1;
unsigned char	zifu_counter	= 0;

//京津冀沪渝豫云辽黑湘  皖鲁新苏浙赣鄂桂甘晋  蒙陕吉闽贵粤青藏川宁  琼
unsigned char Car_HZ_code[31][2] = { "京", "津", "冀", "沪", "渝", "豫", "云", "辽", "黑", "湘", \
	                                 "皖", "鲁", "新", "苏", "浙", "赣", "鄂", "桂", "甘", "晋", "蒙", "陕", "吉", "闽", "贵", "粤", "青", "藏", "川", "宁", "琼" };
//


/*unsigned char Car_num_code[36]={'0','1','2','3','4','5','6','7','8','9',\
   'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R',\
   'S','T','U','V','W','X','Y','Z'};*/
unsigned char	Car_num_code[36][1] = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", \
	                                    "A",   "B", "C", "D", "E", "F", "G", "H", "I", "J", "K","L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z" };

unsigned char	select[] = { 0x0C, 0x06, 0xFF, 0x06, 0x0C };
unsigned char	select_kong[] = { 0x00, 0x00, 0x00, 0x00, 0x00 };
DECL_BMP( 8, 5, select );
DECL_BMP( 8, 5, select_kong );


/*
   京津冀晋蒙辽吉黑沪苏浙皖闽赣鲁豫鄂湘粤新桂琼渝川贵云藏陕甘青宁新港澳
 */
void license_input( unsigned char par )
{
	lcd_fill( 0 );
	switch( par )
	{
		case 1:
			lcd_text12( 0, 20, "京津冀晋沪渝豫云辽黑", 20, LCD_MODE_SET );
			break;
		case 2:
			lcd_text12( 0, 20, "皖鲁新苏浙赣鄂桂甘湘", 20, LCD_MODE_SET );
			break;
		case 3:
			lcd_text12( 0, 20, "蒙陕吉闽贵粤青藏川宁", 20, LCD_MODE_SET );
			break;
		case 4:
			lcd_text12( 0, 20, "琼", 2, LCD_MODE_SET );
			break;
		case 5:
			lcd_text12( 0, 20, "0123456789ABCDEFGHIJ", 20, LCD_MODE_SET );
			break;
		case 6:
			lcd_text12( 0, 20, "KLMNOPQRSTUVWXYZ", 16, LCD_MODE_SET );
			break;
		default:
			break;
	}
	lcd_update_all( );
}

/*
   京津冀沪渝豫云辽
   黑湘皖鲁新苏浙赣
   鄂桂甘晋蒙陕吉闽贵粤青藏川宁琼

   offset:  type==2时有用    ==1时无用
 */
void license_input_az09( unsigned char type, unsigned char offset, unsigned char par )
{
	if( ( type == 1 ) && ( par >= 1 ) && ( par <= 31 ) )
	{
		memcpy( Menu_Car_license, (char*)Car_HZ_code[par - 1], 2 );
		lcd_text12( 0, 0, (char*)Menu_Car_license, 2, LCD_MODE_SET );
	}else if( ( type == 2 ) && ( par >= 1 ) && ( par <= 36 ) )
	{
		memcpy( Menu_Car_license + offset - 1, (char*)Car_num_code[par - 1], 1 );
		lcd_text12( 0, 0, (char*)Menu_Car_license, offset, LCD_MODE_SET ); //Car_license+2+(offset-3)=Car_license+offset-1
	}
}

/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static void msg( void *p )
{
}

/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static void show( void )
{
	memset( Menu_Car_license, 0, sizeof( Menu_Car_license ) );
	CounterBack = 0;
	license_input( 1 );
	lcd_bitmap( ( screen_in_sel - 1 ) * 12, top_line, &BMP_select, LCD_MODE_SET );
	lcd_update_all( );

	screen_flag		= 1;
	screen_in_sel	= 1;
	screen_in_sel2	= 1;
	zifu_counter	= 0;
}

/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static void keypress( unsigned int key )
{
	switch( key )
	{
		case KEY_MENU:
			pMenuItem = &Menu_0_loggingin;
			pMenuItem->show( );

			screen_in_sel	= 1;
			screen_in_sel2	= 1;
			screen_flag		= 0;
			zifu_counter	= 0;

			break;
		case KEY_OK:
			if( screen_flag == 1 )
			{
				if( zifu_counter == 0 )
				{
					zifu_counter = 2;                       //1个汉字=2个字符
				}
				screen_flag = 5;                            //第一个汉字选好，选字符
				lcd_fill( 0 );
				//lcd_text12(0,0,(char *)Car_license,zifu_counter,LCD_MODE_SET);

				license_input( 5 );                         //字母选择
				license_input_az09( 1, 0, screen_in_sel );  //写入选定的汉字
				lcd_bitmap( 0, top_line, &BMP_select, LCD_MODE_SET );
				lcd_update_all( );

				screen_in_sel2 = 1;
			}else if( ( screen_flag == 5 ) && ( zifu_counter < 8 ) )
			{
				zifu_counter++;

				lcd_fill( 0 );
				license_input( 5 );                                     //字母选择
				license_input_az09( 2, zifu_counter, screen_in_sel2 );  //
				//license_input_az09(1,0,screen_in_sel);//写入选定的汉字
				screen_in_sel2 = 1;
				lcd_bitmap( 0, top_line, &BMP_select, LCD_MODE_SET );
				lcd_text12( 0, 0, (char*)Menu_Car_license, zifu_counter, LCD_MODE_SET );
				lcd_update_all( );
				screen_in_sel2 = 1;
			}else if( ( screen_flag == 5 ) && ( zifu_counter == 8 ) )
			{
				screen_flag = 10;                           //车牌号输入完成

				zifu_counter = 0;
				lcd_fill( 0 );
				lcd_text12( 0, 0, (char*)Menu_Car_license, 8, LCD_MODE_SET );
				//lcd_text12(15,1,(char *)Car_license+2,sizeof(Car_license)-2,LCD_MODE_SET);
				lcd_text12( 18, 20, "车牌号输入完成", 14, LCD_MODE_INVERT );
				license_input_az09( 1, 0, screen_in_sel );  //写入选定的汉字
				lcd_update_all( );

				screen_in_sel2 = 1;
			}else if( screen_flag == 10 )
			{
				CarSet_0_counter	= 2;                    //   设置第2项
				pMenuItem			= &Menu_0_loggingin;
				pMenuItem->show( );

				screen_in_sel	= 1;
				screen_in_sel2	= 1;
				screen_flag		= 0;
				zifu_counter	= 0;
			}
			break;
		case KEY_UP:
			if( screen_flag == 1 ) //汉字选择
			{
				if( screen_in_sel >= 2 )
				{
					screen_in_sel--;
				} else if( screen_in_sel == 1 )
				{
					screen_in_sel = 31;
				}
				if( screen_in_sel <= 10 )
				{
					license_input( 1 );
					lcd_bitmap( ( screen_in_sel - 1 ) * width_hz, top_line, &BMP_select, LCD_MODE_SET );
				}else if( ( screen_in_sel > 10 ) && ( screen_in_sel <= 20 ) )
				{
					license_input( 2 );
					lcd_bitmap( ( screen_in_sel - 11 ) * width_hz, top_line, &BMP_select, LCD_MODE_SET );
				}else if( ( screen_in_sel > 20 ) && ( screen_in_sel <= 30 ) )
				{
					license_input( 3 );
					lcd_bitmap( ( screen_in_sel - 21 ) * width_hz, top_line, &BMP_select, LCD_MODE_SET );
				}          else if( screen_in_sel == 31 )
				{
					license_input( 4 );
					lcd_bitmap( ( screen_in_sel - 31 ) * width_hz, top_line, &BMP_select, LCD_MODE_SET );
				}
				lcd_update_all( );
			}else if( screen_flag == 5 ) //字母/数字 选择
			{
				if( screen_in_sel2 >= 2 )
				{
					screen_in_sel2--;
				} else if( screen_in_sel2 == 1 )
				{
					screen_in_sel2 = 36;
				}
				if( screen_in_sel2 <= 20 )
				{
					license_input( 5 );
					lcd_bitmap( ( screen_in_sel2 - 1 ) * width_zf, top_line, &BMP_select, LCD_MODE_SET );
				}else if( ( screen_in_sel2 > 20 ) && ( screen_in_sel2 <= 36 ) )
				{
					license_input( 6 );
					lcd_bitmap( ( screen_in_sel2 - 21 ) * width_zf, top_line, &BMP_select, LCD_MODE_SET );
				}
				//license_input_az09(1,0,screen_in_sel);//写入选定的字母、数字
				lcd_text12( 0, 0, (char*)Menu_Car_license, zifu_counter, LCD_MODE_SET );
				lcd_update_all( );
			}
			break;
		case KEY_DOWN:
			if( screen_flag == 1 ) //汉字选择
			{
				if( screen_in_sel < 31 )
				{
					screen_in_sel++;
				} else if( screen_in_sel == 31 )
				{
					screen_in_sel = 1;
				}
				if( screen_in_sel <= 10 )
				{
					license_input( 1 );
					lcd_bitmap( ( screen_in_sel - 1 ) * width_hz, top_line, &BMP_select, LCD_MODE_SET );
				}else if( ( screen_in_sel > 10 ) && ( screen_in_sel <= 20 ) )
				{
					license_input( 2 );
					lcd_bitmap( ( screen_in_sel - 11 ) * width_hz, top_line, &BMP_select, LCD_MODE_SET );
				}else if( ( screen_in_sel > 20 ) && ( screen_in_sel <= 30 ) )
				{
					license_input( 3 );
					lcd_bitmap( ( screen_in_sel - 21 ) * width_hz, top_line, &BMP_select, LCD_MODE_SET );
				}          else if( screen_in_sel == 31 )
				{
					license_input( 4 );
					lcd_bitmap( ( screen_in_sel - 31 ) * width_hz, top_line, &BMP_select, LCD_MODE_SET );
				}
				lcd_update_all( );
			}else if( screen_flag == 5 ) //字母/数字 选择
			{
				if( screen_in_sel2 < 36 )
				{
					screen_in_sel2++;
				} else if( screen_in_sel2 == 36 )
				{
					screen_in_sel2 = 1;
				}
				if( screen_in_sel2 <= 20 )
				{
					license_input( 5 );
					lcd_bitmap( ( screen_in_sel2 - 1 ) * width_zf, top_line, &BMP_select, LCD_MODE_SET );
				}else if( ( screen_in_sel2 > 20 ) && ( screen_in_sel2 <= 36 ) )
				{
					license_input( 6 );
					lcd_bitmap( ( screen_in_sel2 - 21 ) * width_zf, top_line, &BMP_select, LCD_MODE_SET );
				}
				//license_input_az09(1,0,screen_in_sel);//写入选定的数字、字母
				lcd_text12( 0, 0, (char*)Menu_Car_license, zifu_counter, LCD_MODE_SET );
				lcd_update_all( );
			}

			break;
	}
}

/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static void timetick( unsigned int systick )
{
}

MENUITEM Menu_0_1_license =
{
	"车牌号",
	6,
	0,
	&show,
	&keypress,
	&timetick,
	&msg,
	(void*)0
};

#endif

/************************************** The End Of File **************************************/
