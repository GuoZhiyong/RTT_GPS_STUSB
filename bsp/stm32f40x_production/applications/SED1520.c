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
#include <stdint.h>
#include <string.h>
#include "sed1520.h"
#include "stm32f4xx.h"

/* pixel level bit masks for display */
/* this array is setup to map the order */
/* of bits in a byte to the vertical order */
/* of bits at the LCD controller */
const unsigned char		l_mask_array[8] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 }; /* TODO: avoid or PROGMEM */
static unsigned char	l_display_array[LCD_Y_BYTES][LCD_X_BYTES];

/* control-lines hardware-interface (only "write") */
#define LCD_CMD_MODE( )		LCDCTRLPORT &= ~( 1 << LCDCMDPIN )
#define LCD_DATA_MODE( )	LCDCTRLPORT |= ( 1 << LCDCMDPIN )
#define LCD_ENABLE_E1( )	LCDCTRLPORT &= ~( 1 << LCDE1PIN )
#define LCD_DISABLE_E1( )	LCDCTRLPORT |= ( 1 << LCDE1PIN )
#define LCD_ENABLE_E2( )	LCDCTRLPORT &= ~( 1 << LCDE2PIN )
#define LCD_DISABLE_E2( )	LCDCTRLPORT |= ( 1 << LCDE2PIN )

#define MR		( 1 << 15 )
#define SHCP	( 1 << 12 )
#define DS		( 1 << 14 )
#define STCP1	( 1 << 15 )
#define STCP2	( 1 << 13 )

#define RST0	( 1 << 0 )
#define E1		( 1 << 1 )
#define E2		( 1 << 2 )
#define RW		( 1 << 3 )
#define A0		( 1 << 4 )

