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
 *     bitter    20121102     1.0     build this moudle
 ***********************************************************/

#include <stdio.h>
#include <rtthread.h>
#include <rtdevice.h>
#include "stm32f4xx.h"
#include "board.h"
#include "printer.h"
#include <finsh.h>

#include "sed1520.h"

/*打印头相关*/

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

/*收到的打印数据的编码包括换行符*/
#define PRINTER_DATA_SIZE 2048
unsigned char			printer_data[PRINTER_DATA_SIZE];
struct rt_ringbuffer	rb_printer_data;

uint8_t					ctrlbit_printer_3v3_on = 0;


/*
   要打印的图案 24x24dot 24*3=72byte
   384dot/line  384/8=48
   即 24行，每行384dot=48byte
   但是由于有左边界，右边界，所以并不是8bit或1byte对齐的方式
 */

#define GLYPH_ROW	24
#define GLYPH_COL	48

//static unsigned char print_glyph[GLYPH_ROW][GLYPH_COL] __attribute__( ( at( 0x20001000 ) ) ) = { 0 };
static unsigned char	print_glyph[GLYPH_ROW][GLYPH_COL] = { 0 };

static struct rt_device dev_printer;

struct _PRINTER_PARAM
{
	uint8_t step_delay;                     //步进延时,影响行间隔
	uint8_t gray_level;                     //灰度等级,加热时间
	uint8_t heat_delay[4];                  //加热延时
	uint8_t line_space;                     //行间隔
	uint8_t margin_left;                    //左边界
	uint8_t margin_right;                   //右边界
} printer_param =
{
	1, 2, { 5, 10, 15, 20 }, 6, 0, 0        /*折算到ms*/
};

static unsigned short	dotremain = 384;    //还可使用的dot，每汉字24dot 每ascii 12dots
static unsigned char	print_str[36];      //要打印的单行最大字节数 全ascii 384/12=32  汉字 384/24*3
static unsigned char	print_str_len = 0;

/*要不要4字节对齐*/
static unsigned char font_glyph[80];        //每个汉字或ascii的最大字节数 24x24dot = 72bytes

//延时nus
//nus为要延时的us数.

