/************************************************************
 * Copyright (C), 2008-2012,
 * FileName:		m66
 * Author:			bitter
 * Date:			// ����
 * Description:		// ģ������
 * Version:			// �汾��Ϣ
 * Function List:	// ��Ҫ�������书��
 *     1. -------
 * History:			// ��ʷ�޸ļ�¼
 *     <author>  <time>   <version >   <desc>
 *     David    96/10/12     1.0     build this moudle
 ***********************************************************/

#include <stdio.h>
#include <rtthread.h>
#include <rtdevice.h>
#include "stm32f4xx.h"
#include <board.h>
#include <finsh.h>

#include <gsm.h>

#include "jt808.h"

#include "pt.h"

#ifdef M66

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

/*����һ��gsm�豸*/
static struct rt_device dev_gsm;


/*����һ��uart�豸ָ��,ͬgsmģ�����ӵĴ���
   ָ��һ���Ѿ��򿪵Ĵ���
 */
static struct rt_mailbox	mb_gsmrx;
#define MB_GSMRX_POOL_SIZE 32
static uint8_t				mb_gsmrx_pool[MB_GSMRX_POOL_SIZE];

#define GSM_RX_SIZE 2048
static uint8_t			gsm_rx[GSM_RX_SIZE];
static uint16_t			gsm_rx_wr = 0;

static T_GSM_STATE		gsm_state		= GSM_IDLE;

/*���ڽ��ջ���������*/
#define UART4_RX_SIZE 256
static uint8_t			uart4_rxbuf[UART4_RX_SIZE];
struct rt_ringbuffer	rb_uart4_rx;

/*���������������Ϣ*/
static uint32_t			fgsm_rawdata_out = 0xfffffff;

/*���һ���յ��������ݵ�ʱ��,��ʹ��sem��Ϊ��ʱ�ж�*/
static uint32_t last_tick;



struct _gsm_param
{
	char	imei[16];
	char	imsi[16];
	uint8_t csq;
	char	ip[16];
} gsm_param;



static struct pt	pt_gsm_power;
static struct pt	pt_gsm_socket;


/*�򵥵�ʱ��������*/
struct pt_timer
{
	rt_tick_t	start;
	rt_tick_t	interval;
};

/*��ȡ��ǰʱ��*/
static int pt_clock_time( void )
{
	return rt_tick_get( );
}

/*����Ƿ�ʱ*/
static int pt_timer_expired( struct pt_timer *t )
{
	return (int)( pt_clock_time( ) - t->start ) >= (int)t->interval;
}

/*�趨һ���򵥵Ķ�ʱ��*/
static void pt_timer_set( struct pt_timer * t, int interval )
{
	t->interval = interval;
	t->start	= pt_clock_time( );
}


/***********************************************************
* Function:
* Description: uart4���жϷ�����
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
* Description:	ȱʡ��ϵͳ�ص�������
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
/*�п����������Ҷϣ�Ҳ�п����������ӶϿ����Ƿ�Ҫ֪ͨ? ����Ҫ*/
	if( strncmp( pInfo, "%IPCLOSE", 7 ) == 0 )
	{
	}

//�жϷ���������-�����ϱ�����
	for( i = 0;; i++ )
	{
		if( urc[i].pfunc == NULL )
		{
			break;
		}
		if( strncmp( pInfo, urc[i].code, strlen( urc[i].code ) == 0 ) )
		{
			( urc[i].pfunc )( pInfo, len );
			match = 1; //�Ѵ���
			break;
		}
	}
	if( match )
	{
		return;
	}

//AT����Ľ�����������������������APP������
}

#endif