const unsigned char asc_0608[][6] =
{
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, /*" ",0*/
	{ 0x00, 0x00, 0x5F, 0x00, 0x00, 0x00 }, /*"!",1*/
	{ 0x00, 0x03, 0x00, 0x03, 0x00, 0x00 }, /*""",2*/
	{ 0x0A, 0x1F, 0x0A, 0x1F, 0x0A, 0x00 }, /*"#",3*/
	{ 0x26, 0x49, 0xFF, 0x49, 0x32, 0x00 }, /*"$",4*/
	{ 0x66, 0x19, 0x37, 0x4D, 0x33, 0x00 }, /*"%",5*/
	{ 0x36, 0x49, 0x55, 0x22, 0x50, 0x00 }, /*"&",6*/
	{ 0x00, 0x00, 0x03, 0x00, 0x00, 0x00 }, /*"'",7*/
	{ 0x00, 0x3E, 0x41, 0x80, 0x00, 0x00 }, /*"(",8*/
	{ 0x00, 0x80, 0x41, 0x3E, 0x00, 0x00 }, /*")",9*/
	{ 0x0A, 0x04, 0x1F, 0x04, 0x0A, 0x00 }, /*"*",10*/
	{ 0x08, 0x08, 0x3E, 0x08, 0x08, 0x00 }, /*"+",11*/
	{ 0x00, 0x60, 0xE0, 0x00, 0x00, 0x00 }, /*",",12*/
	{ 0x00, 0x08, 0x08, 0x08, 0x00, 0x00 }, /*"-",13*/
	{ 0x00, 0x00, 0x60, 0x60, 0x00, 0x00 }, /*".",14*/
	{ 0x80, 0x60, 0x18, 0x06, 0x01, 0x00 }, /*"/",15*/
	{ 0x3E, 0x51, 0x49, 0x45, 0x3E, 0x00 }, /*"0",16*/
	{ 0x42, 0x42, 0x7F, 0x40, 0x40, 0x00 }, /*"1",17*/
	{ 0x42, 0x61, 0x51, 0x49, 0x46, 0x00 }, /*"2",18*/
	{ 0x22, 0x41, 0x49, 0x49, 0x36, 0x00 }, /*"3",19*/
	{ 0x18, 0x14, 0x52, 0x7F, 0x50, 0x00 }, /*"4",20*/
	{ 0x27, 0x45, 0x45, 0x45, 0x39, 0x00 }, /*"5",21*/
	{ 0x3E, 0x45, 0x45, 0x45, 0x38, 0x00 }, /*"6",22*/
	{ 0x01, 0x01, 0x71, 0x09, 0x07, 0x00 }, /*"7",23*/
	{ 0x36, 0x49, 0x49, 0x49, 0x36, 0x00 }, /*"8",24*/
	{ 0x0E, 0x51, 0x51, 0x51, 0x3E, 0x00 }, /*"9",25*/
	{ 0x00, 0x00, 0x6C, 0x6C, 0x00, 0x00 }, /*":",26*/
	{ 0x00, 0x6C, 0xEC, 0x00, 0x00, 0x00 }, /*";",27*/
	{ 0x00, 0x08, 0x14, 0x22, 0x41, 0x00 }, /*"<",28*/
	{ 0x14, 0x14, 0x14, 0x14, 0x14, 0x00 }, /*"=",29*/
	{ 0x00, 0x41, 0x22, 0x14, 0x08, 0x00 }, /*">",30*/
	{ 0x02, 0x01, 0x51, 0x09, 0x06, 0x00 }, /*"?",31*/
	{ 0x3E, 0x41, 0x5D, 0x55, 0x5E, 0x00 }, /*"@",32*/
	{ 0x78, 0x16, 0x11, 0x16, 0x78, 0x00 }, /*"A",33*/
	{ 0x7F, 0x49, 0x49, 0x49, 0x36, 0x00 }, /*"B",34*/
	{ 0x3E, 0x41, 0x41, 0x41, 0x22, 0x00 }, /*"C",35*/
	{ 0x7F, 0x41, 0x41, 0x41, 0x3E, 0x00 }, /*"D",36*/
	{ 0x7F, 0x49, 0x49, 0x49, 0x41, 0x00 }, /*"E",37*/
	{ 0x7F, 0x09, 0x09, 0x09, 0x01, 0x00 }, /*"F",38*/
	{ 0x3E, 0x41, 0x41, 0x49, 0x3A, 0x00 }, /*"G",39*/
	{ 0x7F, 0x08, 0x08, 0x08, 0x7F, 0x00 }, /*"H",40*/
	{ 0x41, 0x41, 0x7F, 0x41, 0x41, 0x00 }, /*"I",41*/
	{ 0x30, 0x40, 0x40, 0x40, 0x3F, 0x00 }, /*"J",42*/
	{ 0x7F, 0x08, 0x14, 0x22, 0x41, 0x00 }, /*"K",43*/
	{ 0x7F, 0x40, 0x40, 0x40, 0x40, 0x00 }, /*"L",44*/
	{ 0x7F, 0x02, 0x0C, 0x02, 0x7F, 0x00 }, /*"M",45*/
	{ 0x7F, 0x02, 0x04, 0x08, 0x7F, 0x00 }, /*"N",46*/
	{ 0x3E, 0x41, 0x41, 0x41, 0x3E, 0x00 }, /*"O",47*/
	{ 0x7F, 0x09, 0x09, 0x09, 0x06, 0x00 }, /*"P",48*/
	{ 0x3E, 0x41, 0x61, 0x41, 0xBE, 0x00 }, /*"Q",49*/
	{ 0x7F, 0x09, 0x09, 0x09, 0x76, 0x00 }, /*"R",50*/
	{ 0x26, 0x49, 0x49, 0x49, 0x32, 0x00 }, /*"S",51*/
	{ 0x01, 0x01, 0x7F, 0x01, 0x01, 0x00 }, /*"T",52*/
	{ 0x3F, 0x40, 0x40, 0x40, 0x3F, 0x00 }, /*"U",53*/
	{ 0x07, 0x18, 0x60, 0x18, 0x07, 0x00 }, /*"V",54*/
	{ 0x7F, 0x20, 0x18, 0x20, 0x7F, 0x00 }, /*"W",55*/
	{ 0x63, 0x14, 0x08, 0x14, 0x63, 0x00 }, /*"X",56*/
	{ 0x07, 0x08, 0x70, 0x08, 0x07, 0x00 }, /*"Y",57*/
	{ 0x61, 0x51, 0x49, 0x45, 0x43, 0x00 }, /*"Z",58*/
	{ 0x00, 0x00, 0xFF, 0x80, 0x00, 0x00 }, /*"[",59*/
	{ 0x00, 0x01, 0x06, 0x18, 0x60, 0x80 }, /*"\",60*/
	{ 0x00, 0x80, 0xFF, 0x00, 0x00, 0x00 }, /*"]",61*/
	{ 0x04, 0x02, 0x01, 0x02, 0x04, 0x00 }, /*"^",62*/
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, /*"_",63*/
	{ 0x00, 0x00, 0x01, 0x00, 0x00, 0x00 }, /*"`",64*/
	{ 0x38, 0x44, 0x44, 0x24, 0x7C, 0x00 }, /*"a",65*/
	{ 0x7F, 0x44, 0x44, 0x44, 0x38, 0x00 }, /*"b",66*/
	{ 0x38, 0x44, 0x44, 0x44, 0x48, 0x00 }, /*"c",67*/
	{ 0x38, 0x44, 0x44, 0x44, 0x7F, 0x00 }, /*"d",68*/
	{ 0x38, 0x54, 0x54, 0x54, 0x58, 0x00 }, /*"e",69*/
	{ 0x00, 0x04, 0x7E, 0x05, 0x01, 0x00 }, /*"f",70*/
	{ 0x38, 0x44, 0x44, 0x44, 0xFC, 0x00 }, /*"g",71*/
	{ 0x7F, 0x04, 0x04, 0x04, 0x78, 0x00 }, /*"h",72*/
	{ 0x00, 0x44, 0x7D, 0x40, 0x00, 0x00 }, /*"i",73*/
	{ 0x00, 0x04, 0xFD, 0x00, 0x00, 0x00 }, /*"j",74*/
	{ 0x7F, 0x10, 0x18, 0x24, 0x40, 0x00 }, /*"k",75*/
	{ 0x00, 0x41, 0x7F, 0x40, 0x00, 0x00 }, /*"l",76*/
	{ 0x7C, 0x04, 0x7C, 0x04, 0x78, 0x00 }, /*"m",77*/
	{ 0x7C, 0x08, 0x04, 0x04, 0x78, 0x00 }, /*"n",78*/
	{ 0x38, 0x44, 0x44, 0x44, 0x38, 0x00 }, /*"o",79*/
	{ 0xFC, 0x44, 0x44, 0x44, 0x38, 0x00 }, /*"p",80*/
	{ 0x38, 0x44, 0x44, 0x44, 0xFC, 0x00 }, /*"q",81*/
	{ 0x7C, 0x08, 0x04, 0x04, 0x08, 0x00 }, /*"r",82*/
	{ 0x48, 0x54, 0x54, 0x54, 0x24, 0x00 }, /*"s",83*/
	{ 0x00, 0x04, 0x3F, 0x44, 0x40, 0x00 }, /*"t",84*/
	{ 0x3C, 0x40, 0x40, 0x20, 0x7C, 0x00 }, /*"u",85*/
	{ 0x0C, 0x30, 0x40, 0x30, 0x0C, 0x00 }, /*"v",86*/
	{ 0x3C, 0x40, 0x3C, 0x40, 0x3C, 0x00 }, /*"w",87*/
	{ 0x44, 0x28, 0x10, 0x28, 0x44, 0x00 }, /*"x",88*/
	{ 0x3C, 0x20, 0xa0, 0xa0, 0xFC, 0x00 }, /*"y",89*/
	{ 0x44, 0x64, 0x54, 0x4C, 0x44, 0x00 }, /*"z",90*/
	{ 0x00, 0x08, 0xF7, 0x00, 0x00, 0x00 }, /*"{",91*/
	{ 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00 }, /*"|",92*/
	{ 0x00, 0x00, 0xF7, 0x08, 0x00, 0x00 }, /*"}",93*/
	{ 0x08, 0x04, 0x0C, 0x08, 0x04, 0x00 }, /*"~",94*/
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, /*"",95*/
};


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
void ControlBitShift( unsigned char data )
{
	unsigned char i;

	//IOSET0	= STCP1;
	//IOCLR0 = STCP2;
	GPIO_SetBits( GPIOE, GPIO_Pin_15 );
	GPIO_ResetBits( GPIOE, GPIO_Pin_13 );

	for( i = 0; i < 8; i++ )
	{
		//IOCLR0=SHCP;
		GPIO_ResetBits( GPIOE, GPIO_Pin_12 );
		if( data & 0x80 )
		{
			//IOSET0 = DS;
			GPIO_SetBits( GPIOE, GPIO_Pin_14 );
		} else
		{
			//IOCLR0 = DS;
			GPIO_ResetBits( GPIOE, GPIO_Pin_14 );
		}
		//IOSET0 = SHCP;
		GPIO_SetBits( GPIOE, GPIO_Pin_12 );
		data <<= 1;
	}
	//IOSET0 = STCP2;
	GPIO_SetBits( GPIOE, GPIO_Pin_13 );
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
void DataBitShift( unsigned char data )
{
	unsigned char i;
	//IOCLR0 = STCP1;
	GPIO_SetBits( GPIOE, GPIO_Pin_13 );
	GPIO_ResetBits( GPIOE, GPIO_Pin_15 );
	for( i = 0; i < 8; i++ )
	{
		//IOCLR0=SHCP;
		GPIO_ResetBits( GPIOE, GPIO_Pin_12 );
		if( data & 0x80 )
		{
			//IOSET0 = DS;
			GPIO_SetBits( GPIOE, GPIO_Pin_14 );
		} else
		{
			//IOCLR0 = DS;
			GPIO_ResetBits( GPIOE, GPIO_Pin_14 );
		}
		//IOSET0 = SHCP;
		GPIO_SetBits( GPIOE, GPIO_Pin_12 );
		data <<= 1;
	}
	//IOSET0 = STCP1;
	GPIO_SetBits( GPIOE, GPIO_Pin_15 );
}

/*
**
** low level routine to send a byte value
** to the LCD controller control register.
** entry argument is the data to output
** and the controller to use
** 1: IC 1, 2: IC 2, 3: both ICs
**
*/
void lcd_out_ctl( const unsigned char cmd, const unsigned char ncontr )
{
	unsigned char	ctr;
	unsigned int	i;
//  LCD_CMD_MODE();
//	LCDDATAPORT = cmd;

	ControlBitShift( RST0 | 0x40 );
	DataBitShift( cmd );
	ctr = RST0;
	if( ncontr & 0x01 )
	{
		ctr |= E1;
	}
	if( ncontr & 0x02 )
	{
		ctr |= E2;
	}
	ControlBitShift( ctr | 0x40 );
	//delay(1);
	for( i = 0; i < 0xf; i++ )
	{
	}
	ControlBitShift( RST0 | 0x40 );
}

/*
**
** low level routine to send a byte value
** to the LCD controller data register. entry argument
** is the data to output and the controller-number
**
*/
void lcd_out_dat( const unsigned char dat, const unsigned char ncontr )
{
	unsigned char	ctr;
	unsigned int	i;
//	LCD_DATA_MODE();
//   LCDDATAPORT = dat;

	ctr = RST0 | A0;
	ControlBitShift( ctr | 0x40 );
	DataBitShift( dat );
	if( ncontr & 0x01 )
	{
		ctr |= E1;
	}
	if( ncontr & 0x02 )
	{
		ctr |= E2;
	}
	ControlBitShift( ctr | 0x40 );
	//delay(1);
	for( i = 0; i < 0xf; i++ )
	{
	}
	ControlBitShift( RST0 | A0 | 0x40 );
}

/*
**
** routine to initialize the operation of the LCD display subsystem
**
*/
void lcd_init( void )
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOE, ENABLE );

	GPIO_InitStructure.GPIO_Pin		= GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType	= GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed	= GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd	= GPIO_PuPd_NOPULL;
	GPIO_Init( GPIOE, &GPIO_InitStructure );

	lcd_out_ctl( 0, 3 );
	lcd_out_ctl( LCD_RESET, 3 );

	lcd_out_ctl( LCD_DISP_ON, 3 );
	lcd_out_ctl( LCD_SET_ADC_NOR, 3 ); // !
	lcd_out_ctl( LCD_SET_LINE + 16, 3 );
	lcd_out_ctl( LCD_SET_PAGE + 0, 3 );
	lcd_out_ctl( LCD_SET_COL, 3 );
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