void printer_delay_us( const uint32_t ticks )
{
	rt_thread_delay( ticks );
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
void printer_delay_us1( const uint32_t usec )
{
	__IO uint32_t	count = 0;
	//const uint32_t	utime	= ( 168 * usec / 7 );
	const uint32_t	utime = ( 10 * usec / 7 );
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
* Function:       // 函数名称
* Description:    // 函数功能、性能等的描述
* Input:          // 1.输入参数1，说明，包括每个参数的作用、取值说明及参数间关系
* Input:          // 2.输入参数2，说明，包括每个参数的作用、取值说明及参数间关系
* Output:         // 1.输出参数1，说明
* Return:         // 函数返回值的说明
* Others:         // 其它说明
***********************************************************/
void printer_stop( void )
{
	MTA_L;
	MTAF_L;
	MTB_L;
	MTBF_L;
}

__IO uint8_t fprinting = 0;     //是否正在打印


/***********************************************************
* Function:       // 函数名称
* Description:    // 函数功能、性能等的描述
* Input:          // 1.输入参数1，说明，包括每个参数的作用、取值说明及参数间关系
* Input:          // 2.输入参数2，说明，包括每个参数的作用、取值说明及参数间关系
* Output:         // 1.输出参数1，说明
* Return:         // 函数返回值的说明
* Others:         // 其它说明
***********************************************************/
static void printer_port_init( void )
{
	GPIO_InitTypeDef gpio_init;

	RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOD | RCC_AHB1Periph_GPIOE, ENABLE );
	/*去掉PB4 JTAG功能*/
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
	GPIO_ResetBits( PRINTER_POWER_PORT_5V, PRINTER_POWER_PIN_5V );

	gpio_init.GPIO_Pin = PRINTER_POWER_PIN_3V3;
	GPIO_Init( PRINTER_POWER_PORT_3V3, &gpio_init );
	GPIO_SetBits( PRINTER_POWER_PORT_3V3, PRINTER_POWER_PIN_3V3 );
	//ctrlbit_printer_3v3_on = 0x20;

	gpio_init.GPIO_Pin	= PHE_PIN;
	gpio_init.GPIO_Mode = GPIO_Mode_IN;
	GPIO_Init( PHE_PORT, &gpio_init );
}

/***********************************************************
* Function:       // 函数名称
* Description:    // 函数功能、性能等的描述
* Input:          // 1.输入参数1，说明，包括每个参数的作用、取值说明及参数间关系
* Input:          // 2.输入参数2，说明，包括每个参数的作用、取值说明及参数间关系
* Output:         // 1.输出参数1，说明
* Return:         // 函数返回值的说明
* Others:         // 其它说明
***********************************************************/
void drivers1( void )
{
	MTA_H; MTAF_L; MTB_L; MTBF_H;
	printer_delay_us( printer_param.step_delay );
	MTA_L; MTAF_L; MTB_L; MTBF_H;
	printer_delay_us( printer_param.step_delay );
	MTA_L; MTAF_H; MTB_L; MTBF_H;
	printer_delay_us( printer_param.step_delay );
	MTA_L; MTAF_H; MTB_L; MTBF_L;
	printer_delay_us( printer_param.step_delay );
}

/***********************************************************
* Function:       // 函数名称
* Description:    // 函数功能、性能等的描述
* Input:          // 1.输入参数1，说明，包括每个参数的作用、取值说明及参数间关系
* Input:          // 2.输入参数2，说明，包括每个参数的作用、取值说明及参数间关系
* Output:         // 1.输出参数1，说明
* Return:         // 函数返回值的说明
* Others:         // 其它说明
***********************************************************/
void drivers2( void )
{
	MTA_L; MTAF_H; MTB_H; MTBF_L;
	printer_delay_us( printer_param.step_delay );
	MTA_L; MTAF_L; MTB_H; MTBF_L;
	printer_delay_us( printer_param.step_delay );
	MTA_H; MTAF_L; MTB_H; MTBF_L;
	printer_delay_us( printer_param.step_delay );
	MTA_H; MTAF_L; MTB_L; MTBF_L;
	printer_delay_us( printer_param.step_delay );
}

/*读取参数*/
static void printer_load_param( void )
{
	printer_param.step_delay	= jt808_param.id_0xF043;    //步进延时,影响行间隔
	printer_param.gray_level	= jt808_param.id_0xF044;    //灰度等级,加热时间
	printer_param.heat_delay[0] = jt808_param.id_0xF045;    //加热延时
	printer_param.heat_delay[1] = jt808_param.id_0xF046;	//加热延时
	printer_param.heat_delay[2] = jt808_param.id_0xF047;	//加热延时
	printer_param.heat_delay[3] = jt808_param.id_0xF048;	//加热延时

	printer_param.line_space	= jt808_param.id_0xF040;    //行间隔
	printer_param.margin_left	= jt808_param.id_0xF041;    //左边界
	printer_param.margin_right	= jt808_param.id_0xF042;    //右边界
}


#define PRINTER_IDLE					1
#define PRINTER_GET_DATA_HEAT_1_3		2
#define PRINTER_STOP_HEAT_1_3_HEAT_4_6	3
#define PRINTER_STOP_HEAT_4_6			4
#define PRINTER_STEP_DOTROW1			5
#define PRINTER_STEP_DOTROW2			6
#define PRINTER_STEP_DOTROW3			7
#define PRINTER_STEP_DOTROW4			8
#define PRINTER_STEP_DOTROWEND			9

__IO uint32_t	timebase_1ms		= 0;
uint8_t			print_dotrow		= 0;    /*当前打印的点阵行*/
uint16_t		print_dotrow_end	= 30;   /*结束打印的行号*/
__IO uint8_t	print_stage			= PRINTER_IDLE;

/*1ms 定时中断*/
void TIM3_IRQHandler( void )
{
	unsigned char	*p;
	unsigned char	b, c,col_byte;

	if( TIM_GetITStatus( TIM3, TIM_IT_Update ) == RESET )
	{
		return;
	}
	TIM_ClearITPendingBit( TIM3, TIM_IT_Update );
	if( fprinting == 0 )
	{
		return;
	}
	/*缺纸检测 PA8 缺纸为高*/
	if( GPIO_ReadInputDataBit( PHE_PORT, PHE_PIN ) )
	{
		GPIO_ResetBits( PRINTER_POWER_PORT_5V, PRINTER_POWER_PIN_5V );
		fprinting = 0;
		return;
	}
	timebase_1ms++;

	switch( print_stage )
	{
		case PRINTER_IDLE:
			GPIO_SetBits( PRINTER_POWER_PORT_5V, PRINTER_POWER_PIN_5V );
			print_stage = PRINTER_GET_DATA_HEAT_1_3;
			break;
		case PRINTER_GET_DATA_HEAT_1_3:
			p = print_glyph[print_dotrow];
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
			GPIO_SetBits( LAT_PORT, LAT_PIN );
			GPIO_SetBits( STB1_3_PORT, STB1_3_PIN );    /*加热*/
			print_stage		= PRINTER_STOP_HEAT_1_3_HEAT_4_6;
			timebase_1ms	= 0;                        /*启动定时*/
			break;
		case PRINTER_STOP_HEAT_1_3_HEAT_4_6:
			if( timebase_1ms < printer_param.heat_delay[printer_param.gray_level] )
			{
				break;
			}
			GPIO_ResetBits( STB1_3_PORT, STB1_3_PIN );
			GPIO_SetBits( STB4_6_PORT, STB4_6_PIN );
			print_stage		= PRINTER_STOP_HEAT_4_6;
			timebase_1ms	= 0;
			break;
		case PRINTER_STOP_HEAT_4_6:
			if( timebase_1ms < printer_param.heat_delay[printer_param.gray_level] )
			{
				break;
			}
			GPIO_ResetBits( STB4_6_PORT, STB4_6_PIN );
			print_stage = PRINTER_STEP_DOTROW1;
			break;
		case PRINTER_STEP_DOTROW1:
			GPIO_SetBits( PRINTER_POWER_PORT_5V, PRINTER_POWER_PIN_5V );
			if( print_dotrow & 0x01 ) //driver2
			{
				MTA_L; MTAF_H; MTB_H; MTBF_L;
			}else
			{
				MTA_H; MTAF_L; MTB_L; MTBF_H;
			}
			timebase_1ms	= 0;
			print_stage		= PRINTER_STEP_DOTROW2;
			break;
		case PRINTER_STEP_DOTROW2:
			if( timebase_1ms < printer_param.step_delay )
			{
				break;
			}
			if( print_dotrow & 0x01 ) //driver2
			{
				MTA_L; MTAF_L; MTB_H; MTBF_L;
			}else
			{
				MTA_L; MTAF_L; MTB_L; MTBF_H;
			}
			//rt_kprintf( "5" );
			timebase_1ms	= 0;
			print_stage		= PRINTER_STEP_DOTROW3;
			break;
		case PRINTER_STEP_DOTROW3:
			if( timebase_1ms < printer_param.step_delay )
			{
				break;
			}
			if( print_dotrow & 0x01 ) //driver2
			{
				MTA_H; MTAF_L; MTB_H; MTBF_L;
			}else
			{
				MTA_L; MTAF_H; MTB_L; MTBF_H;
			}
			timebase_1ms	= 0;
			print_stage		= PRINTER_STEP_DOTROW4;
			break;
		case PRINTER_STEP_DOTROW4:
			if( timebase_1ms < printer_param.step_delay )
			{
				break;
			}
			if( print_dotrow & 0x01 ) //driver2
			{
				MTA_H; MTAF_L; MTB_L; MTBF_L;
			}else
			{
				MTA_L; MTAF_H; MTB_L; MTBF_L;
			}
			timebase_1ms	= 0;
			print_stage		= PRINTER_STEP_DOTROWEND;
			break;
		case PRINTER_STEP_DOTROWEND:
			if( timebase_1ms < printer_param.step_delay )
			{
				break;
			}
			/*判断是点阵行的step还是行间距的步进*/
			print_dotrow++;
			if( print_dotrow < 24 )                     /*还在打印*/
			{
				print_stage = PRINTER_GET_DATA_HEAT_1_3;
			}else if( print_dotrow < print_dotrow_end ) /*打印行间距*/
			{
				print_stage = PRINTER_STEP_DOTROW1;
			}else
			{
				MTA_L; MTAF_L; MTB_L; MTBF_L;           /*停止*/
				GPIO_ResetBits( PRINTER_POWER_PORT_5V, PRINTER_POWER_PIN_5V );
				fprinting			= 0;
				print_stage			= PRINTER_IDLE;
				print_dotrow		= 0;
				print_dotrow_end	= 24 + printer_param.line_space;
			}
			break;
	}
}

/*定时器配置*/
void TIM_Config( void )
{
	NVIC_InitTypeDef		NVIC_InitStructure;
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

	/* TIM3 clock enable */
	RCC_APB1PeriphClockCmd( RCC_APB1Periph_TIM3, ENABLE );

	/* Enable the TIM3 gloabal Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel						= TIM3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority	= 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority			= 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd					= ENABLE;
	NVIC_Init( &NVIC_InitStructure );

/* Time base configuration */
	TIM_TimeBaseStructure.TIM_Period		= 1000;             /* 1ms */
	TIM_TimeBaseStructure.TIM_Prescaler		= ( 168 / 2 - 1 );  /* 1M*/
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode	= TIM_CounterMode_Up;
	TIM_ARRPreloadConfig( TIM3, ENABLE );

	TIM_TimeBaseInit( TIM3, &TIM_TimeBaseStructure );

	TIM_ClearFlag( TIM3, TIM_FLAG_Update );
/* TIM Interrupts enable */
	TIM_ITConfig( TIM3, TIM_IT_Update, ENABLE );

/* TIM3 enable counter */
	TIM_Cmd( TIM3, ENABLE );
}

/*
   打印printer_data中的数据，一个可打印的字符行
   传入字符行的长度，是为了调整加热延时
 */
#if 0
void printer_print_glyph( void )
{
	unsigned char	*p;
	unsigned char	b, c, row, col_byte;
/*缺纸检测 PA8 缺纸为高*/
	if( GPIO_ReadInputDataBit( PHE_PORT, PHE_PIN ) )
	{
		rt_kprintf( "NO Paper\r\n" );
		GPIO_ResetBits( PRINTER_POWER_PORT_5V, PRINTER_POWER_PIN_5V );
		printer_stop( );
		return;
	}
	fprinting = 1;

	GPIO_SetBits( PRINTER_POWER_PORT_5V, PRINTER_POWER_PIN_5V );

	for( row = 0; row < 24; row++ )
	{
		p = print_glyph[row];
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
		GPIO_SetBits( LAT_PORT, LAT_PIN );

		GPIO_SetBits( STB1_3_PORT, STB1_3_PIN );
		printer_delay_us( printer_param.heat_delay[printer_param.gray_level] );
		GPIO_ResetBits( STB1_3_PORT, STB1_3_PIN );

		GPIO_SetBits( STB4_6_PORT, STB4_6_PIN );
		printer_delay_us( printer_param.heat_delay[printer_param.gray_level] );
		GPIO_ResetBits( STB4_6_PORT, STB4_6_PIN );

		if( row & 0x01 )
		{
			drivers2( );
		} else
		{
			drivers1( );
		}
	}

	for( col_byte = 0; col_byte < printer_param.line_space; col_byte++ )
	{
		drivers1( );
		drivers2( );
	}
	GPIO_ResetBits( PRINTER_POWER_PORT_5V, PRINTER_POWER_PIN_5V );
	printer_stop( );
	fprinting = 0;
}

#endif


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
	rt_kprintf( "print>" );
	for( row = 0; row < len; row++ )
	{
		rt_kprintf( "%c", *( pstr + row ) );
	}
	rt_kprintf( "\r\n" );
	//addr=0;
	while( charnum )
	{
		for( row = 0; row < 80; row++ )
		{
			font_glyph[row] = 0;
		}
		msb = *p++;
		charnum--;
		if( msb <= 0x80 ) //ascii字符
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
				val_new = ( font_glyph[row * 2] << 24 ) | ( font_glyph[row * 2 + 1] << 16 );    //读出的字库2byte，只有高12位有效，左对齐

				col		= start_col >> 3;                                                       //查找所在的列字节 [0..7]对应字节0
				val_old = ( print_glyph[row][col] << 24 ) | ( print_glyph[row][col + 1] << 16 ) | ( print_glyph[row][col + 2] << 8 );

				offset						= start_col & 0x07;
				val_new						>>= offset;
				val_mask					= ( 0xFFF00000 >> offset );                         //12bit个1的mask
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
				val_new = ( font_glyph[row * 3] << 24 ) | ( font_glyph[row * 3 + 1] << 16 ) | ( font_glyph[row * 3 + 2] << 8 ); //读出的字库24byte

				col		= start_col >> 3;                                                                                       //查找所在的列字节 [0..7]对应字节0
				val_old = ( print_glyph[row][col] << 24 ) | ( print_glyph[row][col + 1] << 16 ) | ( print_glyph[row][col + 2] << 8 ) | print_glyph[row][col + 3];

				offset						= start_col & 0x07;
				val_new						>>= offset;
				val_mask					= ( 0xFFFFFF00 >> offset );                                                         //24bit个1的mask
				val_ret						= val_old ^ ( ( val_new ^ val_old ) & val_mask );
				print_glyph[row][col]		= ( val_ret >> 24 ) & 0xff;
				print_glyph[row][col + 1]	= ( val_ret >> 16 ) & 0xff;
				print_glyph[row][col + 2]	= ( val_ret >> 8 ) & 0xff;
				print_glyph[row][col + 3]	= val_ret & 0xff;
			}
			start_col += 24;
		}
	}

	//启动打印
	//printer_print_glyph();
	print_dotrow	= 0;
	print_stage		= PRINTER_IDLE;
	fprinting		= 1;
}

