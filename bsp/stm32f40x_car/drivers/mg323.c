/************************************************************
 * Copyright (C), 2008-2012,
 * FileName:		mg323
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

#ifdef MG323

#include <stdio.h>
#include <rtthread.h>
#include "stm32f4xx.h"

#include <finsh.h>

typedef void (*URC_CB)(char *s,uint16_t len);

#define GSM_PWR_PORT		GPIOB		
#define GSM_PWR_PIN			GPIO_Pin_15		//PB.15

#define GSM_TERMON_PORT		GPIOB
#define GSM_TERMON_PIN		GPIO_Pin_8		//PB.8

#define GSM_RST_PORT		GPIOB
#define GSM_RST_PIN			GPIO_Pin_9		//PB.9

#define TIMEOUT_SYSSTART_S	5

static struct
{
	rt_device_t dev_uart;
	ONDATA		ondata;
	ONCMD		oncmd;
	ONSTATUS	onstatus;
}priv;



static struct rt_device		dev_gsm;
static struct rt_timer		tmr_gsm;

static struct rt_semaphore	gsm_sem;

#define GSM_RX_SIZE 2048
static uint8_t	gsm_rx[GSM_RX_SIZE];
static uint16_t gsm_rx_wr = 0;

static T_GSM_STATE	gsmstate=GSM_IDLE;

static uint32_t lastticks=0;
static uint32_t	action_timeout=0;

static void urc_cb_default(char *s,uint16_t len)
{
	rt_kprintf("\rrx>%s",s);

}


static void urc_cb_sysstart(char *s,uint16_t len)
{


}


static void urc_cb_shutdown(char *s,uint16_t len)
{


}

static void urc_cb_ciev(char *s,uint16_t len)
{


}

static void urc_cb_ring(char *s,uint16_t len)
{


}

static void urc_cb_cring(char *s,uint16_t len)
{


}

/*
urc: unsolicited result code
*/
struct 
{
	char *code,
	URC_CB pfunc;
}urc[]=
{
	{"^SYSSTART",urc_cb_sysstart},
	{"^SHUTDOWN",urc_cb_shutdown},	
	{"+CIEV:",urc_cb_ciev},
	{"RING",urc_cb_ring},
	{"+CRING:",urc_cb_cring},
	{"+CREG:",urc_cb_default},
	{"^SIS:",urc_cb_default},
	{"+CGEV:",urc_cb_default},
	{"+CGREG:",urc_cb_default},
	{"+CMT:",urc_cb_default},
	{"+CBM:",urc_cb_default},
	{"+CDS:",urc_cb_default},
	{"+CALA:",urc_cb_default},	
	{"CME ERROR",urc_cb_default},
	{"CMS ERROR",urc_cb_default},
	{"",NULL}
};




/***********************************************************
* Function:		gsmrx_cb
* Description:	gsm�յ���Ϣ�Ĵ���
* Input:			char *s 	��Ϣ
				uint16_t len ����
* Output:
* Return:
* Others:
***********************************************************/
static void gsmrx_cb( char *pInfo, uint16_t len )
{
	int i;
	char match=0;
	uint8_t tbl[24]={0,1,2,3,4,5,6,7,8,9,0,0,0,0,0,0,0,0xa,0xb,0xc,0xd,0xe,0xf};
	uint32_t linknum,len;
	int16_t i,count;
	char c,*pmsg;
	uint8_t *p;
/*��������Ϣ*/	
	if(strncmp(pInfo,"%IPDATA",7)==0)
	{
		i=sscanf(pInfo,"%%IPDATA:%d,%d,",&linknum,&len);
		if(i!=2) return;		//û����ȷ������������
		pmsg=pInfo+10;
		while(*pmsg!='"'){
			pmsg++;
		}	
		pmsg++;
		/*ֱ����pInfoҲ����GsmRx�ϲ���*/
		p=pInfo;
		count=0;
		while(*pmsg!='"'){
			c=tbl[*pmsg-'0']<<4;
			pmsg++;
			c|=tbl[*pmsg-'0'];
			pmsg++;
			*p++=c;
			count++;
			if(count>=len) break;
		}	
		*p=0;
		p=pInfo;//ָ����ʼ��
	}

	if(strncmp(s,"%IPCLOSE",7)==0)
	{


	}

	
//�жϷ���������-�����ϱ�����
	for(i=0;;i++)
	{
		if(urc[i].pfunc==NULL) break;
		if(strncmp(pInfo,urc[i].code,strlen(urc[i].code)==0)
		{
			(urc[i].pfunc)(pInfo,len);
			match=1;		//�Ѵ���
			break;	
		}	
	}
	if(match) return;
	
//AT����Ľ�����������������������APP������

	

}

/***********************************************************
* Function:       timer_gsm_cb
* Description:    ��ʱ������,�ж�ִ�еĳ�ʱ
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static void timer_gsm_cb( void* parameter )
{
	case 	
	if(rt_tick_get()-lastticks>action_timeout)
	{
		switch(gsmstate)
		{
			case GSM_POWERON:	/*�ϵ糬ʱ*/
				gsmstate=GSM_IDLE;
				break;
			case GSM_POWEROFF:	/*�ϵ糬ʱ*/
				gsmstate=GSM_IDLE;
				break;
				

		}
	}



}