#if 0
void lcd_update( const unsigned char top, const unsigned char bottom )
{
	unsigned char	x;
	unsigned char	y;
	unsigned char	yt;
	unsigned char	yb;
	unsigned char	*colptr;

	/* setup bytes of range */
	yb	= bottom >> 3;
	yt	= top >> 3;

	for( y = yt; y <= yb; y++ )
	{
		lcd_out_ctl( LCD_SET_PAGE + y, 3 ); /* set page */
		lcd_out_ctl( LCD_SET_COL + 0, 3 );
		colptr = &l_display_array[y][0];

		for( x = 0; x < LCD_X_BYTES; x++ )
		{
			if( x < LCD_X_BYTES / 2 )
			{
				lcd_out_dat( *colptr++, 1 );
			} else
			{
				lcd_out_dat( *colptr++, 2 );
			}
		}
	}
}

#endif

/**/
void lcd_update( const unsigned char top, const unsigned char bottom )
{
	unsigned char	x;
	unsigned char	y;
	unsigned char	yt;
	unsigned char	yb;
	unsigned char	*colptr;

	/* setup bytes of range */
	yb	= bottom >> 3;
	yt	= top >> 3;

	for( y = yt; y <= yb; y++ )
	{
		lcd_out_ctl( LCD_SET_PAGE + y, 3 ); /* set page */
		lcd_out_ctl( LCD_SET_COL + 0, 3 );
		colptr = &l_display_array[y][0];
		for( x = 0; x < 61; x++ ) lcd_out_dat( *colptr++, 1 );
		for( x = 61; x < 122; x++ ) lcd_out_dat( *colptr++, 2 );
	}
}


