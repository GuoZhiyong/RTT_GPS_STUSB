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
#include "jt808_sms.h"

#include "m66.h"
#include "hmi.h"

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

enum _sms_state {
	SMS_IDLE=0,
	SMS_WAIT_CMGR_DATA,
	SMS_WAIT_CMGR_OK,
	SMS_WAIT_CMGD_OK,
	SMS_WAIT_CMGS_GREATER,  /*�ȴ����͵�>*/
	SMS_WAIT_CMGS_OK,       /*�ȴ����͵�>*/
};

enum _sms_state sms_state = SMS_IDLE;

#define GSM_GPIO			GPIOC
#define GSM_TX_PIN			GPIO_Pin_10
#define GSM_TX_PIN_SOURCE	GPIO_PinSource10

#define GSM_RX_PIN			GPIO_Pin_11
#define GSM_RX_PIN_SOURCE	GPIO_PinSource11

//typedef void ( *URC_CB )( char *s, uint16_t len );

#define GSM_PWR_PORT	GPIOD
#define GSM_PWR_PIN		GPIO_Pin_13

#define GSM_TERMON_PORT GPIOD
#define GSM_TERMON_PIN	GPIO_Pin_12

#define GSM_RST_PORT	GPIOD
#define GSM_RST_PIN		GPIO_Pin_11

/*����һ��gsm�豸*/
static struct rt_device dev_gsm;

/*����һ��uart�豸ָ��,ͬgsmģ�����ӵĴ���  ָ��һ���Ѿ��򿪵Ĵ��� */
static struct rt_mailbox	mb_gsmrx;
#define MB_GSMRX_POOL_SIZE 32
static uint8_t				mb_gsmrx_pool[MB_GSMRX_POOL_SIZE];


/*
   AT�����ʹ�õ�mailbox
   �� VOICE TTS SMS TTSʹ��
 */
#define MB_TTS_POOL_SIZE 32
static struct rt_mailbox	mb_tts;
static uint8_t				mb_tts_pool[MB_TTS_POOL_SIZE];

#define MB_AT_TX_POOL_SIZE 32
static struct rt_mailbox	mb_at_tx;
static uint8_t				mb_at_tx_pool[MB_AT_TX_POOL_SIZE];

/* ��Ϣ������ƿ�*/
static struct rt_mailbox mb_sms;
#define MB_SMS_RX_POOL_SIZE 32
/* ��Ϣ�������õ��ķ�����Ϣ���ڴ��*/
static uint8_t mb_sms_pool[MB_SMS_RX_POOL_SIZE];

/*gsm �����ʹ�õ��ź���*/

static struct rt_semaphore sem_at;

#define GSM_RX_SIZE 2048
static uint8_t		gsm_rx[GSM_RX_SIZE];
static uint16_t		gsm_rx_wr = 0;

static T_GSM_STATE	gsm_state = GSM_IDLE;

/*���ڽ��ջ���������*/
#define UART4_RX_SIZE 512
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
static GSM_SOCKET	curr_socket;
//static GSM_SOCKET	* pcurr_socket;

/*����Ϣ���*/
static uint32_t sms_index = 0;   /*Ҫ�������ŵ�������*/


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
	uint8_t		*p	= (uint8_t*)buff + pos;

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

#define RESP_PROCESS
/**/
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
	//rt_kprintf( "\ncimi len=%d  %02x %02x", len, *p, *( p + 1 ) );
	static char mobile[6];
	if( len < 15 )
	{
		return RT_ERROR;
	}
	strip_numstring( p );
	mobile[0]	= ( ( *( p + 3 ) - '0' ) << 4 ) | ( *( p + 4 ) - '0' );
	mobile[1]	= ( ( *( p + 5 ) - '0' ) << 4 ) | ( *( p + 6 ) - '0' );
	mobile[2]	= ( ( *( p + 7 ) - '0' ) << 4 ) | ( *( p + 8 ) - '0' );
	mobile[3]	= ( ( *( p + 9 ) - '0' ) << 4 ) | ( *( p + 10 ) - '0' );
	mobile[4]	= ( ( *( p + 11 ) - '0' ) << 4 ) | ( *( p + 12 ) - '0' );
	mobile[5]	= ( ( *( p + 13 ) - '0' ) << 4 ) | ( *( p + 14 ) - '0' );
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

	memset( gsm_param.ip, 0, 15 );
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
	rt_kprintf( "\nip=%s", gsm_param.ip );
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

	rt_kprintf( "\ndns ip=%s", curr_socket.ip_addr );
	return RT_EOK;
}

