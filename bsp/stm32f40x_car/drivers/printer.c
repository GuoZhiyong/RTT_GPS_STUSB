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
 *     bitter    20121102     1.0     build this moudle
 ***********************************************************/

#include <stdio.h>
#include <rtthread.h>
#include "stm32f4xx.h"

#include "ringbuffer.h"
#include "eeprom.h"
#include "main.h"
#include "gt21.h"

/*��ӡͷ���*/

//#define PAPER_DETCET_PORT	GPIOA
//#define PAPER_DETECT_PIN	GPIO_Pin_8

#define  DATA_CLK_PORT	GPIOB
#define  PRINT_CLK_PIN	GPIO_Pin_6
#define  PRINT_DATA_PIN GPIO_Pin_7

#define  LAT_PORT	GPIOB
#define  LAT_PIN	GPIO_Pin_5

#define  STB1_PORT	GPIOA
#define  STB1_PIN	GPIO_Pin_4

#define  STB2_6_PORT	GPIOB
#define  STB2_PIN		GPIO_Pin_1
#define  STB3_PIN		GPIO_Pin_12
#define  STB4_PIN		GPIO_Pin_13
#define  STB5_PIN		GPIO_Pin_14
#define  STB6_PIN		GPIO_Pin_15

#define  MTA_PORT	GPIOA
#define  MTAP_PIN	GPIO_Pin_0
#define  MTAM_PIN	GPIO_Pin_1

#define  MTB_PORT	GPIOC
#define  MTBP_PIN	GPIO_Pin_14
#define  MTBM_PIN	GPIO_Pin_15

#define  NO_PAPER_OUT_PORT	GPIOB
#define  NO_PAPER_OUT_PIN	GPIO_Pin_8

#define  PHE_PORT	GPIOA
#define  PHE_PIN	GPIO_Pin_8

#define PRINTER_POWER_PORT	GPIOC
#define PRINTER_POWER_PIN	GPIO_Pin_13

/*�յ��Ĵ�ӡ���ݵı���������з�*/
#define PRINTER_DATA_SIZE 2048
unsigned char	printer_data[PRINTER_DATA_SIZE];
RingBuffer		rb_printer_data;


/*
   Ҫ��ӡ��ͼ�� 24x24dot 24*3=72byte
   384dot/line  384/8=48
   �� 24�У�ÿ��384dot=48byte
   ������������߽磬�ұ߽磬���Բ�����8bit��1byte����ķ�ʽ
 */

#define GLYPH_ROW	24
#define GLYPH_COL	48

//static unsigned char print_glyph[GLYPH_ROW][GLYPH_COL] __attribute__( ( at( 0x20001000 ) ) ) = { 0 };
static unsigned char	print_glyph[GLYPH_ROW][GLYPH_COL] = { 0 };

static struct rt_device dev_printer;

struct _PRINTER_PARAM
{
	uint16_t	line_space;                 //�м��
	uint16_t	gray_level;                 //�Ҷ�,����ʱ��
	uint16_t	margin_left;                //��߽�
	uint16_t	margin_right;               //�ұ߽�
} printer_param =
{
	4, 4, 0, 0
};

static unsigned short	dotremain = 384;    //����ʹ�õ�dot��ÿ����24dot ÿascii 12dots
static unsigned char	print_str[36];      //Ҫ��ӡ�ĵ�������ֽ��� ȫascii 384/12=32  ���� 384/24*3
static unsigned char	print_str_len = 0;

/*Ҫ��Ҫ4�ֽڶ���*/
static unsigned char	font_glyph[80];     //ÿ�����ֻ�ascii������ֽ��� 24x24dot = 72bytes

static u8				fac_us	= 0;        //us��ʱ������
static u16				fac_ms	= 0;        //ms��ʱ������
//��ʼ���ӳٺ���
//SYSTICK��ʱ�ӹ̶�ΪHCLKʱ�ӵ�1/8
//SYSCLK:ϵͳʱ��
void delay_init( u8 SYSCLK )
{
	SysTick->CTRL	&= 0xfffffffb;          //bit2���,ѡ���ⲿʱ��  HCLK/8
	fac_us			= SYSCLK / 8;
	fac_ms			= (u16)fac_us * 1000;
}