ALIGN( RT_ALIGN_SIZE )
static char thread_gsm_stack[1024];
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
	rt_err_t		res;
	unsigned char	ch, next;
	/*���ճ�ʱ�ж�*/
	res = rt_sem_take( &gsm_sem, RT_TICK_PER_SECOND / 5 );  //�ȴ�200ms
	if( res == -RT_ETIMEOUT )                               //��ʱ�˳���û�����ݻ�����������
	{
		if( gsm_rx_wr )
		{
			gsmrx_cb( gsm_rx, gsm_rx_wr );
			gsm_rx_wr=0;
		}
	}
	else //�յ�����,������1���ֽڴ���һ��,����û�и����յ���<CRLF>�������ǵȵ���ʱͳһ����
	{
		while( rt_device_read( (rt_device_t)( dev_gsm->userdata ), 0, &ch, 1 ) == 1 )
		{
			gsm_rx[gsm_rx_wr++] = ch;
			if( gsm_rx_wr == GSM_RX_SIZE )
			{
				gsm_rx_wr = 0;
			}
			gsm_rx[gsm_rx_wr] = 0;
		}
	}
}

/***********************************************************
* Function:
* Description:	���յ�serial����������ָʾ
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static rt_err_t mg323_rx_ind( rt_device_t dev, rt_size_t size )
{
	rt_sem_release( &gsm_sem );
	return RT_EOK;
}

/***********************************************************
* Function:
* Description: ���ÿص�ܽţ����Ҳ���gsm��Ӧ�Ĵ����豸
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static rt_err_t mg323_init( rt_device_t dev )
{
	GPIO_InitTypeDef GPIO_InitStructure;


	RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOB , ENABLE );

	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

	GPIO_InitStructure.GPIO_Pin = GSM_PWR_PIN;
	GPIO_Init( GSM_PWR_PORT, &GPIO_InitStructure );
	GPIO_ResetBits( GSM_PWR_PORT, GSM_PWR_PIN );	

	GPIO_InitStructure.GPIO_Pin = GSM_TERMON_PIN;
	GPIO_Init( GSM_TERMON_PORT, &GPIO_InitStructure );
	GPIO_ResetBits( GSM_TERMON_PORT, GSM_TERMON_PIN );	

	
	rt_device_t dev_uart = RT_NULL;
	dev_uart = rt_device_find( GSM_UART_NAME);
	if( dev != RT_NULL && rt_device_open( dev, RT_DEVICE_OFLAG_RDWR ) == RT_EOK )
	{
		dev->userdata = dev_uart; //ָ���������Ĵ���
		rt_device_set_rx_indicate( dev, mg323_rx_ind );
	}else
	{
		rt_kprintf( "GSM: can not find device:\n" );
	}
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
static rt_err_t mg323_open( rt_device_t dev, rt_uint16_t oflag )
{
	if(gsmstate==GSM_IDLE)
	{
		gsmstate=GSM_POWERON;	//��λ�ϵ������
		GPIO_SetBits(GSM_PWR_PORT,GSM_PWR_PIN);
		rt_thread_delay(RT_TICK_PER_SECOND/2);		//500ms
		GPIO_SetBits(GSM_TERMON_PORT,GSM_TERMON_PIN);
		rt_thread_delay(RT_TICK_PER_SECOND/10);		//100ms
		GPIO_ResetBits(GSM_TERMON_PORT,GSM_TERMON_PIN);

		
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
static rt_err_t mg323_close( rt_device_t dev )
{
	gsmstate=GSM_POWEROFF;	//��λ�ϵ������
	return RT_EOK;
}


/***********************************************************
* Function:mg323_read
* Description:����ģʽ�¶�ȡ����
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static rt_size_t mg323_read( rt_device_t dev, rt_off_t pos, void* buff, rt_size_t count )
{
	return RT_EOK;
}

/***********************************************************
* Function:		mg323_write
* Description:	����ģʽ�·������ݣ�Ҫ�����ݽ��з�װ
* Input:		const void* buff	Ҫ���͵�ԭʼ����
    rt_size_t count		Ҫ�������ݵĳ���
* Output:
* Return:
* Others:
***********************************************************/

static rt_size_t mg323_write( rt_device_t dev, rt_off_t pos, const void* buff, rt_size_t count )
{
	rt_size_t ret = RT_EOK;
	return ret;
}



/***********************************************************
* Function:		mg323_control
* Description:	����ģ��
* Input:		rt_uint8_t cmd	��������
    void *arg       ����,����cmd�Ĳ�ͬ�����ݵ����ݸ�ʽ��ͬ
* Output:
* Return:
* Others:
***********************************************************/
static rt_err_t mg323_control( rt_device_t dev, rt_uint8_t cmd, void *arg )
{
	switch( cmd )
	{
		case CMD_STATUS:
			break;
		case CMD_AT_CMD: //����at����,���Ҫ����
			break;
		case CMD_PPP:
			break;
		case CMD_SOCKET:
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
void gsm_init( void )
{
	rt_thread_t tid;

	rt_sem_init( &gsm_sem, "gsm_sem", 0, 0 );

	rt_thread_init( &thread_gsm,
	                "gsm",
	                rt_thread_entry_gsm,
	                RT_NULL,
	                &thread_gsm_stack[0],
	                sizeof( thread_gsm_stack ), 7, 5 );
	rt_thread_startup( &thread_gsm );


	rt_timer_init( &tmr_gsm, \
	               "tmr_gsm", \
	               timer_gsm_cb, NULL, \
	               50, \
	               RT_TIMER_FLAG_PERIODIC );

	dev_gsm.type		= RT_Device_Class_Char;
	dev_gsm.init		= mg323_init;
	dev_gsm.open		= mg323_open;
	dev_gsm.close		= mg323_close;
	dev_gsm.read		= mg323_read;
	dev_gsm.write		= mg323_write;
	dev_gsm.control		= mg323_control;
	dev_gsm.user_data	= RT_NULL;

	rt_device_register( &dev_gsm, "gsm", RT_DEVICE_FLAG_RDWR );
	rt_device_init( &dev_gsm );
	rt_timer_start( &tmr_gsm );
}

#endif

/************************************** The End Of File **************************************/
