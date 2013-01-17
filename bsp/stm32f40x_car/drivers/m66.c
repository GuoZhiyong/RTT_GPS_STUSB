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

#include <finsh.h>

#include <gsm.h>

#include "jt808.h"

#ifdef M66

#define GSM_GPIO                        GPIOC
#define GSM_TX_PIN                      GPIO_Pin_10
#define GSM_TX_PIN_SOURCE       GPIO_PinSource10

#define GSM_RX_PIN                      GPIO_Pin_11
#define GSM_RX_PIN_SOURCE       GPIO_PinSource11

typedef void ( *URC_CB )( char *s, uint16_t len );
typedef rt_err_t ( *RESP_FUNC )( char *s, uint16_t len );

#define GSM_PWR_PORT    GPIOD
#define GSM_PWR_PIN     GPIO_Pin_13

#define GSM_TERMON_PORT GPIOD
#define GSM_TERMON_PIN  GPIO_Pin_12

#define GSM_RST_PORT    GPIOD
#define GSM_RST_PIN     GPIO_Pin_11

/*声明一个gsm设备*/
static struct rt_device dev_gsm;


/*声明一个uart设备指针,同gsm模块连接的串口
   指向一个已经打开的串口
 */
static struct rt_mailbox mb_gsmrx;
#define MB_GSMRX_POOL_SIZE 32
static uint8_t mb_gsmrx_pool[MB_GSMRX_POOL_SIZE];

#define GSM_RX_SIZE 2048
static uint8_t gsm_rx[GSM_RX_SIZE];
static uint16_t gsm_rx_wr = 0;

static T_GSM_STATE gsm_state               = GSM_IDLE;
static T_GSM_STATE socket_state    = SOCKET_IDLE;

static rt_thread_t tid_gsm_subthread = RT_NULL;

/*串口接收缓存区定义*/
#define UART4_RX_SIZE 128
static uint8_t uart4_rxbuf[UART4_RX_SIZE];
struct rt_ringbuffer rb_uart4_rx;

static uint8_t fgsm_rawdata_out = 1;

/*最近一次收到串口数据的时刻,不使用sem作为超时判断*/
static uint32_t last_tick;

struct _gsm_param
{
	char imei[16];
	char imsi[16];
	uint8_t csq;
} gsm_param;