/**/
rt_err_t resp_IPOPENX( char *p, uint16_t len )
{
	char *psrc = p;
	if( strstr( psrc, "CONNECT" ) != RT_NULL )
	{
		return RT_EOK;
	}
	return RT_ERROR;
}

/**/
rt_err_t resp_DEBUG( char *p, uint16_t len )
{
	rt_kprintf( "\nresp_debug>%s", p );
	return RT_ERROR;
}

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
		if( strlen( atcmd ) )                                       /*Ҫ�����ַ���*/
		{
			rt_kprintf( "\n%d gsm>%s", tick_start, atcmd );
			m66_write( &dev_gsm, 0, atcmd, strlen( atcmd ) );
		}
		if( type == RESP_TYPE_NONE )                                /*���ȣ�ֻ�ǹⷢ�ͣ��ϸ���retry=1,��ֻ����һ��*/
		{
			return RT_EOK;
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
					if( strstr( pinfo, compare_str ) != RT_NULL )   /*�ҵ��� todo:��������������ַ��� ��ERROR��δ���*/
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
	pmsg	= RT_NULL;  /*���¿�ʼ�ȴ�*/
	err		= rt_mb_recv( &mb_gsmrx, (rt_uint32_t*)&pmsg, tm );
	if( err == RT_EOK ) /*û�г�ʱ,�ж���Ϣ�Ƿ���ȷ*/
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

#define MODULE_PROCESS

/*gsm����Ĵ����˳�*/
static void rt_thread_gsm_power_on( void* parameter )
{
	int			i;
	AT_CMD_RESP at_init[] =
	{
		{ "",				 RESP_TYPE_STR,			RT_NULL,	"OK",			RT_TICK_PER_SECOND * 5, 1  },
		{ "",				 RESP_TYPE_STR,			RT_NULL,	"OK",			RT_TICK_PER_SECOND * 5, 1  },
		{ "ATE0\r\n",		 RESP_TYPE_STR,			RT_NULL,	"OK",			RT_TICK_PER_SECOND * 5, 1  },
		{ "ATV1\r\n",		 RESP_TYPE_STR,			RT_NULL,	"OK",			RT_TICK_PER_SECOND * 5, 1  },
		{ "AT%TSIM\r\n",	 RESP_TYPE_STR_WITHOK,	RT_NULL,	"%TSIM 1",		RT_TICK_PER_SECOND * 2, 5  },

		{ "AT+CMGF=0\r\n",	 RESP_TYPE_STR,			RT_NULL,	"OK",			RT_TICK_PER_SECOND * 3, 3  },
		{ "AT+CNMI=3,1\r\n", RESP_TYPE_STR,			RT_NULL,	"OK",			RT_TICK_PER_SECOND * 3, 3  },

		{ "AT+CPIN?\r\n",	 RESP_TYPE_STR_WITHOK,	RT_NULL,	"+CPIN: READY", RT_TICK_PER_SECOND * 2, 30 },

		{ "AT+CIMI\r\n",	 RESP_TYPE_FUNC_WITHOK, resp_CIMI,	RT_NULL,		RT_TICK_PER_SECOND * 2, 10 },
		{ "AT+CLIP=1\r\n",	 RESP_TYPE_STR,			RT_NULL,	"OK",			RT_TICK_PER_SECOND * 2, 10 },
		{ "AT+CREG?\r\n",	 RESP_TYPE_FUNC_WITHOK, resp_CGREG, RT_NULL,		RT_TICK_PER_SECOND * 2, 30 },
	};

lbl_poweron_start:
	rt_kprintf( "\n%08d gsm_power_on>start", rt_tick_get( ) );

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
			rt_kprintf( "\n%08d stage=%d", rt_tick_get( ), i );
			goto lbl_poweron_start;
		}
	}

	rt_kprintf( "\n%08d gsm_power_on>end", rt_tick_get( ) );

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
	gsm_state = GSM_ERR;
lbl_gsm_gprs_end:
	rt_kprintf( "\n%08d gsm_gprs>end state=%d", rt_tick_get( ), gsm_state );
}