//��ʱnms
//ע��nms�ķ�Χ
//SysTick->LOADΪ24λ�Ĵ���,����,�����ʱΪ:
//nms<=0xffffff*8*1000/SYSCLK
//SYSCLK��λΪHz,nms��λΪms
//��72M������,nms<=1864
void delay_ms1( u16 nms )
{
	u32 temp;
	SysTick->LOAD	= (u32)nms * fac_ms;                //ʱ�����(SysTick->LOADΪ24bit)
	SysTick->VAL	= 0x00;                             //��ռ�����
	SysTick->CTRL	= 0x01;                             //��ʼ����
	do
	{
		temp = SysTick->CTRL;
	}
	while( temp & 0x01 && !( temp & ( 1 << 16 ) ) );    //�ȴ�ʱ�䵽��
	SysTick->CTRL	= 0x00;                             //�رռ�����
	SysTick->VAL	= 0X00;                             //��ռ�����
}

//��ʱnus
//nusΪҪ��ʱ��us��.
void delay_us( u32 nus )
{
	u32 temp;
	SysTick->LOAD	= nus * fac_us;                     //ʱ�����
	SysTick->VAL	= 0x00;                             //��ռ�����
	SysTick->CTRL	= 0x01;                             //��ʼ����
	do
	{
		temp = SysTick->CTRL;
	}
	while( temp & 0x01 && !( temp & ( 1 << 16 ) ) );    //�ȴ�ʱ�䵽��
	SysTick->CTRL	= 0x00;                             //�رռ�����
	SysTick->VAL	= 0X00;                             //��ռ�����
}

/***********************************************************
* Function:       // ��������
* Description:    // �������ܡ����ܵȵ�����
* Input:          // 1.�������1��˵��������ÿ�����������á�ȡֵ˵�����������ϵ
* Input:          // 2.�������2��˵��������ÿ�����������á�ȡֵ˵�����������ϵ
* Output:         // 1.�������1��˵��
* Return:         // ��������ֵ��˵��
* Others:         // ����˵��
***********************************************************/
void printer_stop( void )
{
	GPIO_ResetBits( MTA_PORT, MTAM_PIN | MTAP_PIN );
	GPIO_ResetBits( MTB_PORT, MTBM_PIN | MTBP_PIN );
}

static uint8_t fprinting = 0;     //�Ƿ����ڴ�ӡ


/***********************************************************
* Function:       // ��������
* Description:    // �������ܡ����ܵȵ�����
* Input:          // 1.�������1��˵��������ÿ�����������á�ȡֵ˵�����������ϵ
* Input:          // 2.�������2��˵��������ÿ�����������á�ȡֵ˵�����������ϵ
* Output:         // 1.�������1��˵��
* Return:         // ��������ֵ��˵��
* Others:         // ����˵��
***********************************************************/
static void printer_port_init( void )
{
	GPIO_InitTypeDef gpio_init;
	RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, ENABLE );

	gpio_init.GPIO_Mode		= GPIO_Mode_Out_PP;
	gpio_init.GPIO_Speed	= GPIO_Speed_2MHz;

	gpio_init.GPIO_Pin = PRINT_DATA_PIN | PRINT_CLK_PIN;
	GPIO_Init( DATA_CLK_PORT, &gpio_init );
	GPIO_ResetBits( DATA_CLK_PORT, PRINT_DATA_PIN | PRINT_CLK_PIN );

	gpio_init.GPIO_Pin = STB6_PIN | STB5_PIN | STB4_PIN | STB3_PIN | STB2_PIN;
	GPIO_Init( STB2_6_PORT, &gpio_init );
	GPIO_ResetBits( STB2_6_PORT, STB6_PIN | STB5_PIN | STB4_PIN | STB3_PIN | STB2_PIN );

	gpio_init.GPIO_Pin = STB1_PIN;
	GPIO_Init( STB1_PORT, &gpio_init );
	GPIO_ResetBits( STB1_PORT, STB1_PIN );

	gpio_init.GPIO_Pin = LAT_PIN;
	GPIO_Init( LAT_PORT, &gpio_init );
	GPIO_ResetBits( LAT_PORT, LAT_PIN );

	gpio_init.GPIO_Pin = MTAP_PIN | MTAM_PIN;
	GPIO_Init( MTA_PORT, &gpio_init );
	GPIO_ResetBits( MTA_PORT, MTAP_PIN | MTAM_PIN );

	gpio_init.GPIO_Pin = MTBP_PIN | MTBM_PIN;
	GPIO_Init( MTB_PORT, &gpio_init );
	GPIO_ResetBits( MTB_PORT, MTBP_PIN | MTBM_PIN );

