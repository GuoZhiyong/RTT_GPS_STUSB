/**************************************************************************
 *
 *   sed1520.c
 *   LCD display controller interface routines for graphics modules
 *   with onboard SED1520 controller(s) in "write-only" setup
 *
 *   Version 1.02 (20051031)
 *
 *   For Atmel AVR controllers with avr-gcc/avr-libc
 *   Copyright (c) 2005
 *     Martin Thomas, Kaiserslautern, Germany
 *     <eversmith@heizung-thomas.de>
 *     http://www.siwawi.arubi.uni-kl.de/avr_projects
 *
 *   Permission to use in NON-COMMERCIAL projects is herbey granted. For
 *   a commercial license contact the author.
 *
 *   The above copyright notice and this permission notice shall be included in all
 *   copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 *   FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 *   COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 *   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 *   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 *   partly based on code published by:
 *   Michael J. Karas and Fabian "ape" Thiele
 *
 *
 ***************************************************************************/


/*
   An Emerging Display EW12A03LY 122x32 Graphics module has been used
   for testing. This module only supports "write". There is no option
   to read data from the SED1520 RAM. The SED1520 R/W line on the
   module is bound to GND according to the datasheet. Because of this
   Read-Modify-Write using the LCD-RAM is not possible with the 12A03
   LCD-Module. So this library uses a "framebuffer" which needs
   ca. 500 bytes of the AVR's SRAM. The libray can of cause be used
   with read/write modules too.
 */

/* tab-width: 4 */

//#include <LPC213x.H>
//#include <includes.h>

#include <stdint.h>
#include "sed1520.h"
#include "board.h"
#include "stm32f4xx.h"
#include "menu_include.h"

/* pixel level bit masks for display */
/* this array is setup to map the order */
/* of bits in a byte to the vertical order */
/* of bits at the LCD controller */
const unsigned char l_mask_array[8] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 }; /* TODO: avoid or PROGMEM */

/* the LCD display image memory */
/* buffer arranged so page memory is sequential in RAM */
static unsigned char l_display_array[LCD_Y_BYTES][LCD_X_BYTES];

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

	ControlBitShift( RST0 | 0x0 | 0x60 );
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
	ControlBitShift( ctr | 0x60 );
	//delay(1);
	for( i = 0; i < 0xf; i++ )
	{
	}
	ControlBitShift( RST0 | 0 | 0x60 );
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
	ControlBitShift( ctr | 0x60 );
	DataBitShift( dat );
	if( ncontr & 0x01 )
	{
		ctr |= E1;
	}
	if( ncontr & 0x02 )
	{
		ctr |= E2;
	}
	ControlBitShift( ctr | 0x60 );
	//delay(1);
	for( i = 0; i < 0xf; i++ )
	{
	}
	ControlBitShift( RST0 | A0 | 0x60 );
}

/*
**
** routine to initialize the operation of the LCD display subsystem
**
*/
void lcd_init( void )
{
	//IODIR0	= MR|SHCP|DS|STCP1|STCP2;
	//IOSET0	= MR;
	//GPIO_SetBits(GPIOA,GPIO_Pin_15);

	lcd_out_ctl( 0, 3 );
	lcd_out_ctl( LCD_RESET, 3 );
	//delay_ms(1);//3

	lcd_out_ctl( LCD_DISP_ON, 3 );
	lcd_out_ctl( LCD_SET_ADC_NOR, 3 ); // !
	lcd_out_ctl( LCD_SET_LINE + 16, 3 );
	lcd_out_ctl( LCD_SET_PAGE + 0, 3 );
	lcd_out_ctl( LCD_SET_COL, 3 );


	/*
	   lcd_out_ctl(0,3);
	   lcd_out_ctl(LCD_RESET,3);

	   lcd_out_ctl(LCD_DISP_ON,3);
	   lcd_out_ctl(LCD_SET_ADC_REV,3); // !
	   lcd_out_ctl(LCD_SET_LINE+0,3);
	   lcd_out_ctl(LCD_SET_PAGE+0,3);
	   lcd_out_ctl(LCD_SET_COL+LCD_STARTCOL_REVERSE,3);
	 */
}