/*
   检查收到的打印数据,形成要打印的一行文字

   GBK 亦采用双字节表示，总体编码范围为8140-FEFE，
   首字节在81-FE 之间，尾字节在40-FE 之间，剔除 xx7F一条线。
   总计23940 个码位，共收入21886个汉字和图形符号，
   其中汉字（包括部首和构件）21003 个，图形符号883 个。


   汉字是24x24  ASCII是12x24

   判断一行的结束

   1.遇到<CR><LF>
   2.一行满 应该计算dot值，而不是byte值，一个汉字24dot (2byte) 一个ASC 12dot(1.5byte)
   3.结束符 0xffff作为打印结束标志

   4.增加一个进纸的指令0x0c

   本函数实际上是获得一个可打印行的内容。
 */

static void printer_get_str_line( void )
{
	unsigned char CH, CL;

	while( ( fprinting == 0 ) && ( rt_ringbuffer_getchar( &rb_printer_data, &CH ) == 1 ) )
	{
		if( CH == 0x00 )
		{
			continue;                               // gb2312的ascii:  0x00 0x31
		}
		if( CH < 0x80 )                             //ASCII
		{
			if( ( CH == 0x0d ) || ( CH == 0x0a ) )  //换行
			{
				if( CH == 0x0a )
				{
					if( print_str_len > 0 )         /*有要打印的数据*/
					{
						printer_get_str_glyph( print_str, print_str_len );
						print_str_len	= 0;
						dotremain		= 384 - printer_param.margin_left - printer_param.margin_right;
					}else /*走纸*/
					{
						print_dotrow		= 24;
						print_dotrow_end	= 48;
						print_stage			= PRINTER_STEP_DOTROW1;
						fprinting			= 1;
					}
				}
			}else
			{
				if( dotremain >= 12 )       //还可打印一个ascii
				{
					print_str[print_str_len++]	= CH;
					dotremain					-= 12;
					if( dotremain < 12 )    //剩下不足放一个ascii,打印吧。
					{
						printer_get_str_glyph( print_str, print_str_len );
						print_str_len	= 0;
						dotremain		= 384 - printer_param.margin_left - printer_param.margin_right;
					}
				}else //这个条件应该执行不到.主要是中英混排时,剩余点阵在12-24 之间又要打印汉字时产生。
				{
					printer_get_str_glyph( print_str, print_str_len );
					print_str[0]	= CH;
					print_str_len	= 1;
					dotremain		= 384 - printer_param.margin_left - printer_param.margin_right - 12;
				}
			}
		}else if( ( CH > 0x80 ) && ( CH < 0xff ) ) //GBK编码，保证取到完整,正好，多一个，多两个
		{
			rt_ringbuffer_getchar( &rb_printer_data, &CL );
			if( ( CL >= 0x40 ) && ( CL <= 0xfe ) )
			{
				if( dotremain >= 24 )
				{
					print_str[print_str_len++]	= CH;
					print_str[print_str_len++]	= CL;
					dotremain					-= 24;
					if( dotremain < 12 ) //剩下不足放一个ascii,打印吧。
					{
						printer_get_str_glyph( print_str, print_str_len );
						print_str_len	= 0;
						dotremain		= 384 - printer_param.margin_left - printer_param.margin_right;
					}
				}else //应该可以放个ASCII,结果来了个汉字 ;-(
				{
					printer_get_str_glyph( print_str, print_str_len );
					print_str[0]	= CH;
					print_str[1]	= CL;
					print_str_len	= 2;
					dotremain		= 384 - printer_param.margin_left - printer_param.margin_right - 24;
				}
			}
		}else if( CH == 0xff ) //结束符 0xFFFF
		{
			if( print_str_len > 0 )
			{
				printer_get_str_glyph( print_str, print_str_len );
			}
			print_str_len	= 0;
			dotremain		= 384 - printer_param.margin_left - printer_param.margin_right;
			//todo 断电
		}
	}
}

