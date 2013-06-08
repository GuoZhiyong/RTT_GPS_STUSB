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

/*����һ��gsm�豸*/
static struct rt_device dev_gsm;


/*����һ��uart�豸ָ��,ͬgsmģ�����ӵĴ���
   ָ��һ���Ѿ��򿪵Ĵ���
 */
static struct rt_mailbox	mb_gsmrx;
#define MB_GSMRX_POOL_SIZE 32
static uint8_t				mb_gsmrx_pool[MB_GSMRX_POOL_SIZE];

/*��������ʹ�õ�mailbox*/
static struct rt_mailbox	mb_tts;
#define MB_TTS_POOL_SIZE 32
static uint8_t				mb_tts_pool[MB_TTS_POOL_SIZE];

enum
{
	GSMBUSY_NONE=0x0,
	GSMBUSY_POWERON,    /*�ϵ�Ĺ�����*/
	GSMBUSY_AT,         /*AT*/
	GSMBUSY_TTS,        /*TTSָ��*/
	GSMBUSY_VOICE,      /*¼��*/
	GSMBUSY_SOCKET,     /*�ϱ�����*/
} gsmbusy = GSMBUSY_NONE;

/*gsm �����ʹ�õ��ź���*/

static struct rt_semaphore sem_at;

#define GSM_RX_SIZE 2048
static uint8_t		gsm_rx[GSM_RX_SIZE];
static uint16_t		gsm_rx_wr = 0;

static T_GSM_STATE	gsm_state = GSM_IDLE;

/*���ڽ��ջ���������*/
#define UART4_RX_SIZE 256
static uint8_t	uart4_rxbuf[UART4_RX_SIZE];
static uint16_t uart4_rxbuf_wr = 0, uart4_rxbuf_rd = 0;

/*���������������Ϣ*/
static uint32_t fgsm_rawdata_out = 0xfffffff;

/*���һ���յ��������ݵ�ʱ��,��ʹ��sem��Ϊ��ʱ�ж�*/
static uint32_t last_tick;

/*�Ƿ��ڲ����У����ܲ�����һ��*/
static uint8_t tts_busy = 0;

struct _gsm_param
{
	char	imei[16];
	char	imsi[16];
	uint8_t csq;
	char	ip[16];
} gsm_param;

/*���ŵ����Ĳ���*/
struct _dial_param
{
	char	*apn;
	char	*user;
	char	*psw;
	uint8_t fconnect;
} dial_param;

/*��ǰ����������*/
static GSM_SOCKET curr_socket;


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

	GPIO_InitStructure.GPIO_OType	= GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Pin		= GPIO_Pin_9;
	GPIO_Init( GPIOD, &GPIO_InitStructure );

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
			*( (int*)arg ) = gsm_state;
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
static void gsmrx_cb( char *pInfo, uint16_t size )
{
	int		i, count, len = size;
	uint8_t tbl[24] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0, 0, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf };
	char	c, *pmsg;
	char	*psrc = RT_NULL, *pdst = RT_NULL;
	int32_t infolen, linkno;

/*��������Ϣ��ֱ��֪ͨ�ϲ����*/
	if( fgsm_rawdata_out )
	{
		rt_kprintf( "\r\n%08d gsm_rx<%s\r\n", rt_tick_get( ), pInfo );
		fgsm_rawdata_out--;
	}

/*�ж�������*/
	psrc = pInfo;
	if( strncmp( psrc, "%IPDATA:", 7 ) == 0 )
	{
		/*����������Ϣ,���������Ż���pdst*/
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
		pmsg	= pdst + 1; /*ָ����һ��λ��*/
		for( i = 0; i < infolen; i++ )
		{
			c		= tbl[*pmsg++ - '0'] << 4;
			c		|= tbl[*pmsg++ - '0'];
			*pdst++ = c;
		}
		gprs_rx( linkno, psrc, infolen );
		return;
	}


