/************************************************************
 * Copyright (C), 2008-2012,
 * FileName:		m66
 * Author:			bitter
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
#include <board.h>
#include <finsh.h>

#include "jt808.h"

#include "gsm.h"
#include "m66.h"

#define GSM_GPIO			GPIOC
#define GSM_TX_PIN			GPIO_Pin_10
#define GSM_TX_PIN_SOURCE	GPIO_PinSource10

#define GSM_RX_PIN			GPIO_Pin_11
#define GSM_RX_PIN_SOURCE	GPIO_PinSource11

typedef void ( *URC_CB )( char *s, uint16_t len );
typedef rt_err_t ( *RESP_FUNC )( char *s, uint16_t len );

#define GSM_PWR_PORT	GPIOD
#define GSM_PWR_PIN		GPIO_Pin_13

#define GSM_TERMON_PORT GPIOD
#define GSM_TERMON_PIN	GPIO_Pin_12

#define GSM_RST_PORT	GPIOD
#define GSM_RST_PIN		GPIO_Pin_11

/*声明一个gsm设备*/
static struct rt_device dev_gsm;


/*声明一个uart设备指针,同gsm模块连接的串口
   指向一个已经打开的串口
 */
static struct rt_mailbox	mb_gsmrx;
#define MB_GSMRX_POOL_SIZE 32
static uint8_t				mb_gsmrx_pool[MB_GSMRX_POOL_SIZE];

/*语音播报使用的mailbox*/
static struct rt_mailbox	mb_tts;
#define MB_TTS_POOL_SIZE 32
static uint8_t				mb_tts_pool[MB_TTS_POOL_SIZE];

/*模块是否忙，在送数、删短信、TTS合成等*/
//static struct rt_semaphore sem_gsmbusy;

enum
{
	GSMBUSY_NONE=0x0,
	GSMBUSY_POWERON,    /*上电的过程中*/
	GSMBUSY_AT,         /*AT*/
	GSMBUSY_TTS,        /*TTS指令*/
	GSMBUSY_VOICE,      /*录音*/
	GSMBUSY_SOCKET,     /*上报数据*/
} gsmbusy = GSMBUSY_NONE;

#define GSM_RX_SIZE 2048
static uint8_t		gsm_rx[GSM_RX_SIZE];
static uint16_t		gsm_rx_wr = 0;

static T_GSM_STATE	gsm_state = GSM_IDLE;

/*串口接收缓存区定义*/
#define UART4_RX_SIZE 256
static uint8_t	uart4_rxbuf[UART4_RX_SIZE];
static uint16_t uart4_rxbuf_wr = 0, uart4_rxbuf_rd = 0;

/*控制输出多少条信息*/
static uint32_t fgsm_rawdata_out = 0xfffffff;

/*最近一次收到串口数据的时刻,不使用sem作为超时判断*/
static uint32_t last_tick;

/*是否在播报中，不能播报下一条*/
static uint8_t tts_busy = 0;

struct _gsm_param
{
	char	imei[16];
	char	imsi[16];
	uint8_t csq;
	char	ip[16];
}				gsm_param;

static uint8_t	socket_linkno = 0;


/***********************************************************
* Function:
* Description: uart4的中断服务函数
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
void UART4_IRQHandler( void )
{
	rt_interrupt_enter( );
	if( USART_GetITStatus( UART4, USART_IT_RXNE ) != RESET )
	{
		uart4_rxbuf[uart4_rxbuf_wr++]	= USART_ReceiveData( UART4 );
		uart4_rxbuf_wr					%= UART4_RX_SIZE;
		USART_ClearITPendingBit( UART4, USART_IT_RXNE );
		last_tick = rt_tick_get( );
	}


/*
   if (USART_GetITStatus(UART4, USART_IT_TC) != RESET)
   {
   USART_ClearITPendingBit(UART4, USART_IT_TC);
   }
 */
	rt_interrupt_leave( );
}

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
static void urc_cb_default( char *s, uint16_t len )
{
	rt_kprintf( "\rrx>%s", s );
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
static void urc_cb_ciev( char *s, uint16_t len )
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
static void urc_cb_ring( char *s, uint16_t len )
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
static void urc_cb_cring( char *s, uint16_t len )
{
}

/*
   urc: unsolicited result code
 */
struct
{
	char	*code;
	URC_CB	pfunc;
} urc[] =
{
	//{ "^SYSSTART", urc_cb_sysstart },
	//{ "^SHUTDOWN", urc_cb_shutdown },
	{ "+CIEV:",	   urc_cb_ciev	  },
	{ "RING",	   urc_cb_ring	  },
	{ "+CRING:",   urc_cb_cring	  },
	{ "+CREG:",	   urc_cb_default },
	{ "^SIS:",	   urc_cb_default },
	{ "+CGEV:",	   urc_cb_default },
	{ "+CGREG:",   urc_cb_default },
	{ "+CMT:",	   urc_cb_default },
	{ "+CBM:",	   urc_cb_default },
	{ "+CDS:",	   urc_cb_default },
	{ "+CALA:",	   urc_cb_default },
	{ "CME ERROR", urc_cb_default },
	{ "CMS ERROR", urc_cb_default },
	{ "",		   NULL			  }
};


/***********************************************************
* Function:		sys_default_cb
* Description:	缺省的系统回调处理函数
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/

static void sys_default_cb( char *pInfo, uint16_t len )
{
	int		i;
	char	match = 0;

	rt_kprintf( "\rrx>%s", pInfo );
/*有可能是主动挂断，也有可能网络链接断开，是否要通知? 不需要*/
	if( strncmp( pInfo, "%IPCLOSE", 7 ) == 0 )
	{
	}

//判断非请求结果码-主动上报命令
	for( i = 0;; i++ )
	{
		if( urc[i].pfunc == NULL )
		{
			break;
		}
		if( strncmp( pInfo, urc[i].code, strlen( urc[i].code ) == 0 ) )
		{
			( urc[i].pfunc )( pInfo, len );
			match = 1; //已处理
			break;
		}
	}
	if( match )
	{
		return;
	}

//AT命令的交互，区分是自身处理还是来自APP的命令
}

