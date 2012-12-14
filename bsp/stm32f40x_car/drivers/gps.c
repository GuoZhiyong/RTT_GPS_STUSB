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
#include "gps.h"


#include <finsh.h>

#define GPS_PWR_PORT	GPIOD
#define GPS_PWR_PIN		GPIO_Pin_10

#define GPS_GPIO_TX	    GPIO_Pin_12      // PC12
#define GPS_GPIO_RX	    GPIO_Pin_2        // PD2
#define GPS_GPIO_TxC	GPIOC
#define GPS_GPIO_RxD	GPIOD
#define RCC_APBPeriph_UART5	RCC_APB1Periph_UART5


/*声明一个gps设备*/
static struct rt_device dev_gps;
static uint32_t	gps_baud=9600;
static uint32_t	gps_out_mode=GPS_OUTMODE_ALL;


/*串口接收缓存区定义*/
#define UART5_RX_SIZE	256
static uint8_t uart5_rxbuf[UART5_RX_SIZE];
static uint16_t uart5_rxbuf_wr=0;


/*gps原始信息数据区定义*/
#define GPS_RAWINFO_SIZE	256
static uint8_t gps_rawinfo[GPS_RAWINFO_SIZE];
static uint16_t gps_rawinfo_wr=0;

static struct rt_semaphore	sem_gps;


/*
gps接收中断处理，收到\n认为收到一包
收到一包调用处理函数
*/
void UART5_IRQHandler( void )
{
	uint8_t ch;
	rt_interrupt_enter( );
	if(USART_GetITStatus(UART5, USART_IT_RXNE) != RESET)
	{
		ch=USART_ReceiveData(UART5);
		if(ch<0x20)
		{
			if(ch==0x0d)
			{
				memcpy(gps_rawinfo,uart5_rxbuf,uart5_rxbuf_wr);
				gps_rawinfo_wr=uart5_rxbuf_wr;
				uart5_rxbuf_wr=0;
				rt_sem_release( &sem_gps);
			}
		}
		else
		{
			uart5_rxbuf[uart5_rxbuf_wr++]=ch;
			if(uart5_rxbuf_wr==UART5_RX_SIZE) uart5_rxbuf_wr=0;
			uart5_rxbuf[uart5_rxbuf_wr]=0;
		}
		USART_ClearITPendingBit(UART5, USART_IT_RXNE);

	}
	//if (USART_GetITStatus(UART5, USART_IT_TC) != RESET)
	//{
	//	USART_ClearITPendingBit(UART5, USART_IT_TC);
	//}
	rt_interrupt_leave( );
}






/*初始化*/
static rt_err_t dev_gps_init( rt_device_t dev )
{
	GPIO_InitTypeDef GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	USART_InitTypeDef USART_InitStructure;


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

	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_Init( GPIOC, &GPIO_InitStructure );

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_Init( GPIOD, &GPIO_InitStructure );

	GPIO_PinAFConfig( GPIOC, GPIO_PinSource12, GPIO_AF_UART5 );
	GPIO_PinAFConfig( GPIOD, GPIO_PinSource2, GPIO_AF_UART5 );

/*NVIC 设置*/
	NVIC_InitStructure.NVIC_IRQChannel = UART5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	USART_InitStructure.USART_BaudRate = gps_baud;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(UART5, &USART_InitStructure);
	
	USART_Cmd(UART5, ENABLE);
	USART_ITConfig( UART5, USART_IT_RXNE, ENABLE );	
	//GPIO_SetBits(GPIOD, GPIO_Pin_10); 

	return RT_EOK;
}


static rt_err_t dev_gps_open( rt_device_t dev, rt_uint16_t oflag )
{
	USART_InitTypeDef USART_InitStructure;

	rt_kprintf("baud=%d\n",gps_baud);
	//rt_hw_interrupt_disable();
	//USART_DeInit(UART5);
	//USART_ITConfig( UART5, USART_IT_RXNE, DISABLE);	
	//USART_Cmd(UART5,DISABLE);
	//RCC_APB1PeriphClockCmd( RCC_APB1Periph_UART5, ENABLE );
	//USART_InitStructure.USART_BaudRate = gps_baud;
	//USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	//USART_InitStructure.USART_StopBits = USART_StopBits_1;
	//USART_InitStructure.USART_Parity = USART_Parity_No;
	//USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	//USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	//USART_Init(UART5, &USART_InitStructure);
	/* Enable USART */
	//USART_Cmd(UART5, ENABLE);
	//USART_ClearITPendingBit(UART5, USART_IT_RXNE);
	//USART_ITConfig( UART5, USART_IT_RXNE, ENABLE );	
	//rt_hw_interrupt_enable();
	//GPIO_SetBits( GPS_PWR_PORT, GPS_PWR_PIN );
	GPIO_SetBits(GPIOD, GPIO_Pin_10); 
	return RT_EOK;
}




static rt_err_t dev_gps_close( rt_device_t dev )
{
	//GPIO_ResetBits( GPS_PWR_PORT, GPS_PWR_PIN );
	GPIO_ResetBits(GPIOD, GPIO_Pin_10); 
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
static void uart5_putc(const char c)
{
	USART_SendData(UART5,  c); 
	while (!(UART5->SR & USART_FLAG_TXE));  
	UART5->DR = (c & 0x1FF);  
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
	rt_size_t len=count;
	uint8_t *p=(uint8_t *)buff;

	while (len)
	{
		USART_SendData(UART5,*p++);
		while (USART_GetFlagStatus(UART5, USART_FLAG_TC) == RESET)
		{}
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
	switch( cmd )
	{
		case CTL_GPS_BAUD:
			break;
		case CTL_GPS_OUTMODE: 
			break;
	}
	return RT_EOK;
}


ALIGN( RT_ALIGN_SIZE )
static char thread_gps_stack[512];
struct rt_thread thread_gps;

static void rt_thread_entry_gps( void* parameter )
{
	rt_err_t		res;
/*gsm的状态切换*/
	while( 1 )
	{
		res = rt_sem_take( &sem_gps, RT_TICK_PER_SECOND / 20 );    //等待100ms,实际上就是变长的延时,最长100ms
		if( res == RT_EOK ) //收到一包数据
		{
			if(strncmp((char*)gps_rawinfo,"$GPRMC",6)==0)
			{
				/*解析*/
				rt_kprintf("$GPRMC\n");
				/*是否传递出去*/
				if(gps_out_mode&GPS_OUTMODE_TRIGGER)
				{

				}
			}
			if(gps_out_mode==GPS_OUTMODE_ALL)
			{
				rt_kprintf("%s\n",gps_rawinfo);
			}
			gps_rawinfo_wr=0;
		}
		rt_thread_delay(RT_TICK_PER_SECOND/20);
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

	rt_device_register( &dev_gps, "gps", RT_DEVICE_FLAG_RDWR|RT_DEVICE_FLAG_STANDALONE);
	rt_device_init( &dev_gps );
}




rt_err_t gps_open(const int baud)
{
	//gps_baud=baud;
	//return dev_gps_open(&dev_gps,RT_DEVICE_OFLAG_RDONLY);
	GPIO_SetBits(GPIOD, GPIO_Pin_10); 
}
FINSH_FUNCTION_EXPORT( gps_open, open gps with baud );

void gps_close(void)
{
	GPIO_ResetBits(GPIOD, GPIO_Pin_10); 
}
FINSH_FUNCTION_EXPORT( gps_close, close gps );


/************************************** The End Of File **************************************/
