/************************************************************
 * Copyright (C), 2008-2012,
 * FileName:		mg323
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
#include "stm32f4xx.h"

#include <finsh.h>

#include <gsm.h>

#ifdef MG323

typedef void ( *URC_CB )( char *s, uint16_t len );

typedef rt_err_t (*RESP_FUNC)(char *s);


#define GSM_PWR_PORT	GPIOB
#define GSM_PWR_PIN		GPIO_Pin_15 //PB.15

#define GSM_TERMON_PORT GPIOB
#define GSM_TERMON_PIN	GPIO_Pin_8  //PB.8

#define GSM_RST_PORT	GPIOB
#define GSM_RST_PIN		GPIO_Pin_9  //PB.9

#define TIMEOUT_SYSSTART_S 5

/*声明一个gsm设备*/
static struct rt_device dev_gsm;


/*声明一个uart设备指针,同gsm模块连接的串口
   指向一个已经打开的串口
 */
static rt_device_t			dev_uart;

static struct rt_timer		tmr_gsm;
static struct rt_semaphore	sem_uart;

static struct rt_semaphore	sem_gsmrx;

#define GSM_RX_SIZE 2048
static uint8_t		gsm_rx[GSM_RX_SIZE];
static uint16_t		gsm_rx_wr = 0;

static T_GSM_STATE	gsmstate = GSM_IDLE;

static uint32_t		lastticks		= 0;
static uint32_t		action_timeout	= 0;

static rt_tick_t	last_state_ticks; /*上一次状态的tick值*/