#endif


/***********************************************************
* Function:
* Description: 将小于0x20的字符忽略掉。并在结尾添加0，转为
   可见的字符串。
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static uint16_t strip_string( char *str )
{
	char		*psrc, *pdst;
	uint16_t	len = 0;
	psrc	= str;
	pdst	= str;
	while( *psrc )
	{
		if( *psrc > 0x20 )
		{
			*pdst++ = toupper( *psrc );
			len++;
		}
		psrc++;
	}
	*pdst = 0;
	return len;
}

/***********************************************************
* Function:
* Description: 将小于0x20的字符忽略掉。只保留数字部分。
               并在结尾添加0，转为可见的字符串。
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static uint16_t strip_numstring( char *str )
{
	char		*psrc, *pdst;
	uint16_t	len = 0;
	psrc	= str;
	pdst	= str;
	while( *psrc )
	{
		if( ( *psrc >= '0' ) && ( *psrc <= '9' ) )
		{
			*pdst++ = *psrc;
			len++;
		}else
		{
			if( len )
			{
				break;
			}
		}
		psrc++;
	}
	*pdst = 0;
	return len;
}

/***********************************************************
* Function:
* Description: 配置控电管脚，配置对应的串口设备uart4
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static rt_err_t m66_init( rt_device_t dev )
{
	GPIO_InitTypeDef	GPIO_InitStructure;
	USART_InitTypeDef	USART_InitStructure;
	NVIC_InitTypeDef	NVIC_InitStructure;

	RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOD, ENABLE );
	RCC_APB1PeriphClockCmd( RCC_APB1Periph_UART4, ENABLE );

	GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType	= GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd	= GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Speed	= GPIO_Speed_50MHz;

	GPIO_InitStructure.GPIO_Pin = GSM_PWR_PIN;
	GPIO_Init( GSM_PWR_PORT, &GPIO_InitStructure );
	GPIO_ResetBits( GSM_PWR_PORT, GSM_PWR_PIN );

	GPIO_InitStructure.GPIO_Pin = GSM_TERMON_PIN;
	GPIO_Init( GSM_TERMON_PORT, &GPIO_InitStructure );
	GPIO_ResetBits( GSM_TERMON_PORT, GSM_TERMON_PIN );


/*
   RESET在开机过程不需要做任何时序配合（和通常CPU 的 reset不同）。
   建议该管脚接OC输出的GPIO，开机时 OC 输出高阻。
 */
	GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType	= GPIO_OType_OD;
	GPIO_InitStructure.GPIO_PuPd	= GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Speed	= GPIO_Speed_50MHz;

	GPIO_InitStructure.GPIO_Pin = GSM_RST_PIN;
	GPIO_Init( GSM_RST_PORT, &GPIO_InitStructure );
	GPIO_SetBits( GSM_RST_PORT, GSM_RST_PIN );

/*uart4 管脚设置*/

	/* Configure USART Tx as alternate function  */
	GPIO_InitStructure.GPIO_OType	= GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd	= GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed	= GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin		= GSM_TX_PIN | GSM_RX_PIN;
	GPIO_Init( GSM_GPIO, &GPIO_InitStructure );

	GPIO_PinAFConfig( GSM_GPIO, GSM_TX_PIN_SOURCE, GPIO_AF_UART4 );
	GPIO_PinAFConfig( GSM_GPIO, GSM_RX_PIN_SOURCE, GPIO_AF_UART4 );

	NVIC_InitStructure.NVIC_IRQChannel						= UART4_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority	= 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority			= 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd					= ENABLE;
	NVIC_Init( &NVIC_InitStructure );

	USART_InitStructure.USART_BaudRate				= 57600;
	USART_InitStructure.USART_WordLength			= USART_WordLength_8b;
	USART_InitStructure.USART_StopBits				= USART_StopBits_1;
	USART_InitStructure.USART_Parity				= USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl	= USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode					= USART_Mode_Rx | USART_Mode_Tx;
	USART_Init( UART4, &USART_InitStructure );
	/* Enable USART */
	USART_Cmd( UART4, ENABLE );
	USART_ITConfig( UART4, USART_IT_RXNE, ENABLE );

	return RT_EOK;
}