//	gpio_init.GPIO_Pin = LED_PIN;
//	GPIO_Init( LED_PORT, &gpio_init );

	gpio_init.GPIO_Pin = NO_PAPER_OUT_PIN;
	GPIO_Init( NO_PAPER_OUT_PORT, &gpio_init );
	GPIO_ResetBits( NO_PAPER_OUT_PORT, NO_PAPER_OUT_PIN );

	gpio_init.GPIO_Pin = PRINTER_POWER_PIN;
	GPIO_Init( PRINTER_POWER_PORT, &gpio_init );
	GPIO_ResetBits( PRINTER_POWER_PORT, PRINTER_POWER_PIN );

	printer_stop( );

	gpio_init.GPIO_Pin	= PHE_PIN;
	gpio_init.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init( PHE_PORT, &gpio_init );
}

#define  mta_print		GPIOA       //PA0
#define  mta_print_pin	GPIO_Pin_0  //PA0
#define  mtaf_print		GPIOA       //PA1
#define  mtaf_print_pin GPIO_Pin_1  //PA1
#define  mtb_print		GPIOC       //PA11
#define  mtb_print_pin	GPIO_Pin_14 //PA11
#define  mtbf_print		GPIOC       //PA12
#define  mtbf_print_pin GPIO_Pin_15 //PA12


/***********************************************************
* Function:       // ��������
* Description:    // �������ܡ����ܵȵ�����
* Input:          // 1.�������1��˵��������ÿ�����������á�ȡֵ˵�����������ϵ
* Input:          // 2.�������2��˵��������ÿ�����������á�ȡֵ˵�����������ϵ
* Output:         // 1.�������1��˵��
* Return:         // ��������ֵ��˵��
* Others:         // ����˵��
***********************************************************/
void drivers1( void )
{
	GPIO_SetBits( mta_print, mta_print_pin );
	GPIO_ResetBits( mtb_print, mtb_print_pin );
	GPIO_ResetBits( mtaf_print, mtaf_print_pin );
	GPIO_SetBits( mtbf_print, mtbf_print_pin );
	delay_us( 500 );
	GPIO_ResetBits( mta_print, mta_print_pin );
	GPIO_ResetBits( mtb_print, mtb_print_pin );
	GPIO_ResetBits( mtaf_print, mtaf_print_pin );
	GPIO_SetBits( mtbf_print, mtbf_print_pin );
	delay_us( 500 );
	GPIO_ResetBits( mta_print, mta_print_pin );
	GPIO_ResetBits( mtb_print, mtb_print_pin );
	GPIO_SetBits( mtaf_print, mtaf_print_pin );
	GPIO_SetBits( mtbf_print, mtbf_print_pin );
	delay_us( 500 );
	GPIO_ResetBits( mta_print, mta_print_pin );
	GPIO_ResetBits( mtb_print, mtb_print_pin );
	GPIO_SetBits( mtaf_print, mtaf_print_pin );
	GPIO_ResetBits( mtbf_print, mtbf_print_pin );
	delay_us( 500 );
}

/***********************************************************
* Function:       // ��������
* Description:    // �������ܡ����ܵȵ�����
* Input:          // 1.�������1��˵��������ÿ�����������á�ȡֵ˵�����������ϵ
* Input:          // 2.�������2��˵��������ÿ�����������á�ȡֵ˵�����������ϵ
* Output:         // 1.�������1��˵��
* Return:         // ��������ֵ��˵��
* Others:         // ����˵��
***********************************************************/
void drivers2( void )
{
	GPIO_ResetBits( mta_print, mta_print_pin );
	GPIO_SetBits( mtb_print, mtb_print_pin );
	GPIO_SetBits( mtaf_print, mtaf_print_pin );
	GPIO_ResetBits( mtbf_print, mtbf_print_pin );
	delay_us( 500 );
	GPIO_ResetBits( mta_print, mta_print_pin );
	GPIO_SetBits( mtb_print, mtb_print_pin );
	GPIO_ResetBits( mtaf_print, mtaf_print_pin );
	GPIO_ResetBits( mtbf_print, mtbf_print_pin );
	delay_us( 500 );
	GPIO_SetBits( mta_print, mta_print_pin );
	GPIO_SetBits( mtb_print, mtb_print_pin );
	GPIO_ResetBits( mtaf_print, mtaf_print_pin );
	GPIO_ResetBits( mtbf_print, mtbf_print_pin );
	delay_us( 500 );
	GPIO_SetBits( mta_print, mta_print_pin );
	GPIO_ResetBits( mtb_print, mtb_print_pin );
	GPIO_ResetBits( mtaf_print, mtaf_print_pin );
	GPIO_ResetBits( mtbf_print, mtbf_print_pin );
	delay_us( 500 );
}