static rt_thread_t	tid_gsm_power = RT_NULL;


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
int ondata_default( uint8_t *pInfo, uint16_t len )
{
	rt_kprintf( "%ld(%d)ondata>", rt_tick_get( ), len );
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
int oncmd_default( uint8_t *pInfo, uint16_t len )
{
	rt_kprintf( "%ld(%d)oncmd>", rt_tick_get( ), len );
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
int onstatus_default( uint32_t *urc )
{
	rt_kprintf( "%ld onstatus>", rt_tick_get( ) );
	return RT_EOK;
}

T_GSM_OPS gsm_ops_default =
{
	ondata_default,
	oncmd_default,
	onstatus_default,
};


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
static void urc_cb_sysstart( char *s, uint16_t len )
{
	if( gsmstate == GSM_POWERON ) /*上电过程中收到该命令*/
	{
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
static void urc_cb_shutdown( char *s, uint16_t len )
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
	{ "^SYSSTART", urc_cb_sysstart },
	{ "^SHUTDOWN", urc_cb_shutdown },
	{ "+CIEV:",	   urc_cb_ciev	   },
	{ "RING",	   urc_cb_ring	   },
	{ "+CRING:",   urc_cb_cring	   },
	{ "+CREG:",	   urc_cb_default  },
	{ "^SIS:",	   urc_cb_default  },
	{ "+CGEV:",	   urc_cb_default  },
	{ "+CGREG:",   urc_cb_default  },
	{ "+CMT:",	   urc_cb_default  },
	{ "+CBM:",	   urc_cb_default  },
	{ "+CDS:",	   urc_cb_default  },
	{ "+CALA:",	   urc_cb_default  },
	{ "CME ERROR", urc_cb_default  },
	{ "CMS ERROR", urc_cb_default  },
	{ "",		   NULL			   }
};


/***********************************************************
* Function:		gsmrx_cb
* Description:	gsm收到信息的处理
* Input:			char *s     信息
    uint16_t len 长度
* Output:
* Return:
* Others:
***********************************************************/
static void gsmrx_cb( char *pInfo, uint16_t len )
{
	int			i, count;
	char		match	= 0;
	uint8_t		tbl[24] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0, 0, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf };
	uint32_t	linknum, infolen;
	char		c, *pmsg;
	uint8_t		*p;
/*网络侧的信息，直接通知上层软件*/
	if( strncmp( pInfo, "%IPDATA", 7 ) == 0 )
	{
		i = sscanf( pInfo, "%%IPDATA:%d,%d,", &linknum, &infolen );
		if( i != 2 )
		{
			return; //没有正确解析三个参数
		}
		pmsg = pInfo + 10;
		while( *pmsg != '"' )
		{
			pmsg++;
		}
		pmsg++;
		/*直接在pInfo也就是GsmRx上操作*/
		p		= pInfo;
		count	= 0;
		while( *pmsg != '"' )
		{
			c = tbl[*pmsg - '0'] << 4;
			pmsg++;
			c |= tbl[*pmsg - '0'];
			pmsg++;
			*p++ = c;
			count++;
			if( count >= infolen )
			{
				break;
			}
		}
		*p	= 0;
		p	= pInfo; //指到开始处
		( (T_GSM_OPS*)( dev_gsm.user_data ) )->ondata( p, count );
	}

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



static uint16_t stripstring(char *from,char *to)
{
	char *psrc,*pdst;
	uint16_t len=0;
	psrc=from;
	pdst=to;
	while(*psrc){
		if(*psrc>0x20){
			*pdst++=toupper(*psrc);
			len++;
		}
		psrc++;
	}
	*pdst=0;
	return len;
}




/***********************************************************
* Function:
* Description:	接收到serial驱动的数据指示
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static rt_err_t mg323_rx_ind( rt_device_t dev, rt_size_t size )
{
	rt_sem_release( &sem_uart );
	return RT_EOK;
}

/***********************************************************
* Function:
* Description: 配置控电管脚，查找并打开gsm对应的串口设备
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static rt_err_t mg323_init( rt_device_t dev )
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOB, ENABLE );

	GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType	= GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd	= GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed	= GPIO_Speed_50MHz;

	GPIO_InitStructure.GPIO_Pin = GSM_PWR_PIN;
	GPIO_Init( GSM_PWR_PORT, &GPIO_InitStructure );
	GPIO_ResetBits( GSM_PWR_PORT, GSM_PWR_PIN );

	GPIO_InitStructure.GPIO_Pin = GSM_TERMON_PIN;
	GPIO_Init( GSM_TERMON_PORT, &GPIO_InitStructure );
	GPIO_ResetBits( GSM_TERMON_PORT, GSM_TERMON_PIN );

	dev_uart = rt_device_find( GSM_UART_NAME );
	if( dev_uart != RT_NULL && rt_device_open( dev_uart, RT_DEVICE_OFLAG_RDWR ) == RT_EOK )
	{
		rt_device_set_rx_indicate( dev_uart, mg323_rx_ind );
	}else
	{
		rt_kprintf( "GSM: can not find uart\n" );
		return RT_EEMPTY;
	}
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
static rt_err_t mg323_open( rt_device_t dev, rt_uint16_t oflag )
{
	if( gsmstate == GSM_IDLE )
	{
		gsmstate = GSM_POWERON; //置位上电过程中
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
	gsmstate = GSM_POWEROFF; //置位断电过程中
	return RT_EOK;
}

/***********************************************************
* Function:mg323_read
* Description:数据模式下读取数据
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
* Description:	数据模式下发送数据，要对数据进行封装
* Input:		const void* buff	要发送的原始数据
    rt_size_t count		要发送数据的长度
* Output:
* Return:
* Others:
***********************************************************/

static rt_size_t mg323_write( rt_device_t dev, rt_off_t pos, const void* buff, rt_size_t count )
{
	rt_size_t ret = RT_EOK;
	rt_device_write(dev_uart,pos,buff,count);
	return ret;
}

/***********************************************************
* Function:		mg323_control
* Description:	控制模块
* Input:		rt_uint8_t cmd	命令类型
    void *arg       参数,依据cmd的不同，传递的数据格式不同
* Output:
* Return:
* Others:
***********************************************************/
static rt_err_t mg323_control( rt_device_t dev, rt_uint8_t cmd, void *arg )
{
	switch( cmd )
	{
		case CTL_STATUS:
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

/*
+CREG:0,1
+CGREG:0,5
*/
rt_err_t RespFunc_CGREG(char *p)
{
	uint32_t i,n,code;
	rt_err_t res=RT_EOK;
	i=sscanf(p,"%*[^:]:%d,%d",&n,&code);
	if(i!=2) return RT_ERROR;
	if((code!=1)&&(code!=5)){
		res=RT_ERROR;
	}	
	return res;
}

rt_err_t RespFunc_COPS(char *p)
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

rt_err_t RespFunc_CIFSR(char *p)
{
	uint8_t i=0;
	rt_err_t res=RT_ERROR;
	int32_t PARAM_LocalIP[4];
	i=sscanf(p,"%u.%d.%d.%d",
			&(PARAM_LocalIP[0]),
			&(PARAM_LocalIP[1]),
			&(PARAM_LocalIP[2]),
			&(PARAM_LocalIP[3]));
	if(i==4) res=RT_EOK;
	return res;

}



/*
SIM卡的IMSI号码为4600 00783208249，
			   460 00 18 23 20 86 42

接口号数据字段的内容为IMSI号码的后12位
其6个字节的内容为 0x00 0x07 0x83 0x20 0x82 0x49

*/
rt_err_t RespFunc_CIMI(char *p)
{
	rt_err_t res=RT_ERROR;
	char *pimsi,i;
	if (strlen(p)<15) return RT_ERROR;
	if(strncmp(p,"460",3)==0){
		//strncpy(sys_param.imsi,p,15);
		res=RT_EOK;
	}	
	return res;

}







/*等待固定信息的返回*/
rt_err_t WaitResp( char *resp, rt_tick_t ticks )
{
	rt_tick_t	start		= rt_tick_get( );
	rt_tick_t	localticks	= ticks;
	char		*p;
	rt_err_t	ret;

/*等待收到信息*/
lbl_waitresp_again:
	ret = rt_sem_take( &sem_gsmrx, localticks );
	if( ret == -RT_ETIMEOUT )                               //超时退出，没有数据或接收数据完毕
	{
		return ret;
	}else /*收到信息，要判断是不是需要的*/
	{
		p = strstr( gsm_rx, resp );
		if( p )                                             /*找到了*/
		{
			return RT_EOK;
		} /*没找到，继续等，这个信息如何处理?比如来短信的通知等URC*/
		else
		{
			localticks = start + ticks - rt_tick_get( );    /*计算剩余的时间*/
			if( localticks > 1 )
			{
				goto lbl_waitresp_again;
			}
			return -RT_ETIMEOUT;
		}
	}
}

/*
   检查指定时间内的返回情况
   并调用专门的函数处理返回结果
 */
int8_t CheckResp( uint32_t ticks, RESP_FUNC resp_func )
{
	rt_err_t	ret=RT_EOK;

	/*等待收到信息*/
lbl_checkresp_again:
	ret = rt_sem_take( &sem_gsmrx, ticks );
	if( ret == -RT_ETIMEOUT )                               //超时退出，没有数据或接收数据完毕
	{
		return ret;
	}else /*收到信息，要判断是不是需要的*/
	{
		resp_func(gsm_rx);
	}
	return ret;
}


int8_t SendATCmdWaitRespStr(char *atcmd,
                uint32_t ticks,
                char * respstring,
                uint8_t no_of_attempts)
{
  rt_err_t ret_val = RT_ERROR;
  uint8_t i;

  for (i = 0; i < no_of_attempts; i++) 
  {
  		
  	mg323_write(dev_uart,0,atcmd,strlen(atcmd));
    ret_val = WaitResp(respstring,ticks); 
    if (ret_val == RT_EOK) {
        break;  
     }	
  }
  return (ret_val);
}



int8_t SendATCmdWaitRespFunc(char *atcmd,
                uint32_t ticks,
                RESP_FUNC respfunc,
                uint8_t no_of_attempts)
{
  int8_t ret_val = RT_ERROR;
  uint8_t i;

  for (i = 0; i < no_of_attempts; i++) {
  	//printf("\r\n%d(waitrespfunc)>%s",OSTimeGet(),AT_cmd_string);
    mg323_write(dev_uart,0,atcmd,strlen(atcmd));
    ret_val = CheckResp(ticks,respfunc); 
    if (ret_val == RT_EOK) {
        break;  
     }	
  }
  return (ret_val);
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
static void rt_thread_entry_gsm_poweron( void* parameter )
{
	rt_err_t res;
	
/*此处可通过rt_thread_self获取线程信息*/
lbl_start_pwr_on:
	rt_kprintf( "\r\n%ld>gsm pwr on start", rt_tick_get( ) );
	GPIO_SetBits( GSM_PWR_PORT, GSM_PWR_PIN );
	rt_thread_delay( RT_TICK_PER_SECOND / 2 );  //500ms
	GPIO_SetBits( GSM_TERMON_PORT, GSM_TERMON_PIN );
	rt_thread_delay( RT_TICK_PER_SECOND / 10 ); //100ms
	GPIO_ResetBits( GSM_TERMON_PORT, GSM_TERMON_PIN );
	rt_kprintf( "\r\n%ld>gsm pwr on end", rt_tick_get( ) );

	res=WaitResp( "^SYSSTART", RT_TICK_PER_SECOND * 10 );
	if(res!=RT_EOK)
	{
		rt_kprintf("\r\n%d>re gsm pwron\r\n",rt_tick_get());
		goto lbl_start_pwr_on;
	}	
	res=SendATCmdWaitRespStr("ATE0\r\n",3*RT_TICK_PER_SECOND,"OK",3);
	res=SendATCmdWaitRespStr("ATV1\r\n",3*RT_TICK_PER_SECOND,"OK",3);
	res=SendATCmdWaitRespStr("AT%SLEEP=0\r\n",3*RT_TICK_PER_SECOND,"OK",3);
	res=SendATCmdWaitRespStr("AT%TSIM\r\n",3*RT_TICK_PER_SECOND,"%TSIM1",3);
	if(res!=RT_EOK) goto lbl_start_pwr_on;
	res=SendATCmdWaitRespFunc("AT+COPS?\r\n",3*RT_TICK_PER_SECOND,RespFunc_COPS,10);
	res=SendATCmdWaitRespFunc("AT+CREG?\r\n",3*RT_TICK_PER_SECOND,RespFunc_CGREG,10);
	res=SendATCmdWaitRespFunc("AT+CIMI\r\n",3*RT_TICK_PER_SECOND,RespFunc_CIMI,3);
	res=SendATCmdWaitRespFunc("AT+CGREG?\r\n",3*RT_TICK_PER_SECOND,RespFunc_CGREG,10);
	res=SendATCmdWaitRespStr("AT+CGATT?\r\n",3*RT_TICK_PER_SECOND,"+CGATT:1",10);	
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
static void rt_thread_entry_gsm_poweroff( void* parameter )
{
/*此处可通过rt_thread_self获取线程信息*/
	rt_kprintf( "\r\n%ld>gsm pwr off start", rt_tick_get( ) );
	GPIO_SetBits( GSM_PWR_PORT, GSM_PWR_PIN );
	rt_thread_delay( RT_TICK_PER_SECOND / 2 );  //500ms
	GPIO_SetBits( GSM_TERMON_PORT, GSM_TERMON_PIN );
	rt_thread_delay( RT_TICK_PER_SECOND / 10 ); //100ms
	GPIO_ResetBits( GSM_TERMON_PORT, GSM_TERMON_PIN );
	rt_kprintf( "\r\n%ld>gsm pwr off end", rt_tick_get( ) );

}


/***********************************************************
* Function:		rt_thread_entry_gsm_ppp
* Description:	建立链接
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static void rt_thread_entry_gsm_ppp( void* parameter )
{
	

}



ALIGN( RT_ALIGN_SIZE )
static char thread_gsm_stack[2048];
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
	rt_tick_t		curr_ticks;
	rt_err_t		res;
	unsigned char	ch, next;

/*gsm的状态切换*/
	curr_ticks = rt_tick_get( );
	switch( gsmstate )
	{
		case GSM_IDLE:
			break;
		case GSM_POWERON:                               /*上电过程中，控制IO的操作放在此处，如何保证时间的可靠*/
			if( tid_gsm_power == RT_NULL )
			{
				tid_gsm_power = rt_thread_create( "pwron",
				                                  rt_thread_entry_gsm_poweron, (void*)1,
				                                  512,  /* thread stack size */
				                                  25,   /* thread priority */
				                                  5     /* thread time slice*/
				                                  );
				if( tid_gsm_power != RT_NULL )
				{
					rt_thread_startup( tid_gsm_power );
				}
			}
			break;
		case GSM_POWEROFF:                              /*断电电过程中，控制IO的操作放在此处，如何保证时间的可靠*/
			if( tid_gsm_power == RT_NULL )
			{
				tid_gsm_power = rt_thread_create( "pwroff",
				                                  rt_thread_entry_gsm_poweroff, (void*)1,
				                                  512,  /* thread stack size */
				                                  25,   /* thread priority */
				                                  5     /* thread time slice*/
				                                  );
				if( tid_gsm_power != RT_NULL )
				{
					rt_thread_startup( tid_gsm_power );
				}
			}
			break;
		case GSM_AT:
			break;

		case GSM_PPP:
			break;

		case GSM_DATA:
			break;
	}

/*接收超时判断*/
	res = rt_sem_take( &sem_uart, RT_TICK_PER_SECOND / 10 );    //等待100ms,实际上就是变长的延时,最长100ms
	if( res == -RT_ETIMEOUT )                                   //超时退出，没有数据或接收数据完毕
	{
		if( gsm_rx_wr )
		{
			gsmrx_cb( gsm_rx, gsm_rx_wr );
			gsm_rx_wr = 0;
		}
	}else //收到数据,理论上1个字节触发一次,这里没有根据收到的<CRLF>处理，而是等到超时统一处理
	{
		while( rt_device_read( dev_uart, 0, &ch, 1 ) == 1 )
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
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
void gsm_init( T_GSM_OPS *gsm_ops )
{
	rt_thread_t tid;

	rt_sem_init( &sem_uart, "sem_uart", 0, 0 );
	rt_sem_init( &sem_gsmrx, "sem_gsmrx", 0, 0 );

	rt_thread_init( &thread_gsm,
	                "gsm",
	                rt_thread_entry_gsm,
	                RT_NULL,
	                &thread_gsm_stack[0],
	                sizeof( thread_gsm_stack ), 7, 5 );
	rt_thread_startup( &thread_gsm );
/*
	rt_timer_init( &tmr_gsm, \
	               "tmr_gsm", \
	               timer_gsm_cb, NULL, \
	               50, \
	               RT_TIMER_FLAG_PERIODIC );
*/
	dev_gsm.type	= RT_Device_Class_Char;
	dev_gsm.init	= mg323_init;
	dev_gsm.open	= mg323_open;
	dev_gsm.close	= mg323_close;
	dev_gsm.read	= mg323_read;
	dev_gsm.write	= mg323_write;
	dev_gsm.control = mg323_control;

	dev_gsm.user_data = &gsm_ops_default;
	if( gsm_ops != NULL )
	{
		dev_gsm.user_data = gsm_ops;
	}

	rt_device_register( &dev_gsm, "gsm", RT_DEVICE_FLAG_RDWR );
	rt_device_init( &dev_gsm );
	rt_timer_start( &tmr_gsm );
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
	return mg323_open( &dev_gsm, RT_DEVICE_OFLAG_RDWR );
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
rt_err_t gsm_close( void )
{
	return mg323_close( &dev_gsm );
}

FINSH_FUNCTION_EXPORT( gsm_close, close gsm );

/*设置链接的socket参数*/
rt_err_t gsm_control_socket( uint8_t linkno, char type, char *ip, uint32_t port )
{
	return mg323_control( &dev_gsm, 0, NULL );
}

FINSH_FUNCTION_EXPORT( gsm_control_socket, control gsm );


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
	return mg323_write( &dev_gsm, 0, sinfo, strlen( sinfo ) );
}

FINSH_FUNCTION_EXPORT( gsm_write, write gsm );

#endif

#endif

/************************************** The End Of File **************************************/