/**/
static rt_err_t printer_init( rt_device_t dev )
{
//	delay_init( 72 );

	rt_ringbuffer_init( &rb_printer_data, printer_data, PRINTER_DATA_SIZE );
	printer_load_param( );
	dotremain = 384 - printer_param.margin_left - printer_param.margin_right; //新的一行可打印字符点阵数
	printer_port_init( );
	printer_stop( );

	return RT_EOK;
}

/***********************************************************
* Function:
* Description:    在此是否应该给打印上电，只是在要打印时上电
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static rt_err_t printer_open( rt_device_t dev, rt_uint16_t oflag )
{
	//GPIO_SetBits( PRINTER_POWER_PORT_3V3, PRINTER_POWER_PIN_3V3 );
	ctrlbit_printer_3v3_on = 0x20;
	return RT_EOK;
}

/***********************************************************
* Function:       // 函数名称
* Description:    // 函数功能、性能等的描述
* Input:          // 1.输入参数1，说明，包括每个参数的作用、取值说明及参数间关系
* Input:          // 2.输入参数2，说明，包括每个参数的作用、取值说明及参数间关系
* Output:         // 1.输出参数1，说明
* Return:         // 函数返回值的说明
* Others:         // 其它说明
***********************************************************/
static rt_size_t printer_read( rt_device_t dev, rt_off_t pos, void* buff, rt_size_t count )
{
	return RT_EOK;
}

