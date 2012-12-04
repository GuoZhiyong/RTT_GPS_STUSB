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
#include "board.h"
#include "ringbuffer.h"
#include "printer.h"
#include <finsh.h>

/*��ӡͷ���*/

//#define PAPER_DETCET_PORT	GPIOA
//#define PAPER_DETECT_PIN	GPIO_Pin_8

#define  CLK_PORT	GPIOE
#define  CLK_PIN	GPIO_Pin_0
#define  DI_PORT	GPIOB
#define  DI_PIN		GPIO_Pin_9

#define  LAT_PORT	GPIOC
#define  LAT_PIN	GPIO_Pin_13

#define  STB1_3_PORT	GPIOE
#define  STB1_3_PIN		GPIO_Pin_6

#define  STB4_6_PORT	GPIOE
#define  STB4_6_PIN		GPIO_Pin_4

#define  MTA_PORT	GPIOE
#define  MTA_PIN	GPIO_Pin_3

#define  MTAF_PORT	GPIOE
#define  MTAF_PIN	GPIO_Pin_5

#define  MTB_PORT	GPIOE
#define  MTB_PIN	GPIO_Pin_1

#define  MTBF_PORT	GPIOE
#define  MTBF_PIN	GPIO_Pin_2

//#define  NO_PAPER_OUT_PORT	GPIOB
//#define  NO_PAPER_OUT_PIN	GPIO_Pin_8

#define  PHE_PORT	GPIOD
#define  PHE_PIN	GPIO_Pin_8

#define PRINTER_POWER_PORT_3V3	GPIOD
#define PRINTER_POWER_PIN_3V3	GPIO_Pin_4

#define PRINTER_POWER_PORT_5V	GPIOB
#define PRINTER_POWER_PIN_5V	GPIO_Pin_7



#define MTA_H	( MTA_PORT->BSRRL = MTA_PIN )
#define MTA_L	( MTA_PORT->BSRRH = MTA_PIN )

#define MTAF_H	( MTAF_PORT->BSRRL = MTAF_PIN )
#define MTAF_L	( MTAF_PORT->BSRRH = MTAF_PIN )

#define MTB_H	( MTB_PORT->BSRRL = MTB_PIN )
#define MTB_L	( MTB_PORT->BSRRH = MTB_PIN )

#define MTBF_H	( MTBF_PORT->BSRRL = MTBF_PIN )
#define MTBF_L	( MTBF_PORT->BSRRH = MTBF_PIN )

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
static struct rt_timer	tmr_printer;

struct _PRINTER_PARAM
{
	uint16_t	step_delay;                 //������ʱ,Ӱ���м��
	uint16_t	gray_level;                 //�Ҷȵȼ�,����ʱ��
	uint32_t	heat_delay[4];              //������ʱ
	uint16_t	line_space;                 //�м��
	uint16_t	margin_left;                //��߽�
	uint16_t	margin_right;               //�ұ߽�
} printer_param =
{
	1000, 2, { 5000, 10000, 15000, 20000 }, 4, 0, 0
};

static unsigned short	dotremain = 384;    //����ʹ�õ�dot��ÿ����24dot ÿascii 12dots
static unsigned char	print_str[36];      //Ҫ��ӡ�ĵ�������ֽ��� ȫascii 384/12=32  ���� 384/24*3
static unsigned char	print_str_len = 0;

/*Ҫ��Ҫ4�ֽڶ���*/
static unsigned char font_glyph[80];        //ÿ�����ֻ�ascii������ֽ��� 24x24dot = 72bytes

//��ʱnus
//nusΪҪ��ʱ��us��.


/*
   static void delay_us (const uint32_t usec)
   {

   __IO uint32_t count = 0;
   const uint32_t utime = (120 * usec / 7);
   do
   {
    if ( ++count > utime )
    {
      return ;
    }
   }while (1);
   }
 */