/**/
void lcd_update_region( uint8_t left,uint8_t top, uint8_t right,uint8_t bottom )
{
	unsigned char	x;
	unsigned char	y;
	unsigned char	yt;
	unsigned char	yb;
	unsigned char	*colptr;

	/* setup bytes of range */
	yb	= bottom >> 3;
	yt	= top >> 3;

	for( y = yt; y <= yb; y++ )
	{
		lcd_out_ctl( LCD_SET_PAGE + y, 3 ); /* set page */
		lcd_out_ctl( LCD_SET_COL + 0, 3 );
		colptr = &l_display_array[y][0];
		for( x = 0; x < 61; x++ ) lcd_out_dat( *colptr++, 1 );
		for( x = 61; x < 122; x++ ) lcd_out_dat( *colptr++, 2 );
	}
}





/*清空RAM*/
void lcd_fill( const unsigned char pattern )
{
	unsigned char page, col;

	lcd_out_ctl( LCD_DISP_OFF, 3 );
	for( page = 0; page < LCD_Y_BYTES; page++ )
	{
		for( col = 0; col < LCD_X_BYTES; col++ )
		{
			l_display_array[page][col] = pattern;
		}
	}
	lcd_out_ctl( LCD_DISP_ON, 3 );
}

/*填充或清除指定的区域*/
void lcd_fill_1(uint8_t left,uint8_t top,uint8_t right,uint8_t bottom,uint8_t pattern)
{
	uint32_t val,val_old,mask;
	uint8_t col;
	
	mask=0xFFFFFFFF<<top;
	mask&=(0xFFFFFFFF>>(31-bottom));

	for(col=left;col<=right;col++)
	{
		val_old=l_display_array[0][col];
		val_old|=l_display_array[1][col]<<8;
		val_old|=l_display_array[2][col]<<16;
		val_old|=l_display_array[3][col]<<24;

		if(0==pattern) /*清除 特定位*/
		{
			val=val_old&(~mask);
		}	
		else
		{
			val=val_old|mask;
		}
		l_display_array[0][col]	= val & 0xff;
		l_display_array[1][col]	= ( val & 0xff00 ) >> 8;
		l_display_array[2][col]	= ( val & 0xff0000 ) >> 16;
		l_display_array[3][col]	= ( val & 0xff000000 ) >> 24;
	}
}




