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
#include "stm32f4xx.h"

#include <finsh.h>

/*声明一个cam设备*/
static struct rt_device dev_cam;

/*串口接收缓存区定义*/
#define UART2_RXBUF_SIZE 1024
static uint8_t	uart2_rxbuf[UART2_RXBUF_SIZE];
static uint16_t uart2_rxbuf_wr	= 0, uart2_rxbuf_rd = 0;
static uint32_t last_rx_tick	= 0;

#define UART2_RX_SIZE 1024
static uint8_t	uart2_rx[UART2_RX_SIZE];
static uint16_t uart2_rx_wr = 0;

/*
   cam接收中断处理，收到\n认为收到一包
   收到一包调用处理函数
 */

void USART2_IRQHandler( void )
{
	rt_interrupt_enter( );
	if( USART_GetITStatus( USART2, USART_IT_RXNE ) != RESET )
	{
		uart2_rxbuf[uart2_rxbuf_wr++]	= USART_ReceiveData( USART2 );
		uart2_rxbuf_wr					%= UART2_RXBUF_SIZE;
		USART_ClearITPendingBit( USART2, USART_IT_RXNE );
		last_rx_tick = rt_tick_get( );
	}
	rt_interrupt_leave( );
}


/*初始化*/
static rt_err_t dev_cam_init( rt_device_t dev )
{
	GPIO_InitTypeDef	GPIO_InitStructure;
	USART_InitTypeDef	USART_InitStructure;
	NVIC_InitTypeDef	NVIC_InitStructure;

	RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOA, ENABLE );
	RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOB, ENABLE );
	RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOC, ENABLE );
	RCC_APB1PeriphClockCmd( RCC_APB1Periph_USART2, ENABLE );

	GPIO_PinAFConfig( GPIOA, GPIO_PinSource2, GPIO_AF_USART2 );
	GPIO_PinAFConfig( GPIOA, GPIO_PinSource3, GPIO_AF_USART2 );

	GPIO_InitStructure.GPIO_OType	= GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd	= GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed	= GPIO_Speed_50MHz;

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2|GPIO_Pin_3;
	GPIO_Init( GPIOA, &GPIO_InitStructure );



	USART_InitStructure.USART_BaudRate				= 57600; //485
	USART_InitStructure.USART_WordLength			= USART_WordLength_8b;
	USART_InitStructure.USART_StopBits				= USART_StopBits_1;
	USART_InitStructure.USART_Parity				= USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl	= USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode					= USART_Mode_Rx | USART_Mode_Tx;
	USART_Init( USART2, &USART_InitStructure );

	/* Enable USART */
	USART_Cmd( USART2, ENABLE );
	
	USART_ITConfig( USART2, USART_IT_RXNE, ENABLE );
	USART_GetFlagStatus( USART2, USART_FLAG_TC );

	NVIC_InitStructure.NVIC_IRQChannel						= USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority	= 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority			= 2;
	NVIC_InitStructure.NVIC_IRQChannelCmd					= ENABLE;
	NVIC_Init( &NVIC_InitStructure );

	/*
	   STM32 Pin              Remark
	   PC4                 485_Rx_Tx 控制	0:Rx	1: Tx
	   PB.8                 485 电源    1: ON  0:OFF
	 */

	GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType	= GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed	= GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd	= GPIO_PuPd_NOPULL;

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;   //--------- 485 外设置的电
	GPIO_Init( GPIOB, &GPIO_InitStructure );
	GPIO_ResetBits( GPIOB, GPIO_Pin_8 );

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;   //--------- 485const   收发控制线
	GPIO_Init( GPIOC, &GPIO_InitStructure );
	GPIO_ResetBits( GPIOC, GPIO_Pin_4 );

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
static rt_err_t dev_cam_open( rt_device_t dev, rt_uint16_t oflag )
{
	GPIO_SetBits( GPIOB, GPIO_Pin_8 );
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
static rt_err_t dev_cam_close( rt_device_t dev )
{
	GPIO_ResetBits( GPIOB, GPIO_Pin_8 );
	return RT_EOK;
}

/***********************************************************
* Function:cam_read
* Description:数据模式下读取数据
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static rt_size_t dev_cam_read( rt_device_t dev, rt_off_t pos, void* buff, rt_size_t count )
{
	return RT_EOK;
}

/* write one character to serial, must not trigger interrupt */
static void uart2_putc( const char c )
{
	USART_SendData( USART2, c );
	while( !( USART2->SR & USART_FLAG_TC ) )
	{
		;
	}
	USART2->DR = ( c & 0x1FF );
}



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
* Function:		cam_write
* Description:	数据模式下发送数据，要对数据进行封装
* Input:		const void* buff	要发送的原始数据
       rt_size_t count	要发送数据的长度
       rt_off_t pos		使用的socket编号
* Output:
* Return:
* Others:
***********************************************************/

static rt_size_t dev_cam_write( rt_device_t dev, rt_off_t pos, const void* buff, rt_size_t count )
{
	rt_size_t	len = count;
	uint8_t		*p	= (uint8_t*)buff;

	GPIO_SetBits( GPIOC, GPIO_Pin_4 );
	//delay_us(1000);
	while( len )
	{
		USART_SendData( USART2, *p++ );
		while( USART_GetFlagStatus( USART2, USART_FLAG_TC ) == RESET )
		{
		}
		len--;
	}
	//delay_us(3000);
	GPIO_ResetBits( GPIOC, GPIO_Pin_4 );
	return RT_EOK;
}

/***********************************************************
* Function:		cam_control
* Description:	控制模块
* Input:		rt_uint8_t cmd	命令类型
    void *arg       参数,依据cmd的不同，传递的数据格式不同
* Output:
* Return:
* Others:
***********************************************************/
static rt_err_t dev_cam_control( rt_device_t dev, rt_uint8_t cmd, void *arg )
{
	return RT_EOK;
}


typedef enum 
{
	CAM_NONE=0,
	CAM_IDLE=1,
	CAM_START,
	CAM_PHOTO,
	CAM_END
}CAM_STATE;


typedef enum
{
	RX_IDLE=0,
	RX_SYNC1,
	RX_SYNC2,
	RX_HEAD,
	RX_DATA,
	RX_FCS,
	RX_0D,
	RX_0A,
}CAM_RX_STATE;	







/**/

ALIGN( RT_ALIGN_SIZE )
static char thread_cam_stack[1024];
struct rt_thread thread_cam;
/**/
static void rt_thread_entry_cam( void* parameter )
{

	uint8_t Take_photo[10] = { 0x40, 0x40, 0x61, 0x81, 0x02, 0X00, 0X00, 0X02, 0X0D, 0X0A };	//----	报警拍照命令
	uint8_t Fectch_photo[10] = { 0x40, 0x40, 0x62, 0x81, 0x02, 0X00, 0XFF, 0XFF, 0X0D, 0X0A };	//----- 报警取图命令

	uint8_t ch,cam_no=0x1;
	__IO uint8_t cam_rxed = 0;
	CAM_STATE cam_state=CAM_IDLE;

	__IO uint32_t tick;
	uint32_t cam_photo_size=0;
	uint8_t cam_last_page=0;
	uint16_t page_seq;
	uint16_t page_size;
	uint32_t i;

	uint8_t cam_rx_head[5];
	uint8_t cam_rx_head_wr=0;
	uint8_t retry=3;  /*老摄像头一次命令不能成功*/

	CAM_RX_STATE cam_rx_state=RX_IDLE;

	rt_thread_delay( RT_TICK_PER_SECOND *2 );
	dev_cam_open(&dev_cam,RT_DEVICE_FLAG_RDWR);

	while( 1 )
	{
		rt_thread_delay( RT_TICK_PER_SECOND / 20 );
		/*串口是否收到数据*/
		while( uart2_rxbuf_rd != uart2_rxbuf_wr )
		{
			ch				= uart2_rxbuf[uart2_rxbuf_rd++];
			uart2_rxbuf_rd	%= UART2_RXBUF_SIZE;
			//rt_kprintf("%02X",ch);
			switch(cam_rx_state)
			{
				case RX_DATA: /*保存信息格式: 位置(2B) 大小(2B) FLAG_FF 数据 0D 0A*/
					uart2_rx[uart2_rx_wr++] = ch;
					uart2_rx_wr				%= UART2_RX_SIZE;
					if(uart2_rx_wr==page_size) cam_rx_state=RX_FCS;
					break;	
				case RX_IDLE:
					if(ch==0x40) cam_rx_state=RX_SYNC1;
					break;
				case RX_SYNC1:
					if(ch==0x40) cam_rx_state=RX_SYNC2;
					else cam_rx_state=RX_IDLE;
					break;
				case RX_SYNC2:
					if(ch==0x63)
					{
						cam_rx_head_wr=0;
						cam_rx_state=RX_HEAD;
					}	
					else 
						cam_rx_state=RX_IDLE;
					break;
				case RX_HEAD:
					cam_rx_head[cam_rx_head_wr++]=ch;
					if(cam_rx_head_wr==5)
					{
						uart2_rx_wr=0;
						page_size=(cam_rx_head[3]<<8)|cam_rx_head[2];
						cam_rx_state=RX_DATA;
					}
					break;
				case RX_FCS:
					cam_rx_state=RX_0D;
					break;		
				case RX_0D:
					if(ch==0x0d) cam_rx_state=RX_0A;
					else cam_rx_state=RX_IDLE;
					break;	
				case RX_0A:
					if(ch==0x0a) cam_rxed=1;
					cam_rx_state=RX_IDLE;
					break;	
			}
		}
		/*cam接收处理*/
		switch( cam_state )
		{
			case CAM_IDLE:
				dev_cam_open(&dev_cam,RT_DEVICE_FLAG_RDWR);
				tick=rt_tick_get();
				retry=3;
				cam_state = CAM_START;
				break;
			case CAM_START:
				if(rt_tick_get()-tick<RT_TICK_PER_SECOND/10) break;
				Take_photo[4]=cam_no;
				rt_kprintf("\r\n%d>CAM%d START\r\n",rt_tick_get()*10,cam_no);
				dev_cam_write( &dev_cam, 0, Take_photo, 10 );
				
				cam_photo_size=0;
				cam_last_page=0;
				tick=rt_tick_get();
				cam_state = CAM_PHOTO;
				break;
			case CAM_PHOTO:
				if(rt_tick_get()-tick>RT_TICK_PER_SECOND*5)/*超时*/
				{
					rt_kprintf("\r\nCAM%d timeout\r\n",cam_no);
					retry--;
					if(retry) 
					{
						cam_state=CAM_START;
						tick=rt_tick_get();
					}	
					else
						cam_state=CAM_END;
				}
				if(cam_rxed==0) break;  /*没有收到数据，跳出*/
				cam_rxed=0;	/*收到数据，清除*/
				/*收到数据,存储,判断是否图片结束*/
				if(uart2_rx_wr>512)		/*数据大于512,非法*/
				{
					uart2_rx_wr=0;
					rt_kprintf("\r\nCAM%d invalided\r\n",cam_no);
					cam_state=CAM_END;
					break;
				}
				if(uart2_rx_wr==512)
				{
					if((uart2_rx[510]==0xff)&&(uart2_rx[511]==0xD9))
					{
						cam_last_page=1;
					}
				}
				else
				{
					cam_last_page=1;
				}
				cam_photo_size+=uart2_rx_wr;
				/*保存数据*/
				rt_kprintf("\r\n%d>CAM%d photo %d bytes\r\n",rt_tick_get()*10,cam_no,cam_photo_size);
				uart2_rx_wr=0;
				
				if(cam_last_page)
				{
					cam_state=CAM_END;
					break;
				}
				Fectch_photo[4]=cam_no;
				dev_cam_write( &dev_cam, 0, Fectch_photo, 10 );
				tick=rt_tick_get();
				break;
			case CAM_END:
				cam_no++;
				if(cam_no>4)
					cam_state=CAM_NONE;
				else
					cam_state=CAM_IDLE;
				break;
		}
	}
}

/*cam设备初始化*/
void cam_init( void )
{
	dev_cam.type	= RT_Device_Class_Char;
	dev_cam.init	= dev_cam_init;
	dev_cam.open	= dev_cam_open;
	dev_cam.close	= dev_cam_close;
	dev_cam.read	= dev_cam_read;
	dev_cam.write	= dev_cam_write;
	dev_cam.control = dev_cam_control;

	rt_device_register( &dev_cam, "cam", RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_STANDALONE );
	rt_device_init( &dev_cam );

rt_thread_init( &thread_cam,
				"cam",
				rt_thread_entry_cam,
				RT_NULL,
				&thread_cam_stack[0],
				sizeof( thread_cam_stack ), 18, 5 );
rt_thread_startup( &thread_cam );

	


}

/*cam开关*/
rt_err_t cam_onoff( uint8_t openflag )
{
	if( openflag == 0 )
	{
		GPIO_ResetBits( GPIOB, GPIO_Pin_8 );
	} else
	{
		GPIO_SetBits( GPIOB, GPIO_Pin_8 );
	}
	return 0;
}

FINSH_FUNCTION_EXPORT( cam_onoff, cam_onoff([1 | 0] ) );

/**/
rt_size_t cam( uint8_t ch)
{
	if(ch==0)
		GPIO_ResetBits( GPIOC, GPIO_Pin_4 );
	else	
		GPIO_SetBits( GPIOC, GPIO_Pin_4 );
	return 0;
}

FINSH_FUNCTION_EXPORT( cam, write to cam );

/************************************** The End Of File **************************************/