/***********************************************************
* Function:       // 函数名称
* Description:    // 函数功能、性能等的描述
* Input:          // 1.输入参数1，说明，包括每个参数的作用、取值说明及参数间关系
* Input:          // 2.输入参数2，说明，包括每个参数的作用、取值说明及参数间关系
* Output:         // 1.输出参数1，说明
* Return:         // 函数返回值的说明
* Others:         // 其它说明
***********************************************************/
static rt_size_t printer_write( rt_device_t dev, rt_off_t pos, const void* buff, rt_size_t count )
{
	rt_size_t ret = RT_EOK;
	ret = rt_ringbuffer_put( &rb_printer_data, (unsigned char*)buff, count );
	return ret;
}

/***********************************************************
* Function:       // 函数名称
* Description:    // 函数功能、性能等的描述
* Input:          // 1.输入参数1，说明，包括每个参数的作用、取值说明及参数间关系
* Input:          // 2.输入参数2，说明，包括每个参数的作用、取值说明及参数间关系
* Output:         // 1.输出参数1，说明
* Return:         // 函数返回值的说明
* Others:         // 其它说明
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
* Description:在此给打印机断电
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static rt_err_t printer_close( rt_device_t dev )
{
	//GPIO_ResetBits( PRINTER_POWER_PORT_3V3, PRINTER_POWER_PIN_3V3 );
	ctrlbit_printer_3v3_on = 0;
	GPIO_ResetBits( PRINTER_POWER_PORT_5V, PRINTER_POWER_PIN_5V );
	return RT_EOK;
}

ALIGN( RT_ALIGN_SIZE )
static char thread_printer_stack[512];
struct rt_thread thread_printer;


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static void rt_thread_entry_printer( void* parameter )
{
	TIM_Config( );
	while( 1 )
	{
		printer_get_str_line( );
		rt_thread_delay( RT_TICK_PER_SECOND / 10 );
	}
}

/***********************************************************
* Function:       // 函数名称
* Description:    // 函数功能、性能等的描述
* Input:          // 1.输入参数1，说明，包括每个参数的作用、取值说明及参数间关系
* Input:          // 2.输入参数2，说明，包括每个参数的作用、取值说明及参数间关系
* Output:         // 1.输出参数1，说明
* Return:         // 函数返回值的说明
* Others:         // 其它说明
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

	rt_device_register( &dev_printer, "printer", RT_DEVICE_FLAG_WRONLY );
	rt_device_init( &dev_printer );

	rt_thread_init( &thread_printer,
	                "print",
	                rt_thread_entry_printer,
	                RT_NULL,
	                &thread_printer_stack[0],
	                sizeof( thread_printer_stack ), 12, 5 );
	rt_thread_startup( &thread_printer );
}

/***********************************************************
* Function:       printer_str
* Description:    输出生成的字符串
* Input:          const char *str
* Output:         字符串的glyph
* Return:         void
* Others:         无
***********************************************************/
void printer( const char *str )
{
	printer_write( &dev_printer, 0, str, strlen( str ) );
}