/*
   �Ҷȼ��𣬼�����ʱ��ʱ��Խ����ɫԽ��
   ԭ��
   level   delay
   1       2000
   2       3000
   3       4000
   ...
   8       9000

   ���ڸĳ�1024
 */
static void printer_heatdelay( void )
{
	delay_us( ( param_gray_level + 1 ) << 10 ); //����1024��
}

/*
   ��ӡprinter_data�е����ݣ�һ���ɴ�ӡ���ַ���
   �����ַ��еĳ��ȣ���Ϊ�˵���������ʱ
 */
void printer_print_glyph( unsigned char len )
{
	unsigned char	*p;
	unsigned char	b, c, row, col_byte;
/*ȱֽ��� PA8 ȱֽΪ��*/
	if( GPIO_ReadInputDataBit( PHE_PORT, PHE_PIN ) )
	{
		GPIO_SetBits( NO_PAPER_OUT_PORT, NO_PAPER_OUT_PIN );
		return;
	}else
	{
		GPIO_ResetBits( NO_PAPER_OUT_PORT, NO_PAPER_OUT_PIN );
	}

	GPIO_SetBits( PRINTER_POWER_PORT, PRINTER_POWER_PIN );

	for( row = 0; row < 12; row++ )
	{
		p = print_glyph[row * 2 + 0];
		for( col_byte = 0; col_byte < GLYPH_COL; col_byte++ )
		{
			c = *p++;
			for( b = 0; b < 8; b++ )
			{
				GPIO_ResetBits( DATA_CLK_PORT, PRINT_CLK_PIN );

				if( 0x80 == ( c & 0x80 ) )
				{
					GPIO_SetBits( DATA_CLK_PORT, PRINT_DATA_PIN );
				}else
				{
					GPIO_ResetBits( DATA_CLK_PORT, PRINT_DATA_PIN );
				}
				c <<= 1;
				GPIO_SetBits( DATA_CLK_PORT, PRINT_CLK_PIN );
			}
		}
		GPIO_ResetBits( LAT_PORT, LAT_PIN );
		GPIO_SetBits( LAT_PORT, LAT_PIN );

		GPIO_SetBits( STB1_PORT, STB1_PIN );
		GPIO_SetBits( STB2_6_PORT, STB2_PIN | STB3_PIN );
		printer_heatdelay( );
		GPIO_ResetBits( STB1_PORT, STB1_PIN );
		GPIO_ResetBits( STB2_6_PORT, STB2_PIN | STB3_PIN );

		GPIO_SetBits( STB2_6_PORT, STB4_PIN | STB5_PIN | STB6_PIN );
		printer_heatdelay( );
		GPIO_ResetBits( STB2_6_PORT, STB4_PIN | STB5_PIN | STB6_PIN );
		drivers1( );

		p = print_glyph[row * 2 + 1];
		for( col_byte = 0; col_byte < GLYPH_COL; col_byte++ )
		{
			c = *p++;
			for( b = 0; b < 8; b++ )
			{
				GPIO_ResetBits( DATA_CLK_PORT, PRINT_CLK_PIN );

				if( 0x80 == ( c & 0x80 ) )
				{
					GPIO_SetBits( DATA_CLK_PORT, PRINT_DATA_PIN );
				}else
				{
					GPIO_ResetBits( DATA_CLK_PORT, PRINT_DATA_PIN );
				}
				c <<= 1;
				GPIO_SetBits( DATA_CLK_PORT, PRINT_CLK_PIN );
			}
		}
		GPIO_ResetBits( LAT_PORT, LAT_PIN );
		GPIO_SetBits( LAT_PORT, LAT_PIN );

		GPIO_SetBits( STB1_PORT, STB1_PIN );
		GPIO_SetBits( STB2_6_PORT, STB2_PIN | STB3_PIN );
		printer_heatdelay( );
		GPIO_ResetBits( STB1_PORT, STB1_PIN );
		GPIO_ResetBits( STB2_6_PORT, STB2_PIN | STB3_PIN );

		GPIO_SetBits( STB2_6_PORT, STB4_PIN | STB5_PIN | STB6_PIN );
		printer_heatdelay( );
		GPIO_ResetBits( STB2_6_PORT, STB4_PIN | STB5_PIN | STB6_PIN );

		drivers2( );
	}

	for( col_byte = 0; col_byte < param_line_space; col_byte++ )
	{
		drivers1( );
		drivers2( );
	}

	printer_stop( );
	GPIO_ResetBits( PRINTER_POWER_PORT, PRINTER_POWER_PIN );
}