/*������·ά��,ֻά��һ��������·���ϲ㴦��*/
static void rt_thread_gsm_socket( void* parameter )
{
	char		buf[128];
	rt_err_t	err;
	AT_CMD_RESP at_cmd_resp;

	/*�Ҷ�����*/
	if( curr_socket.state == CONNECT_CLOSING )
	{
		sprintf( buf, "AT%%IPCLOSE=%d\r\n", curr_socket.linkno );
		err = gsm_send( buf, RT_NULL, "OK", RESP_TYPE_STR, RT_TICK_PER_SECOND * 10, 1 );
		if( err != RT_EOK )
		{
			curr_socket.state = CONNECT_IDLE;
		}
		goto lbl_gsm_socket_end;
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

	if( curr_socket.state == SOCKET_DNS ) /*Ҫȥ��SOCKET_DNS���״̬��*/
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
	rt_kprintf( "\n%d gsm_socket>end socket.state=%d", rt_tick_get( ), curr_socket.state );
}

/*������Ϣ�������*/
rt_err_t dbgmsg( uint32_t i )
{
	if( i == 0 )
	{
		rt_kprintf( "\ndebmsg=%d", fgsm_rawdata_out );
	} else
	{
		fgsm_rawdata_out = i;
	}
	return RT_EOK;
}

FINSH_FUNCTION_EXPORT( dbgmsg, dbgmsg count );

#define SOCKET_PROCESS


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
		}else if( c == 0x7D )
		{
			m66_write( &dev_gsm, 0, "7D01", 4 );
			rt_kprintf( "%s", "7D01" );
		}else
		{
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
		len--;
	}
/*�ٷ���fcs*/
	if( fcs == 0x7E )
	{
		m66_write( &dev_gsm, 0, "7D02", 4 );
		rt_kprintf( "%s", "7D02" );
	}else if( fcs == 0x7D )
	{
		m66_write( &dev_gsm, 0, "7D01", 4 );
		rt_kprintf( "%s", "7D01" );
	}else
	{
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
	}
/*�ٷ���7Eβ*/
	m66_write( &dev_gsm, 0, "7E", 2 );
	rt_kprintf( "%s", "7E" );

	m66_write( &dev_gsm, 0, buf_end, 3 );
	rt_kprintf( "%s", buf_end );

	ret = gsm_send( "", RT_NULL, "%IPSENDX:", RESP_TYPE_STR, RT_TICK_PER_SECOND * 10, 1 );
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
	if( cmd != SOCKET_STATE_GET )
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
void ctl_socket_open( uint8_t linkno, char type, char* remoteip, uint16_t remoteport )
{
	curr_socket.linkno	= linkno;
	curr_socket.type	= type;
	strcpy( &( curr_socket.ipstr[0] ), remoteip );
	curr_socket.port	= remoteport;
	curr_socket.state	= SOCKET_START; /**/
	gsm_state			= GSM_SOCKET_PROC;
}



/**/
void ctl_socket_close( uint8_t linkno )
{
	curr_socket.linkno	= linkno;
	curr_socket.state	= SOCKET_CLOSE;
	gsm_state			= GSM_SOCKET_PROC;
}

#define TTS_PROCESS


/*
   tts���������Ĵ���

   ��ͨ��
   %TTS: 0 �ж�tts״̬(���ɲ�����ÿ�ζ������)
   ����AT%TTS? ��ѯ״̬
 */