/*最大支持4个链接*/
#define MAX_SOCKET_NUM 4
struct
{
	char connect_state;                 /*连接状态0:不连接 1:可用就连接 3:等待DNS中 4:已连接*/
	char domain_name[32];               /*域名*/
	char ip[16];                        /*dns后的IP*/
	uint16_t port;                      /**/
} gsm_socket[MAX_SOCKET_NUM];


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
		rt_ringbuffer_putchar( &rb_uart4_rx, USART_ReceiveData( UART4 ) );
		USART_ClearITPendingBit( UART4, USART_IT_RXNE );
		//rt_sem_release( &sem_uart );
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
	char    *code;
	URC_CB pfunc;
} urc[] =
{
	//{ "^SYSSTART", urc_cb_sysstart },
	//{ "^SHUTDOWN", urc_cb_shutdown },
	{ "+CIEV:",        urc_cb_ciev    },
	{ "RING",          urc_cb_ring    },
	{ "+CRING:",   urc_cb_cring       },
	{ "+CREG:",        urc_cb_default },
	{ "^SIS:",         urc_cb_default },
	{ "+CGEV:",        urc_cb_default },
	{ "+CGREG:",   urc_cb_default },
	{ "+CMT:",         urc_cb_default },
	{ "+CBM:",         urc_cb_default },
	{ "+CDS:",         urc_cb_default },
	{ "+CALA:",        urc_cb_default },
	{ "CME ERROR", urc_cb_default },
	{ "CMS ERROR", urc_cb_default },
	{ "",              NULL                   }
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
	int i;
	char match = 0;

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
	char            *psrc, *pdst;
	uint16_t len = 0;
	psrc    = str;
	pdst    = str;
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
	char            *psrc, *pdst;
	uint16_t len = 0;
	psrc    = str;
	pdst    = str;
	while( *psrc )
	{
		if( ( *psrc >= '0' ) && ( *psrc <= '9' ) )
		{
			*pdst++ = *psrc;
			len++;
		}
		else
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
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOD, ENABLE );
	RCC_APB1PeriphClockCmd( RCC_APB1Periph_UART4, ENABLE );

	GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType   = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd    = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Speed   = GPIO_Speed_50MHz;

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
	GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType   = GPIO_OType_OD;
	GPIO_InitStructure.GPIO_PuPd    = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Speed   = GPIO_Speed_50MHz;

	GPIO_InitStructure.GPIO_Pin = GSM_RST_PIN;
	GPIO_Init( GSM_RST_PORT, &GPIO_InitStructure );
	GPIO_SetBits( GSM_RST_PORT, GSM_RST_PIN );

/*uart4 管脚设置*/

	/* Configure USART Tx as alternate function  */
	GPIO_InitStructure.GPIO_OType   = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd    = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed   = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin             = GSM_TX_PIN | GSM_RX_PIN;
	GPIO_Init( GSM_GPIO, &GPIO_InitStructure );

	GPIO_PinAFConfig( GSM_GPIO, GSM_TX_PIN_SOURCE, GPIO_AF_UART4 );
	GPIO_PinAFConfig( GSM_GPIO, GSM_RX_PIN_SOURCE, GPIO_AF_UART4 );

	NVIC_InitStructure.NVIC_IRQChannel                                              = UART4_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority    = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority                   = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd                                   = ENABLE;
	NVIC_Init( &NVIC_InitStructure );

	USART_InitStructure.USART_BaudRate                              = 57600;
	USART_InitStructure.USART_WordLength                    = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits                              = USART_StopBits_1;
	USART_InitStructure.USART_Parity                                = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl   = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode                                  = USART_Mode_Rx | USART_Mode_Tx;
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
	while( !( UART4->SR & USART_FLAG_TXE ) )
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
	rt_size_t len = count;
	uint8_t         *p      = (uint8_t*)buff;

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
		break;
	case CTL_AT_CMD:         //发送at命令,结果要返回
		break;
	case CTL_PPP:
		break;
	case CTL_SOCKET:
		break;
	}
	return RT_EOK;
}

/*
   +CREG:0,1
   +CGREG:0,5
 */
rt_err_t RespFunc_CGREG( char *p, uint16_t len )
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