/***********************************************************
* Function:
* Description: ��С��0x20���ַ����Ե������ڽ�β���0��תΪ
   �ɼ����ַ�����
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
* Description: ��С��0x20���ַ����Ե���ֻ�������ֲ��֡�
               ���ڽ�β���0��תΪ�ɼ����ַ�����
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
* Description: ���ÿص�ܽţ����ö�Ӧ�Ĵ����豸uart4
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
   RESET�ڿ������̲���Ҫ���κ�ʱ����ϣ���ͨ��CPU �� reset��ͬ����
   ����ùܽŽ�OC�����GPIO������ʱ OC ������衣
 */
	GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType	= GPIO_OType_OD;
	GPIO_InitStructure.GPIO_PuPd	= GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Speed	= GPIO_Speed_50MHz;

	GPIO_InitStructure.GPIO_Pin = GSM_RST_PIN;
	GPIO_Init( GSM_RST_PORT, &GPIO_InitStructure );
	GPIO_SetBits( GSM_RST_PORT, GSM_RST_PIN );

/*uart4 �ܽ�����*/

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
* Function:	�ṩ������thread���ã����豸����ʱ�ж�
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
		gsm_state = GSM_POWERON; //��λ�ϵ������
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
	gsm_state = GSM_POWEROFF; //��λ�ϵ������
	return RT_EOK;
}

/***********************************************************
* Function:m66_read
* Description:����ģʽ�¶�ȡ����
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
* Description:	����ģʽ�·������ݣ�Ҫ�����ݽ��з�װ
* Input:		const void* buff	Ҫ���͵�ԭʼ����
       rt_size_t count	Ҫ�������ݵĳ���
       rt_off_t pos		ʹ�õ�socket���
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
			*((int*)arg)=gsm_state;
			break;
		case CTL_AT_CMD: //����at����,���Ҫ����
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
* Description:	gsm�յ���Ϣ�Ĵ���
* Input:			char *s     ��Ϣ
    uint16_t len ����
* Output:
* Return:
* Others:
***********************************************************/
static void gsmrx_cb( uint8_t *pInfo, uint16_t len )
{
	int		i, count,newlen;
	uint8_t tbl[24] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0, 0, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf };
	char	c, *pmsg;
	uint8_t *psrc, *pdst;
	int32_t infolen, link;

/*��������Ϣ��ֱ��֪ͨ�ϲ����*/
	if( fgsm_rawdata_out )
	{
		rt_kprintf( "\r\n%08d gsm_rx>%s\r\n", rt_tick_get( ), pInfo );
		fgsm_rawdata_out--;
	}

/*
�᷵������<0d><0a>OK<0d><0a>֮�������
ȥ��ǰ�治��Ҫ��0d0a �Ϳո�Ȳ��ɼ��ַ�
*/
	count=0;
	while((*(pInfo+count)<0x21)&&(count<len))
	{
		count++;
	}
	psrc=pInfo+count;
	newlen=len-count;
	
	if(strncmp(psrc,"%IPDATA:",7)==0)
	{
		/*����������Ϣ,���������Ż���pdst*/
		i = sscanf( psrc, "%%IPDATA:%d,%d,%s", &link, &infolen, pdst );
		if( i != 3 ) return;
		if( infolen < 11 ) return;
		if( *pdst != '"' ) return;
		psrc=pdst;
		pmsg = pdst+1;	/*ָ����һ��λ��*/
		for(i=0;i<infolen;i++)
		{
			c = tbl[*pmsg++ - '0'] << 4;
			c |= tbl[*pmsg++ - '0'];
			*pdst++ = c;
		}
		gprs_rx(link, psrc, infolen );
		return;
	}

	/*ֱ�ӷ��͵�Mailbox��*/
	pmsg = rt_malloc( len );
	if( pmsg != RT_NULL )
	{
		*pmsg			= len >> 8;
		*( pmsg + 1 )	= len & 0xff;
		memcpy( pmsg + 2, pInfo, len );
		rt_mb_send( &mb_gsmrx, (rt_uint32_t)pmsg );
	}
	return;
}


/*  */
rt_err_t pt_resp_STR_OK( char *p, uint16_t len )
{
	char *pfind = RT_NULL;
	pfind = strstr( p, "OK" );
	if( pfind )
	{
		return RT_EOK; /*�ҵ���*/
	}
	return RT_ERROR;
}