void tts_proc( void )
{
	rt_size_t	len;
	uint8_t		*pinfo, *p;
	uint8_t		c;
	T_GSM_STATE oldstate;

	char		buf[20];
	char		tbl[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
/*gsm�ڴ�����������*/
	if( gsm_state != GSM_TCPIP )
	{
		if( gsm_state != GSM_AT )
		{
			return;
		}
	}

/*�Ƿ�����ϢҪ����*/
	if( rt_mb_recv( &mb_tts, (rt_uint32_t*)&pinfo, 0 ) != RT_EOK )
	{
		return;
	}

	oldstate	= gsm_state;
	gsm_state	= GSM_AT_SEND;
	GPIO_ResetBits( GPIOD, GPIO_Pin_9 ); /*������*/
	sprintf( buf, "AT%%TTS=2,3,5,\"" );
	rt_device_write( &dev_gsm, 0, buf, strlen( buf ) );
	rt_kprintf( "%s", buf );
	len = ( *pinfo << 8 ) | ( *( pinfo + 1 ) );
	p	= pinfo + 2;
	while( len-- )
	{
		c		= *p++;
		buf[0]	= tbl[c >> 4];
		buf[1]	= tbl[c & 0x0f];
		rt_device_write( &dev_gsm, 0, buf, 2 );
		rt_kprintf( "%c%c", buf[0], buf[1] );
	}
	buf[0]	= '"';
	buf[1]	= 0x0d;
	buf[2]	= 0x0a;
	buf[3]	= 0;
	rt_device_write( &dev_gsm, 0, buf, 3 );
	rt_kprintf( "%s", buf );
/*���жϣ���gsmrx_cb�д���*/
	rt_free( pinfo );
	gsm_send( "", RT_NULL, "%TTS: 0", RESP_TYPE_STR, RT_TICK_PER_SECOND * 35, 1 );
	GPIO_SetBits( GPIOD, GPIO_Pin_9 ); /*�ع���*/
	gsm_state = oldstate;
}

/*
   �յ�tts��Ϣ������
   ����0:OK
    1:����RAM����
 */
rt_size_t tts_write( char* info, uint16_t len )
{
	uint8_t *pmsg;
	/*ֱ�ӷ��͵�Mailbox��,�ڲ�����*/
	pmsg = rt_malloc( len + 2 );
	if( pmsg != RT_NULL )
	{
		*pmsg			= len >> 8;
		*( pmsg + 1 )	= len & 0xff;
		memcpy( pmsg + 2, info, len );
		rt_mb_send( &mb_tts, (rt_uint32_t)pmsg );
		return 0;
	}
	return 1;
}

/**/
rt_err_t tts( char *s )
{
	tts_write( s, strlen( s ) );
	return RT_EOK;
}

FINSH_FUNCTION_EXPORT( tts, tts send );

#define AT_CMD_PROCESS


/*
   ͨ����AT�������ȥ����ϵͳ����������������
   ���ڶ��Ų�����Ҫȷ�ϣ������Ƕ����Ϣ��������ʱ
   at������յ�OK��ʱ�˳�
 */
void at_cmd_proc( void )
{
	rt_err_t	ret;
	uint8_t		*pinfo, *p;
	T_GSM_STATE oldstate;
	/*����Ƿ����AT���������״̬*/
	if( gsm_state != GSM_TCPIP )
	{
		if( gsm_state != GSM_AT )
		{
			return;
		}
	}

/*�Ƿ�����ϢҪ����*/
	if( rt_mb_recv( &mb_at_tx, (rt_uint32_t*)&pinfo, 0 ) != RT_EOK )
	{
		return;
	}
	oldstate	= gsm_state;
	gsm_state	= GSM_AT_SEND;
	p			= pinfo + 2;
	ret			= gsm_send( (char*)p, RT_NULL, "OK", RESP_TYPE_STR, RT_TICK_PER_SECOND * 10, 1 );
	//ret = gsm_send( (char*)p, RT_NULL, "OK", RESP_TYPE_NONE, RT_TICK_PER_SECOND , 1 );
	rt_kprintf( "\nat_tx=%d", ret );
	rt_free( pinfo );
	gsm_state = oldstate;
}

/*
   ����AT����
   ��α�֤������,������ִ�У�����ȴ���ʱ��

 */
rt_size_t at( char *sinfo )
{
	char		*pmsg;
	uint16_t	count;
	count = strlen( sinfo );

	/*ֱ�ӷ��͵�Mailbox��,�ڲ�����*/
	pmsg = rt_malloc( count + 3 );
	if( pmsg != RT_NULL )
	{
		count					= sprintf( pmsg + 2, "%s\r\n", sinfo );
		pmsg[0]					= count >> 8;
		pmsg[1]					= count & 0xff;
		*( pmsg + count + 2 )	= 0;
		rt_mb_send( &mb_at_tx, (rt_uint32_t)pmsg );
		return 0;
	}
	return 1;
}

FINSH_FUNCTION_EXPORT( at, write gsm );

#define SMS_PROCESS


/*
   0:û�ж��Ŵ������� ����ֵ �к����Ĵ���

 */
uint8_t sms_rx( char *pinfo, uint16_t size )
{
	int			st, index, count;
	char		buf[40];                                        /*160��OCT��Ҫ320byte*/
	uint32_t	tick = rt_tick_get( );

/*�����������ķ�������Ϣ*/
	if( strncmp( pinfo, "+CMTI: \"SM\",", 12 ) == 0 )           /*SM������ʱ�򣬻�浽ME�У���ζ�*/
	{
		if( sscanf( pinfo + 12, "%d", &index ) )
		{
			sms_index = index;
			rt_mb_send( &mb_sms, 0x1 );                         /*֪ͨҪ������*/
			return 1;
		}
	}else if( strncmp( pinfo, "+CMGR: ", 7 ) == 0 )             /*+CMGR: 0,156,�п����Ǵ��ڶ��Ķ���*/
	{
		if( sscanf( pinfo + 7, "%d,%d", &st, &count ) == 2 )    /*�õ���Ϣ�ĳ���*/
		{
			sms_state = SMS_WAIT_CMGR_DATA;
			return 1;
		}
	}else if( strncmp( pinfo, "+CMT:", 5 ) == 0 )               /*û�����������Ķ���*/
	{
	}

	switch( sms_state )
	{
		case SMS_WAIT_CMGR_DATA:                                /*�յ���������*/
			jt808_sms_rx( pinfo, size );                        /*��������*/
			sms_state = SMS_WAIT_CMGR_OK;
			break;
		case SMS_WAIT_CMGR_OK:
			if( strncmp( pinfo, "OK", 2 ) == 0 )                /*��������Ϣ,ɾ��*/
			{
				rt_mb_send( &mb_sms, 2 );                       /*֪ͨҪɾ��*/
				sms_state = SMS_IDLE;
			}
			break;
		case SMS_WAIT_CMGD_OK:
			if( strncmp( pinfo, "OK", 2 ) == 0 )                /*ɾ�����ųɹ�*/
			{
				sms_state	= SMS_IDLE;
				sms_index	= 0;
			}
			break;
	}


/*
   if( tick - sms_tick > RT_TICK_PER_SECOND * 10 )
   {
   rt_kprintf( "\n��,��ʱ��" );
   sms_state = SMS_IDLE;
   }
 */
	return sms_state;
}

/*���Ͷ���Ϣ,�������շ�,SMSC,7bit��������Ϣ*/
void sms_tx( char* info, uint8_t tp_len )
{
	uint8_t len = strlen( info );
	char	*p;
	p = rt_malloc( len + 2 );
	if( p != RT_NULL )
	{
		p[0] = tp_len; /*tp_len AT+CMGSʱ��Ҫ*/
		strcpy( p + 1, info );
		rt_mb_send( &mb_sms, (uint32_t)p );
	}
}

/*
   �������������Ҫ�������ж����Ϣ����ʱ�����������ʱ������
   ʹ��mailbox�����ŶӴ����ȡ��ɾ������
 */
void sms_proc( void )
{
	uint32_t	i;
	char		*sms_send;
	char		buf[40];
	rt_err_t	ret;
	uint16_t	len;

	T_GSM_STATE oldstate;

	if( sms_state != SMS_IDLE )
	{
		return;
	}
	/*����Ƿ����AT���������״̬*/
	if( gsm_state != GSM_TCPIP )
	{
		if( gsm_state != GSM_AT )
		{
			return;
		}
	}
	if( rt_mb_recv( &mb_sms, &i, 0 ) != RT_EOK ) /*�յ����Ų���*/
	{
		return;
	}
	oldstate	= gsm_state;
	gsm_state	= GSM_AT_SEND;
	switch( i )
	{
		case 0x1:   /*����Ϣ0x1*/
			sprintf( buf, "AT+CMGR=%d\r\n", sms_index );
			ret = gsm_send( buf, RT_NULL, RT_NULL, RESP_TYPE_NONE, RT_TICK_PER_SECOND, 1 );
			break;
		case 0x2:   /*ɾ����Ϣ0x2*/
			if( sms_index )
			{
				sprintf( buf, "AT+CMGD=%d\r\n", sms_index );
				ret			= gsm_send( buf, RT_NULL, RT_NULL, RESP_TYPE_NONE, RT_TICK_PER_SECOND, 1 );
				sms_state	= SMS_WAIT_CMGD_OK;
			}
			break;
		default:                                /*������Ϣ*/
			sms_send	= (char*)i;
			len			= sms_send[0];          /*tp_len*/
			sprintf( buf, "AT+CMGS=%d\r", len );
			ret = gsm_send( buf, RT_NULL, ">", RESP_TYPE_STR, RT_TICK_PER_SECOND * 5, 1 );
			if( ret == RT_EOK )
			{
				len = strlen( sms_send + 1 );   /*��ʵ��Ϣ����,����CTRL_Z*/
				m66_write( &dev_gsm, 1, sms_send, len );
				//rt_kprintf("\nSMS>SEND(len=%d):%s",len,sms_send+1);
			}
			//rt_thread_delay(RT_TICK_PER_SECOND/4);
			buf[0]	= 0x1A;
			buf[1]	= 0x0;
			ret		= gsm_send( buf, RT_NULL, "+CMGS:", RESP_TYPE_STR_WITHOK, RT_TICK_PER_SECOND * 20, 1 );
			rt_free( (void*)i );
			break;
	}
	gsm_state = oldstate;
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
	int		i, len = size;
	uint8_t tbl[24] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0, 0, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf };
	char	c, *pmsg;
	char	*psrc = RT_NULL, *pdst = RT_NULL;
	int32_t infolen, linkno;

/*��������Ϣ��ֱ��֪ͨ�ϲ����*/
	if( fgsm_rawdata_out )
	{
		rt_kprintf( "\n%d gsm<%s", rt_tick_get( ), pInfo );
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
/*	00002381 gsm<%IPCLOSE:1*/

	if( strncmp( psrc, "%IPCLOSE:", 9 ) == 0 )
	{
		c = *( psrc + 9 ) - 0x30;
		cb_socket_close( c );
		return;
	}

	if( strncmp( psrc, "%TSIM 0", 7 ) == 0 ) /*û��SIM��*/
	{
		//pop_msg("SIM��������",RT_TICK_PER_SECOND*1000);
		return;
	}

	if( sms_rx( pInfo, size ) ) /*һ�������Ĵ������?*/
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

#define RT_THREAD_ENTRY_GSM

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
		switch( gsm_state )
		{
			case GSM_POWERON:
				rt_thread_gsm_power_on( RT_NULL );
				break;
			case GSM_POWEROFF:
				GPIO_ResetBits( GSM_PWR_PORT, GSM_PWR_PIN );
				rt_thread_delay( RT_TICK_PER_SECOND * 5 ); /*��ʱ5��������*/
				gsm_state = GSM_IDLE;
				break;
			case  GSM_GPRS:
				rt_thread_gsm_gprs( RT_NULL );
				break;
			case GSM_SOCKET_PROC:
				rt_thread_gsm_socket( RT_NULL );
				break;
			case GSM_IDLE:
				break;
			case GSM_TCPIP:
				break;
			case GSM_AT:
				break;
			case GSM_AT_SEND:
				break;				
			case GSM_ERR:
				break;
		}
		sms_proc( );
		tts_proc( );    /*tts����*/
		at_cmd_proc( ); /*at�����*/
		rt_thread_delay( RT_TICK_PER_SECOND / 20 );
	}
}