/***********************************************************
* Function:	提供给其他thread调用，打开设备，超时判断
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static rt_err_t m66_open( rt_device_t dev, rt_uint16_t oflag )
{
	if( gsm_state == GSM_IDLE )
	{
		gsm_state = GSM_POWERON; //置位上电过程中
	}
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
static rt_err_t m66_close( rt_device_t dev )
{
	gsm_state = GSM_POWEROFF; //置位断电过程中
	return RT_EOK;
}

/***********************************************************
* Function:m66_read
* Description:数据模式下读取数据
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static rt_size_t m66_read( rt_device_t dev, rt_off_t pos, void* buff, rt_size_t count )
{
	return RT_EOK;
}

/* write one character to serial, must not trigger interrupt */
static void uart4_putc( const char c )
{
	USART_SendData( UART4, c );
	while( !( UART4->SR & USART_FLAG_TC ) )
	{
		;
	}
	UART4->DR = ( c & 0x1FF );
}

/***********************************************************
* Function:		m66_write
* Description:	数据模式下发送数据，要对数据进行封装
* Input:		const void* buff	要发送的原始数据
       rt_size_t count	要发送数据的长度
       rt_off_t pos		使用的socket编号
* Output:
* Return:
* Others:
***********************************************************/

static rt_size_t m66_write( rt_device_t dev, rt_off_t pos, const void* buff, rt_size_t count )
{
	rt_size_t	len = count;
	uint8_t		*p	= (uint8_t*)buff;

	while( len )
	{
		USART_SendData( UART4, *p++ );
		while( USART_GetFlagStatus( UART4, USART_FLAG_TC ) == RESET )
		{
		}
		len--;
	}
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
static rt_err_t m66_control( rt_device_t dev, rt_uint8_t cmd, void *arg )
{
	switch( cmd )
	{
		case CTL_STATUS:
			*( (int*)arg ) = gsm_state;
			break;
		case CTL_AT_CMD: //发送at命令,结果要返回
			break;
		case CTL_PPP:
			break;
		case CTL_SOCKET:
			break;
	}
	return RT_EOK;
}

/***********************************************************
* Function:		gsmrx_cb
* Description:	gsm收到信息的处理
* Input:			char *s     信息
    uint16_t len 长度
* Output:
* Return:
* Others:
***********************************************************/
static void gsmrx_cb( uint8_t *pInfo, uint16_t size )
{
	int		i, count, len = size;
	uint8_t tbl[24] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0, 0, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf };
	char	c, *pmsg;
	uint8_t *psrc, *pdst;
	int32_t infolen, linkno;

/*网络侧的信息，直接通知上层软件*/
	if( fgsm_rawdata_out )
	{
		rt_kprintf( "\r\n%08d gsm_rx>%s\r\n", rt_tick_get( ), pInfo );
		fgsm_rawdata_out--;
	}

/*判读并处理*/
	psrc = pInfo;
	if( strncmp( psrc, "%IPDATA:", 7 ) == 0 )
	{
		/*解析出净信息,编译器会优化掉pdst*/
		i = sscanf( psrc, "%%IPDATA:%d,%d,%s", &linkno, &infolen, pdst );
		if( i != 3 )
		{
			return;
		}
		if( infolen < 11 )
		{
			return;
		}
		if( *pdst != '"' )
		{
			return;
		}
		psrc	= pdst;
		pmsg	= pdst + 1; /*指向下一个位置*/
		for( i = 0; i < infolen; i++ )
		{
			c		= tbl[*pmsg++ - '0'] << 4;
			c		|= tbl[*pmsg++ - '0'];
			*pdst++ = c;
		}
		gprs_rx( linkno, psrc, infolen );
		return;
	}
/*TTS*/


/*
   if( strncmp( psrc, "%TTS: 0", 7 ) == 0 )
   {
   tts_busy = 0;
   }
 */

/*短信*/
	if( strncmp( psrc, "+CMTI:\"SM\",", 11 ) == 0 )
	{
	}

	/*直接发送到Mailbox中,内部处理*/
	pmsg = rt_malloc( len + 2 );
	if( pmsg != RT_NULL )
	{
		*pmsg			= len >> 8;
		*( pmsg + 1 )	= len;
		memcpy( pmsg + 2, pInfo, len );
		rt_mb_send( &mb_gsmrx, (rt_uint32_t)pmsg );
	}
	return;
}

/*响应CREG或CGREG
   +CREG:0,1
   +CGREG:0,5

 */
rt_err_t resp_CGREG( char *p, uint16_t len )
{
	uint32_t i, n, code;
	i = sscanf( p, "%*[^:]:%d,%d", &n, &code );
	if( i != 2 )
	{
		return RT_ERROR;
	}
	if( ( code != 1 ) && ( code != 5 ) )
	{
		return RT_ERROR;
	}
	return RT_EOK;
}

/*
   SIM卡的IMSI号码为4600 00783208249，
      460 00 18 23 20 86 42

   接口号数据字段的内容为IMSI号码的后12位
   其6个字节的内容为 0x00 0x07 0x83 0x20 0x82 0x49

 */