/*
   Merge bits from two values according to a mask
   unsigned int a;    // value to merge in non-masked bits
   unsigned int b;    // value to merge in masked bits
   unsigned int mask; // 1 where bits from b should be selected; 0 where from a.
   unsigned int r;    // result of (a & ~mask) | (b & mask) goes here

   r = a ^ ((a ^ b) & mask);

 */

void spi_get_str_glyph_print( unsigned char *pstr, unsigned char len )
{
	unsigned long	addr;
	unsigned char	charnum = len;
	unsigned char	* p		= pstr;
	unsigned char	msb, lsb;
	unsigned int	start_col = param_margin_left;

	unsigned char	row, col, offset;
	unsigned int	val_old, val_new, val_mask, val_ret;

	for( row = 0; row < GLYPH_ROW; row++ )
	{
		for( col = 0; col < GLYPH_COL; col++ )
		{
			print_glyph[row][col] = 0;
		}
	}

	while( charnum )
	{
		for( row = 0; row < 80; row++ )
		{
			font_glyph[row] = 0;
		}
		msb = *p++;
		charnum--;
		if( msb <= 0x80 )
		{
			addr = ( msb - 0x20 ) * 48 + 507600;
			spi_read( addr, 48, font_glyph );
			for( row = 0; row < 24; row++ )
			{
				val_new = ( font_glyph[row * 2] << 24 ) | ( font_glyph[row * 2 + 1] << 16 );    //�������ֿ�2byte��ֻ�и�12λ��Ч�������

				col		= start_col >> 3;                                                       //�������ڵ����ֽ� [0..7]��Ӧ�ֽ�0
				val_old = ( print_glyph[row][col] << 24 ) | ( print_glyph[row][col + 1] << 16 ) | ( print_glyph[row][col + 2] << 8 );

				offset						= start_col & 0x07;
				val_new						>>= offset;
				val_mask					= ( 0xFFF00000 >> offset );                         //12bit��1��mask
				val_ret						= val_old ^ ( ( val_new ^ val_old ) & val_mask );
				print_glyph[row][col]		= ( val_ret >> 24 ) & 0xff;
				print_glyph[row][col + 1]	= ( val_ret >> 16 ) & 0xff;
				print_glyph[row][col + 2]	= ( val_ret >> 8 ) & 0xff;
			}
			start_col += 12;
		}else
		{
			lsb = *p++;
			charnum--;
			if( ( msb >= 0xa1 ) && ( msb <= 0xa3 ) && ( lsb >= 0xa1 ) )
			{
				addr = ( ( ( (unsigned long)msb ) - 0xa1 ) * 94 + ( ( (unsigned long)lsb ) - 0xa1 ) ) * 72;
				spi_read( addr, 72, font_glyph );
			}else if( ( msb >= 0xb0 ) && ( msb <= 0xf7 ) && ( lsb >= 0xa1 ) )
			{
				addr = ( ( ( (unsigned long)msb ) - 0xb0 ) * 94 + ( ( (unsigned long)lsb ) - 0xa1 ) ) * 72 + 282 * 72;
				spi_read( addr, 72, font_glyph );
			}
			for( row = 0; row < 24; row++ )
			{
				val_new = ( font_glyph[row * 3] << 24 ) | ( font_glyph[row * 3 + 1] << 16 ) | ( font_glyph[row * 3 + 2] << 8 ); //�������ֿ�24byte

				col		= start_col >> 3;                                                                                       //�������ڵ����ֽ� [0..7]��Ӧ�ֽ�0
				val_old = ( print_glyph[row][col] << 24 ) | ( print_glyph[row][col + 1] << 16 ) | ( print_glyph[row][col + 2] << 8 ) | print_glyph[row][col + 3];

				offset						= start_col & 0x07;
				val_new						>>= offset;
				val_mask					= ( 0xFFFFFF00 >> offset );                                                         //24bit��1��mask
				val_ret						= val_old ^ ( ( val_new ^ val_old ) & val_mask );
				print_glyph[row][col]		= ( val_ret >> 24 ) & 0xff;
				print_glyph[row][col + 1]	= ( val_ret >> 16 ) & 0xff;
				print_glyph[row][col + 2]	= ( val_ret >> 8 ) & 0xff;
				print_glyph[row][col + 3]	= val_ret & 0xff;
			}
			start_col += 24;
		}
	}


/*
   for(row=0;row<24;row++)
   {
   printf("\r\n");
   for(col=0;col<48;col++) printf("%02x ",print_glyph[row][col]);
   }
 */
	//������ӡ
	printer_print_glyph( len );
}

