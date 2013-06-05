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
#include "stm32f4xx.h"
#include <rtthread.h>
#include <finsh.h>

#include "msglist.h"

#define  CTL_RS485_RX	GPIO_ResetBits( GPIOC, GPIO_Pin_4 )
#define  CTL_RS485_TX	GPIO_SetBits( GPIOC, GPIO_Pin_4 )
#define  CTL_RS485_ON	GPIO_SetBits( GPIOB, GPIO_Pin_8 )
#define  CTL_RS485_OFF	GPIO_ResetBits( GPIOB, GPIO_Pin_8 )

#define UART2_RX_SIZE 256
static uint8_t	uart2_rx[UART2_RX_SIZE];
static uint16_t uart2_rx_wr = 0;
static uint16_t uart2_rx_rd = 0;

#define RS485_RX_SIZE 600
static uint8_t				rs485_rx[RS485_RX_SIZE];
static uint16_t				rs485_rx_wr = 0;
static uint16_t				rs485_rx_rd = 0;

static struct rt_semaphore	sem_rs485;


/***********************************************************
* Function:
* Description:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
void USART2_IRQHandler( void )
{
	rt_interrupt_enter( );

	if( USART_GetITStatus( USART2, USART_IT_RXNE ) != RESET )
	{
		uart2_rx[uart2_rx_wr++] = USART_ReceiveData( USART2 ) & 0xFF;
		uart2_rx_wr				%= UART2_RX_SIZE;
		USART_ClearITPendingBit( USART2, USART_IT_RXNE );
	}
	rt_interrupt_leave( );
}

#if 0
_485_dev_rx[_485dev_wr++] = data;

switch( _485_RXstatus._485_receiveflag )
{
	case  IDLE_485:
		if( ( 0xAA != _485_dev_rx[0] ) && ( 0x40 != _485_dev_rx[0] ) )
		{ //	判断 第一个字节是否合法，否则直接清除
			_485dev_wr = 0;
			break;
		}
		switch( _485dev_wr )
		{
			case  2:   if( ( 0x55 != _485_dev_rx[1] ) && ( 0x40 != _485_dev_rx[1] ) )
				{
					_485dev_wr = 0;
				}
				break;
			case 3:
				//----------  Check  LCD  --------------------------
				if( ( 0xAA == _485_dev_rx[0] ) && ( 0x55 == _485_dev_rx[1] ) )
				{ //AA 55 08 83 30 00 02 1E FF FF 00					08 是长度(包含1BYTE 指令2 BYTES  地址)
					_485_RXstatus._485_RxLen		= _485_dev_rx[2];
					_485_RXstatus._485_receiveflag	= LARGE_LCD;
				}else
				if( 0x40 != _485_dev_rx[0] )
				{
					_485dev_wr = 0;
				}
				break;
			case 8:                                                     //		  -------  Camera  Data  -----------
				if( ( 0x40 == _485_dev_rx[0] ) && ( 0x40 == _485_dev_rx[1] ) && ( 0x63 == _485_dev_rx[2] ) )
				{                                                       //	40	40	63	FF	FF	00	02		   00  02  是长度  (小端)
					_485_RXstatus._485_RxLen = ( (u16)_485_dev_rx[6] << 8 ) + (u16)_485_dev_rx[5];
					//----------------------------------
					_485dev_wr						= 0;                //clear now , the bytes  receiving	later  is  pic data
					_485_RXstatus._485_receiveflag	= CAMERA_Related;   //	return Idle
				}else
				{
					_485dev_wr = 0;                                     // clear
				}
				break;
		}

		break;
	case LARGE_LCD:
		if( _485dev_wr >= ( _485_RXstatus._485_RxLen + 3 ) )            //	AA	55	08
		{
			//	send msg_queue
			_485_MsgQue_sruct.info	= _485_dev_rx;
			_485_MsgQue_sruct.len	= _485dev_wr;
			// rt_mq_send( &_485_MsgQue, (void*)&_485_MsgQue_sruct, _485_MsgQue_sruct.len+ 2 );
			//--------------------------
			memset( DwinLCD.RxInfo, 0, sizeof( (const char*)DwinLCD.RxInfo ) );
			memcpy( DwinLCD.RxInfo, _485_dev_rx, _485_RXstatus._485_RxLen );
			DwinLCD.RxInfolen		= _485_RXstatus._485_RxLen;
			DwinLCD.Process_Enable	= 1;
			//--------------------------
			_485dev_wr						= 0; // clear
			_485_RXstatus._485_receiveflag	= IDLE_485;
		}
		break;
	case  CAMERA_Related:
		if( _485dev_wr >= _485_RXstatus._485_RxLen )
		{
			memset( _485_content, 0, sizeof( _485_content ) );
			//--------------------------------------------------
			memcpy( _485_content, _485_dev_rx, _485_RXstatus._485_RxLen );
			_485_content_wr			= _485_RXstatus._485_RxLen; // Packet info len
			_485_CameraData_Enable	= 1;                        // 图片数据过来了
			//---------------------------------------------------
			_485dev_wr						= 0;                // clear
			_485_RXstatus._485_receiveflag	= IDLE_485;
		}
		break;
	default:
		_485dev_wr						= 0;
		_485_RXstatus._485_receiveflag	= IDLE_485;
		break;
}

#endif

#define    IDLE_485			0
#define    DEV_CAMERA	1
#define    DEV_LARGE_LCD		3

static uint8_t rx485_stage = IDLE_485;

/*
接收CAMERA和大屏的数据

*/