/*
   00060726 gsm_rx>+CMTI: "SM",1
   ����,�ж�Ϣ��*/
	if( strncmp( psrc, "+CMTI: \"SM\",", 11 ) == 0 )
	{
	}

	/*ֱ�ӷ��͵�Mailbox��,�ڲ�����*/
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

/*
   ��ӦCREG��CGREG
   00017796 gsm_rx<+CREG: 0,3

   00018596 gsm_rx<+CREG: 0,1

   ��
   i = sscanf( p, "%*[^:]:%d,%d", &n, &code );
   ��������!(������λ���й�?)

 */
rt_err_t resp_CGREG( char *p, uint16_t len )
{
	char	* pfind;
	char	* psrc;
	char	c;

	psrc	= p;
	pfind	= strchr( psrc, ',' );
	if( pfind != RT_NULL )
	{
		c = *( pfind + 1 );
		if( ( c == '1' ) || ( c == '5' ) )
		{
			return RT_EOK;
		}
	}
	return RT_ERROR;
}

/*
   SIM����IMSI����Ϊ4600 00783208249��
      460 00 18 23 20 86 42

   �ӿں������ֶε�����ΪIMSI����ĺ�12λ
   ��6���ֽڵ�����Ϊ 0x00 0x07 0x83 0x20 0x82 0x49

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

/*��ӦCGSN**/
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

/*
   %ETCPIP:1,"10.24.44.142","0.0.0.0","0.0.0.0"
   ֻ���ҵ�һ��IP,  LocalIP

 */