/*
   �����յ������������ڴ�ӡ�����������

   ����̴���
   ���ݱ���(��û���㹻�Ŀռ䣬�򷵻�ʧ��)
 */
int printer_rx( unsigned char *pdata, unsigned int len )
{
	int				i, j;
	unsigned char	c1, c2;
	//extern void uart2_send_frame( unsigned char chn, unsigned char *pdata, unsigned short datalen );

/*�����������*/
	if( pdata[0] == 0x1b )
	{
		c1 = pdata[1];
		if( c1 == 0x4a ) //��ӡ����ֽn����
		{
			for( i = 0; i < pdata[2]; i++ )
			{
				drivers1( );
				drivers2( );
				printer_stop( );
			}
		}
		if( c1 == 0x64 ) //��ӡ����ֽn�ַ���
		{
			for( i = 0; i < pdata[2] * 24; i++ )
			{
				drivers1( );
				drivers2( );
				printer_stop( );
			}
		}
		if( c1 == 0x33 ) //�����м��n����
		{
			c2 = pdata[2];
			if( c2 > 29 )
			{
				c2 = c2 % 29;
			} else
			{
				c2 = 4;
			}
			param_line_space = c2;
			EE_WriteVariable( VirtAddVarTab[0], param_line_space );
		}

		if( c1 == 0x32 )    //�����м��4����(Ĭ��ֵ)
		{
			EE_WriteVariable( VirtAddVarTab[0], 4 );
			param_line_space = 4;
		}
		if( c1 == 0x6d )    //��ӡ�Ҷ� 8�� 1-8
		{
			c2 = pdata[2];
			if( ( c2 < 1 ) && ( c2 > 8 ) )
			{
				c2 = 4;
			}
			param_gray_level = c2;
			EE_WriteVariable( VirtAddVarTab[1], param_gray_level );
		}
		if( c1 == 0x6c ) //������߾�
		{
			c2 = pdata[2];
			if( c2 > 29 )
			{
				c2 = c2 % 29;
			}
			param_margin_left = c2;
			EE_WriteVariable( VirtAddVarTab[2], param_margin_left );
		}
		if( c1 == 0x51 ) //�����ұ߾�
		{
			c2 = pdata[2];
			if( c2 > 29 )
			{
				c2 = c2 % 29;
			}
			param_margin_right = c2;
			EE_WriteVariable( VirtAddVarTab[3], param_margin_right );
		}
		if( ( c1 == 0x28 ) && ( pdata[2] == 0x45 ) ) //������
		{
			j = 0;
			for( i = 3; i < len; i++ )
			{
				j	= j * 10;
				j	+= ( pdata[i] - 0x30 );
			}
			param_margin_baud = j / 100;
			EE_WriteVariable( VirtAddVarTab[4], param_margin_baud );
		}
		if( c1 == 0x40 ) //��������
		{
			//EE_Format();
			param_line_space	= 4;
			param_gray_level	= 4;
			param_margin_left	= 0;
			param_margin_right	= 0;
			param_margin_baud	= 96;
			EE_WriteVariable( VirtAddVarTab[0], param_line_space );
			EE_WriteVariable( VirtAddVarTab[1], param_gray_level );
			EE_WriteVariable( VirtAddVarTab[2], param_margin_left );
			EE_WriteVariable( VirtAddVarTab[3], param_margin_right );
			EE_WriteVariable( VirtAddVarTab[4], param_margin_baud );
		}
		return 0;
	}
/*״̬��ѯ ����:0  ȱֽ:0x04*/
	if( ( len == 3 ) && ( pdata[0] == 0x10 ) && ( pdata[1] == 0x04 ) && ( pdata[2] == 0x05 ) )
	{
		if( GPIO_ReadInputDataBit( PHE_PORT, PHE_PIN ) )
		{
			GPIO_SetBits( NO_PAPER_OUT_PORT, NO_PAPER_OUT_PIN );
			uart2_send_frame( '1', 1, "\x04", 1 );
		}else
		{
			GPIO_ResetBits( NO_PAPER_OUT_PORT, NO_PAPER_OUT_PIN );
			uart2_send_frame( '1', 1, "\x00", 1 );
		}

		return 0;
	}

/*���ݰ������з�*/
	return ringbuffer_put_data( &rb_printer_data, len, pdata );
}

