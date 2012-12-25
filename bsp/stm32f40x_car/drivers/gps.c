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
#include <stdio.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <dfs_posix.h>

#include "stm32f4xx.h"
#include "gps.h"
#include "jt808.h"

#include <finsh.h>

#define GPS_PWR_PORT	GPIOD
#define GPS_PWR_PIN		GPIO_Pin_10

#define GPS_GPIO_TX			GPIO_Pin_12 // PC12
#define GPS_GPIO_RX			GPIO_Pin_2  // PD2
#define GPS_GPIO_TxC		GPIOC
#define GPS_GPIO_RxD		GPIOD
#define RCC_APBPeriph_UART5 RCC_APB1Periph_UART5

/*声明一个gps设备*/
static struct rt_device dev_gps;
static uint32_t			gps_out_mode = GPS_OUTMODE_ALL;

/*串口接收缓存区定义*/
#define UART5_RX_SIZE 256
static uint8_t	uart5_rxbuf[UART5_RX_SIZE];
static uint16_t uart5_rxbuf_wr = 0;

/*gps原始信息数据区定义*/
#define GPS_RAWINFO_SIZE 256
static uint8_t				gps_rawinfo[GPS_RAWINFO_SIZE];
static uint16_t				gps_rawinfo_wr = 0;

static struct rt_semaphore	sem_gps;

uint8_t						flag_bd_upgrade_uart = 0;

extern struct rt_device		dev_vuart;

static rt_uint8_t			*ptr_mem_packet = RT_NULL;


/*
   gps接收中断处理，收到\n认为收到一包
   收到一包调用处理函数
 */

void UART5_IRQHandler( void )
{
	static uint8_t	last_ch = 0;
	uint8_t			ch;
	rt_interrupt_enter( );
	if( USART_GetITStatus( UART5, USART_IT_RXNE ) != RESET )
	{
		ch = USART_ReceiveData( UART5 );
		if( ( ch == 0x0a ) && ( last_ch == 0x0d ) ) /*遇到0d 0a 表明结束*/
		{
			uart5_rxbuf[uart5_rxbuf_wr++] = ch;
			memcpy( gps_rawinfo, uart5_rxbuf, uart5_rxbuf_wr );
			gps_rawinfo_wr	= uart5_rxbuf_wr;
			uart5_rxbuf_wr	= 0;
			rt_sem_release( &sem_gps );
		}else
		{
			uart5_rxbuf[uart5_rxbuf_wr++] = ch;
			if( uart5_rxbuf_wr == UART5_RX_SIZE )
			{
				uart5_rxbuf_wr = 0;
			}
			uart5_rxbuf[uart5_rxbuf_wr] = 0;
		}
		last_ch = ch;
		USART_ClearITPendingBit( UART5, USART_IT_RXNE );
	}
	rt_interrupt_leave( );
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
void gps_baud( int baud )
{
	USART_InitTypeDef USART_InitStructure;
	USART_InitStructure.USART_BaudRate				= baud;
	USART_InitStructure.USART_WordLength			= USART_WordLength_8b;
	USART_InitStructure.USART_StopBits				= USART_StopBits_1;
	USART_InitStructure.USART_Parity				= USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl	= USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode					= USART_Mode_Rx | USART_Mode_Tx;
	USART_Init( UART5, &USART_InitStructure );
}

FINSH_FUNCTION_EXPORT( gps_baud, config gsp_baud );

/*初始化*/
static rt_err_t dev_gps_init( rt_device_t dev )
{
	GPIO_InitTypeDef	GPIO_InitStructure;
	NVIC_InitTypeDef	NVIC_InitStructure;

	RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOC, ENABLE );
	RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOD, ENABLE );
	RCC_APB1PeriphClockCmd( RCC_APB1Periph_UART5, ENABLE );

	GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType	= GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd	= GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Speed	= GPIO_Speed_50MHz;

	GPIO_InitStructure.GPIO_Pin = GPS_PWR_PIN;
	GPIO_Init( GPS_PWR_PORT, &GPIO_InitStructure );
	GPIO_ResetBits( GPS_PWR_PORT, GPS_PWR_PIN );