/*��ӦCREG��CGREG
   +CREG:0,1
   +CGREG:0,5

 */
rt_err_t pt_resp_CGREG( char *p, uint16_t len )
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
   SIM����IMSI����Ϊ4600 00783208249��
      460 00 18 23 20 86 42

   �ӿں������ֶε�����ΪIMSI����ĺ�12λ
   ��6���ֽڵ�����Ϊ 0x00 0x07 0x83 0x20 0x82 0x49

 */
rt_err_t pt_resp_CIMI( char *p, uint16_t len )
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

/*��ӦCGSN**/
rt_err_t pt_resp_CGSN( char *p, uint16_t len )
{
	char	*pimsi, i;
	char	*pstr;
	if( len < 15 )
	{
		return RT_ERROR;
	}
	strip_numstring( p );
	strcpy( gsm_param.imsi, p );
	return RT_EOK;
}

/*��ӦCPIN*/
rt_err_t pt_resp_CPIN( char *p, uint16_t len )
{
	char *pstr = RT_NULL;
	pstr = strstr( p, "+CPIN: READY" ); //+CPIN: READY
	if( pstr )
	{
		return RT_EOK;                  /*�ҵ���*/
	}
	return RT_ERROR;
}

/* +CSQ: 31, 99 */
rt_err_t pt_resp_CSQ( char *p, uint16_t len )
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
rt_err_t pt_resp_CGATT( char *p, uint16_t len )
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
rt_err_t pt_resp_ETCPIP( char *p, uint16_t len )
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
rt_err_t pt_resp_DNSR( char *p, uint16_t len )
{
	uint8_t i, stage = 0;
	char	*psrc	= p;
	char	*pdst;
	if( strstr( p, "%DNSR" ) ==RT_NULL ) return RT_ERROR;
	while( 1 )
	{
		if( stage == 0 )
		{
			if( *psrc == ':' )
			{
				for( i = 0; i < 4; i++ )
				{
					if( curr_gsm_socket.state == SOCKET_DNS )
					{
						pdst = curr_gsm_socket.ip_str;
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
	rt_kprintf( "dns ip=%s\r\n", curr_gsm_socket.ip_str );
	return RT_EOK;
}


rt_err_t pt_resp_IPOPENX( char *p, uint16_t len )
{
	uint8_t i, stage = 0;
	char	*psrc	= p;
	char	*pdst;

	if( strstr( p, "CONNECT" ) !=RT_NULL ) return RT_EOK;
	return RT_ERROR;
}


/*ͨ�ü�麯�������Ƿ��յ����ݣ���������ݺ����ж�*/
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
/*��Ӧ��ִ����Ӧ�ĺ���*/
	len = ( ( *pstr ) << 8 ) | ( *( pstr + 1 ) );
	ret = func( pstr + 2, len );
	rt_kprintf( "pt_resp ret=%x\r\n", ret );
	rt_free( pstr );
	return ret;
}






typedef rt_err_t ( *AT_RESP )( char *p, uint16_t len );
typedef struct
{
	char		*atcmd;
	AT_RESP		resp;
	uint16_t	timeout;
	uint16_t	retry;
}AT_CMD_RESP;

/*gsm����Ĵ����˳�*/




/**/
static int protothread_gsm_power( struct pt *pt )
{
	static char	str_CGDCONT[64];
	static char	str_ETCPIP[32];

	static AT_CMD_RESP	at_init[] =
	{
		{ RT_NULL,				 pt_resp_STR_OK, RT_TICK_PER_SECOND * 10, 1	 },
		{ RT_NULL,				 pt_resp_STR_OK, RT_TICK_PER_SECOND * 10, 1	 },
		{ "ATE0\r\n",			 pt_resp_STR_OK, RT_TICK_PER_SECOND * 5,  1	 },
		{ "ATV1\r\n",			 pt_resp_STR_OK, RT_TICK_PER_SECOND * 5,  1	 },
		{ "AT+CPIN?\r\n",		 pt_resp_CPIN,	 RT_TICK_PER_SECOND * 3,  10 },
		{ "AT+CREG?\r\n",		 pt_resp_CGREG,	 RT_TICK_PER_SECOND * 3,  10 },
		{ "AT+CIMI\r\n",		 pt_resp_CIMI,	 RT_TICK_PER_SECOND * 3,  1	 },
		{ "AT+CGREG?\r\n",		 pt_resp_CGREG,	 RT_TICK_PER_SECOND * 3,  10 },
		{ "AT+CGATT?\r\n",		 pt_resp_CGATT,	 RT_TICK_PER_SECOND * 3,  10 },
		{ str_CGDCONT,			 pt_resp_STR_OK, RT_TICK_PER_SECOND * 10, 1	 },
		{ str_ETCPIP,			 pt_resp_STR_OK, RT_TICK_PER_SECOND * 30, 1	 },
		{ "AT%ETCPIP?\r\n",		 pt_resp_ETCPIP, RT_TICK_PER_SECOND * 3,  1	 },
		{ "AT%IOMODE=1,2,1\r\n", pt_resp_STR_OK, RT_TICK_PER_SECOND * 3,  1	 },
	};
	static struct pt_timer timer_gsm_power;
	static uint8_t		at_init_index	= 0;
	static uint8_t		at_init_retry	= 0;
	rt_err_t			ret;

	PT_BEGIN( pt );
	if( gsm_state == GSM_POWERON )
	{
		
		sprintf( str_CGDCONT, "AT+CGDCONT=1,\"IP\",\"%s\"\r\n", curr_gsm_socket.apn );
		sprintf( str_ETCPIP, "AT%%ETCPIP=1,\"%s\",\"%s\"\r\n", curr_gsm_socket.user, curr_gsm_socket.psw );

		GPIO_ResetBits( GSM_PWR_PORT, GSM_PWR_PIN );
		GPIO_ResetBits( GSM_TERMON_PORT, GSM_TERMON_PIN );
		pt_timer_set( &timer_gsm_power, 200 );
		PT_WAIT_UNTIL( pt, pt_timer_expired( &timer_gsm_power ) );
		GPIO_SetBits( GSM_TERMON_PORT, GSM_TERMON_PIN );
		GPIO_SetBits( GSM_PWR_PORT, GSM_PWR_PIN );

		for( at_init_index = 0; at_init_index < sizeof( at_init ) / sizeof( AT_CMD_RESP ); at_init_index++ )
		{
			for( at_init_retry = 0; at_init_retry < at_init[at_init_index].retry; at_init_retry++ )
			{
				if( at_init[at_init_index].atcmd != RT_NULL )
				{
					rt_kprintf( "%08d gsm_send>%s", rt_tick_get( ), at_init[at_init_index].atcmd );
					m66_write( &dev_gsm, 0, at_init[at_init_index].atcmd, strlen( at_init[at_init_index].atcmd ) );
				}
				pt_timer_set( &timer_gsm_power, at_init[at_init_index].timeout );
				PT_WAIT_UNTIL( pt, pt_timer_expired( &timer_gsm_power ) || ( RT_EOK == pt_resp( at_init[at_init_index].resp ) ) );
				if( pt_timer_expired( &timer_gsm_power ) ) /*��ʱ*/
				{
					rt_kprintf( "timeout\r\n" );
					
				}else
				{
					break;
				}
			}
			if( pt_timer_expired( &timer_gsm_power ) ) /*��Ϊ��ʱ�˳���*/
			{
				PT_EXIT( pt );
			}
		}
		gsm_state		= GSM_AT;                   /*�л���AT״̬*/
	}
	if( gsm_state == GSM_POWEROFF )
	{
		GPIO_ResetBits( GSM_PWR_PORT, GSM_PWR_PIN );
		GPIO_ResetBits( GSM_TERMON_PORT, GSM_TERMON_PIN );
		gsm_state=GSM_IDLE;
	}
	PT_END( pt );
}



/*������·ά��,ֻά��һ��������·���ϲ㴦��*/
static int gsm_socket_process(struct pt *pt)
{
	static char			buf[64];
	static uint8_t		i,j;
	static struct pt_timer timer_gsm_socket;

	PT_BEGIN(pt);
	if( gsm_state == GSM_AT )
	{
		if( curr_gsm_socket.state == SOCKET_DNS )
		{
			for(j=0;j<4;j++)
			{
				sprintf( buf, "AT%%DNSR=\"%s\"\r\n", curr_gsm_socket.ip_domain);
				rt_kprintf("%08d gsm_send>%s", rt_tick_get( ),buf);
				m66_write( &dev_gsm, 0, buf, sizeof( buf ) );
				pt_timer_set( &timer_gsm_socket, RT_TICK_PER_SECOND * 5 );
				PT_WAIT_UNTIL( pt, pt_timer_expired( &timer_gsm_socket ) || ( RT_EOK == pt_resp( pt_resp_DNSR ) ) );
				if( pt_timer_expired( &timer_gsm_socket ) ) /*��ʱ*/
				{
					rt_kprintf( "timeout\r\n" );
				}
				else
				{
					curr_gsm_socket.state = SOCKET_CONNECT;
					break;
				}
			}
			if( pt_timer_expired( &timer_gsm_socket ) ) /*��ʱ*/
			{
				curr_gsm_socket.state = SOCKET_DNS_ERR;
			}
				
		}
			
		if(curr_gsm_socket.state == SOCKET_CONNECT)
		{
			if(curr_gsm_socket.type=='u')
			{
				sprintf(buf,"AT%%IPOPENX=%d,\"UDP\",\"%s\",%d\r\n",1,curr_gsm_socket.ip_str,curr_gsm_socket.port);
			}
			else
			{
				sprintf(buf,"AT%%IPOPENX=%d,\"TCP\",\"%s\",%d\r\n",1,curr_gsm_socket.ip_str,curr_gsm_socket.port);
			}
			rt_kprintf("%08d gsm_send>%s", rt_tick_get( ),buf);
			m66_write( &dev_gsm, 0, buf, sizeof( buf ) );
			pt_timer_set( &timer_gsm_socket, RT_TICK_PER_SECOND * 35 );
			PT_WAIT_UNTIL( pt, pt_timer_expired( &timer_gsm_socket ) || ( RT_EOK == pt_resp( pt_resp_IPOPENX)));
			if( pt_timer_expired( &timer_gsm_socket ) ) /*��ʱ*/
			{
				rt_kprintf( "timeout\r\n" );
				curr_gsm_socket.state = SOCKET_CONNECT_ERR;
			}
			else
			{
				curr_gsm_socket.state = SOCKET_READY;
				break;
			}
		}

	}
	PT_END( pt );
}




ALIGN( RT_ALIGN_SIZE )
static char thread_gsm_stack[512];
struct rt_thread thread_gsm;


/***********************************************************
* Function:       rt_thread_entry_gsm
* Description:    ���մ���״̬ת��
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static void rt_thread_entry_gsm( void* parameter )
{
	rt_tick_t		curr_ticks;
	rt_err_t		res;
	unsigned char	ch;

	PT_INIT( &pt_gsm_power );
	PT_INIT( &pt_gsm_socket );

	while( 1 )
	{
		while( rt_ringbuffer_getchar( &rb_uart4_rx, &ch ) == 1 ) /*������ʱ����������*/
		{
			gsm_rx[gsm_rx_wr++] = ch;
			if( gsm_rx_wr == GSM_RX_SIZE )
			{
				gsm_rx_wr = 0;
			}
			gsm_rx[gsm_rx_wr] = 0;
		}
		if( rt_tick_get( ) - last_tick > RT_TICK_PER_SECOND / 10 )  //�ȴ�100ms,ʵ���Ͼ��Ǳ䳤����ʱ,���100ms������һ�����ݰ�
		{
			if( gsm_rx_wr )
			{
				gsmrx_cb( gsm_rx, gsm_rx_wr );                      /*������Ϣ�Ĵ�����*/
				gsm_rx_wr = 0;
			}
		}
		protothread_gsm_power( &pt_gsm_power );
		gsm_socket_process(&pt_gsm_socket);
		rt_thread_delay( RT_TICK_PER_SECOND / 20 );
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

/*��ʼ�����ڽ��ջ�����*/
	rt_ringbuffer_init( &rb_uart4_rx, uart4_rxbuf, UART4_RX_SIZE );

	rt_mb_init( &mb_gsmrx, "gsm_rx", &mb_gsmrx_pool, MB_GSMRX_POOL_SIZE / 4, RT_IPC_FLAG_FIFO );

	rt_thread_init( &thread_gsm,
	                "gsm",
	                rt_thread_entry_gsm,
	                RT_NULL,
	                &thread_gsm_stack[0],
	                sizeof( thread_gsm_stack ), 7, 5 );
	rt_thread_startup( &thread_gsm );

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


/*����gsm״̬ 0 ��ѯ*/
rt_err_t config_gsmstate( int cmd )
{
	if(cmd)	gsm_state = cmd;
	return gsm_state;
}

FINSH_FUNCTION_EXPORT( config_gsmstate, control gsm state );


/*
�ж�һ���ַ����ǲ��Ǳ�ʾip��str
����ɡ[[0..9|.] ���

'.' 0x2e
'/' 0x2f
'0' 0x30
'9' 0x39
��һ�¡����ж� '/'
*/
static uint8_t is_ipstr(char * str)
{
	char *p=str;
	while(*p!=NULL)
	{
		if((*p>'9')||(*p<'.')) return 0;
		p++;
	}
	return 1;

}


/*������Ϣ�������*/
rt_err_t dbgmsg( uint32_t i)
{
	if(i==0) rt_kprintf("debmsg=%d\r\n",fgsm_rawdata_out);
	else
		fgsm_rawdata_out=i;
	return RT_EOK;
}

FINSH_FUNCTION_EXPORT( dbgmsg, dbgmsg count );




/*����AT����*/
rt_size_t gsm_write( char *sinfo )
{
	return m66_write( &dev_gsm, 0, sinfo, strlen( sinfo ) );
}

FINSH_FUNCTION_EXPORT( gsm_write, write gsm );


rt_size_t gsm_ipsend( uint8_t* buff, rt_size_t count ,rt_int32_t timeout)
{
	rt_size_t	len = count;
	uint8_t		*p	= (uint8_t*)buff;
	uint8_t		c;
	char		*pstr;
	rt_err_t	ret;	

	uint8_t		buf_start[20];
	uint8_t		buf_end[4]={'"',0x0d,0x0a,0x0};
	
	uint8_t 	tbl[16]={'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

	sprintf(buf_start,"AT%%IPSENDX=1,\"");
	m66_write( &dev_gsm,0,buf_start,strlen(buf_start));
	rt_kprintf("%s",buf_start);
	while( len )
	{
		c=*p++;
		
		USART_SendData( UART4, tbl[c>>4] );
		while( USART_GetFlagStatus( UART4, USART_FLAG_TC ) == RESET )
		{
		}
		rt_kprintf("%c",tbl[c>>4]);
		USART_SendData( UART4, tbl[c&0x0f] );
		while( USART_GetFlagStatus( UART4, USART_FLAG_TC ) == RESET )
		{
		}
		rt_kprintf("%c",tbl[c&0x0f]);
		len--;
	}
	m66_write( &dev_gsm,0,buf_end,3);
	rt_kprintf("%s",buf_end);

	ret = rt_mb_recv( &mb_gsmrx, (rt_uint32_t*)&pstr, timeout );
	if( ret == -RT_ETIMEOUT )
	{
		return RT_ETIMEOUT;
	}
	len = ( ( *pstr ) << 8 ) | ( *( pstr + 1 ) );
	if(strstr(pstr+2,"OK")) ret=RT_EOK;
	else ret=RT_ERROR;
	rt_free( pstr );
	
	return ret;
}


#endif

/************************************** The End Of File **************************************/