/*清空屏幕显示*/
void lcd_clear( const unsigned char top, const unsigned char bottom )
{
	unsigned char	x;
	unsigned char	y;
	unsigned char	yt;
	unsigned char	yb;

	yb	= bottom >> 3;
	yt	= top >> 3;

	for( y = yt; y <= yb; y++ )
	{
		lcd_out_ctl( LCD_SET_PAGE + y, 3 ); /* set page */
		lcd_out_ctl( LCD_SET_COL + 0, 3 );
		for( x = 0; x < 61; x++ ) lcd_out_dat( 0,1 );
		for( x = 61; x < 122; x++ ) lcd_out_dat( 0,2 );
	}
}

/*绘制图片文字*/
void lcd_bitmap( const uint8_t left, const uint8_t top, const struct IMG_DEF *img_ptr, const uint8_t mode )
{
	uint8_t width, heigth, h, w, pattern, mask;
	uint8_t * ptable;

	uint8_t bitnum, bitmask;
	uint8_t page, col, vdata;

	width	= img_ptr->width_in_pixels;
	heigth	= img_ptr->height_in_pixels;
	ptable	= (uint8_t*)( img_ptr->char_table );

	mask	= 0x80;
	pattern = *ptable;

	for( h = 0; h < heigth; h++ ) /**/
	{
		page	= ( h + top ) >> 3;
		bitnum	= ( h + top ) & 0x07;
		bitmask = ( 1 << bitnum );
		for( w = 0; w < width; w++ )
		{
			col		= left + w;
			vdata	= l_display_array[page][col];
			switch( mode )
			{
				case LCD_MODE_SET: /*不管原来的数据，直接设置为pattern的值*/
					if( pattern & mask )
					{
						vdata |= bitmask;
					} else
					{
						vdata &= ~bitmask;
					}
					break;
				case LCD_MODE_CLEAR:    /*不管原来的数据，清除原来的值=>0*/
					vdata &= ~bitmask;
					break;
				case LCD_MODE_XOR:      /*原来的数据，直接设置为pattern的值*/
					if( vdata & bitmask )
					{
						if( pattern & mask )
						{
							vdata &= ~bitmask;
						} else
						{
							vdata |= bitmask;
						}
					}else
					{
						if( pattern & mask )
						{
							vdata |= bitmask;
						} else
						{
							vdata &= ~bitmask;
						}
					}
					break;
				case LCD_MODE_INVERT: /*不管原来的数据，直接设置为pattern的值*/
					if( pattern & mask )
					{
						vdata &= ~bitmask;
					} else
					{
						vdata |= bitmask;
					}
					break;
			}
			l_display_array[page][col]	= vdata;
			mask						>>= 1;
			if( mask == 0 )
			{
				mask = 0x80;
				ptable++;
				pattern = *ptable;
			}
		}
		if( mask != 0x80 ) /*一行中的列已处理完*/
		{
			mask = 0x80;
			ptable++;
			pattern = *ptable;
		}
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
void lcd_asc0608( char left, char top, char *p, const char mode )
{
	int				charnum = strlen( p );
	int				i;
	char			msb;

	unsigned char	start_col = left;
	unsigned int	val, val_old, val_new, val_mask;
	while( charnum )
	{
		msb = *p++;
		charnum--;
		if( msb > 0x7f )
		{
			return;
		}
		if( msb < 0x20 )
		{
			return;
		}

		val_mask = ( 0xff << top );             /*8bit*/
		for( i = 0; i < 6; i++ )                /*一个字符*/
		{
			val		= asc_0608[msb - 0x20][i];  /*找到值*/
			val		= ( val << top );           /*根据top偏移*/
			val_old = l_display_array[0][start_col] | ( l_display_array[1][start_col] << 8 ) | ( l_display_array[2][start_col] << 16 ) | ( l_display_array[3][start_col] << 24 );
			if( mode == LCD_MODE_SET )
			{
				val_new = ( val_old & ( ~val_mask ) ) | val;
			}else if( mode == LCD_MODE_INVERT )
			{
				val_new = ( val_old | val_mask ) & ( ~val );
			}

			l_display_array[0][start_col]	= val_new & 0xff;
			l_display_array[1][start_col]	= ( val_new & 0xff00 ) >> 8;
			l_display_array[2][start_col]	= ( val_new & 0xff0000 ) >> 16;
			l_display_array[3][start_col]	= ( val_new & 0xff000000 ) >> 24;
			start_col++;
		}
	}
}

/*水平线,要保留原来的值*/
void lcd_hline(uint8_t from,uint8_t to ,uint8_t line)
{
	uint8_t row,col,offset=1;
	row=line/8;
	offset=(1<<(line%8));
	for(col=from;col<=to;col++)
	{
		l_display_array[row][col]|=offset;
	}


}

/*垂直线*/
void lcd_vline(uint8_t col,uint8_t from,uint8_t to)
{
	uint32_t i,j;
	i=0xFFFFFFFF<<from;
	j=0xFFFFFFFF>>(31-to);
	i&=j;
	l_display_array[0][col]	= i & 0xff;
	l_display_array[1][col]	= ( i & 0xff00 ) >> 8;
	l_display_array[2][col]	= ( i & 0xff0000 ) >> 16;
	l_display_array[3][col]	= ( i & 0xff000000 ) >> 24;
}



/************************************** The End Of File **************************************/