static uart2_addbyte( uint8_t ch )
{


}

ALIGN( RT_ALIGN_SIZE )
static char thread_rs485_stack[512];
struct rt_thread thread_rs485;


/***********************************************************
* Function:       rt_thread_entry_485
* Description:    大屏、拍照处理
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static void rt_thread_entry_rs485( void* parameter )
{
	rt_tick_t		curr_ticks;
	rt_err_t		res;
	unsigned char	ch;

	while( 1 )
	{
		while( uart2_rx_rd != uart2_rx_wr )
		{
			ch			= uart2_rx[uart2_rx_rd++];
			uart2_rx_rd %= UART2_RX_SIZE;
			uart2_addbyte( ch );
		}
		rt_thread_delay( RT_TICK_PER_SECOND / 20 );
	}
}

/***********************************************************
* Function:
* Description:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static rt_err_t   uart2_init( void )
{
	GPIO_InitTypeDef	GPIO_InitStructure;
	USART_InitTypeDef	USART_InitStructure;
	NVIC_InitTypeDef	NVIC_InitStructure;

	//  1 . Clock

	/* Enable USART2 and GPIOA clocks */
	RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOA, ENABLE );
	/* Enable USART2 clock */
	RCC_APB1PeriphClockCmd( RCC_APB1Periph_USART2, ENABLE );
	RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOB, ENABLE );

	//   2.  GPIO
	GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType	= GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd	= GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed	= GPIO_Speed_50MHz;

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;
	GPIO_Init( GPIOA, &GPIO_InitStructure );

	/* Connect alternate function */
	GPIO_PinAFConfig( GPIOA, GPIO_PinSource2, GPIO_AF_USART2 );
	GPIO_PinAFConfig( GPIOA, GPIO_PinSource3, GPIO_AF_USART2 );

	//  3.  Interrupt
	/* Enable the USART2 Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel						= USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority	= 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority			= 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd					= ENABLE;
	NVIC_Init( &NVIC_InitStructure );

	//   4.  uart  Initial
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

	/* -------------- 485  操作相关 -----------------	*/


	/*
	   STM32 Pin	           Remark
	   PC4		            485_Rx_Tx 控制   0:Rx    1: Tx
	   PD0		            485 电源	1: ON  0:OFF
	 */

	/*  管脚初始化 设置为 推挽输出 */

	GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType	= GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed	= GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd	= GPIO_PuPd_NOPULL;

	// ------------- PD10     --------------------------------
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;   //--------- 485 外设置的电
	GPIO_Init( GPIOB, &GPIO_InitStructure );

	CTL_RS485_ON;

	//------------------- PC4------------------------------
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;   //--------- 485const	收发控制线
	GPIO_Init( GPIOC, &GPIO_InitStructure );
	CTL_RS485_RX;
	return RT_EOK;
}

/***********************************************************
* Function:
* Description:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
void rs485_init( void )
{
	rt_thread_t tid;

	uart2_init( );
	rt_sem_init( &sem_rs485, "sem_rs485", 0, RT_IPC_FLAG_FIFO );

	rt_thread_init( &thread_rs485,
	                "rs485",
	                rt_thread_entry_rs485,
	                RT_NULL,
	                &thread_rs485_stack[0],
	                sizeof( thread_rs485_stack ), 14, 5 );
	rt_thread_startup( &thread_rs485 );
}

/***********************************************************
* Function:  拍照
* Description:
* Input:  uint8_t ch  通道
   uint16_t cmd  0:停止拍照  0xFFFF 录像   其他表示 拍照张数
   uint16_t interval   拍照间隔
   uint8_t mode:   保存标志 1 保存 0 实时上传
* Output:
* Return:
* Others:
***********************************************************/

/*也可以传递一个struct,再修改finsh的命令*/



void cam_photo( uint8_t ch, uint16_t cmd, uint16_t interval, uint8_t savemode )
{
/*添加到拍照list*/
}

FINSH_FUNCTION_EXPORT( cam_photo, take photo );

/************************************** The End Of File **************************************/