/*uart5 管脚设置*/

	GPIO_InitStructure.GPIO_OType	= GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd	= GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed	= GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin		= GPIO_Pin_12;
	GPIO_Init( GPIOC, &GPIO_InitStructure );

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_Init( GPIOD, &GPIO_InitStructure );

	GPIO_PinAFConfig( GPIOC, GPIO_PinSource12, GPIO_AF_UART5 );
	GPIO_PinAFConfig( GPIOD, GPIO_PinSource2, GPIO_AF_UART5 );

/*NVIC 设置*/
	NVIC_InitStructure.NVIC_IRQChannel						= UART5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority	= 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority			= 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd					= ENABLE;
	NVIC_Init( &NVIC_InitStructure );

	gps_baud( 9600 );
	USART_Cmd( UART5, ENABLE );
	USART_ITConfig( UART5, USART_IT_RXNE, ENABLE );

	GPIO_SetBits( GPIOD, GPIO_Pin_10 );

	return RT_EOK;
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
static rt_err_t dev_gps_open( rt_device_t dev, rt_uint16_t oflag )
{
	GPIO_SetBits( GPS_PWR_PORT, GPS_PWR_PIN );
	return RT_EOK;
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
static rt_err_t dev_gps_close( rt_device_t dev )
{
	GPIO_ResetBits( GPS_PWR_PORT, GPS_PWR_PIN );
	return RT_EOK;
}

/***********************************************************
* Function:gps_read
* Description:数据模式下读取数据
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static rt_size_t dev_gps_read( rt_device_t dev, rt_off_t pos, void* buff, rt_size_t count )
{
	return RT_EOK;
}

/* write one character to serial, must not trigger interrupt */
static void uart5_putc( const char c )
{
	USART_SendData( UART5, c );
	while( !( UART5->SR & USART_FLAG_TXE ) )
	{
		;
	}
	UART5->DR = ( c & 0x1FF );
}

/***********************************************************
* Function:		gps_write
* Description:	数据模式下发送数据，要对数据进行封装
* Input:		const void* buff	要发送的原始数据
       rt_size_t count	要发送数据的长度
       rt_off_t pos		使用的socket编号
* Output:
* Return:
* Others:
***********************************************************/

static rt_size_t dev_gps_write( rt_device_t dev, rt_off_t pos, const void* buff, rt_size_t count )
{
	rt_size_t	len = count;
	uint8_t		*p	= (uint8_t*)buff;

	while( len )
	{
		USART_SendData( UART5, *p++ );
		while( USART_GetFlagStatus( UART5, USART_FLAG_TC ) == RESET )
		{
		}
		len--;
	}
	return RT_EOK;
}

/***********************************************************
* Function:		gps_control
* Description:	控制模块
* Input:		rt_uint8_t cmd	命令类型
    void *arg       参数,依据cmd的不同，传递的数据格式不同
* Output:
* Return:
* Others:
***********************************************************/
static rt_err_t dev_gps_control( rt_device_t dev, rt_uint8_t cmd, void *arg )
{
	int i = *(int*)arg;
	switch( cmd )
	{
		case CTL_GPS_OUTMODE:
			break;
		case CTL_GPS_BAUD:
			gps_baud( i );
	}
	return RT_EOK;
}

ALIGN( RT_ALIGN_SIZE )
static char thread_gps_stack[512];
struct rt_thread thread_gps;


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static void rt_thread_entry_gps( void* parameter )
{
	rt_err_t res;
	while( 1 )
	{
		res = rt_sem_take( &sem_gps, RT_TICK_PER_SECOND / 20 ); //等待100ms,实际上就是变长的延时,最长100ms
		if( res == RT_EOK )                                     //收到一包数据
		{
			if( flag_bd_upgrade_uart == 0 )
			{
				gps_rx( gps_rawinfo, gps_rawinfo_wr );
				gps_rawinfo_wr = 0;
			}else
			{
				if( gps_rawinfo[0] == 0x40 )
				{
					rt_device_write( &dev_vuart, 0, gps_rawinfo, gps_rawinfo_wr );
				}
			}
		}
		rt_thread_delay( RT_TICK_PER_SECOND / 20 );
	}
}

/*gps设备初始化*/
void gps_init( void )
{
	rt_sem_init( &sem_gps, "sem_gps", 0, 0 );

	rt_thread_init( &thread_gps,
	                "gps",
	                rt_thread_entry_gps,
	                RT_NULL,
	                &thread_gps_stack[0],
	                sizeof( thread_gps_stack ), 11, 5 );
	rt_thread_startup( &thread_gps );

	dev_gps.type	= RT_Device_Class_Char;
	dev_gps.init	= dev_gps_init;
	dev_gps.open	= dev_gps_open;
	dev_gps.close	= dev_gps_close;
	dev_gps.read	= dev_gps_read;
	dev_gps.write	= dev_gps_write;
	dev_gps.control = dev_gps_control;

	rt_device_register( &dev_gps, "gps", RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_STANDALONE );
	rt_device_init( &dev_gps );
}

/*gps开关*/
rt_err_t gps_onoff( uint8_t openflag )
{
	if( openflag == 0 )
	{
		GPIO_ResetBits( GPIOD, GPIO_Pin_10 );
	} else
	{
		GPIO_SetBits( GPIOD, GPIO_Pin_10 );
	}
	return 0;
}

FINSH_FUNCTION_EXPORT( gps_onoff, gps_onoff([1 | 0] ) );

/*线程退出的cleanup函数*/
static void cleanup( struct rt_thread *tid )
{
	if( ptr_mem_packet != RT_NULL )
	{
		rt_free( ptr_mem_packet );
	}
}

/*升级时的调试输出,当前已无法打印输出,串口被占用*/
static void msg_uart_usb( int res )
{
	//rt_kprintf("bd>%d\r\n",res);
}

/*更新北斗线程，接收调试串口来的数据，透传到gps串口中*/

void thread_gps_upgrade_uart( void* parameter )
{
#define BD_SYNC_40	0
#define BD_SYNC_0D	1
#define BD_SYNC_0A	2

/*定义一个函数指针，用作结果处理	*/

	void			( *msg )( unsigned int );
	unsigned int	resultcode;

	rt_uint8_t		buf[256];
	rt_uint8_t		*p;
	rt_uint16_t		count = 0;
	rt_uint16_t		i;
	rt_size_t		len;
	rt_uint32_t		baud				= 9600;
	rt_uint16_t		packetnum			= 0;
	uint8_t			bd_packet_status	= BD_SYNC_40;   /*北斗升级报文接收状态*/
	uint8_t			last_char			= 0x0;

	rt_tick_t		last_sendtick = 0;                  /*北斗更新时记录收到应答的时刻*/

	msg = parameter;

	ptr_mem_packet = rt_malloc( 1200 );
	if( ptr_mem_packet == RT_NULL )
	{
		resultcode = BDUPG_RES_RAM;
		msg( resultcode );
		return;
	}
	flag_bd_upgrade_uart = 1;

	dev_vuart.flag &= ~RT_DEVICE_FLAG_STREAM;
	rt_device_control( &dev_vuart, 0x03, &baud );
	p = ptr_mem_packet;

	while( 1 )
	{
		if( ( last_sendtick > 0 ) && ( rt_tick_get( ) - last_sendtick > RT_TICK_PER_SECOND * 12 ) )
		{
			/*升级程序发送数据，收到应答，再次发送数据。超时10s*/
			resultcode = BDUPG_RES_TIMEOUT;
			msg( resultcode );
			goto end_upgrade_uart_memfree;
		}
		while( ( len = rt_device_read( &dev_vuart, 0, buf, 256 ) ) > 0 )
		{
			for( i = 0; i < len; i++ )
			{
				switch( bd_packet_status )
				{
					case BD_SYNC_40:
						if( buf[i] == 0x40 )
						{
							*p++				= 0x40;
							bd_packet_status	= BD_SYNC_0A;
							count				= 1;
						}
						break;
					case BD_SYNC_0A:
						if( ( buf[i] == 0x0a ) && ( last_char == 0x0d ) )
						{
							*p = 0x0a;
							count++;
							dev_gps_write( &dev_gps, 0, ptr_mem_packet, count );
							packetnum++;                                            /*显示传递的包数*/
							msg( packetnum );
							last_sendtick = rt_tick_get( );
							if( memcmp( ptr_mem_packet, "\x40\x41\xc0", 3 ) == 0 )  /*修改波特率*/
							{
								baud = ( *( ptr_mem_packet + 4 ) << 24 ) | ( *( ptr_mem_packet + 5 ) << 16 ) | ( *( ptr_mem_packet + 6 ) << 8 ) | *( ptr_mem_packet + 7 );
								gps_baud( baud );
								uart1_baud( baud );
							}
							if( memcmp( ptr_mem_packet, "\x40\x34\xc0", 3 ) == 0 )  /*模块软件复位*/
							{
								resultcode = 0;
								msg( resultcode );                                  /*通知lcd显示完成*/
								goto end_upgrade_uart_memfree;
							}
							p					= ptr_mem_packet;
							bd_packet_status	= BD_SYNC_40;
						}else
						{
							*p++ = buf[i];
							count++;
						}
						break;
				}
				last_char = buf[i];
			}
		}
		rt_thread_delay( RT_TICK_PER_SECOND / 50 );
	}

end_upgrade_uart_memfree:
	rt_free( ptr_mem_packet );
	ptr_mem_packet = RT_NULL;
//end_upgrade_uart:
	baud = 115200;
	uart1_baud( baud );
	flag_bd_upgrade_uart = 0;
}

/*更新北斗线程，接收调试串口来的数据，透传到gps串口中*/
void thread_gps_upgrade_udisk( void* parameter )
{
	void			( *msg )( unsigned int );
	unsigned int	resultcode;

	int				fd, size, count = 0;

	msg = parameter;

	ptr_mem_packet = rt_malloc( 1200 );
	if( ptr_mem_packet == RT_NULL )
	{
		resultcode = BDUPG_RES_RAM;
		msg( resultcode );
		return;
	}
/*查找U盘*/
	while( 1 )
	{
		if( rt_device_find( "udisk" ) == RT_NULL ) /*没有找到*/
		{
			count++;
			if( count <= 10 )
			{
				msg( BDUPG_RES_USB_NOEXIST );
			}else
			{
				msg( BDUPG_RES_USB_NOEXIST ); /*指示U盘不存在*/
				goto end_upgrade_usb;
			}
		}else
		{
			break;
		}
	}

/*查找指定文件BEIDOU.IMG*/
	while( 1 )
	{
		fd = open( "/udisk/BEIDOU.IMG", O_RDONLY, 0 );
		if( fd >= 0 )
		{
		}else
		{
			msg( BDUPG_RES_USB_NOFILE );
		}
	}

end_upgrade_usb:

	rt_free( ptr_mem_packet );
	ptr_mem_packet = RT_NULL;
}

/*gps升级*/
rt_err_t gps_upgrade( char *src )
{
	rt_thread_t tid;
	int			buad = 9600;
	rt_kprintf( "\nNow upgrade from %s\n", src );
	if( strncmp( src, "COM", 3 ) == 0 ) /*串口升级*/
	{
		tid = rt_thread_create( "upgrade", thread_gps_upgrade_uart, (void*)msg_uart_usb, 2048, 5, 5 );
		if( tid != RT_NULL )
		{
			rt_thread_startup( tid );
		}else
		{
			rt_kprintf( "\n Upgrade from uart fail\n" );
		}
	}else
	{
		tid = rt_thread_create( "upgrade", thread_gps_upgrade_udisk, (void*)msg_uart_usb, 512, 7, 5 );
		if( tid != RT_NULL )
		{
			rt_thread_startup( tid );
		}else
		{
			rt_kprintf( "\n Upgrade from uart fail\n" );
		}
	}
}

FINSH_FUNCTION_EXPORT( gps_upgrade, upgrade bd_gps );


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
rt_size_t gps_write( uint8_t *p, uint8_t len )
{
	return dev_gps_write( &dev_gps, 0, p, len );
}

FINSH_FUNCTION_EXPORT( gps_write, write to gps );

/************************************** The End Of File **************************************/