#define RT_THREAD_ENTRY_GSM_RX

ALIGN( RT_ALIGN_SIZE )
static char thread_gsm_rx_stack[1024];
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
					gsmrx_cb( (char*)gsm_rx, gsm_rx_wr );           /*������Ϣ�Ĵ�����*/
				}
				gsm_rx_wr = 0;
			}
		}
		if( rt_tick_get( ) - last_tick > RT_TICK_PER_SECOND / 5 )   //�ȴ�200ms,ʵ���Ͼ��Ǳ䳤����ʱ,���100ms������һ�����ݰ�
		{
			if( gsm_rx_wr )
			{
				gsmrx_cb( (char*)gsm_rx, gsm_rx_wr );               /*������Ϣ�Ĵ�����*/
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
	rt_sem_init( &sem_at, "sem_at", 1, RT_IPC_FLAG_FIFO );
	rt_mb_init( &mb_gsmrx, "mb_gsmrx", &mb_gsmrx_pool, MB_GSMRX_POOL_SIZE / 4, RT_IPC_FLAG_FIFO );
	rt_mb_init( &mb_tts, "mb_tts", &mb_tts_pool, MB_TTS_POOL_SIZE / 4, RT_IPC_FLAG_FIFO );
	rt_mb_init( &mb_at_tx, "mb_at_tx", &mb_at_tx_pool, MB_AT_TX_POOL_SIZE / 4, RT_IPC_FLAG_FIFO );
	rt_mb_init( &mb_sms, "mb_sms_rx", &mb_sms_pool, MB_SMS_RX_POOL_SIZE / 4, RT_IPC_FLAG_FIFO );

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

/************************************** The End Of File **************************************/