rt_err_t resp_ETCPIP( char *p, uint16_t len )
{
	uint8_t stage	= 0;
	char	*psrc	= p;
	char	*pdst	= gsm_param.ip;

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

#if 0
rt_err_t resp_DNSR( char *p, uint16_t len )
{
	uint8_t i, stage = 0;
	char	*psrc	= p;
	char	*pdst	= RT_NULL;
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
					if( curr_socket.state == SOCKET_DNS )
					{
						pdst = curr_socket.ip_addr;
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
	rt_kprintf( "dns ip=%s\r\n", curr_socket.ip_addr );
	return RT_EOK;
}

#endif


/*
   AT%DNSR="www.google.com"
   %DNSR:74.125.153.147
   OK
 */

rt_err_t resp_DNSR( char *p, uint16_t len )
{
	char *psrc = p;

	if( strstr( psrc, "%DNSR:" ) == RT_NULL )
	{
		return RT_ERROR;
	}
	psrc = p;
	memset( curr_socket.ip_addr, 0, 15 );
	memcpy( curr_socket.ip_addr, psrc + 6, len - 6 );

	rt_kprintf( "dns ip=%s\r\n", curr_socket.ip_addr );
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
	char *psrc = p;
	if( strstr( psrc, "CONNECT" ) != RT_NULL )
	{
		return RT_EOK;
	}
	return RT_ERROR;
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
rt_err_t resp_DEBUG( char *p, uint16_t len )
{
	rt_kprintf( "%s", p );
	return RT_ERROR;
}

typedef rt_err_t ( *AT_RESP )( char *p, uint16_t len );
typedef struct
{
	char		*atcmd;
	AT_RESP		resp;
	uint16_t	timeout;
	uint16_t	retry;
}AT_CMD_RESP;

/*�ȴ��̶��ַ����ķ���*/
rt_err_t gsm_wait_str( char *respstr, uint32_t timeout )
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
		if( err != -RT_ETIMEOUT )                           /*û�г�ʱ,�ж���Ϣ�Ƿ���ȷ*/
		{
			if( strstr( pmsg + 2, respstr ) != RT_NULL )    /*ǰ�����ֽ�Ϊ���ȣ��ҵ���*/
			{
				rt_free( pmsg );                            /*�ͷ�*/
				return RT_EOK;
			}
			rt_free( pmsg );                                /*�ͷ�*/
			/*����ʣ�µĳ�ʱʱ��,������������ִ�е���ʱ�������,Ҫ�ж�*/
			if( rt_tick_get( ) < tick_end )                 /*��û�г�ʱ*/
			{
				tm = tick_end - rt_tick_get( );
			}else
			{
				flag_wait = 0;
			}
		}else /*�Ѿ���ʱ*/
		{
			flag_wait = 0;
		}
	}
	return RT_ETIMEOUT;
}

/*
   ����AT������ȴ���Ӧ�ַ���
   ���� code.google.com/p/gsm-playground��ʵ��
 */

rt_err_t gsm_send_wait_str( char *AT_cmd_string,
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

	err = rt_sem_take( &sem_at, timeout );
	if( err != RT_EOK  )
	{
		return err;
	}

	for( i = 0; i < no_of_attempts; i++ )
	{
		tick_start	= rt_tick_get( );
		tick_end	= tick_start + timeout;
		tm			= timeout;
		flag_wait	= 1;
		rt_kprintf( "%08d gsm>%s\r\n", tick_start, AT_cmd_string );
		m66_write( &dev_gsm, 0, AT_cmd_string, strlen( AT_cmd_string ) );
		while( flag_wait )
		{
			err = rt_mb_recv( &mb_gsmrx, (rt_uint32_t*)&pmsg, tm );
			if( err ==RT_EOK )                           /*û�г�ʱ,�ж���Ϣ�Ƿ���ȷ*/
			{
				if( strstr( pmsg + 2, respstr ) != RT_NULL )    /*�ҵ���*/
				{
					rt_free( pmsg );                            /*�ͷ�*/
					rt_sem_release(&sem_at);
					return RT_EOK;
				}
				rt_free( pmsg );                                /*�ͷ�*/
				/*����ʣ�µĳ�ʱʱ��,������������ִ�е���ʱ�������,Ҫ�ж�*/
				if( rt_tick_get( ) < tick_end )                 /*��û�г�ʱ*/
				{
					tm = tick_end - rt_tick_get( );
				}else
				{
					flag_wait = 0;
				}
			}else /*�Ѿ���ʱ*/
			{
				flag_wait = 0;
			}
		}
	}
	rt_sem_release(&sem_at);
	return ( -RT_ETIMEOUT );
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
rt_err_t gsm_send_wait_str_ok( char *AT_cmd_string,
                               uint32_t timeout,
                               char * respstr,
                               uint8_t no_of_attempts )
{
	rt_err_t		err;
	uint8_t			i = 0;
	char			*pmsg;
	uint32_t		tick_start, tick_end;
	uint32_t		tm;
	__IO uint8_t	flag_wait = 1;
	err = rt_sem_take( &sem_at, timeout );
	if( err != RT_EOK )
	{
		return err;
	}	

	for( i = 0; i < no_of_attempts; i++ )
	{
		tick_start	= rt_tick_get( );
		tick_end	= tick_start + timeout;
		tm			= timeout;
		flag_wait	= 1;
		rt_kprintf( "%08d gsm>%s\r\n", tick_start, AT_cmd_string );
		m66_write( &dev_gsm, 0, AT_cmd_string, strlen( AT_cmd_string ) );
		while( flag_wait )
		{
			err = rt_mb_recv( &mb_gsmrx, (rt_uint32_t*)&pmsg, tm );
			if( err ==RT_EOK )                           /*û�г�ʱ,�ж���Ϣ�Ƿ���ȷ*/
			{
				if( strstr( pmsg + 2, respstr ) != RT_NULL )    /*�ҵ���*/
				{
					rt_free( pmsg );                            /*�ͷ�*/
					goto lbl_send_wait_ok;
				}
				rt_free( pmsg );                                /*�ͷ�*/
				/*����ʣ�µĳ�ʱʱ��,������������ִ�е���ʱ�������,Ҫ�ж�*/
				if( rt_tick_get( ) < tick_end )                 /*��û�г�ʱ*/
				{
					tm = tick_end - rt_tick_get( );
				}else
				{
					flag_wait = 0;
				}
			}else /*�Ѿ���ʱ*/
			{
				flag_wait = 0;
			}
		}
	}
	rt_sem_release(&sem_at);
	return -RT_ETIMEOUT;

lbl_send_wait_ok:
	err = gsm_wait_str( "OK", RT_TICK_PER_SECOND * 2 );
	rt_sem_release(&sem_at);
	return err;
}

/*
   ����AT������ȴ���Ӧ��������
   ���� code.google.com/p/gsm-playground��ʵ��
 */

int8_t gsm_send_wait_func( char *AT_cmd_string,
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

	err = rt_sem_take( &sem_at, timeout );
	if( err != RT_EOK )
	{
		return err;
	}

	for( i = 0; i < no_of_attempts; i++ )
	{
		tick_start	= rt_tick_get( );
		tick_end	= tick_start + timeout;
		tm			= timeout;
		flag_wait	= 1;
		rt_kprintf( "%08d gsm>%s\r\n", tick_start, AT_cmd_string );
		m66_write( &dev_gsm, 0, AT_cmd_string, strlen( AT_cmd_string ) );
		while( flag_wait )
		{
			err = rt_mb_recv( &mb_gsmrx, (rt_uint32_t*)&pmsg, tm );
			if( err ==RT_EOK )                   /*û�г�ʱ,�ж���Ϣ�Ƿ���ȷ*/
			{
				len		= ( *pmsg << 8 ) | ( *( pmsg + 1 ) );
				pinfo	= pmsg + 2;
				if( respfunc( pinfo, len ) == RT_EOK )  /*�ҵ���*/
				{
					rt_free( pmsg );                    /*�ͷ�*/
					rt_sem_release(&sem_at);
					return RT_EOK;
				}
				rt_free( pmsg );                        /*�ͷ�*/
				/*����ʣ�µĳ�ʱʱ��,������������ִ�е���ʱ�������,Ҫ�ж�*/
				if( rt_tick_get( ) < tick_end )         /*��û�г�ʱ*/
				{
					tm = tick_end - rt_tick_get( );
				}else
				{
					flag_wait = 0;
				}
			}else /*�Ѿ���ʱ*/
			{
				flag_wait = 0;
			}
		}
	}
	rt_sem_release(&sem_at);
	return ( -RT_ETIMEOUT );
}

/*
   �ж�һ���ַ����ǲ��Ǳ�ʾip��str
   ����ɡ[[0..9|.] ���
   '.' 0x2e   '/' 0x2f   '0' 0x30  '9' 0x39   ��һ�¡����ж� '/'
   ����ֵ
   0:��ʾ�����ĵ�ַ
   1:��ʾ��IP��ַ

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

/*gsm����Ĵ����˳�*/
static void rt_thread_gsm_power_on( void* parameter )
{
	rt_err_t ret;

lbl_poweron_start:
	rt_kprintf( "%08d gsm_power_on>start\r\n", rt_tick_get( ) );
//	gsm_state = GSM_POWERONING; /*��λ�ϵ������*/

	GPIO_ResetBits( GSM_PWR_PORT, GSM_PWR_PIN );
	GPIO_ResetBits( GSM_TERMON_PORT, GSM_TERMON_PIN );
	rt_thread_delay( RT_TICK_PER_SECOND / 10 );
	GPIO_SetBits( GSM_TERMON_PORT, GSM_TERMON_PIN );
	GPIO_SetBits( GSM_PWR_PORT, GSM_PWR_PIN );

	if( gsm_wait_str( "OK", RT_TICK_PER_SECOND * 5 ) != RT_EOK )
	{
		goto lbl_poweron_start;
	}

	if( gsm_wait_str( "OK", RT_TICK_PER_SECOND * 5 ) != RT_EOK  )
	{
		goto lbl_poweron_start;
	}

	if( gsm_send_wait_str( "ATE0\r\n", RT_TICK_PER_SECOND * 5, "OK", 1 ) != RT_EOK  )
	{
		goto lbl_poweron_start;
	}

	ret = gsm_send_wait_str( "ATV1\r\n", RT_TICK_PER_SECOND * 5, "OK", 1 );
	if( ret != RT_EOK  )
	{
		goto lbl_poweron_start;
	}

	ret = gsm_send_wait_str_ok( "AT%TSIM\r\n", RT_TICK_PER_SECOND * 5, "%TSIM 1", 1 );
	if( ret != RT_EOK  )
	{
		goto lbl_poweron_start;
	}

	ret = gsm_send_wait_str_ok( "AT+CPIN?\r\n", RT_TICK_PER_SECOND * 2, "+CPIN: READY", 30 );
	if( ret != RT_EOK  )
	{
		goto lbl_poweron_start;
	}

	ret = gsm_send_wait_func( "AT+CIMI\r\n", RT_TICK_PER_SECOND * 2, resp_CIMI, 10 );
	ret = gsm_wait_str( "OK", RT_TICK_PER_SECOND * 2 );
	if( ret != RT_EOK  )
	{
		goto lbl_poweron_start;
	}

	ret = gsm_send_wait_str( "AT+CLIP=1\r\n", RT_TICK_PER_SECOND * 2, "OK", 2 );
	if( ret != RT_EOK  )
	{
		goto lbl_poweron_start;
	}

	ret = gsm_send_wait_func( "AT+CREG?\r\n", RT_TICK_PER_SECOND * 2, resp_CGREG, 30 );
	ret = gsm_wait_str( "OK", RT_TICK_PER_SECOND * 2 );
	if( ret != RT_EOK  )
	{
		goto lbl_poweron_start;
	}

	rt_kprintf( "%08d gsm_power_on>end\r\n", rt_tick_get( ) );

	gsm_state = GSM_AT; /*��ǰ����AT״̬,���Բ��ţ�����*/
}

/*������·ά��,ֻά��һ��������·���ϲ㴦��*/
static void rt_thread_gsm_socket( void* parameter )
{
	char		buf[128];
	rt_err_t	err;

	/*�Ҷ�����*/
	if( curr_socket.state == SOCKET_CLOSE )
	{
		return;
	}

	/*��������*/
	if( curr_socket.state != SOCKET_START )
	{
		return;
	}

	if( is_ipaddr( curr_socket.ipstr ) ) /*��IP��ַ*/
	{
		strcpy( curr_socket.ip_addr, curr_socket.ipstr );
		curr_socket.state = SOCKET_CONNECT;
	}else
	{
		curr_socket.state = SOCKET_DNS;
	}

	if( curr_socket.state == SOCKET_DNS )
	{
		sprintf( buf, "AT%%DNSR=\"%s\"\r\n", curr_socket.ipstr );
		err = gsm_send_wait_func( buf, RT_TICK_PER_SECOND * 10, resp_DNSR, 1 );
		if( err != RT_EOK )
		{
			curr_socket.state = SOCKET_DNS_ERR;
			goto lbl_gsm_socket_end;
		}
		curr_socket.state = SOCKET_CONNECT;
	}

	if( curr_socket.state == SOCKET_CONNECT )
	{
		if( curr_socket.type == 'u' )
		{
			sprintf( buf, "AT%%IPOPENX=%d,\"UDP\",\"%s\",%d\r\n", curr_socket.linkno, curr_socket.ip_addr, curr_socket.port );
		}else
		{
			sprintf( buf, "AT%%IPOPENX=%d,\"TCP\",\"%s\",%d\r\n", curr_socket.linkno, curr_socket.ip_addr, curr_socket.port );
		}
		err = gsm_send_wait_str( buf, RT_TICK_PER_SECOND * 10, "CONNECT", 1 );
		if( err != RT_EOK )
		{
			curr_socket.state = SOCKET_CONNECT_ERR;
			goto lbl_gsm_socket_end;
		}
	}
	curr_socket.state = SOCKET_READY;
lbl_gsm_socket_end:
	gsm_state = GSM_TCPIP; /*socket���̴�����ɣ������state��*/
	rt_kprintf( "%08d gsm_socket>end socket.state=%d\r\n", rt_tick_get( ), curr_socket.state );
}

/*���Ƶ��������Ƕ���*/
static void rt_thread_gsm_gprs( void* parameter )
{
	char		buf[128];
	rt_err_t	err;

/*�ж�Ҫִ�������Ķ���*/

	if( dial_param.fconnect == 0 ) /*����*/
	{
		err = gsm_send_wait_str( "AT%IPCLOSE=1\r\n", RT_TICK_PER_SECOND, "OK", 1 );
		if( err != RT_EOK  )
		{
			goto lbl_gsm_gprs_end_err;
		}
		err = gsm_send_wait_str( "AT%IPCLOSE=2\r\n", RT_TICK_PER_SECOND, "OK", 1 );
		if( err != RT_EOK  )
		{
			goto lbl_gsm_gprs_end_err;
		}
		err = gsm_send_wait_str( "AT%IPCLOSE=3\r\n", RT_TICK_PER_SECOND, "OK", 1 );
		if( err != RT_EOK  )
		{
			goto lbl_gsm_gprs_end_err;
		}
		err = gsm_send_wait_str( "AT%IPCLOSE=5\r\n", RT_TICK_PER_SECOND, "OK", 1 );
		err = gsm_wait_str( "%IPCLOSE:5", RT_TICK_PER_SECOND * 30 );
		if( err != RT_EOK  )
		{
			goto lbl_gsm_gprs_end_err;
		}
		gsm_state = GSM_AT;
		goto lbl_gsm_gprs_end;
	}

	if( dial_param.fconnect == 1 ) /*�������*/
	{
		err = gsm_send_wait_str_ok( "AT+CGATT?\r\n", RT_TICK_PER_SECOND * 2, "+CGATT: 1", 50 );
		if( err != RT_EOK  )
		{
			goto lbl_gsm_gprs_end_err;
		}

		sprintf( buf, "AT+CGDCONT=1,\"IP\",\"%s\"\r\n", dial_param.apn );
		err = gsm_send_wait_str( buf, RT_TICK_PER_SECOND * 10, "OK", 1 );
		if( err != RT_EOK )
		{
			goto lbl_gsm_gprs_end_err;
		}
		if( ( strlen( dial_param.user ) == 0 ) && ( strlen( dial_param.user ) == 0 ) )
		{
			err = gsm_send_wait_str( "AT%ETCPIP\r\n", RT_TICK_PER_SECOND * 151, "OK", 1 );
		}else
		{
			sprintf( buf, "AT%ETCPIP=\"%s\",\"%s\"\r\n", dial_param.user, dial_param.psw );
			err = gsm_send_wait_str( buf, RT_TICK_PER_SECOND * 151, "OK", 1 );
		}
		if( err != RT_EOK )
		{
			goto lbl_gsm_gprs_end_err;
		}

		err = gsm_send_wait_func( "AT%ETCPIP?\r\n", RT_TICK_PER_SECOND * 10, resp_ETCPIP, 1 );
		err = gsm_wait_str( "OK", RT_TICK_PER_SECOND * 2 );
		if( err != RT_EOK )
		{
			goto lbl_gsm_gprs_end_err;
		}

		err = gsm_send_wait_str( "AT%IOMODE=1,2,1\r\n", RT_TICK_PER_SECOND * 10, "OK", 1 );
		if( err != RT_EOK )
		{
			goto lbl_gsm_gprs_end_err;
		}

		gsm_state = GSM_TCPIP;
		goto lbl_gsm_gprs_end;
	}
lbl_gsm_gprs_end_err:
	gsm_state = GSM_ERR_GPRS;
lbl_gsm_gprs_end:
	rt_kprintf( "%08d gsm_gprs>end state=%d\r\n", rt_tick_get( ), gsm_state );
}

/*
   tts���������Ĵ���

   ��ͨ��
   %TTS: 0 �ж�tts״̬(���ɲ�����ÿ�ζ������)
   ����AT%TTS? ��ѯ״̬
 */
void tts_process( void )
{
	rt_err_t	ret;
	rt_size_t	len;
	uint8_t		*pinfo, *p;
	uint8_t		c;

	char		buf[20];
	char		tbl[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
/*��ǰ���ڲ�����*/
	if( tts_busy == 1 )
	{
		return;
	}
/*gsm�ڴ�����������*/
	if( gsmbusy )
	{
		return;
	}
	gsmbusy = GSMBUSY_TTS;
/*�Ƿ�����ϢҪ����*/
	ret = rt_mb_recv( &mb_tts, (rt_uint32_t*)&pinfo, 0 );
	if( ret == -RT_ETIMEOUT )
	{
		gsmbusy = GSMBUSY_NONE;
		return;
	}

	GPIO_ResetBits( GPIOD, GPIO_Pin_9 ); /*������*/

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

/*���жϣ���gsmrx_cb�д���*/
	rt_free( pinfo );
	gsm_wait_str( "%TTS: 0", RT_TICK_PER_SECOND * 5 );
	GPIO_SetBits( GPIOD, GPIO_Pin_9 ); /*�ع���*/
	gsmbusy = GSMBUSY_NONE;
}

ALIGN( RT_ALIGN_SIZE )
static char thread_gsm_stack[512];
struct rt_thread thread_gsm;


/*
   ״̬ת����ͬʱ�����������������š�TTS��¼���ȹ���
 */
static void rt_thread_entry_gsm( void* parameter )
{
	while( 1 )
	{
		if( gsm_state == GSM_POWERON )
		{
			rt_thread_gsm_power_on( RT_NULL );
		}

		if( gsm_state == GSM_GPRS )
		{
			rt_thread_gsm_gprs( RT_NULL );
		}

		if( gsm_state == GSM_SOCKET_PROC )
		{
			rt_thread_gsm_socket( RT_NULL );
		}
		if( gsm_state == GSM_POWEROFF )
		{
		}

		tts_process( );   /*����tts��Ϣ*/
		rt_thread_delay( RT_TICK_PER_SECOND / 20 );
	}
}

ALIGN( RT_ALIGN_SIZE )
static char thread_gsm_rx_stack[512];
struct rt_thread thread_gsm_rx;


/*
   ״̬ת����ͬʱ������š�TTS��¼���ȹ���
 */
static void rt_thread_entry_gsm_rx( void* parameter )
{
	unsigned char ch;

	while( 1 )
	{
		while( uart4_rxbuf_rd != uart4_rxbuf_wr )   /*������ʱ����������*/
		{
			ch				= uart4_rxbuf[uart4_rxbuf_rd++];
			uart4_rxbuf_rd	%= UART4_RX_SIZE;
			if( ch > 0x1F )                         /*�ɼ��ַ��ű���*/
			{
				gsm_rx[gsm_rx_wr++] = ch;
				gsm_rx_wr			%= GSM_RX_SIZE;
				gsm_rx[gsm_rx_wr]	= 0;
			}
			if( ch == 0x0d )
			{
				if( gsm_rx_wr )
				{
					gsmrx_cb( gsm_rx, gsm_rx_wr );                  /*������Ϣ�Ĵ�����*/
				}
				gsm_rx_wr = 0;
			}
		}
		if( rt_tick_get( ) - last_tick > RT_TICK_PER_SECOND / 10 )  //�ȴ�100ms,ʵ���Ͼ��Ǳ䳤����ʱ,���100ms������һ�����ݰ�
		{
			if( gsm_rx_wr )
			{
				gsmrx_cb( gsm_rx, gsm_rx_wr );                      /*������Ϣ�Ĵ�����*/
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

	rt_sem_init( &sem_at, "sem_at", 1, RT_IPC_FLAG_FIFO );

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

/*������Ϣ�������*/
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


/*
   ����AT����
   ��α�֤������,������ִ�У�����ȴ���ʱ��

 */
rt_size_t at_write( char *sinfo, uint8_t timeout_s )
{
	gsm_send_wait_func( sinfo, RT_TICK_PER_SECOND * timeout_s, resp_DEBUG, 1 );
	//gsm_wait_str("",RT_TICK_PER_SECOND * timeout_s);
	return RT_EOK;
}

FINSH_FUNCTION_EXPORT( at_write, write gsm );


/***********************************************************
* Function: ֱ�ӷ�����Ϣ��Ҫ��808ת���m66ת��
* Description:
* Input:   ԭʼ��Ϣ
* Output:
* Return:
* Others:
***********************************************************/
rt_size_t socket_write( uint8_t linkno, uint8_t* buff, rt_size_t count )
{
	rt_size_t	len = count;
	uint8_t		*p	= (uint8_t*)buff;
	uint8_t		c;
	char		*pstr;
	rt_err_t	ret;

	char		buf_start[20];
	char		buf_end[4] = { '"', 0x0d, 0x0a, 0x0 };

	uint8_t		tbl[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

	sprintf( buf_start, "AT%%IPSENDX=%d,\"", linkno );
	m66_write( &dev_gsm, 0, buf_start, strlen( buf_start ) );
	rt_kprintf( "%s", buf_start );
	while( len )
	{
		c = *p++;
		if( c == 0x7E )
		{
			m66_write( &dev_gsm, 0, "7D02", 4 );
			rt_kprintf( "%s", "7D02" );
		}

		if( c == 0x7d )
		{
			m66_write( &dev_gsm, 0, "7D01", 4 );
			rt_kprintf( "%s", "7D01" );
		}
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

	ret = gsm_wait_str( "OK", RT_TICK_PER_SECOND * 10 );
	return ret;
}

FINSH_FUNCTION_EXPORT( socket_write, write socket );


/*
   �յ�tts��Ϣ������
   ����0:OK
    1:����RAM����
 */
rt_size_t tts_write( char* info )
{
	uint8_t		*pmsg;
	uint16_t	count;
	count = strlen( info );

	/*ֱ�ӷ��͵�Mailbox��,�ڲ�����*/
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

/*����gsm״̬ 0 ��ѯ*/
rt_err_t gsmstate( int cmd )
{
	if( cmd )
	{
		gsm_state = cmd;
	}
	return gsm_state;
}

FINSH_FUNCTION_EXPORT( gsmstate, control gsm state );

/*����socket״̬ 0 ��ѯ*/
rt_err_t socketstate( int cmd )
{
	if( cmd )
	{
		curr_socket.state = cmd;
	}
	return curr_socket.state;
}

FINSH_FUNCTION_EXPORT( socketstate, control socket state );

/*���Ƶ�¼��gprs*/
void ctl_gprs( char* apn, char* user, char*psw, uint8_t fdial )
{
	dial_param.apn		= apn;
	dial_param.user		= user;
	dial_param.psw		= psw;
	dial_param.fconnect = fdial;
	gsm_state			= GSM_GPRS;
}

/*����Զ�̵�ַ*/
void ctl_socket( uint8_t linkno, char type, char* remoteip, uint16_t remoteport, uint8_t fconnect )
{
	curr_socket.linkno	= linkno;
	curr_socket.type	= type;

	strcpy( &( curr_socket.ipstr[0] ), remoteip );
	curr_socket.port = remoteport;
	if( fconnect )
	{
		curr_socket.state = SOCKET_START;
	}else
	{
		curr_socket.state = SOCKET_CLOSE;
	}
	gsm_state = GSM_SOCKET_PROC;
}

/************************************** The End Of File **************************************/