/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
rt_err_t RespFunc_COPS( char *p, uint16_t len )
{
/*
   char *oper=(char*)0;
   oper=strstr(p,"CHN-CUGSM");
   if(oper){
   strcpy((char*)sys_param.apn,"UNINET");
   return AT_RESP_OK;
   }
   oper=strstr(p,"CHINAMOBILE");
   if(oper){
   strcpy((char*)sys_param.apn,"CMNET");
   return AT_RESP_OK;
   }
 */
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
rt_err_t RespFunc_CIFSR( char *p, uint16_t len )
{
	uint8_t i       = 0;
	rt_err_t res = RT_ERROR;
	int32_t PARAM_LocalIP[4];
	i = sscanf( p, "%u.%d.%d.%d",
	            &( PARAM_LocalIP[0] ),
	            &( PARAM_LocalIP[1] ),
	            &( PARAM_LocalIP[2] ),
	            &( PARAM_LocalIP[3] ) );
	if( i == 4 )
	{
		res = RT_EOK;
	}
	return res;
}

/*
   SIM卡的IMSI号码为4600 00783208249，
      460 00 18 23 20 86 42

   接口号数据字段的内容为IMSI号码的后12位
   其6个字节的内容为 0x00 0x07 0x83 0x20 0x82 0x49

 */
rt_err_t RespFunc_CIMI( char *p, uint16_t len )
{
	char *pimsi, i;
	rt_kprintf( "cimi len=%d\n", len );
	if( len < 15 )
	{
		return RT_ERROR;
	}
	strip_numstring( p );
	strcpy( gsm_param.imsi, p );
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
rt_err_t RespFunc_CGSN( char *p, uint16_t len )
{
	char *pimsi, i;
	if( len < 15 )
	{
		return RT_ERROR;
	}
	strip_numstring( p );
	strcpy( gsm_param.imsi, p );
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
rt_err_t RespFunc_CPIN( char *p, uint16_t len )
{
	char *pstr;
	pstr = strstr( p, "+CPIN: READY" );
	if( pstr )
	{
		return RT_EOK; /*找到了*/
	}
	return RT_ERROR;
}

/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:		+CSQ: 31, 99
* Return:
* Others:
***********************************************************/
rt_err_t RespFunc_CSQ( char *p, uint16_t len )
{
	rt_err_t res = RT_ERROR;
	uint32_t i, n, code;

	i = sscanf( p, "+CSQ%*[^:]:%d,%d", &n, &code );
	if( i != 2 )
	{
		return RT_ERROR;
	}
	gsm_param.csq = n;
	return RT_EOK;
}

/*等待固定信息的返回*/
rt_err_t WaitResp( char *resp, rt_tick_t ticks )
{
	rt_tick_t start           = rt_tick_get( );
	rt_tick_t localticks      = ticks;
	char            *pstr, *p;
	rt_err_t ret;

/*等待收到信息*/
lbl_waitresp_again:
	ret = rt_mb_recv( &mb_gsmrx, (rt_uint32_t*)&pstr, localticks );
	if( ret == -RT_ETIMEOUT )
	{
		return ret;                                 //超时退出，没有数据或接收数据完毕
	}
	strip_string( pstr );
	p = strstr( pstr, resp );
	if( p )
	{
		rt_free( pstr );
		return RT_EOK;                              /*找到了*/
	}
	/*没找到，继续等，这个信息如何处理?比如来短信的通知等URC*/
	localticks = start + ticks - rt_tick_get( );    /*计算剩余的时间*/
	if( localticks > 0 )
	{
		goto lbl_waitresp_again;
	}
	return RT_ETIMEOUT;
}

/*
   检查指定时间内的返回情况
   并调用专门的函数处理返回结果
 */
int8_t CheckResp( uint32_t ticks, RESP_FUNC resp_func )
{
	rt_tick_t start           = rt_tick_get( );
	rt_tick_t localticks      = ticks;
	rt_err_t ret                     = RT_EOK;
	char            * pstr;
/*等待收到信息*/
lbl_checkresp_again:
	ret = rt_mb_recv( &mb_gsmrx, (rt_uint32_t*)&pstr, localticks );
	if( ret == -RT_ETIMEOUT )
	{
		//rt_free(pstr);		/*没有收到数据，无需释放*/
		return ret; //超时退出，没有数据或接收数据完毕
	}
	/*收到信息，要判断是不是需要的*/
	ret = resp_func( pstr, strlen( pstr ) );
	if( ret == RT_EOK )
	{
		rt_free( pstr );
		return RT_EOK;
	}
	localticks = start + ticks - rt_tick_get( ); /*计算剩余的时间*/
	if( localticks > 0 )
	{
		goto lbl_checkresp_again;
	}
	return RT_ETIMEOUT;
}

/***********************************************************
* Function:		SendATCmdWaitRespStr
* Description:	发送命令，等待返回指定的字符串
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
int8_t SendATCmdWaitRespStr( char *atcmd,
                             uint32_t ticks,
                             char * respstring,
                             uint8_t no_of_attempts )
{
	rt_err_t ret_val = RT_ERROR;
	uint8_t i;

	for( i = 0; i < no_of_attempts; i++ )
	{
		rt_kprintf( "gsm_send>%s", atcmd );
		m66_write( &dev_gsm, 0, atcmd, strlen( atcmd ) );
		ret_val = WaitResp( respstring, ticks );
		if( ret_val == RT_EOK )
		{
			break;
		}
	}
	return ( ret_val );
}

/***********************************************************
* Function:		SendATCmdWaitRespFunc
* Description:	发送命令，并将返回结果调用指定的处理函数
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
int8_t SendATCmdWaitRespFunc( char *atcmd,
                              uint32_t ticks,
                              RESP_FUNC respfunc,
                              uint8_t no_of_attempts )
{
	int8_t ret_val = RT_ERROR;
	uint8_t i;

	for( i = 0; i < no_of_attempts; i++ )
	{
		rt_kprintf( "gsm_send>%s", atcmd );
		m66_write( &dev_gsm, 0, atcmd, strlen( atcmd ) );
		ret_val = CheckResp( ticks, respfunc );
		if( ret_val == RT_EOK )
		{
			break;
		}
	}
	return ( ret_val );
}

/*线程退出的cleanup函数*/
void cleanup( struct rt_thread *tid )
{
	tid_gsm_subthread = RT_NULL;
}

/***********************************************************
* Function:
* Description:	模块上电并完成AT命令初始化
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static void gsm_poweron( void* parameter )
{
	rt_err_t res;

/*此处可通过rt_thread_self获取线程信息*/
lbl_start_pwr_on:

	GPIO_SetBits( GSM_PWR_PORT, GSM_PWR_PIN );
	GPIO_SetBits( GSM_TERMON_PORT, GSM_TERMON_PIN );
	rt_thread_delay( RT_TICK_PER_SECOND / 10 );
	GPIO_ResetBits( GSM_TERMON_PORT, GSM_TERMON_PIN );
	rt_thread_delay( RT_TICK_PER_SECOND );
	GPIO_SetBits( GSM_TERMON_PORT, GSM_TERMON_PIN );
/*
	res = WaitResp( "AT-Command Interpreter ready", RT_TICK_PER_SECOND * 5 );
	if( res != RT_EOK )
	{
		rt_kprintf( "\r\n%d>re gsm pwron\r\n", rt_tick_get( ) );
		GPIO_SetBits( GSM_TERMON_PORT, GSM_TERMON_PIN );
		GPIO_SetBits( GSM_PWR_PORT, GSM_PWR_PIN );
		rt_thread_delay( RT_TICK_PER_SECOND * 3 );
		goto lbl_start_pwr_on;
	}
*/	
	res = WaitResp( "OK", RT_TICK_PER_SECOND * 5 );
	if( res != RT_EOK )
	{
		rt_kprintf( "\r\n%d>re gsm pwron\r\n", rt_tick_get( ) );
		GPIO_SetBits( GSM_TERMON_PORT, GSM_TERMON_PIN );
		GPIO_SetBits( GSM_PWR_PORT, GSM_PWR_PIN );
		rt_thread_delay( RT_TICK_PER_SECOND * 3 );
		goto lbl_start_pwr_on;
	}

	res = WaitResp( "OK", RT_TICK_PER_SECOND * 5 );
	if( res != RT_EOK )
	{
		rt_kprintf( "\r\n%d>re gsm pwron\r\n", rt_tick_get( ) );
		GPIO_SetBits( GSM_TERMON_PORT, GSM_TERMON_PIN );
		GPIO_SetBits( GSM_PWR_PORT, GSM_PWR_PIN );
		rt_thread_delay( RT_TICK_PER_SECOND * 3 );
		goto lbl_start_pwr_on;
	}


	
	res = SendATCmdWaitRespStr( "AT\r\n", 3 * RT_TICK_PER_SECOND, "OK", 3 );
	res = SendATCmdWaitRespStr( "ATE0\r\n", 3 * RT_TICK_PER_SECOND, "OK", 3 );
	res = SendATCmdWaitRespStr( "ATV1\r\n", 3 * RT_TICK_PER_SECOND, "OK", 3 );
	res = SendATCmdWaitRespFunc( "AT+CPIN?\r\n", 3 * RT_TICK_PER_SECOND, RespFunc_CPIN, 3 );
	if( res != RT_EOK )
	{
		goto lbl_start_pwr_on;
	}
	res                     = SendATCmdWaitRespFunc( "AT+COPS?\r\n", 3 * RT_TICK_PER_SECOND, RespFunc_COPS, 10 );
	res                     = SendATCmdWaitRespFunc( "AT+CREG?\r\n", 3 * RT_TICK_PER_SECOND, RespFunc_CGREG, 10 );
	res                     = SendATCmdWaitRespFunc( "AT+CIMI\r\n", 3 * RT_TICK_PER_SECOND, RespFunc_CIMI, 3 );
	res                     = SendATCmdWaitRespFunc( "AT+CGREG?\r\n", 3 * RT_TICK_PER_SECOND, RespFunc_CGREG, 10 );
	res                     = SendATCmdWaitRespStr( "AT+CGATT?\r\n", 3 * RT_TICK_PER_SECOND, "+CGATT:1", 10 );
	gsm_state       = GSM_AT; /*切换到AT状态*/
}

/***********************************************************
* Function:	rt_thread_entry_gsm_poweroff
* Description: 挂断当前链接，并关闭模块
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static void gsm_poweroff( void* parameter )
{
	rt_err_t res;

/*此处可通过rt_thread_self获取线程信息*/

/*关闭连接*/

/*模块断电*/
lbl_start_pwr_off:

	rt_kprintf( "\r\n%ld>gsm pwr off start", rt_tick_get( ) );
	GPIO_SetBits( GSM_PWR_PORT, GSM_PWR_PIN );
	GPIO_SetBits( GSM_TERMON_PORT, GSM_TERMON_PIN );
	rt_thread_delay( RT_TICK_PER_SECOND / 10 );
	GPIO_ResetBits( GSM_TERMON_PORT, GSM_TERMON_PIN );
	rt_thread_delay( RT_TICK_PER_SECOND );
	GPIO_SetBits( GSM_TERMON_PORT, GSM_TERMON_PIN );
	//SendATCmdWaitRespStr("AT^SMSO\r\n",RT_TICK_PER_SECOND,"OK",1);
	rt_kprintf( "\r\n%ld>gsm pwr on end", rt_tick_get( ) );

	res = WaitResp( "^SHUTDOWN", RT_TICK_PER_SECOND * 10 );
	if( res != RT_EOK )
	{
		rt_kprintf( "\r\n%d>re gsm pwron\r\n", rt_tick_get( ) );
		goto lbl_start_pwr_off;
	}

	rt_kprintf( "\r\n%ld>gsm pwr off end", rt_tick_get( ) );
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
static void gsmrx_cb( uint8_t *pInfo, uint16_t len )
{
	int i, count;
	uint8_t tbl[24]={0,1,2,3,4,5,6,7,8,9,0,0,0,0,0,0,0,0xa,0xb,0xc,0xd,0xe,0xf};
	char c, *pmsg;
	uint8_t *psrc, *pdst;
	int32_t infolen, link;

/*网络侧的信息，直接通知上层软件*/
	if( fgsm_rawdata_out )
	{
		rt_kprintf( "\r\ngsm>%s\r\n", pInfo );
	}

	psrc    = RT_NULL;
	psrc    = strstr( pInfo, "%IPDATA:" );
	if( psrc != RT_NULL )
	{
		/*解析出净信息*/
		i = sscanf( psrc, "%%IPDATA:%d,%d,%s", &link, &infolen, pdst );
		if(i!=3) return;
		if(infolen<11) return;
		if(*pdst!='"') return;
		pmsg=pdst++;

		while(*pmsg!='"') {
			c=tbl[*pmsg-'0']<<4;
			pmsg++;
			c|=tbl[*pmsg-'0'];
			pmsg++;
			*pdst++=c;
			count++;
			if(count>=infolen) break;
		}
		gprs_rx( pdst, infolen );
	}
	else
	{
		/*要判断是否有gsm的子线程在工作，*/
		pmsg = rt_malloc( len );
		if( pmsg != RT_NULL )
		{
			memcpy( pmsg, pInfo, len );
			rt_mb_send( &mb_gsmrx, (rt_uint32_t)pmsg );
		}
		/*释放收到一包信息的信号,要考虑处理时间过长的问题*/
		return;
	}
}


/***********************************************************
* Function:
* Description:	发送数据
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
int gsm_send( uint8_t *pinfo, uint16_t len )
{
	int i;
/*查找可用的socket*/
	for(i=0; i<MAX_SOCKET_NUM; i++)
	{
		if (gsm_socket[i].connect_state==4)
		{

		}


	}
}

ALIGN( RT_ALIGN_SIZE )
static char thread_socket_stack[512];
struct rt_thread thread_socket;


/***********************************************************
* Function:
* Description: socket链接处理
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static void rt_thread_entry_socket( void* parameter )
{
	int i;
	rt_err_t res;
	char dialstr[128];
/*查询gsm状态,是否初始化内部tcpip*/
	if( ( gsm_state == GSM_AT ) && ( socket_state == SOCKET_TCPIP_INIT ) )
	{
		sprintf(dialstr,"AT+CGDCONT=1,\"IP\",\"%s\"\r\n",jt808_param.apn);
		res=SendATCmdWaitRespStr(dialstr,3*RT_TICK_PER_SECOND,"OK",1);
		if(res!=RT_EOK)
		{
			gsm_state=GSM_POWERON;
		}
		res=SendATCmdWaitRespStr("AT%ETCPIP\r\n",10*RT_TICK_PER_SECOND,"OK",1);
		if(res!=RT_EOK)
		{
			gsm_state=GSM_POWERON;
		}
		res=SendATCmdWaitRespStr("AT%IOMODE=1,2,1\r\n",5*RT_TICK_PER_SECOND,"OK",1);
		if(res!=RT_EOK)
		{
			gsm_state=GSM_POWERON;
		}
		socket_state=SOCKET_READY;
	}

/*查询链接状态*/
	if( socket_state == SOCKET_READY )
	{
		for( i = 0; i < MAX_SOCKET_NUM; i++ )
		{
			if( gsm_socket[i].connect_state == 1 )
			{
			}
		}
	}

/*确认dns结果*/
	rt_thread_delay( RT_TICK_PER_SECOND / 10 );
}

