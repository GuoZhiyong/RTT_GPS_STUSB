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

#include "m66.h"

#define GSM_GPIO			GPIOC
#define GSM_TX_PIN			GPIO_Pin_10
#define GSM_TX_PIN_SOURCE	GPIO_PinSource10

#define GSM_RX_PIN			GPIO_Pin_11
#define GSM_RX_PIN_SOURCE	GPIO_PinSource11

typedef void ( *URC_CB )( char *s, uint16_t len );

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
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
rt_err_t resp_strOK( char *p, uint16_t len )
{
	char *psrc = p;

	if( strstr( psrc, "OK" ) == RT_NULL )
	{
		return -RT_ERROR;
	}
	return RT_EOK;
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
	char			*atcmd;
	enum RESP_TYPE	type;           /*�ж��Ǵ����ַ����Ƚ�,��������Ӧ����*/
	AT_RESP			resp;
	char			*compare_str;   /*Ҫ�Ƚϵ��ַ���*/
	uint16_t		timeout;
	uint16_t		retry;
}AT_CMD_RESP;

#if 0
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
			if( err == RT_EOK )                                 /*û�г�ʱ,�ж���Ϣ�Ƿ���ȷ*/
			{
				if( strstr( pmsg + 2, respstr ) != RT_NULL )    /*�ҵ���*/
				{
					rt_free( pmsg );                            /*�ͷ�*/
					rt_sem_release( &sem_at );
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
	rt_sem_release( &sem_at );
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
			if( err == RT_EOK )                                 /*û�г�ʱ,�ж���Ϣ�Ƿ���ȷ*/
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
	rt_sem_release( &sem_at );
	return -RT_ETIMEOUT;

lbl_send_wait_ok:
	err = gsm_wait_str( "OK", RT_TICK_PER_SECOND * 2 );
	rt_sem_release( &sem_at );
	return err;
}

/*
   ����AT������ȴ���Ӧ��������
   ���� code.google.com/p/gsm-playground��ʵ��
 */

rt_err_t gsm_send_wait_func( char *AT_cmd_string,
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
			if( err == RT_EOK )                         /*û�г�ʱ,�ж���Ϣ�Ƿ���ȷ*/
			{
				len		= ( *pmsg << 8 ) | ( *( pmsg + 1 ) );
				pinfo	= pmsg + 2;
				if( respfunc( pinfo, len ) == RT_EOK )  /*�ҵ���*/
				{
					rt_free( pmsg );                    /*�ͷ�*/
					rt_sem_release( &sem_at );
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
	rt_sem_release( &sem_at );
	return ( -RT_ETIMEOUT );
}

#endif


/*
   ����AT������ȴ���Ӧ��������
   ���� code.google.com/p/gsm-playground��ʵ��
 */

//rt_err_t gsm_send( AT_CMD_RESP* pat_cmd_resp )
rt_err_t gsm_send( char *atcmd,
                   RESP_FUNC respfunc,
                   char * compare_str,
                   uint8_t type,
                   uint32_t timeout,
                   uint8_t retry )

{
	rt_err_t		err;
	uint8_t			i;
	char			*pmsg;
	uint32_t		tick_start, tick_end;
	uint32_t		tm;
	__IO uint8_t	flag_wait;

	char			* pinfo;
	uint16_t		len;

	for( i = 0; i < retry; i++ )
	{
		tick_start	= rt_tick_get( );
		tick_end	= tick_start + timeout;
		tm			= timeout;
		flag_wait	= 1;
		if( strlen( atcmd ) ) /*Ҫ�����ַ���*/
		{
			rt_kprintf( "%08d gsm>%s\r\n", tick_start, atcmd );
			m66_write( &dev_gsm, 0, atcmd, strlen( atcmd ) );
		}
		while( flag_wait )
		{
			err = rt_mb_recv( &mb_gsmrx, (rt_uint32_t*)&pmsg, tm );
			if( err == RT_EOK )                                     /*û�г�ʱ,�ж���Ϣ�Ƿ���ȷ*/
			{
				len		= ( *pmsg << 8 ) | ( *( pmsg + 1 ) );
				pinfo	= pmsg + 2;

				if( type >= RESP_TYPE_STR )
				{
					if( strstr( pinfo, compare_str ) != RT_NULL )   /*�ҵ���*/
					{
						rt_free( pmsg );
						if( type == RESP_TYPE_STR_WITHOK )
						{
							goto lbl_send_wait_ok;
						}
						return RT_EOK;
					}
				}else if( respfunc( pinfo, len ) == RT_EOK )    /*�ҵ���*/
				{
					rt_free( pmsg );                            /*�ͷ�*/
					if( type == RESP_TYPE_FUNC_WITHOK )
					{
						goto lbl_send_wait_ok;
					}
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
	return ( -RT_ETIMEOUT );

lbl_send_wait_ok:
	err = rt_mb_recv( &mb_gsmrx, (rt_uint32_t*)&pmsg, tm );
	if( err == RT_EOK )   /*û�г�ʱ,�ж���Ϣ�Ƿ���ȷ*/
	{
		if( strstr( pmsg + 2, "OK" ) != RT_NULL )
		{
			rt_free( pmsg );
			return RT_EOK;
		}
	}
	rt_free( pmsg );
	return RT_ERROR;
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

#if 0
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

	if( gsm_wait_str( "OK", RT_TICK_PER_SECOND * 5 ) != RT_EOK )
	{
		goto lbl_poweron_start;
	}

	if( gsm_send_wait_str( "ATE0\r\n", RT_TICK_PER_SECOND * 5, "OK", 1 ) != RT_EOK )
	{
		goto lbl_poweron_start;
	}

	ret = gsm_send_wait_str( "ATV1\r\n", RT_TICK_PER_SECOND * 5, "OK", 1 );
	if( ret != RT_EOK )
	{
		goto lbl_poweron_start;
	}

	ret = gsm_send_wait_str_ok( "AT%TSIM\r\n", RT_TICK_PER_SECOND * 5, "%TSIM 1", 1 );
	if( ret != RT_EOK )
	{
		goto lbl_poweron_start;
	}

	ret = gsm_send_wait_str_ok( "AT+CPIN?\r\n", RT_TICK_PER_SECOND * 2, "+CPIN: READY", 30 );
	if( ret != RT_EOK )
	{
		goto lbl_poweron_start;
	}

	ret = gsm_send_wait_func( "AT+CIMI\r\n", RT_TICK_PER_SECOND * 2, resp_CIMI, 10 );
	ret = gsm_wait_str( "OK", RT_TICK_PER_SECOND * 2 );
	if( ret != RT_EOK )
	{
		goto lbl_poweron_start;
	}

	ret = gsm_send_wait_str( "AT+CLIP=1\r\n", RT_TICK_PER_SECOND * 2, "OK", 2 );
	if( ret != RT_EOK )
	{
		goto lbl_poweron_start;
	}

	ret = gsm_send_wait_func( "AT+CREG?\r\n", RT_TICK_PER_SECOND * 2, resp_CGREG, 30 );
	ret = gsm_wait_str( "OK", RT_TICK_PER_SECOND * 2 );
	if( ret != RT_EOK )
	{
		goto lbl_poweron_start;
	}

	rt_kprintf( "%08d gsm_power_on>end\r\n", rt_tick_get( ) );

	gsm_state = GSM_AT; /*��ǰ����AT״̬,���Բ��ţ�����*/
}

#endif

/*gsm����Ĵ����˳�*/
static void rt_thread_gsm_power_on( void* parameter )
{
	rt_err_t	ret;
	int			i;
	AT_CMD_RESP at_init[] =
	{
		{ "",				 RESP_TYPE_STR,			RT_NULL,	"OK",			RT_TICK_PER_SECOND * 5, 1  },
		{ "",				 RESP_TYPE_STR,			RT_NULL,	"OK",			RT_TICK_PER_SECOND * 5, 1  },
		{ "ATE0\r\n",		 RESP_TYPE_STR,			RT_NULL,	"OK",			RT_TICK_PER_SECOND * 5, 1  },
		{ "ATV1\r\n",		 RESP_TYPE_STR,			RT_NULL,	"OK",			RT_TICK_PER_SECOND * 5, 1  },
		{ "AT%TSIM\r\n",	 RESP_TYPE_STR_WITHOK,	RT_NULL,	"%TSIM 1",		RT_TICK_PER_SECOND * 2, 5  },

		{ "AT+CMGF=0\r\n",	 RESP_TYPE_STR,			RT_NULL,	"OK",			RT_TICK_PER_SECOND * 3, 3  },
		{ "AT+CNMI=1,2\r\n", RESP_TYPE_STR,			RT_NULL,	"OK",			RT_TICK_PER_SECOND * 3, 3  },

		{ "AT+CPIN?\r\n",	 RESP_TYPE_STR_WITHOK,	RT_NULL,	"+CPIN: READY", RT_TICK_PER_SECOND * 2, 30 },

		{ "AT+CIMI\r\n",	 RESP_TYPE_FUNC_WITHOK, resp_CIMI,	RT_NULL,		RT_TICK_PER_SECOND * 2, 10 },
		{ "AT+CLIP=1\r\n",	 RESP_TYPE_STR,			RT_NULL,	"OK",			RT_TICK_PER_SECOND * 2, 10 },
		{ "AT+CREG?\r\n",	 RESP_TYPE_FUNC_WITHOK, resp_CGREG, RT_NULL,		RT_TICK_PER_SECOND * 2, 30 },
	};

lbl_poweron_start:
	rt_kprintf( "%08d gsm_power_on>start\r\n", rt_tick_get( ) );

	GPIO_ResetBits( GSM_PWR_PORT, GSM_PWR_PIN );
	GPIO_ResetBits( GSM_TERMON_PORT, GSM_TERMON_PIN );
	rt_thread_delay( RT_TICK_PER_SECOND / 10 );
	GPIO_SetBits( GSM_TERMON_PORT, GSM_TERMON_PIN );
	GPIO_SetBits( GSM_PWR_PORT, GSM_PWR_PIN );

	for( i = 0; i < sizeof( at_init ) / sizeof( AT_CMD_RESP ); i++ )
	{
		if( gsm_send( at_init[i].atcmd, \
		              at_init[i].resp, \
		              at_init[i].compare_str, \
		              at_init[i].type, \
		              at_init[i].timeout, \
		              at_init[i].retry ) != RT_EOK )
		{
			/*todo ���������֪ͨ��ʾ*/
			rt_kprintf( "%08d stage=%d\r\n", rt_tick_get( ), i );
			goto lbl_poweron_start;
		}
	}

	rt_kprintf( "%08d gsm_power_on>end\r\n", rt_tick_get( ) );

	gsm_state = GSM_AT; /*��ǰ����AT״̬,���Բ��ţ�����*/
}

/*���Ƶ��������Ƕ���*/
static void rt_thread_gsm_gprs( void* parameter )
{
	char		buf[128];
	rt_err_t	err;

/*�ж�Ҫִ�������Ķ���*/

	if( dial_param.fconnect == 0 ) /*����*/
	{
		err = gsm_send( "AT%IPCLOSE=1\r\n", RT_NULL, "OK", RESP_TYPE_STR, RT_TICK_PER_SECOND * 35, 1 );
		if( err != RT_EOK )
		{
			goto lbl_gsm_gprs_end_err;
		}
		err = gsm_send( "AT%IPCLOSE=2\r\n", RT_NULL, "OK", RESP_TYPE_STR, RT_TICK_PER_SECOND * 35, 1 );

		if( err != RT_EOK )
		{
			goto lbl_gsm_gprs_end_err;
		}
		err = gsm_send( "AT%IPCLOSE=3\r\n", RT_NULL, "OK", RESP_TYPE_STR, RT_TICK_PER_SECOND * 35, 1 );

		if( err != RT_EOK )
		{
			goto lbl_gsm_gprs_end_err;
		}
		err = gsm_send( "AT%IPCLOSE=5\r\n", RT_NULL, "OK", RESP_TYPE_STR, RT_TICK_PER_SECOND * 35, 1 );
		err = gsm_send( "", RT_NULL, "%IPCLOSE:5", RESP_TYPE_STR, RT_TICK_PER_SECOND, 1 );
		if( err != RT_EOK )
		{
			goto lbl_gsm_gprs_end_err;
		}
		gsm_state = GSM_AT;
		goto lbl_gsm_gprs_end;
	}

	if( dial_param.fconnect == 1 ) /*�������*/
	{
		err = gsm_send( "AT+CGATT?\r\n", RT_NULL, "+CGATT: 1", RESP_TYPE_STR_WITHOK, RT_TICK_PER_SECOND * 2, 50 );
		if( err != RT_EOK )
		{
			goto lbl_gsm_gprs_end_err;
		}

		sprintf( buf, "AT+CGDCONT=1,\"IP\",\"%s\"\r\n", dial_param.apn );

		err = gsm_send( buf, RT_NULL, "OK", RESP_TYPE_STR, RT_TICK_PER_SECOND * 10, 2 );
		if( err != RT_EOK )
		{
			goto lbl_gsm_gprs_end_err;
		}

		if( ( strlen( dial_param.user ) == 0 ) && ( strlen( dial_param.user ) == 0 ) )
		{
			err = gsm_send( "AT%ETCPIP\r\n", RT_NULL, "OK", RESP_TYPE_STR, RT_TICK_PER_SECOND * 151, 1 );
		}else
		{
			sprintf( buf, "AT%ETCPIP=\"%s\",\"%s\"\r\n", dial_param.user, dial_param.psw );
			err = gsm_send( buf, RT_NULL, "OK", RESP_TYPE_STR, RT_TICK_PER_SECOND * 151, 1 );
		}
		if( err != RT_EOK )
		{
			goto lbl_gsm_gprs_end_err;
		}

		err = gsm_send( "AT%ETCPIP?\r\n", resp_ETCPIP, RT_NULL, RESP_TYPE_FUNC_WITHOK, RT_TICK_PER_SECOND * 10, 1 );
		if( err != RT_EOK )
		{
			goto lbl_gsm_gprs_end_err;
		}

		err = gsm_send( "AT%IOMODE=1,2,1\r\n", RT_NULL, "OK", RESP_TYPE_STR, RT_TICK_PER_SECOND * 10, 1 );
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

/*������·ά��,ֻά��һ��������·���ϲ㴦��*/
static void rt_thread_gsm_socket( void* parameter )
{
	char		buf[128];
	rt_err_t	err;
	int			i;
	AT_CMD_RESP at_cmd_resp;

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
		err = gsm_send( buf, resp_DNSR, RT_NULL, RESP_TYPE_FUNC_WITHOK, RT_TICK_PER_SECOND * 10, 1 );
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
		err = gsm_send( buf, RT_NULL, "CONNECT", RESP_TYPE_STR, RT_TICK_PER_SECOND * 10, 1 );
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
		rt_kprintf( "\r\n%08d gsm<%s\r\n", rt_tick_get( ), pInfo );
		fgsm_rawdata_out--;
	}

/*�ж�������*/
	psrc	= pInfo;
	pdst	= pInfo;
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
		psrc	= pdst;     /*ָ��""����*/
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

	if( strncmp( psrc, "%IPCLOSE:", 9 ) == 0 )
	{
		c = *( psrc + 9 ) - 0x30;
		cb_socket_state( c, SOCKET_CLOSE );
		return;
	}

	if( SMS_rx_pro( pInfo, size ) )
	{
		return;
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

	uint8_t		fcs = 0;

	char		buf_start[20];
	char		buf_end[4] = { '"', 0x0d, 0x0a, 0x0 };

	uint8_t		tbl[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

	sprintf( buf_start, "AT%%IPSENDX=%d,\"", linkno );
	m66_write( &dev_gsm, 0, buf_start, strlen( buf_start ) );
	rt_kprintf( "%s", buf_start );
	m66_write( &dev_gsm, 0, "7E", 2 );
	rt_kprintf( "%s", "7E" );
	while( len )
	{
		c	= *p++;
		fcs ^= c; /*����fcs*/
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
/*�ٷ���fcs*/
	if( fcs == 0x7E )
	{
		m66_write( &dev_gsm, 0, "7D02", 4 );
		rt_kprintf( "%s", "7D02" );
	}

	if( fcs == 0x7d )
	{
		m66_write( &dev_gsm, 0, "7D01", 4 );
		rt_kprintf( "%s", "7D01" );
	}
	USART_SendData( UART4, tbl[fcs >> 4] );
	while( USART_GetFlagStatus( UART4, USART_FLAG_TC ) == RESET )
	{
	}
	rt_kprintf( "%c", tbl[fcs >> 4] );
	USART_SendData( UART4, tbl[fcs & 0x0f] );
	while( USART_GetFlagStatus( UART4, USART_FLAG_TC ) == RESET )
	{
	}
	rt_kprintf( "%c", tbl[fcs & 0x0f] );
	m66_write( &dev_gsm, 0, "7E", 2 );
	rt_kprintf( "%s", "7E" );

	m66_write( &dev_gsm, 0, buf_end, 3 );

	rt_kprintf( "%s", buf_end );

	ret = gsm_send( "", RT_NULL, "OK", RESP_TYPE_STR, RT_TICK_PER_SECOND * 10, 1 );
	return ret;
}

/*����gsm״̬ 0 ��ѯ*/
T_GSM_STATE gsmstate( T_GSM_STATE cmd )
{
	if( cmd != GSM_STATE_GET )
	{
		gsm_state = cmd;
	}
	return gsm_state;
}

FINSH_FUNCTION_EXPORT( gsmstate, control gsm state );

/*����socket״̬ 0 ��ѯ*/
T_SOCKET_STATE socketstate( T_SOCKET_STATE cmd )
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