FINSH_FUNCTION_EXPORT( printer, print string test );

#if 0


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
void step2( const int count, const int delay )
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
		printer_delay_us( delay );
		MTA_L; MTAF_L; MTB_L; MTBF_H;
		printer_delay_us( delay );
		MTA_L; MTAF_H; MTB_L; MTBF_H;
		printer_delay_us( delay );
		MTA_L; MTAF_H; MTB_L; MTBF_L;
		printer_delay_us( delay );
		//drivers2();
		MTA_L; MTAF_H; MTB_H; MTBF_L;
		printer_delay_us( delay );
		MTA_L; MTAF_L; MTB_H; MTBF_L;
		printer_delay_us( delay );
		MTA_H; MTAF_L; MTB_H; MTBF_L;
		printer_delay_us( delay );
		MTA_H; MTAF_L; MTB_L; MTBF_L;
		printer_delay_us( delay );
		printer_stop( );
	}
	GPIO_ResetBits( PRINTER_POWER_PORT_5V, PRINTER_POWER_PIN_5V );
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
void step1( const int count, const int delay )
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
		drivers2( );
		drivers1( );
		printer_stop( );
	}

	GPIO_ResetBits( PRINTER_POWER_PORT_5V, PRINTER_POWER_PIN_5V );
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
void step( const int count, const int delay )
{
	print_dotrow		= 24;
	print_dotrow_end	= 24 + count;
	print_stage			= PRINTER_STEP_DOTROW1;
	fprinting			= 1;
}

FINSH_FUNCTION_EXPORT( step, print step test );
#endif

/************************************** The End Of File **************************************/