static void delay_us( const uint32_t usec )
{
	__IO uint32_t	count	= 0;
	const uint32_t	utime	= ( 168 * usec / 7 );
	do
	{
		if( ++count > utime )
		{
			return;
		}
	}
	while( 1 );
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
	MTA_L;
	MTAF_L;
	MTB_L;
	MTBF_L;
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

	RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOB |RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOD | RCC_AHB1Periph_GPIOE, ENABLE );
	/*ȥ��PB4 JTAG����*/
	//GPIOB->AFR[0]|=(1<<16);
	GPIO_PinAFConfig( GPIOB, GPIO_Pin_4, 1 );

	gpio_init.GPIO_Mode		= GPIO_Mode_OUT;
	gpio_init.GPIO_OType	= GPIO_OType_PP;
	gpio_init.GPIO_Speed	= GPIO_Speed_50MHz;
	gpio_init.GPIO_PuPd		= GPIO_PuPd_NOPULL;

	gpio_init.GPIO_Pin = DI_PIN;
	GPIO_Init( DI_PORT, &gpio_init );
	GPIO_ResetBits( DI_PORT, DI_PIN );

	gpio_init.GPIO_Pin = CLK_PIN;
	GPIO_Init( CLK_PORT, &gpio_init );
	GPIO_ResetBits( CLK_PORT, CLK_PIN );

	gpio_init.GPIO_Pin = STB4_6_PIN;
	GPIO_Init( STB4_6_PORT, &gpio_init );
	GPIO_ResetBits( STB4_6_PORT, STB4_6_PIN );

	gpio_init.GPIO_Pin = STB1_3_PIN;
	GPIO_Init( STB1_3_PORT, &gpio_init );
	GPIO_ResetBits( STB1_3_PORT, STB1_3_PIN );

	gpio_init.GPIO_Pin = LAT_PIN;
	GPIO_Init( LAT_PORT, &gpio_init );
	GPIO_ResetBits( LAT_PORT, LAT_PIN );

	gpio_init.GPIO_Pin = MTA_PIN;
	GPIO_Init( MTA_PORT, &gpio_init );
	MTA_L;

	gpio_init.GPIO_Pin = MTAF_PIN;
	GPIO_Init( MTAF_PORT, &gpio_init );
	MTAF_L;

	gpio_init.GPIO_Pin = MTB_PIN;
	GPIO_Init( MTB_PORT, &gpio_init );
	MTB_L;

	gpio_init.GPIO_Pin = MTBF_PIN;
	GPIO_Init( MTBF_PORT, &gpio_init );
	MTBF_L;

	gpio_init.GPIO_Pin = PRINTER_POWER_PIN_5V;
	GPIO_Init( PRINTER_POWER_PORT_5V, &gpio_init );
	GPIO_ResetBits( PRINTER_POWER_PORT_5V, PRINTER_POWER_PIN_5V);

	gpio_init.GPIO_Pin = PRINTER_POWER_PIN_3V3;
	GPIO_Init( PRINTER_POWER_PORT_3V3, &gpio_init );
	GPIO_SetBits( PRINTER_POWER_PORT_3V3, PRINTER_POWER_PIN_3V3);


	gpio_init.GPIO_Pin	= PHE_PIN;
	gpio_init.GPIO_Mode = GPIO_Mode_IN;
	GPIO_Init( PHE_PORT, &gpio_init );
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
void drivers1( void )
{
	MTA_H; MTAF_L; MTB_L; MTBF_H;
	delay_us( printer_param.step_delay );
	MTA_L; MTAF_L; MTB_L; MTBF_H;
	delay_us( printer_param.step_delay );
	MTA_L; MTAF_H; MTB_L; MTBF_H;
	delay_us( printer_param.step_delay );
	MTA_L; MTAF_H; MTB_L; MTBF_L;
	delay_us( printer_param.step_delay );
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
	MTA_L; MTAF_H; MTB_H; MTBF_L;
	delay_us( printer_param.step_delay );
	MTA_L; MTAF_L; MTB_H; MTBF_L;
	delay_us( printer_param.step_delay );
	MTA_H; MTAF_L; MTB_H; MTBF_L;
	delay_us( printer_param.step_delay );
	MTA_H; MTAF_L; MTB_L; MTBF_L;
	delay_us( printer_param.step_delay );
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

/*
   ��ӡprinter_data�е����ݣ�һ���ɴ�ӡ���ַ���
   �����ַ��еĳ��ȣ���Ϊ�˵���������ʱ
 */
void printer_print_glyph( unsigned char len )
{
	unsigned char	*p;
	unsigned char	b, c, row, col_byte;
/*ȱֽ��� PA8 ȱֽΪ��*/


/*
   if( GPIO_ReadInputDataBit( PHE_PORT, PHE_PIN ) )
   {
   GPIO_SetBits( NO_PAPER_OUT_PORT, NO_PAPER_OUT_PIN );
   return;
   }else
   {
   GPIO_ResetBits( NO_PAPER_OUT_PORT, NO_PAPER_OUT_PIN );
   }
 */
	fprinting = 1;
	GPIO_SetBits( PRINTER_POWER_PORT_5V, PRINTER_POWER_PIN_5V );

	for( row = 0; row < 12; row++ )
	{
		p = print_glyph[row * 2 + 0];
		for( col_byte = 0; col_byte < GLYPH_COL; col_byte++ )
		{
			c = *p++;
			for( b = 0; b < 8; b++ )
			{
				GPIO_ResetBits( CLK_PORT, CLK_PIN );

				if( 0x80 == ( c & 0x80 ) )
				{
					GPIO_SetBits( DI_PORT, DI_PIN );
				}else
				{
					GPIO_ResetBits( DI_PORT, DI_PIN );
				}
				c <<= 1;
				GPIO_SetBits( CLK_PORT, CLK_PIN );
			}
		}
		GPIO_ResetBits( LAT_PORT, LAT_PIN );
		//delay_us(5);
		GPIO_SetBits( LAT_PORT, LAT_PIN );

		GPIO_SetBits( STB1_3_PORT, STB1_3_PIN );
		delay_us( printer_param.heat_delay[printer_param.gray_level] );
		GPIO_ResetBits( STB1_3_PORT, STB1_3_PIN );

		GPIO_SetBits( STB4_6_PORT, STB4_6_PIN );
		delay_us( printer_param.heat_delay[printer_param.gray_level] );
		GPIO_ResetBits( STB4_6_PORT, STB4_6_PIN );
		drivers1( );

		p = print_glyph[row * 2 + 1];
		for( col_byte = 0; col_byte < GLYPH_COL; col_byte++ )
		{
			c = *p++;
			for( b = 0; b < 8; b++ )
			{
				GPIO_ResetBits( CLK_PORT, CLK_PIN );

				if( 0x80 == ( c & 0x80 ) )
				{
					GPIO_SetBits( DI_PORT, DI_PIN );
				}else
				{
					GPIO_ResetBits( DI_PORT, DI_PIN );
				}
				c <<= 1;
				GPIO_SetBits( CLK_PORT, CLK_PIN );
			}
		}
		GPIO_ResetBits( LAT_PORT, LAT_PIN );
		//delay_us(5);
		GPIO_SetBits( LAT_PORT, LAT_PIN );

		GPIO_SetBits( STB1_3_PORT, STB1_3_PIN );
		delay_us( printer_param.heat_delay[printer_param.gray_level] );
		GPIO_ResetBits( STB1_3_PORT, STB1_3_PIN );

		GPIO_SetBits( STB4_6_PORT, STB4_6_PIN );
		delay_us( printer_param.heat_delay[printer_param.gray_level] );
		GPIO_ResetBits( STB4_6_PORT, STB4_6_PIN );

		drivers2( );
	}

	for( col_byte = 0; col_byte < printer_param.line_space; col_byte++ )
	{
		drivers1( );
		drivers2( );
	}

	printer_stop( );
	GPIO_ResetBits( PRINTER_POWER_PORT_5V, PRINTER_POWER_PIN_5V );
	fprinting = 0;
}

/*
   Merge bits from two values according to a mask
   unsigned int a;    // value to merge in non-masked bits
   unsigned int b;    // value to merge in masked bits
   unsigned int mask; // 1 where bits from b should be selected; 0 where from a.
   unsigned int r;    // result of (a & ~mask) | (b & mask) goes here

   r = a ^ ((a ^ b) & mask);

 */




/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
void printer_get_str_glyph( unsigned char *pstr, unsigned char len )
{
	unsigned long	addr;
	unsigned char	charnum = len;
	unsigned char	* p		= pstr;
	unsigned char	msb, lsb;
	unsigned int	start_col = printer_param.margin_left;

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
		if( msb <= 0x80 ) //ascii�ַ�
		{
			addr = ( msb - 0x20 ) * 48 + FONT_ASC1224_ADDR;
			for( offset = 0; offset < 12; offset++ )
			{
				val_new						= *(__IO uint32_t*)addr;
				font_glyph[offset * 4 + 0]	= (unsigned char)( val_new & 0xff );
				font_glyph[offset * 4 + 1]	= (unsigned char)( val_new >> 8 );
				font_glyph[offset * 4 + 2]	= (unsigned char)( val_new >> 16 );
				font_glyph[offset * 4 + 3]	= (unsigned char)( val_new >> 24 );
				addr						+= 4;
			}

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
				addr = FONT_HZ2424_ADDR + ( ( ( (unsigned long)msb ) - 0xa1 ) * 94 + ( ( (unsigned long)lsb ) - 0xa1 ) ) * 72;
			}else if( ( msb >= 0xb0 ) && ( msb <= 0xf7 ) && ( lsb >= 0xa1 ) )
			{
				addr = FONT_HZ2424_ADDR + ( ( ( (unsigned long)msb ) - 0xb0 ) * 94 + ( ( (unsigned long)lsb ) - 0xa1 ) ) * 72 + 282 * 72;
			}
			for( offset = 0; offset < 18; offset++ )
			{
				val_new						= *(__IO uint32_t*)addr;
				font_glyph[offset * 4 + 0]	= (unsigned char)( val_new & 0xff );
				font_glyph[offset * 4 + 1]	= (unsigned char)( val_new >> 8 );
				font_glyph[offset * 4 + 2]	= (unsigned char)( val_new >> 16 );
				font_glyph[offset * 4 + 3]	= (unsigned char)( val_new >> 24 );

				addr += 4;
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
   rt_kprintf("\r\n");
   for(col=0;col<48;col++) rt_kprintf("%02x ",print_glyph[row][col]);
   }
 */
	//������ӡ
	printer_print_glyph( len );
}

/*
   ����յ��Ĵ�ӡ����,�γ�Ҫ��ӡ��һ������

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
static void printer_get_str_line( void )
{
	unsigned char CH, CL;

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
					printer_get_str_glyph( print_str, print_str_len );
				}
				print_str_len	= 0;
				dotremain		= 384 - printer_param.margin_left - printer_param.margin_right;
			}else
			{
				if( dotremain >= 12 )       //���ɴ�ӡһ��ascii
				{
					print_str[print_str_len++]	= CH;
					dotremain					-= 12;
					if( dotremain < 12 )    //ʣ�²����һ��ascii,��ӡ�ɡ�
					{
						printer_get_str_glyph( print_str, print_str_len );
						print_str_len	= 0;
						dotremain		= 384 - printer_param.margin_left - printer_param.margin_right;
					}
				}else //�������Ӧ��ִ�в���.��Ҫ����Ӣ����ʱ,ʣ�������12-24 ֮����Ҫ��ӡ����ʱ������
				{
					printer_get_str_glyph( print_str, print_str_len );
					print_str[0]	= CH;
					print_str_len	= 1;
					dotremain		= 384 - printer_param.margin_left - printer_param.margin_right - 12;
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
						printer_get_str_glyph( print_str, print_str_len );
						print_str_len	= 0;
						dotremain		= 384 - printer_param.margin_left - printer_param.margin_right;
					}
				}else //Ӧ�ÿ��ԷŸ�ASCII,������˸����� ;-(
				{
					printer_get_str_glyph( print_str, print_str_len );
					print_str[0]	= CH;
					print_str[1]	= CL;
					print_str_len	= 2;
					dotremain		= 384 - printer_param.margin_left - printer_param.margin_right - 24;
				}
			}
		}else if( CH == 0xff ) //������ 0xFFFF
		{
			if( print_str_len > 0 )
			{
				printer_get_str_glyph( print_str, print_str_len );
			}
			print_str_len	= 0;
			dotremain		= 384 - printer_param.margin_left - printer_param.margin_right;
			//todo �ϵ�
		}
	}
}

/*
   ��ӡ����ʱ���
 */
static void timer_printer_cb( void* parameter )
{
	if( fprinting )
	{
		return;
	}
	printer_get_str_line( );
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
static rt_err_t printer_init( rt_device_t dev )
{
//	delay_init( 72 );

	ringbuffer_init( &rb_printer_data, PRINTER_DATA_SIZE, printer_data );
	printer_load_param( );
	dotremain = 384 - printer_param.margin_left - printer_param.margin_right; //�µ�һ�пɴ�ӡ�ַ�������
	printer_port_init( );
	printer_stop( );

	return RT_EOK;
}

/***********************************************************
* Function:
* Description:    �ڴ��Ƿ�Ӧ�ø���ӡ�ϵ磬ֻ����Ҫ��ӡʱ�ϵ�
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static rt_err_t printer_open( rt_device_t dev, rt_uint16_t oflag )
{
	GPIO_SetBits( PRINTER_POWER_PORT_3V3, PRINTER_POWER_PIN_3V3 );
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
static rt_size_t printer_read( rt_device_t dev, rt_off_t pos, void* buff, rt_size_t count )
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
static rt_size_t printer_write( rt_device_t dev, rt_off_t pos, const void* buff, rt_size_t count )
{
	rt_size_t ret = RT_EOK;
	ret = ringbuffer_put_data( &rb_printer_data, count, (unsigned char*)buff );
	return ret;
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
static rt_err_t printer_control( rt_device_t dev, rt_uint8_t cmd, void *arg )
{
	uint32_t	code = *(uint32_t*)arg;
	int			i;
	switch( cmd )
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
			for( i = 0; i < code * 24; i++ )
			{
				drivers1( );
				drivers2( );
				printer_stop( );
			}
			break;
		case PRINTER_CMD_FACTORY:
			break;
		case PRINTER_CMD_GRAYLEVEL:
			printer_param.gray_level = code;
			break;
		case PRINTER_CMD_LINESPACE_DEFAULT:
			printer_param.line_space = code;
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
			printer_param.margin_left = code;
			break;
		case PRINTER_CMD_MARGIN_RIGHT:
			printer_param.margin_right = code;
			break;
	}
	return RT_EOK;
}

/***********************************************************
* Function:
* Description:�ڴ˸���ӡ���ϵ�
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static rt_err_t printer_close( rt_device_t dev )
{
	GPIO_ResetBits( PRINTER_POWER_PORT_3V3, PRINTER_POWER_PIN_3V3 );
	GPIO_ResetBits( PRINTER_POWER_PORT_5V, PRINTER_POWER_PIN_5V );
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
	rt_timer_init( &tmr_printer, \
	               "tmr_p", \
	               timer_printer_cb, NULL, \
	               50, \
	               RT_TIMER_FLAG_PERIODIC );

	dev_printer.type		= RT_Device_Class_Char;
	dev_printer.init		= printer_init;
	dev_printer.open		= printer_open;
	dev_printer.close		= printer_close;
	dev_printer.read		= printer_read;
	dev_printer.write		= printer_write;
	dev_printer.control		= printer_control;
	dev_printer.user_data	= RT_NULL;

	rt_device_register( &dev_printer, "printer", RT_DEVICE_FLAG_WRONLY );
	rt_device_init( &dev_printer );
	rt_timer_start( &tmr_printer );
}

/***********************************************************
* Function:       printer_str
* Description:    ������ɵ��ַ���
* Input:          const char *str
* Output:         �ַ�����glyph
* Return:         void
* Others:         ��
***********************************************************/
void printer( const char *str )
{
	printer_write( &dev_printer, 0, str, strlen( str ) );
}

FINSH_FUNCTION_EXPORT( printer, print string test );


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
void step( const int count, const int delay )
{
	int i;

	if( GPIO_ReadInputDataBit( PHE_PORT, PHE_PIN ) )
	{
		rt_kprintf( "NO Paper\r\n" );
		return;
	}
	GPIO_SetBits( PRINTER_POWER_PORT_5V, PRINTER_POWER_PIN_5V );
	for( i = 0; i < count; i++ )
	{
		//drivers1();
		MTA_H; MTAF_L; MTB_L; MTBF_H;
		delay_us( delay );
		MTA_L; MTAF_L; MTB_L; MTBF_H;
		delay_us( delay );
		MTA_L; MTAF_H; MTB_L; MTBF_H;
		delay_us( delay );
		MTA_L; MTAF_H; MTB_L; MTBF_L;
		delay_us( delay );
		//drivers2();
		MTA_L; MTAF_H; MTB_H; MTBF_L;
		delay_us( delay );
		MTA_L; MTAF_L; MTB_H; MTBF_L;
		delay_us( delay );
		MTA_H; MTAF_L; MTB_H; MTBF_L;
		delay_us( delay );
		MTA_H; MTAF_L; MTB_L; MTBF_L;
		delay_us( delay );
		printer_stop( );
	}
	GPIO_ResetBits( PRINTER_POWER_PORT_5V, PRINTER_POWER_PIN_5V );
}

FINSH_FUNCTION_EXPORT( step, print step test );

/************************************** The End Of File **************************************/