/*
   �����Եļ���ӡͨ��������,�γ�Ҫ��ӡ��һ������

   GBK �����˫�ֽڱ�ʾ��������뷶ΧΪ8140-FEFE��
   ���ֽ���81-FE ֮�䣬β�ֽ���40-FE ֮�䣬�޳� xx7Fһ���ߡ�
   �ܼ�23940 ����λ��������21886�����ֺ�ͼ�η��ţ�
   ���к��֣��������׺͹�����21003 ����ͼ�η���883 ����


   ������24x24  ASCII��12x24

   �ж�һ�еĽ���

   1.����<CR><LF>
   2.һ���� Ӧ�ü���dotֵ��������byteֵ��һ������24dot (2byte) һ��ASC 12dot(1.5byte)
   3.������ 0xffff��Ϊ��ӡ������־


   ������ʵ�����ǻ��һ���ɴ�ӡ�е����ݡ�
 */
void printer_proc( void )
{
	unsigned char CH, CL;

	if( fprinting )
	{
		return;
	}

	while( ringbuffer_get( &rb_printer_data, &CH ) == 0 )
	{
		if( CH == 0x00 )
		{
			continue;                               // gb2312��ascii:  0x00 0x31
		}
		if( CH < 0x80 )                             //ASCII
		{
			if( ( CH == 0x0d ) || ( CH == 0x0a ) )  //����
			{
				if( print_str_len > 0 )
				{
					spi_get_str_glyph_print( print_str, print_str_len );
				}
				print_str_len	= 0;
				dotremain		= 384 - param_margin_left - param_margin_right;
			}else
			{
				if( dotremain >= 12 )       //���ɴ�ӡһ��ascii
				{
					print_str[print_str_len++]	= CH;
					dotremain					-= 12;
					if( dotremain < 12 )    //ʣ�²����һ��ascii,��ӡ�ɡ�
					{
						spi_get_str_glyph_print( print_str, print_str_len );
						print_str_len	= 0;
						dotremain		= 384 - param_margin_left - param_margin_right;
					}
				}else //�������Ӧ��ִ�в���.��Ҫ����Ӣ����ʱ,ʣ�������12-24 ֮����Ҫ��ӡ����ʱ������
				{
					spi_get_str_glyph_print( print_str, print_str_len );
					print_str[0]	= CH;
					print_str_len	= 1;
					dotremain		= 384 - param_margin_left - param_margin_right - 12;
				}
			}
		}else if( ( CH > 0x80 ) && ( CH < 0xff ) ) //GBK���룬��֤ȡ������,���ã���һ����������
		{
			ringbuffer_get( &rb_printer_data, &CL );
			if( ( CL >= 0x40 ) && ( CL <= 0xfe ) )
			{
				if( dotremain >= 24 )
				{
					print_str[print_str_len++]	= CH;
					print_str[print_str_len++]	= CL;
					dotremain					-= 24;
					if( dotremain < 12 ) //ʣ�²����һ��ascii,��ӡ�ɡ�
					{
						spi_get_str_glyph_print( print_str, print_str_len );
						print_str_len	= 0;
						dotremain		= 384 - param_margin_left - param_margin_right;
					}
				}else //Ӧ�ÿ��ԷŸ�ASCII,������˸����� ;-(
				{
					spi_get_str_glyph_print( print_str, print_str_len );
					print_str[0]	= CH;
					print_str[1]	= CL;
					print_str_len	= 2;
					dotremain		= 384 - param_margin_left - param_margin_right - 24;
				}
			}
		}else if( CH == 0xff ) //������ 0xFFFF
		{
			if( print_str_len > 0 )
			{
				spi_get_str_glyph_print( print_str, print_str_len );
			}
			print_str_len	= 0;
			dotremain		= 384 - param_margin_left - param_margin_right;
			//todo �ϵ�
		}
	}
}