/* fill buffer and LCD with pattern */
void lcd_fill( const unsigned char pattern )
{
	unsigned char page, col;
	lcd_init( );
	lcd_out_ctl( LCD_DISP_OFF, 3 );
	for( page = 0; page < LCD_Y_BYTES; page++ )
	{
		for( col = 0; col < LCD_X_BYTES; col++ )
		{
			l_display_array[page][col] = pattern;
		}
	}
	lcd_update_all( );
	lcd_out_ctl( LCD_DISP_ON, 3 );
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
void lcd_fill_2( const unsigned char pattern, unsigned char Pag_start, unsigned char Pag_end, unsigned char Col_start, unsigned char col_end )
{
	unsigned char page, col;

	for( page = Pag_start; page < Pag_end; page++ )
	{
		for( col = Col_start; col < col_end; col++ )
		{
			l_display_array[page][col] = pattern;
		}
	}
	lcd_update_all( );
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
void lcd_fill_Page( const unsigned char pattern, unsigned char startpage, unsigned char endpage )
{
	unsigned char page, col;

	for( page = startpage; page < endpage; page++ )
	{
		for( col = 0; col < LCD_X_BYTES; col++ )
		{
			l_display_array[page][col] = pattern;
		}
	}
	lcd_update_all( );
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
void lcd_erase( void )
{
	lcd_fill( 0x00 );
	lcd_update_all( );
}

/*
**
** Updates area of the display. Writes data from "framebuffer"
** RAM to the lcd display controller RAM.
**
** Arguments Used:
**    top     top line of area to update.
**    bottom  bottom line of area to update.
**    from MJK-Code
**
*/
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
//     lcd_out_ctl(LCD_SET_COL+LCD_STARTCOL_REVERSE,3);
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

/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
void lcd_update_all( void )
{
	lcd_update( SCRN_TOP, SCRN_BOTTOM );
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
void lcd_bitmap( const uint8_t left, const uint8_t top, IMG_DEF *img_ptr, const uint8_t mode )
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

/*
   绘制12点阵的字符，包括中文和英文


 */
void lcd_text12( char left, char top, char *pinfo, char len, const char mode )
{
	int				charnum = len;
	int				i;
	char			msb, lsb;

	int				addr;
	unsigned char	start_col = left;
	unsigned int	val_old, val_new, val_mask;

	unsigned int	glyph[12]; /*保存一个字符的点阵信息，以逐列式*/
	uint8_t			*p = pinfo;

	while( charnum )
	{
		for( i = 0; i < 12; i++ )
		{
			glyph[i] = 0;
		}
		msb = *p++;
		charnum--;
		if( msb <= 0x80 ) //ascii字符 0612
		{
			addr = ( msb - 0x20 ) * 12 + FONT_ASC0612_ADDR;
			for( i = 0; i < 3; i++ )
			{
				val_new				= *(__IO uint32_t*)addr;
				glyph[i * 2 + 0]	= ( val_new & 0xffff );
				glyph[i * 2 + 1]	= ( val_new & 0xffff0000 ) >> 16;
				addr				+= 4;
			}

			val_mask = ( ( 0xfff ) << top ); /*12bit*/

			/*加上top的偏移*/
			for( i = 0; i < 6; i++ )
			{
				glyph[i] <<= top;

				val_old = l_display_array[0][start_col] | ( l_display_array[1][start_col] << 8 ) | ( l_display_array[2][start_col] << 16 ) | ( l_display_array[3][start_col] << 24 );
				if( mode == LCD_MODE_SET )
				{
					val_new = val_old & ( ~val_mask ) | glyph[i];
				}else if( mode == LCD_MODE_INVERT )
				{
					val_new = ( val_old | val_mask ) & ( ~glyph[i] );
				}
				l_display_array[0][start_col]	= val_new & 0xff;
				l_display_array[1][start_col]	= ( val_new & 0xff00 ) >> 8;
				l_display_array[2][start_col]	= ( val_new & 0xff0000 ) >> 16;
				l_display_array[3][start_col]	= ( val_new & 0xff000000 ) >> 24;
				start_col++;
			}
		}else
		{
			lsb = *p++;
			charnum--;
			if( ( msb >= 0xa1 ) && ( msb <= 0xa3 ) && ( lsb >= 0xa1 ) )
			{
				addr = FONT_HZ1212_ADDR + ( ( ( (unsigned long)msb ) - 0xa1 ) * 94 + ( ( (unsigned long)lsb ) - 0xa1 ) ) * 24;
			}else if( ( msb >= 0xb0 ) && ( msb <= 0xf7 ) && ( lsb >= 0xa1 ) )
			{
				addr = FONT_HZ1212_ADDR + ( ( ( (unsigned long)msb ) - 0xb0 ) * 94 + ( ( (unsigned long)lsb ) - 0xa1 ) ) * 24 + 282 * 24;
			}
			for( i = 0; i < 6; i++ )
			{
				val_new				= *(__IO uint32_t*)addr;
				glyph[i * 2 + 0]	= ( val_new & 0xffff );
				glyph[i * 2 + 1]	= ( val_new & 0xffff0000 ) >> 16;
				addr				+= 4;
			}
			val_mask = ( ( 0xfff ) << top ); /*12bit*/

			/*加上top的偏移*/
			for( i = 0; i < 12; i++ )
			{
				glyph[i] <<= top;
				/*通过start_col映射到I_display_array中，注意mask*/
				val_old = l_display_array[0][start_col] | ( l_display_array[1][start_col] << 8 ) | ( l_display_array[2][start_col] << 16 ) | ( l_display_array[3][start_col] << 24 );
				if( mode == LCD_MODE_SET )
				{
					val_new = val_old & ( ~val_mask ) | glyph[i];
				}else if( mode == LCD_MODE_INVERT )
				{
					val_new = ( val_old | val_mask ) & ( ~glyph[i] );
				}
				l_display_array[0][start_col]	= val_new & 0xff;
				l_display_array[1][start_col]	= ( val_new & 0xff00 ) >> 8;
				l_display_array[2][start_col]	= ( val_new & 0xff0000 ) >> 16;
				l_display_array[3][start_col]	= ( val_new & 0xff000000 ) >> 24;
				start_col++;
			}
		}
	}
}

/*绘制点*/
void lcd_dot( const unsigned char x, const unsigned char y, const unsigned char mode )
{
	unsigned char	bitnum, bitmask, yByte;
	unsigned char	*pBuffer; /* pointer used for optimisation */

	if( ( x > SCRN_RIGHT ) || ( y > SCRN_BOTTOM ) )
	{
		return;
	}

	yByte	= y >> 3;
	bitnum	= y & 0x07;
	bitmask = l_mask_array[bitnum]; // bitmask = ( 1 << (y & 0x07) );
	pBuffer = &( l_display_array[yByte][x] );
	switch( mode )
	{
		case LCD_MODE_SET:
			*pBuffer |= bitmask;
			break;
		case LCD_MODE_CLEAR:
			*pBuffer &= ~bitmask;
			break;
		case LCD_MODE_XOR:
			*pBuffer ^= bitmask;
			break;
		case LCD_MODE_INVERT:
			if( ( *pBuffer ) & bitmask > 0 )
			{
				*pBuffer &= ~bitmask;
			}else
			{
				*pBuffer |= bitmask;
			}
			break;
		default: break;
	}
}

/*绘制线段*/
void lcd_drawline( int x1, int y1, int x2, int y2, const char mode )
{
	int dx	= x2 - x1;
	int dy	= y2 - y1;
	int ux	= ( ( dx > 0 ) << 1 ) - 1;  //x的增量方向，取或-1
	int uy	= ( ( dy > 0 ) << 1 ) - 1;  //y的增量方向，取或-1
	int x	= x1, y = y1, eps;          //eps为累加误差

	eps = 0; dx = abs( dx ); dy = abs( dy );
	if( dx > dy )
	{
		for( x = x1; x != x2; x += ux )
		{
			lcd_dot( x, y,mode );
			eps += dy;
			if( ( eps << 1 ) >= dx )
			{
				y += uy; eps -= dx;
			}
		}
	}else
	{
		for( y = y1; y != y2; y += uy )
		{
			lcd_dot( x, y,mode );
			eps += dx;
			if( ( eps << 1 ) >= dy )
			{
				x += ux; eps -= dy;
			}
		}
	}
}

/************************************** The End Of File **************************************/