rt_err_t resp_CIMI( char *p, uint16_t len )
{
	rt_kprintf( "cimi len=%d  %02x %02x\n", len, *p, *( p + 1 ) );
	if( len < 15 )
	{
		return RT_ERROR;
	}
	strip_numstring( p );
	strcpy( gsm_param.imsi, p );
	return RT_EOK;
}

/*响应CGSN**/
rt_err_t resp_CGSN( char *p, uint16_t len )
{
	if( len < 15 )
	{
		return RT_ERROR;
	}
	strip_numstring( p );
	strcpy( gsm_param.imsi, p );
	return RT_EOK;
}

/*响应CPIN*/
rt_err_t resp_CPIN( char *p, uint16_t len )
{
	char *pstr = RT_NULL;
	pstr = strstr( p, "+CPIN: READY" ); //+CPIN: READY
	if( pstr )
	{
		return RT_EOK;                  /*找到了*/
	}
	return RT_ERROR;
}

/* +CSQ: 31, 99 */
rt_err_t resp_CSQ( char *p, uint16_t len )
{
	uint32_t i, n, code;
	i = sscanf( p, "+CSQ%*[^:]:%d,%d", &n, &code );
	if( i != 2 )
	{
		return RT_ERROR;
	}
	gsm_param.csq = n;
	return RT_EOK;
}

/*+CGATT:1*/
rt_err_t resp_CGATT( char *p, uint16_t len )
{
	char *pfind = RT_NULL;
	pfind = strstr( p, "+CGATT: 1" );
	if( pfind )
	{
		return RT_EOK;
	}
	return RT_ERROR;
}

/*%ETCPIP:1,"10.24.44.142","0.0.0.0","0.0.0.0"*/
rt_err_t resp_ETCPIP( char *p, uint16_t len )
{
	rt_err_t	res		= RT_ERROR;
	uint8_t		stage	= 0;
	char		*psrc	= p;
	char		*pdst	= gsm_param.ip;

	while( 1 )
	{
		if( stage == 0 )
		{
			if( *psrc == '"' )
			{
				stage = 1;
			}
			psrc++;
		}else
		{
			if( *psrc == '"' )
			{
				break;
			}
			*pdst = *psrc;
			pdst++;
			psrc++;
		}
	}
	rt_kprintf( "ip=%s\r\n", gsm_param.ip );
	return RT_EOK;
}

/*
   AT%DNSR="www.google.com"
   %DNSR:74.125.153.147
   OK
 */