/***********************************************************
* Function:       // printer_load_param
* Description:    // ��ȡ����
* Input:          // ��
* Output:         // ��
* Return:         // ��
* Others:         // ����˵��
***********************************************************/
static void printer_load_param( void )
{
}

/***********************************************************
* Function:       // ��������
* Description:    // �������ܡ����ܵȵ�����
* Input:          // 1.�������1��˵��������ÿ�����������á�ȡֵ˵�����������ϵ
* Input:          // 2.�������2��˵��������ÿ�����������á�ȡֵ˵�����������ϵ
* Output:         // 1.�������1��˵��
* Return:         // ��������ֵ��˵��
* Others:         // ����˵��
***********************************************************/
static void printer_save_param( void )
{
}




static rt_err_t printer_init(rt_dev_t dev)
{
	delay_init( 72 );

	ringbuffer_init( &rb_printer_data, PRINTER_DATA_SIZE, printer_data );

	printer_load_param( );
	dotremain = 384 - printer_param.margin_left - printer_param.margin_right; //�µ�һ�пɴ�ӡ�ַ�������
	//spi_font_init( );
	printer_port_init( );
	printer_stop( );

	return RT_EOK;

}

static rt_err_t printer_open( rt_device_t dev, rt_uint16_t oflag )
{
	return RT_EOK;
}

static rt_size_t printer_read( rt_device_t dev, rt_off_t pos, void* buff, rt_size_t count )
{
	return RT_EOK;

}

static rt_size_t printer_write( rt_device_t dev, rt_off_t pos, const void* buff, rt_size_t count )
{




	return RT_EOK;
}

static rt_err_t printer_control( rt_device_t dev, rt_uint8_t cmd, void *arg )
{
	uint8_t code=*(uint8_t *)arg;
	int i;
	switch(cmd)
	{
		case PRINTER_CMD_CHRLINE_N:
			for( i = 0; i < code; i++ )
			{
				drivers1( );
				drivers2( );
				printer_stop( );
			}
			break;
		case PRINTER_CMD_DOTLINE_N:
			for( i = 0; i < code* 24; i++ )
			{
				drivers1( );
				drivers2( );
				printer_stop( );
			}
			break;
		case PRINTER_CMD_FACTORY:
			break;
		case PRINTER_CMD_GRAYLEVEL:
			break;
		case PRINTER_CMD_LINESPACE_DEFAULT:
			break;
		case PRINTER_CMD_LINESPACE_N:
			if( code > 29 )
			{
				code = code % 29;
			} else
			{
				code = 4;
			}
			break;
		case PRINTER_CMD_MARGIN_LEFT:
			break;
		case PRINTER_CMD_MARGIN_RIGHT:
			break;
	}
	return RT_EOK;

}

static rt_err_t printer_close( rt_device_t dev )
{
	return RT_EOK;

}
/***********************************************************
* Function:       // ��������
* Description:    // �������ܡ����ܵȵ�����
* Input:          // 1.�������1��˵��������ÿ�����������á�ȡֵ˵�����������ϵ
* Input:          // 2.�������2��˵��������ÿ�����������á�ȡֵ˵�����������ϵ
* Output:         // 1.�������1��˵��
* Return:         // ��������ֵ��˵��
* Others:         // ����˵��
***********************************************************/
void printer_driver_init( void )
{
	dev_printer.type		= RT_Device_Class_Char;
	dev_printer.init		= printer_init;
	dev_printer.open		= printer_open;
	dev_printer.close		= printer_close;
	dev_printer.read		= printer_read;
	dev_printer.write		= printer_write;
	dev_printer.control		= printer_control;
	dev_printer.user_data	= RT_NULL;

	rt_device_register( &dev_printer, "printer", RT_DEVICE_FLAG_WRONLY);
	rt_device_init(&dev_printer );
}

/************************************** The End Of File **************************************/