ALIGN( RT_ALIGN_SIZE )
static char thread_gsm_stack[512];
struct rt_thread thread_gsm;


/***********************************************************
* Function:       rt_thread_entry_gsm
* Description:    接收处理，状态转换
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static void rt_thread_entry_gsm( void* parameter )
{
	rt_tick_t curr_ticks;
	rt_err_t res;
	unsigned char ch;

/*gsm的状态切换*/
	while( 1 )
	{
		curr_ticks = rt_tick_get( );
		switch( gsm_state )
		{
		case GSM_IDLE:
			break;
		case GSM_POWERON:         /*上电过程中，控制IO的操作放在此处，如何保证时间的可靠*/
			if( tid_gsm_subthread == RT_NULL )
			{
				tid_gsm_subthread = rt_thread_create( "pwron", gsm_poweron, (void*)1, 512, 25, 5 );
				rt_kprintf( "\r\ntid_gsm_subthread=%p\r\n", tid_gsm_subthread );
				if( tid_gsm_subthread != RT_NULL )
				{
					tid_gsm_subthread->cleanup = cleanup;
					rt_thread_startup( tid_gsm_subthread );
				}
			}
			break;
		case GSM_POWEROFF:         /*断电电过程中，控制IO的操作放在此处，如何保证时间的可靠*/
			if( tid_gsm_subthread == RT_NULL )
			{
				tid_gsm_subthread = rt_thread_create( "pwroff", gsm_poweroff, (void*)1, 512, 25, 5 );
				if( tid_gsm_subthread != RT_NULL )
				{
					rt_thread_startup( tid_gsm_subthread );
				}
			}
			break;
		case GSM_AT:
			break;
		}

/*接收超时判断*/
		while( rt_ringbuffer_getchar( &rb_uart4_rx, &ch ) == 1 ) /*有数据时，保存数据*/
		{
			gsm_rx[gsm_rx_wr++] = ch;
			if( gsm_rx_wr == GSM_RX_SIZE )
			{
				gsm_rx_wr = 0;
			}
			gsm_rx[gsm_rx_wr] = 0;
		}
		if( rt_tick_get( ) - last_tick > RT_TICK_PER_SECOND / 10 )  //等待100ms,实际上就是变长的延时,最迟100ms处理完一个数据包
		{
			if( gsm_rx_wr )
			{
				gsmrx_cb( gsm_rx, gsm_rx_wr );                      /*接收信息的处理函数*/
				gsm_rx_wr = 0;
			}
		}
		rt_thread_delay( RT_TICK_PER_SECOND / 50 );                 /*延时20ms*/
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
void gsm_init( void )
{
	rt_thread_t tid;

/*初始化串口接收缓冲区*/
	rt_ringbuffer_init( &rb_uart4_rx, uart4_rxbuf, UART4_RX_SIZE );

	rt_mb_init( &mb_gsmrx, "gsm_rx", &mb_gsmrx_pool, MB_GSMRX_POOL_SIZE / 4, RT_IPC_FLAG_FIFO );

	rt_thread_init( &thread_gsm,
	                "gsm",
	                rt_thread_entry_gsm,
	                RT_NULL,
	                &thread_gsm_stack[0],
	                sizeof( thread_gsm_stack ), 7, 5 );
	rt_thread_startup( &thread_gsm );

/*创建socket管理线程*/
	rt_thread_init( &thread_socket,
	                "socket",
	                rt_thread_entry_socket,
	                RT_NULL,
	                &thread_socket_stack[0],
	                sizeof( thread_socket_stack ), 8, 5 );
	rt_thread_startup( &thread_socket );

	dev_gsm.type    = RT_Device_Class_Char;
	dev_gsm.init    = m66_init;
	dev_gsm.open    = m66_open;
	dev_gsm.close   = m66_close;
	dev_gsm.read    = m66_read;
	dev_gsm.write   = m66_write;
	dev_gsm.control = m66_control;

	rt_device_register( &dev_gsm, "gsm", RT_DEVICE_FLAG_RDWR );
	rt_device_init( &dev_gsm );
}

#ifdef TEST_GSM


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
rt_err_t gsm_open( void )
{
	return m66_open( &dev_gsm, RT_DEVICE_OFLAG_RDWR );
}

FINSH_FUNCTION_EXPORT( gsm_open, open gsm );


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
rt_err_t gsm_status( void )
{
	char *st[] = {
		"空闲",
		"上电过程",
		"断电过程中",
		"AT命令",
		"PPP连接状态",
		"数据状态",
	};
	rt_kprintf( "gsm>%s\n", st[gsm_state] );
}

FINSH_FUNCTION_EXPORT( gsm_status, gsm status );


/***********************************************************
* Function:
* Description: 释放链接，gsm断电
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
rt_err_t gsm_close( void )
{
	return m66_close( &dev_gsm );
}

FINSH_FUNCTION_EXPORT( gsm_close, close gsm );


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
rt_size_t gsm_write( char *sinfo )
{
	return m66_write( &dev_gsm, 0, sinfo, strlen( sinfo ) );
}

FINSH_FUNCTION_EXPORT( gsm_write, write gsm );

#if 0

/*设置链接的socket参数*/
rt_err_t sock_config( uint8_t linkno, char* apn, char *connect_str )
{
	if( linkno >= GSM_MAX_SOCK_NUM )
	{
		return RT_ERROR;
	}
	if( gsm_socket[linkno].active != 0 )
	{
		return RT_ERROR;
	}
	gsm_socket[linkno].active = 0;
	strcpy( gsm_socket[linkno].apn, apn );
	strcpy( gsm_socket[linkno].conn_str, connect_str );
	return RT_EOK;
}

FINSH_FUNCTION_EXPORT( sock_config, < linkno > < apn > < connstr > );

/*控制socket链接*/
rt_err_t sock_control( uint8_t linkno, uint8_t action )
{
	if( linkno >= GSM_MAX_SOCK_NUM )
	{
		return RT_ERROR;
	}
	if( gsm_socket[linkno].active ^ action ) /*有状态变化*/
	{
		rt_kprintf( "sock>%d\n", __LINE__ );
		if( action == 1 )
		{
			if( tid_gsm_subthread == RT_NULL )
			{
				tid_gsm_subthread = rt_thread_create( "ppp", gsm_socket_open, (void*)&linkno, 512, 25, 5 );
				if( tid_gsm_subthread != RT_NULL )
				{
					tid_gsm_subthread->cleanup = cleanup;
					rt_thread_startup( tid_gsm_subthread );
					rt_kprintf( "sock>%d\n", __LINE__ );
					return RT_EOK;
				}
				rt_kprintf( "sock>%d\n", __LINE__ );
				return RT_ERROR;
			}
			rt_kprintf( "sock>%d\n", __LINE__ );
			return RT_ERROR;
		}
		else
		{
			if( tid_gsm_subthread == RT_NULL )
			{
				tid_gsm_subthread = rt_thread_create( "ppp", gsm_socket_close, (void*)linkno, 512, 25, 5 );
				if( tid_gsm_subthread != RT_NULL )
				{
					rt_thread_startup( tid_gsm_subthread );
					rt_kprintf( "sock>%d\n", __LINE__ );
					return RT_EOK;
				}
				rt_kprintf( "sock>%d\n", __LINE__ );
				return RT_ERROR;
			}
			rt_kprintf( "sock>%d\n", __LINE__ );
			return RT_ERROR;
		}
	}
	return RT_EOK;
}

FINSH_FUNCTION_EXPORT( sock_control, < linkno > < 0 | 1 > );

#endif

#endif
#endif

/************************************** The End Of File **************************************/