rt_err_t resp_DNSR( char *p, uint16_t len )
{
	uint8_t i, stage = 0;
	char	*psrc = p;
	char	*pdst;
	if( strstr( p, "%DNSR" ) == RT_NULL )
	{
		return RT_ERROR;
	}
	while( 1 )
	{
		if( stage == 0 )
		{
			if( *psrc == ':' )
			{
				for( i = 0; i < 4; i++ )
				{
					if( psocket->state == SOCKET_DNS )
					{
						pdst = psocket->ip_addr;
						break;
					}
				}
				stage = 1;
			}
			psrc++;
		}else
		{
			if( *psrc <= 0x20 )
			{
				break;
			}
			*pdst = *psrc;
			pdst++;
			psrc++;
		}
	}
	rt_kprintf( "dns ip=%s\r\n", psocket->ip_addr );
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
rt_err_t resp_IPOPENX( char *p, uint16_t len )
{
	uint8_t i;
	char	*psrc = p;
	char	*pdst;

	if( strstr( p, "CONNECT" ) != RT_NULL )
	{
		return RT_EOK;
	}
	return RT_ERROR;
}

#if 0
/*通用检查函数，看是否收到数据，如果有数据后再判断*/
rt_err_t pt_resp( RESP_FUNC func )
{
	char		*pstr;
	uint16_t	len;
	rt_err_t	ret;
	ret = rt_mb_recv( &mb_gsmrx, (rt_uint32_t*)&pstr, 0 );
	if( ret == -RT_ETIMEOUT )
	{
		return RT_ETIMEOUT;
	}
/*有应答，执行相应的函数*/
	len = ( ( *pstr ) << 8 ) | ( *( pstr + 1 ) );
	ret = func( pstr + 2, len );
//	rt_kprintf( "pt_resp ret=%x\r\n", ret );
	rt_free( pstr );
	return ret;
}

#endif

typedef rt_err_t ( *AT_RESP )( char *p, uint16_t len );
typedef struct
{
	char		*atcmd;
	AT_RESP		resp;
	uint16_t	timeout;
	uint16_t	retry;
}AT_CMD_RESP;

/*等待固定字符串的返回*/
rt_err_t WaitString( char *respstr, uint32_t timeout )
{
	rt_err_t		err;

	uint8_t			*pmsg;

	uint32_t		tick_start, tick_end;
	uint32_t		tm;
	__IO uint8_t	flag_wait = 1;

	tick_start	= rt_tick_get( );
	tick_end	= tick_start + timeout;
	tm			= timeout;

	while( flag_wait )
	{
		err = rt_mb_recv( &mb_gsmrx, (rt_uint32_t*)&pmsg, tm );
		if( err != -RT_ETIMEOUT )                           /*没有超时,判断信息是否正确*/
		{
			if( strstr( pmsg + 2, respstr ) != RT_NULL )    /*前两个字节为长度，找到了*/
			{
				rt_free( pmsg );                            /*释放*/
				return RT_EOK;
			}
			rt_free( pmsg );                                /*释放*/
			/*计算剩下的超时时间,由于其他任务执行的延时，会溢出,要判断*/
			if( rt_tick_get( ) < tick_end )                 /*还没有超时*/
			{
				tm = tick_end - rt_tick_get( );
			}else
			{
				flag_wait = 0;
			}
		}else /*已经超时*/
		{
			flag_wait = 0;
		}
	}
	return RT_ETIMEOUT;
}

/*
   发送AT命令，并等待响应字符串
   参照 code.google.com/p/gsm-playground的实现
 */
rt_err_t SendATCmdWaitRespStr( char *AT_cmd_string,
                               uint32_t timeout,
                               char * respstr,
                               uint8_t no_of_attempts )
{
	rt_err_t		err;
	uint8_t			i;
	char			*pmsg;
	uint32_t		tick_start, tick_end;
	uint32_t		tm;
	__IO uint8_t	flag_wait;

	for( i = 0; i < no_of_attempts; i++ )
	{
		tick_start	= rt_tick_get( );
		tick_end	= tick_start + timeout;
		tm			= timeout;
		flag_wait	= 1;
		m66_write( &dev_gsm, 0, AT_cmd_string, strlen( AT_cmd_string ) );
		while( flag_wait )
		{
			err = rt_mb_recv( &mb_gsmrx, (rt_uint32_t*)&pmsg, tm );
			if( err != -RT_ETIMEOUT )                           /*没有超时,判断信息是否正确*/
			{
				if( strstr( pmsg + 2, respstr ) != RT_NULL )    /*找到了*/
				{
					rt_free( pmsg );                            /*释放*/
					return RT_EOK;
				}
				rt_free( pmsg );                                /*释放*/
				/*计算剩下的超时时间,由于其他任务执行的延时，会溢出,要判断*/
				if( rt_tick_get( ) < tick_end )                 /*还没有超时*/
				{
					tm = tick_end - rt_tick_get( );
				}else
				{
					flag_wait = 0;
				}
			}else /*已经超时*/
			{
				flag_wait = 0;
			}
		}
	}
	return ( RT_ETIMEOUT );
}

/*
   发送AT命令，并等待响应函数处理
   参照 code.google.com/p/gsm-playground的实现
 */

int8_t SendATCmdWaitRespFunc( char *AT_cmd_string,
                              uint32_t timeout,
                              RESP_FUNC respfunc,
                              uint8_t no_of_attempts )
{
	rt_err_t		err;
	uint8_t			i;
	char			*pmsg;
	uint32_t		tick_start, tick_end;
	uint32_t		tm;
	__IO uint8_t	flag_wait;

	char			* pinfo;
	uint16_t		len;

	for( i = 0; i < no_of_attempts; i++ )
	{
		tick_start	= rt_tick_get( );
		tick_end	= tick_start + timeout;
		tm			= timeout;
		flag_wait	= 1;
		m66_write( &dev_gsm, 0, AT_cmd_string, strlen( AT_cmd_string ) );
		while( flag_wait )
		{
			err = rt_mb_recv( &mb_gsmrx, (rt_uint32_t*)&pmsg, tm );
			if( err != -RT_ETIMEOUT )                   /*没有超时,判断信息是否正确*/
			{
				len		= ( *pmsg << 8 ) | ( *( pmsg + 1 ) );
				pinfo	= pmsg + 2;
				if( respfunc( pinfo, len ) == RT_EOK )  /*找到了*/
				{
					rt_free( pmsg );                    /*释放*/
					return RT_EOK;
				}
				rt_free( pmsg );                        /*释放*/
				/*计算剩下的超时时间,由于其他任务执行的延时，会溢出,要判断*/
				if( rt_tick_get( ) < tick_end )         /*还没有超时*/
				{
					tm = tick_end - rt_tick_get( );
				}else
				{
					flag_wait = 0;
				}
			}else /*已经超时*/
			{
				flag_wait = 0;
			}
		}
	}
	return ( RT_ETIMEOUT );
}

/*
   判断一个字符串是不是表示ip的str
   如果由[0..9|.] 组成
   '.' 0x2e   '/' 0x2f   '0' 0x30  '9' 0x39   简化一下。不判断 '/'
   返回值
   0:表示域名的地址
   1:表示是IP地址

 */
static uint8_t is_ipaddr( char * str )
{
	char *p = str;
	while( *p != NULL )
	{
		if( ( *p > '9' ) || ( *p < '.' ) )
		{
			return 0;
		}
		p++;
	}
	return 1;
}

/*gsm供电的处理纤程*/
static void rt_thread_gsm_power_on( void* parameter )
{
	static uint8_t	at_init_index	= 0;
	static uint8_t	at_init_retry	= 0;
	rt_err_t		ret;

lbl_poweron_start:
	rt_kprintf( "%08d gsm_power_on>start\r\n", rt_tick_get( ) );

	GPIO_ResetBits( GSM_PWR_PORT, GSM_PWR_PIN );
	GPIO_ResetBits( GSM_TERMON_PORT, GSM_TERMON_PIN );
	rt_thread_delay( RT_TICK_PER_SECOND );
	GPIO_SetBits( GSM_TERMON_PORT, GSM_TERMON_PIN );
	GPIO_SetBits( GSM_PWR_PORT, GSM_PWR_PIN );
	ret = WaitString( "OK", RT_TICK_PER_SECOND * 5 );
	if( ret == RT_ETIMEOUT )
	{
		goto lbl_poweron_start;
	}

	ret = WaitString( "OK", RT_TICK_PER_SECOND * 5 );
	if( ret == RT_ETIMEOUT )
	{
		goto lbl_poweron_start;
	}

	ret = SendATCmdWaitRespStr( "ATE0\r\n", RT_TICK_PER_SECOND * 5, "OK", 1 );
	if( ret == RT_ETIMEOUT )
	{
		goto lbl_poweron_start;
	}

	ret = SendATCmdWaitRespStr( "ATV1\r\n", RT_TICK_PER_SECOND * 5, "OK", 1 );
	if( ret == RT_ETIMEOUT )
	{
		goto lbl_poweron_start;
	}

	ret = SendATCmdWaitRespStr( "AT+CPIN?\r\n", RT_TICK_PER_SECOND * 3, "+CPIN: READY", 10 );
	ret = WaitString( "OK", RT_TICK_PER_SECOND * 5 );
	if( ret == RT_ETIMEOUT )
	{
		goto lbl_poweron_start;
	}

	ret = SendATCmdWaitRespFunc( "AT+CREG?\r\n", RT_TICK_PER_SECOND * 3, resp_CGREG, 10 );
	ret = WaitString( "OK", RT_TICK_PER_SECOND * 5 );
	if( ret == RT_ETIMEOUT )
	{
		goto lbl_poweron_start;
	}

	ret = SendATCmdWaitRespFunc( "AT+CIMI\r\n", RT_TICK_PER_SECOND * 3, resp_CIMI, 10 );
	if( ret == RT_ETIMEOUT )
	{
		goto lbl_poweron_start;
	}

	ret = SendATCmdWaitRespFunc( "AT+CGATT?\r\n", RT_TICK_PER_SECOND * 3, resp_CGATT, 10 );
	ret = WaitString( "OK", RT_TICK_PER_SECOND * 5 );
	if( ret == RT_ETIMEOUT )
	{
		goto lbl_poweron_start;
	}
	rt_kprintf( "%08d gsm_power_on>end\r\n", rt_tick_get( ) );
}

/*关于链路维护,只维护一个，多链路由上层处理*/
static void rt_thread_gsm_socket( void* parameter )
{
	uint8_t		buf[64];
	uint8_t		dns_retry;
	rt_err_t	err;

	GSM_SOCKET	* psocket = (GSM_SOCKET*)parameter;

	RT_ASSERT( gsm_state != GSM_AT );
	RT_ASSERT( psocket == RT_NULL );

	if( psocket->state == SOCKET_INIT )
	{
		sprintf( buf, "AT+CGDCONT=1,\"IP\",\"%s\"\r\n", psocket->apn );
		err = SendATCmdWaitRespStr( buf, RT_TICK_PER_SECOND * 10, "OK", 1 );
		if( err != RT_EOK )
		{
			goto lbl_gsm_socket_end;
		}
		psocket->state = SOCKET_START;
	}
	if( psocket->state == SOCKET_START )
	{
		err = SendATCmdWaitRespFunc( "AT%ETCPIP?\r\n", RT_TICK_PER_SECOND * 10, resp_ETCPIP, 1 );
		if( err != RT_EOK )
		{
			goto lbl_gsm_socket_end;
		}

		err = SendATCmdWaitRespStr( "AT%IOMODE=1,2,1\r\n", RT_TICK_PER_SECOND * 10, "OK", 1 );
		if( err != RT_EOK )
		{
			goto lbl_gsm_socket_end;
		}
	}
	
	if( is_ipaddr( psocket->ipstr ) ) /*是IP地址*/
	{
		psocket->state = SOCKET_CONNECT;
	}else
	{
		psocket->state = SOCKET_DNS;
	}

	if( psocket->state == SOCKET_DNS )
	{
		sprintf( buf, "AT%%DNSR=\"%s\"\r\n", psocket->ipstr );
		rt_kprintf( "%08d gsm_send>%s", rt_tick_get( ), buf );

		err = SendATCmdWaitRespFunc( buf, RT_TICK_PER_SECOND * 10, resp_DNSR, 1 );
		if( err != RT_EOK )
		{
			psocket->state = SOCKET_DNS_ERR;
			goto lbl_gsm_socket_end;
		}
		psocket->state = SOCKET_CONNECT;
	}

	if( psocket->state == SOCKET_CONNECT )
	{
		if( psocket->type == 'u' )
		{
			sprintf( buf, "AT%%IPOPENX=%d,\"UDP\",\"%s\",%d\r\n", 1, psocket->ip_addr, psocket->port );
		}else
		{
			sprintf( buf, "AT%%IPOPENX=%d,\"TCP\",\"%s\",%d\r\n", 1, psocket->ip_addr, psocket->port );
		}
		err = SendATCmdWaitRespFunc( buf, RT_TICK_PER_SECOND * 10, resp_IPOPENX, 1 );
		if( err != RT_EOK )
		{
			psocket->state = SOCKET_CONNECT_ERR;
			goto lbl_gsm_socket_end;
		}
	}

	/*挂断连接*/
	if( psocket->state == SOCKET_CLOSE )
	{
	}

lbl_gsm_socket_end:
	rt_kprintf( "%08d gsm_socket>end\r\n" );
}



/*
   tts语音播报的处理

   是通过
   %TTS: 0 判断tts状态(怀疑并不是每次都有输出)
   还是AT%TTS? 查询状态
 */
void tts_process( void )
{
	rt_err_t	ret;
	rt_size_t	len;
	uint8_t		*pinfo, *p;
	uint8_t		c;

	uint8_t		buf[20];
	uint8_t		tbl[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
/*当前正在播报中*/
	if( tts_busy == 1 )
	{
		return;
	}
/*gsm在处理其他命令*/
	if( gsmbusy )
	{
		return;
	}
	gsmbusy = GSMBUSY_TTS;
/*是否有信息要播报*/
	ret = rt_mb_recv( &mb_tts, (rt_uint32_t*)&pinfo, 0 );
	if( ret == -RT_ETIMEOUT )
	{
		gsmbusy = GSMBUSY_NONE;
		return;
	}

	sprintf( buf, "AT%%TTS=2,3,5,\"" );

	m66_write( &dev_gsm, 0, buf, strlen( buf ) );
	rt_kprintf( "%s", buf );

	len = ( *pinfo << 8 ) | ( *( pinfo + 1 ) );
	p	= pinfo + 2;
	while( len-- )
	{
		c = *p++;

		USART_SendData( UART4, tbl[c >> 4] );
		while( USART_GetFlagStatus( UART4, USART_FLAG_TC ) == RESET )
		{
		}
		rt_kprintf( "%c", tbl[c >> 4] );
		USART_SendData( UART4, tbl[c & 0x0f] );
		while( USART_GetFlagStatus( UART4, USART_FLAG_TC ) == RESET )
		{
		}
		rt_kprintf( "%c", tbl[c & 0x0f] );
	}
	buf[0]	= '"';
	buf[1]	= 0x0d;
	buf[2]	= 0x0a;
	buf[3]	= 0;
	m66_write( &dev_gsm, 0, buf, 3 );

	rt_kprintf( "%s", buf );

/*不判断，在gsmrx_cb中处理*/
	rt_free( pinfo );
	WaitString( "%TTS: 0", RT_TICK_PER_SECOND * 5 );
	gsmbusy = GSMBUSY_NONE;
}

ALIGN( RT_ALIGN_SIZE )
static char thread_gsm_stack[512];
struct rt_thread thread_gsm;


/*
   状态转换，同时处理开机、登网、短信、TTS、录音等过程
 */
static void rt_thread_entry_gsm( void* parameter )
{
	rt_tick_t		curr_ticks;
	rt_err_t		res;
	unsigned char	last_ch, ch;
	rt_thread_t		tid;

	while( 1 )
	{
		if( gsm_state == GSM_POWERON )
		{
			tid = rt_thread_create( "gsm_pwron", rt_thread_gsm_power_on, RT_NULL, 512, 7, 5 );
			rt_thread_startup( tid );
			gsm_state = GSM_POWERONING;
		}
		

//		protothread_gsm_power( &pt_gsm_power ); /*处理开机*/
//		gsm_socket_process( &pt_gsm_socket );   /*处理登网*/
		tts_process( );          /*处理tts信息*/
		rt_thread_delay( RT_TICK_PER_SECOND / 20 );
	}
}

ALIGN( RT_ALIGN_SIZE )
static char thread_gsm_rx_stack[512];
struct rt_thread thread_gsm_rx;


/*
   状态转换，同时处理短信、TTS、录音等过程
 */
static void rt_thread_entry_gsm_rx( void* parameter )
{
	rt_tick_t		curr_ticks;
	rt_err_t		res;
	unsigned char	last_ch, ch;

	while( 1 )
	{
		while( uart4_rxbuf_rd != uart4_rxbuf_wr ) /*有数据时，保存数据*/
		{
			ch				= uart4_rxbuf[uart4_rxbuf_rd++];
			uart4_rxbuf_rd	%= UART4_RX_SIZE;
			if( ch > 0x1F )
			{
				gsm_rx[gsm_rx_wr++] = ch;
				gsm_rx_wr			%= GSM_RX_SIZE;
				gsm_rx[gsm_rx_wr]	= 0;
			}
			if( ch == 0x0a )
			{
				if( gsm_rx_wr )
				{
					gsmrx_cb( gsm_rx, gsm_rx_wr );                  /*接收信息的处理函数*/
				}
				gsm_rx_wr = 0;
			}
		}
		if( rt_tick_get( ) - last_tick > RT_TICK_PER_SECOND / 10 )  //等待100ms,实际上就是变长的延时,最迟100ms处理完一个数据包
		{
			if( gsm_rx_wr )
			{
				gsmrx_cb( gsm_rx, gsm_rx_wr );                      /*接收信息的处理函数*/
				gsm_rx_wr = 0;
			}
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
void gsm_init( void )
{
	rt_thread_t tid;

	rt_mb_init( &mb_gsmrx, "mb_gsmrx", &mb_gsmrx_pool, MB_GSMRX_POOL_SIZE / 4, RT_IPC_FLAG_FIFO );
	rt_mb_init( &mb_tts, "mb_tts", &mb_tts_pool, MB_TTS_POOL_SIZE / 4, RT_IPC_FLAG_FIFO );

	rt_thread_init( &thread_gsm,
	                "gsm",
	                rt_thread_entry_gsm,
	                RT_NULL,
	                &thread_gsm_stack[0],
	                sizeof( thread_gsm_stack ), 7, 5 );
	rt_thread_startup( &thread_gsm );

	rt_thread_init( &thread_gsm_rx,
	                "gsm_rx",
	                rt_thread_entry_gsm_rx,
	                RT_NULL,
	                &thread_gsm_rx_stack[0],
	                sizeof( thread_gsm_rx_stack ), 6, 5 );
	rt_thread_startup( &thread_gsm_rx );

	dev_gsm.type	= RT_Device_Class_Char;
	dev_gsm.init	= m66_init;
	dev_gsm.open	= m66_open;
	dev_gsm.close	= m66_close;
	dev_gsm.read	= m66_read;
	dev_gsm.write	= m66_write;
	dev_gsm.control = m66_control;

	rt_device_register( &dev_gsm, "gsm", RT_DEVICE_FLAG_RDWR );
	rt_device_init( &dev_gsm );
}

/*控制gsm状态 0 查询*/
rt_err_t gsmstate( int cmd )
{
	if( cmd )
	{
		gsm_state = cmd;
	}
	return gsm_state;
}

FINSH_FUNCTION_EXPORT( gsmstate, control gsm state );

/*调试信息控制输出*/
rt_err_t dbgmsg( uint32_t i )
{
	if( i == 0 )
	{
		rt_kprintf( "debmsg=%d\r\n", fgsm_rawdata_out );
	} else
	{
		fgsm_rawdata_out = i;
	}
	return RT_EOK;
}

FINSH_FUNCTION_EXPORT( dbgmsg, dbgmsg count );

/*发送AT命令*/
rt_size_t at_write( char *sinfo )
{
	rt_err_t ret;

	m66_write( &dev_gsm, 0, sinfo, strlen( sinfo ) );

	return RT_EOK;
}

FINSH_FUNCTION_EXPORT( at_write, write gsm );


/***********************************************************
* Function:
* Description:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
rt_size_t socket_write( uint8_t linkno, uint8_t* buff, rt_size_t count, rt_int32_t timeout )
{
	rt_size_t	len = count;
	uint8_t		*p	= (uint8_t*)buff;
	uint8_t		c;
	char		*pstr;
	rt_err_t	ret;

	uint8_t		buf_start[20];
	uint8_t		buf_end[4] = { '"', 0x0d, 0x0a, 0x0 };

	uint8_t		tbl[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

	sprintf( buf_start, "AT%%IPSENDX=%d,\"", linkno );
	m66_write( &dev_gsm, 0, buf_start, strlen( buf_start ) );
	rt_kprintf( "%s", buf_start );
	while( len )
	{
		c = *p++;

		USART_SendData( UART4, tbl[c >> 4] );
		while( USART_GetFlagStatus( UART4, USART_FLAG_TC ) == RESET )
		{
		}
		rt_kprintf( "%c", tbl[c >> 4] );
		USART_SendData( UART4, tbl[c & 0x0f] );
		while( USART_GetFlagStatus( UART4, USART_FLAG_TC ) == RESET )
		{
		}
		rt_kprintf( "%c", tbl[c & 0x0f] );
		len--;
	}
	m66_write( &dev_gsm, 0, buf_end, 3 );

	rt_kprintf( "%s", buf_end );

/*todo 在此处处理是否为最优*/
	ret = rt_mb_recv( &mb_gsmrx, (rt_uint32_t*)&pstr, timeout );
	if( ret == -RT_ETIMEOUT )
	{
		return RT_ETIMEOUT;
	}
	len = ( ( *pstr ) << 8 ) | ( *( pstr + 1 ) );
	if( strstr( pstr + 2, "OK" ) )
	{
		ret = RT_EOK;
	} else
	{
		ret = RT_ERROR;
	}
	rt_free( pstr );

	return ret;
}

FINSH_FUNCTION_EXPORT( socket_write, write socket );


/*
   收到tts信息并发送
   返回0:OK
    1:分配RAM错误
 */
rt_size_t tts_write( char* info )
{
	uint8_t		*pmsg;
	uint16_t	count;
	count = strlen( info );

	/*直接发送到Mailbox中,内部处理*/
	pmsg = rt_malloc( count + 2 );
	if( pmsg != RT_NULL )
	{
		*pmsg			= count >> 8;
		*( pmsg + 1 )	= count & 0xff;
		memcpy( pmsg + 2, info, count );
		rt_mb_send( &mb_tts, (rt_uint32_t)pmsg );
		return 0;
	}
	return 1;
}

FINSH_FUNCTION_EXPORT( tts_write, tts send );

/************************************** The End Of File **************************************/
