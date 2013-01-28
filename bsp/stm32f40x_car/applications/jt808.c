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
#include <board.h>
#include <rtthread.h>
#include "stm32f4xx.h"
#include "gsm.h"
#include "jt808.h"
#include "msglist.h"


#define MULTI_PROCESS






static struct rt_mailbox	mb_gprsdata;
#define MB_GPRSDATA_POOL_SIZE 32
static uint8_t				mb_gprsdata_pool[MB_GPRSDATA_POOL_SIZE];

static struct rt_mailbox	mb_gpsdata;
#define MB_GPSDATA_POOL_SIZE 32
static uint8_t				mb_gpsdata_pool[MB_GPSDATA_POOL_SIZE];



uint32_t jt808_alarm=0x0;
uint32_t jt808_status=0x0;



static rt_device_t pdev_gsm=RT_NULL;

/*系统参数*/
JT808_PARAM jt808_param;

/*发送信息列表*/
MsgList* list_jt808_tx;

/*接收信息列表*/
MsgList* list_jt808_rx;

/*
   jt808格式数据解码判断
   <标识0x7e><消息头><消息体><校验码><标识0x7e>

   返回有效的数据长度,为0 表明有错

 */
static uint16_t jt808_decode_fcs( uint8_t *pinfo, uint16_t length )
{
	uint8_t		*psrc, *pdst;
	uint16_t	count,len;
	uint8_t		fstuff	= 0; /*是否字节填充*/
	uint8_t		fcs		= 0;

	if( length < 5 )return 0;
	if( *pinfo != 0x7e )return 0;
	if( *( pinfo + length - 1 )!=0x7e )	return 0;
	psrc	= pinfo + 1; /*1byte标识后为正式信息*/
	pdst	= pinfo;
	count =0;	/*转义后的长度*/
	len =length-2;  /*去掉标识位的数据长度*/
	
	while( len)
	{
		if( fstuff )
		{
			if( *psrc == 0x02 )
			{
				*pdst = 0x7e;
			}
			if( *psrc == 0x01 )
			{
				*pdst = 0x7d;
			}
			fstuff = 0;
			count++;
			fcs ^= *pdst;
		}
		else
		{
			if( *psrc == 0x7d )
			{
				fstuff = 1;
			} else
			{
				*pdst	= *psrc;
				fcs		^= *pdst;
				count++;
			}
		}
		psrc++;
		pdst++;
		len--;
	}
	if( fcs != 0 )
	{
		rt_kprintf( "%s>fcs error\n", __func__ );
		return 0;
	}
	return count;
}

#if 0


/*
   对数据进行FCS计算或校验
 */
static uint8_t jt808_fcs( uint8_t * pinfo, uint16_t length )
{
	uint8_t		fcs = 0;
	uint8_t		*psrc;
	uint16_t	i;
	psrc = pinfo;
	for( i = 0; i < length; i++ )
	{
		fcs ^= *psrc;
	}
	return fcs;
}

#endif


/*
   jt808格式数据FCS计算，并编码
   返回编码后的数据长度
 */
static uint16_t jt808_encode( uint8_t * pinfo, uint16_t length )
{
	return 1;
}

/*
   jt808终端发送信息
   并将相关信息注册到接收信息的处理线程中
   需要传递消息ID,和消息体，由jt808_send线程完成
    消息的填充
    发送和重发机制
    流水号
    已发信息的回收free
   传递进来的格式
   <msgid 2bytes><msg_len 2bytes><msgbody nbytes>

 */
static void jt808_send( void* parameter )
{
}

/*
比较并定位node位置,index
*/
static int jt808_msg_cmp(void* ctx, void* data)
{
//	JT808_MSG_NODE* psrc=(JT808_MSG_NODE*)ctx;
//	JT808_MSG_NODE* pdst=(JT808_MSG_NODE*)data;
//	if(psrc->msg_sn==pdst->msg_sn) return 0;
	return 1;

}


/*
处理每个要发送信息的状态
现在允许并行处理吗?
*/
static MsgListRet jt808_msg_tx_proc(void* ctx, void* data)
{
	MsgListNode* pnode=(MsgListNode*)data;
	JT808_TX_MSG_NODEDATA* pnodedata=(JT808_TX_MSG_NODEDATA*)(pnode->data);

	if(pnodedata->state==IDLE) /*空闲，发送信息或超时后没有数据*/
	{
		if(pnodedata->retry==pnodedata->max_retry)	/*已经达到重试次数*/
		{
			/*表示发送失败*/
			rt_free(pnodedata->pmsg); /*删除节点数据*/
			pnode->prev->next=pnode->next; /*删除节点*/
			pnode->next->prev=pnode->prev;
			msglist_node_destroy(pnode);
			return MSGLIST_RET_DELETE_NODE;
		}
		else
		{
			pnodedata->retry++;
			rt_device_write(pdev_gsm,0,pnodedata->pmsg,pnodedata->msg_len);
			pnodedata->tick=rt_tick_get();
			pnodedata->timeout=pnodedata->max_retry*pnodedata->timeout;
			pnodedata->state=WAIT_ACK;
			rt_kprintf("send retry=%d,timeout=%d\r\n",pnodedata->retry,pnodedata->timeout);
		}	
		return MSGLIST_RET_OK;
	}
	
	if(pnodedata->state==WAIT_ACK)
	{
		if(rt_tick_get()-pnodedata->tick>pnodedata->timeout)
		{
			pnodedata->state=IDLE;
		}
	}
	return MSGLIST_RET_OK;
}



typedef __packed struct
{
	uint16_t seq;	/*应答流水号*/
	uint16_t id;	/*应答ID*/
	uint8_t	result;	/*结果*/
}JT808_MSG_ACK;



/*
中心接收到消息的处理，主要是应答的处理
*ctx:传递进来收到的信息
*/
static MsgListRet jt808_msg_rx_ack_proc(void* ctx, void* data)
{
	MsgListNode* pnode=(MsgListNode*)data;
	JT808_RX_MSG_NODEDATA* pnodedata=(JT808_RX_MSG_NODEDATA*)(pnode->data);
	if(pnode==NULL) return MSGLIST_RET_OK;



	
}




/*平台通用应答,收到信息，停止发送*/
static int handle_jt808_msg_0x8001(uint8_t *pinfo,uint16_t len)
{
	uint16_t msg_id;
	uint8_t *psrc=pinfo;
	MsgListNode* iter;
	MsgListNode* iter_next;
#ifdef MULTI_PROCESS
	/*多处理*/	
			{
				//msglist_foreach(list_jt808_tx,jt808_msg_rx_ack_proc,RT_NULL);
				iter=list_jt808_tx->first;
				while(iter!=RT_NULL)
				{
					iter_next=iter->next;
					if(jt808_msg_rx_ack_proc(RT_NULL,iter)==MSGLIST_RET_DELETE_NODE)
					{
						iter=iter_next;
					}	
				}
			}	
	/*逐条处理*/
#else
			jt808_msg_rx_ack_proc(RT_NULL,list_jt808_tx->first);
		
#endif


	







	
	/* 若没有分包处理的话  消息头长12  从0开始计算第12个字节是消息体得主体
	   13 14	对应的终端消息流水号
	   15 16	对应终端的消息
	 */
	msg_id = ( *( psrc + 14 ) << 8 ) + *( psrc + 15 );
	
	switch( msg_id )	// 判断对应终端消息的ID做区分处理
	{
		case 0x0200:	//	对应位置消息的应答
			rt_kprintf( "\r\nCentre ACK!\r\n" );
			break;
		case 0x0002:	//	心跳包的应答
			rt_kprintf( "\r\n  Centre  Heart ACK!\r\n" );
			break;
		case 0x0101:	//	终端注销应答
			break;
		case 0x0102:	//	终端鉴权
			break;
		case 0x0800:	// 多媒体事件信息上传
			break;
		case 0x0702:
			rt_kprintf( "\r\n  驾驶员信息上报---中心应答!  \r\n" );
			break;
		case 0x0701:
			rt_kprintf( "\r\n	电子运单上报---中心应答!  \r\n");
			break;
		default:
			break;
	}


}


/* 监控中心对终端注册消息的应答*/
static int handle_jt808_msg_0x8100(uint8_t* pinfo,uint16_t len)
{
return 1;

}

/**/
static int handle_jt808_msg_0x8103(uint8_t* pinfo,uint16_t len)
{
return 1;

}

/**/
static int handle_jt808_msg_0x8104(uint8_t* pinfo,uint16_t len)
{

return 1;
}

/**/
static int handle_jt808_msg_0x8105(uint8_t* pinfo,uint16_t len)
{

return 1;
}

/**/
static int handle_jt808_msg_0x8201(uint8_t* pinfo,uint16_t len)
{

return 1;
}

/**/
static int handle_jt808_msg_0x8202(uint8_t* pinfo,uint16_t len)
{

return 1;
}

/**/
static int handle_jt808_msg_0x8300(uint8_t* pinfo,uint16_t len)
{

return 1;
}

/**/
static int handle_jt808_msg_0x8301(uint8_t* pinfo,uint16_t len)
{

return 1;
}
/**/
static int handle_jt808_msg_0x8302(uint8_t* pinfo,uint16_t len)
{

return 1;
}
/**/
static int handle_jt808_msg_0x8303(uint8_t* pinfo,uint16_t len)
{

return 1;
}
/**/
static int handle_jt808_msg_0x8304(uint8_t* pinfo,uint16_t len)
{

return 1;
}
/**/
static int handle_jt808_msg_0x8400(uint8_t* pinfo,uint16_t len)
{

return 1;
}
/**/
static int handle_jt808_msg_0x8401(uint8_t* pinfo,uint16_t len)
{

return 1;
}
/**/
static int handle_jt808_msg_0x8500(uint8_t* pinfo,uint16_t len)
{

return 1;
}
/**/
static int handle_jt808_msg_0x8600(uint8_t* pinfo,uint16_t len)
{

return 1;
}
/**/
static int handle_jt808_msg_0x8601(uint8_t* pinfo,uint16_t len)
{

return 1;
}
/**/
static int handle_jt808_msg_0x8602(uint8_t* pinfo,uint16_t len)
{

return 1;
}
/**/
static int handle_jt808_msg_0x8603(uint8_t* pinfo,uint16_t len)
{

return 1;
}
/**/
static int handle_jt808_msg_0x8604(uint8_t* pinfo,uint16_t len)
{

return 1;
}
/**/
static int handle_jt808_msg_0x8605(uint8_t* pinfo,uint16_t len)
{

return 1;
}
/**/
static int handle_jt808_msg_0x8606(uint8_t* pinfo,uint16_t len)
{

return 1;
}
/**/
static int handle_jt808_msg_0x8607(uint8_t* pinfo,uint16_t len)
{

return 1;
}

/**/
static int handle_jt808_msg_0x8700(uint8_t* pinfo,uint16_t len)
{

return 1;
}
/**/
static int handle_jt808_msg_0x8701(uint8_t* pinfo,uint16_t len)
{

return 1;
}
/**/
static int handle_jt808_msg_0x8800(uint8_t* pinfo,uint16_t len)
{

return 1;
}
/**/
static int handle_jt808_msg_0x8801(uint8_t* pinfo,uint16_t len)
{

return 1;
}
/**/
static int handle_jt808_msg_0x8802(uint8_t* pinfo,uint16_t len)
{

return 1;
}
/**/
static int handle_jt808_msg_0x8803(uint8_t* pinfo,uint16_t len)
{

return 1;
}
/**/
static int handle_jt808_msg_0x8804(uint8_t* pinfo,uint16_t len)
{

return 1;
}
/**/
static int handle_jt808_msg_0x8805(uint8_t* pinfo,uint16_t len)
{

return 1;
}
/**/
static int handle_jt808_msg_0x8900(uint8_t* pinfo,uint16_t len)
{

return 1;
}
/**/
static int handle_jt808_msg_0x8A00(uint8_t* pinfo,uint16_t len)
{

return 1;
}


static int handle_jt808_msg_default(uint8_t* pinfo,uint16_t len)
{

return 1;
} 

#define DECL_RX_MSG_HANDLE(a) {a,handle_jt808_msg_##a}

HANDLE_JT808_RX_MSG handle_jt808_rx_msg[]=
{
	DECL_RX_MSG_HANDLE(0x8100),  //  监控中心对终端注册消息的应答
	DECL_RX_MSG_HANDLE(0x8103),	//	设置终端参数
	DECL_RX_MSG_HANDLE(0x8104),	//	查询终端参数
	DECL_RX_MSG_HANDLE(0x8105),	// 终端控制
	DECL_RX_MSG_HANDLE(0x8201),	// 位置信息查询    位置信息查询消息体为空
	DECL_RX_MSG_HANDLE(0x8202),	// 临时位置跟踪控制
	DECL_RX_MSG_HANDLE(0x8300),	//	文本信息下发
	DECL_RX_MSG_HANDLE(0x8301),	//	事件设置
	DECL_RX_MSG_HANDLE(0x8302),	// 提问下发
	DECL_RX_MSG_HANDLE(0x8303),	//	信息点播菜单设置
	DECL_RX_MSG_HANDLE(0x8304),	//	信息服务
	DECL_RX_MSG_HANDLE(0x8400),	//	电话回拨
	DECL_RX_MSG_HANDLE(0x8401),	//	设置电话本
	DECL_RX_MSG_HANDLE(0x8500),	//	车辆控制
	DECL_RX_MSG_HANDLE(0x8600),	//	设置圆形区域
	DECL_RX_MSG_HANDLE(0x8601),	//	删除圆形区域
	DECL_RX_MSG_HANDLE(0x8602),	//	设置矩形区域
	DECL_RX_MSG_HANDLE(0x8603),	//	删除矩形区域
	DECL_RX_MSG_HANDLE(0x8604),	//	多边形区域
	DECL_RX_MSG_HANDLE(0x8605),	//	删除多边区域
	DECL_RX_MSG_HANDLE(0x8606),	//	设置路线
	DECL_RX_MSG_HANDLE(0x8607),	//	删除路线
	DECL_RX_MSG_HANDLE(0x8700),	//	行车记录仪数据采集命令
	DECL_RX_MSG_HANDLE(0x8701),	//	行驶记录仪参数下传命令
	DECL_RX_MSG_HANDLE(0x8800),	//	多媒体数据上传应答
	DECL_RX_MSG_HANDLE(0x8801),	//	摄像头立即拍照
	DECL_RX_MSG_HANDLE(0x8802),	//	存储多媒体数据检索
	DECL_RX_MSG_HANDLE(0x8803),	//	存储多媒体数据上传命令
	DECL_RX_MSG_HANDLE(0x8804),	//	录音开始命令
	DECL_RX_MSG_HANDLE(0x8805),	//	单条存储多媒体数据检索上传命令 ---- 补充协议要求
	DECL_RX_MSG_HANDLE(0x8900),	//	数据下行透传
	DECL_RX_MSG_HANDLE(0x8A00),	//	平台RSA公钥 
	{0x0000,RT_NULL},

};


/*
   分析jt808格式的数据
   <linkno><长度2byte><标识0x7e><消息头><消息体><校验码><标识0x7e>

 */
uint16_t jt808_rx_analy( uint8_t * pinfo)
{
	uint8_t		*psrc;
	uint16_t	len;
	uint8_t		linkno;
	uint16_t	i;
	uint8_t		flag_find=0;

	MsgListNode* node;
	JT808_RX_MSG_NODEDATA*	nodedata;

	MsgListNode* iter;
	JT808_RX_MSG_NODEDATA*	iterdata;

	linkno=pinfo[0];
	len = ( pinfo[1] << 8 ) | pinfo[2];
	rt_kprintf( "gsm>rx %d bytes\r\n", len );
	len = jt808_decode_fcs( pinfo + 3, len );
	if( len == 0 )	/*格式不正确*/
	{	
		rt_free(pinfo);
		return 1;
	}
	nodedata=rt_malloc(sizeof(JT808_RX_MSG_NODEDATA));
	if(nodedata==RT_NULL) /*无法处理此信息*/
	{
		rt_free(pinfo);
		return 1;
	}

	psrc=pinfo;
	nodedata->linkno=psrc[0];
	nodedata->id=(*( psrc + 0 ) << 8) | *( psrc + 1 );
	nodedata->attr=(*( psrc + 2 ) << 8) | *( psrc + 3 );
	memcpy( nodedata->mobileno, psrc + 4, 6 );
	nodedata->seq = (*( psrc + 10 ) << 8) | *( psrc + 11 );
	nodedata->pmsg=pinfo;
	nodedata->msg_len=len;
	nodedata->tick=rt_tick_get();  
/*创建新的节点*/
	node=msglist_node_create((void*)nodedata);
	if(node==RT_NULL)
	{
		rt_free(nodedata);
		rt_free(pinfo);
		return 1;
	}

	if( nodedata->attr & 0x2000 )  /*分包处理*/
	{
		nodedata->packetcount= (*( psrc + 12 ) << 8) | *( psrc + 13 );
		nodedata->packetno = (*( psrc + 14 ) << 8) | *( psrc + 15 );
/*看是不是第一个分包*/
		flag_find=0;
		iter=list_jt808_rx->first;
		while(iter!=NULL)
		{
			iterdata=(JT808_RX_MSG_NODEDATA*)(iter->data);
			if(iterdata->id==nodedata->id)  /*判断的消息ID一致,即便已有分包，但ID不一致，也认为是新包*/
			{
				flag_find=1;
				break;
			}
			iter=iter->next;
		}
/*找到，不是第一个分包，要对已有的分包排序*/		
		if(flag_find) /*找到了 iter，开始遍历sibling并插入*/
		{
			if(iterdata->packetno<nodedata->packetno) /*在主干上,改变主干上的节点*/
			{
				node->prev=iter->prev;
				node->next=iter->next; /*替换原来的位置*/
				node->sibling_dn=iter;
				iter->sibling_up=node;
			}
			else /*在多包信息的分支上*/
			{
				flag_find=0;
				while(iter->sibling_dn!=NULL)
				{
					iter=iter->sibling_dn;
					iterdata=iter->data;
					if(iterdata->packetno<nodedata->packetno)
					{
						node->sibling_up=iter->sibling_up;
						node->sibling_dn=iter;
						iter->sibling_up=node;
						flag_find=1;
						break;
					}
				}
				if(flag_find==0) /*结尾也没有找到*/
				{
					node->sibling_up=iter;
					iter->sibling_dn=node;
				}
			}
		}
		else	/*没找到，没有使用msglist_append(list_jt808_rx,nodedata);*/
		{
			if(list_jt808_rx->first==NULL) /*是第一个节点*/
			{
				list_jt808_rx->first->data=nodedata;
			}
			else /*已有节点，添加到最后*/
			{
				iter=iter->prev;  /*此时iter为NULL,应指向前一个有效node*/
				iter->next=node; 
				node->prev=iter;
			}
		}
	}
	else /*不是分包节点，添加到接收信息的list中,没有使用msglist_append(list_jt808_rx,nodedata);*/
	{
		iter=list_jt808_rx->first;
		if(iter==NULL)
		{
			list_jt808_rx->first=node;
		}
		else
		{
			while(iter!=NULL)
			{
				iter=iter->next;
			}
			iter->prev->next=node;
		}
	}

/*
	for(i=0;i<sizeof(handle_jt808_rx_msg)/sizeof(HANDLE_JT808_RX_MSG);i++)
	{
		if(msg_id==handle_jt808_rx_msg[i].id)
		{
			handle_jt808_rx_msg[i].func(psrc,msg_len);
			flag_find=1;
			break;
		}
	
	}
	if(!flag_find) handle_jt808_msg_default(psrc,msg_len);
*/

}




/*
   连接状态维护
   jt808协议处理

 */
ALIGN( RT_ALIGN_SIZE )
static char thread_jt808_stack[512];
struct rt_thread thread_jt808;


/***/
static void rt_thread_entry_jt808( void* parameter )
{
	rt_err_t	ret;
	uint8_t		*pstr;
	uint32_t	gsm_status;
	

	MsgListNode* iter;
	MsgListNode* iter_next;


	list_jt808_tx=msglist_create();
	list_jt808_rx=msglist_create();


	pdev_gsm=rt_device_find("gsm");  /*没有出错处理,未找到怎么办*/

/*读取参数，并配置*/	

	while( 1 )
	{
/*接收gprs信息*/
		ret = rt_mb_recv( &mb_gprsdata, (rt_uint32_t*)&pstr, 5 );
		if( ret == RT_EOK )
		{
			jt808_rx_analy(pstr);
			//rt_free( pstr );  /*不能过早的释放*/
		}
/*接收gps信息*/		
		ret = rt_mb_recv(&mb_gpsdata,(rt_uint32_t*)&pstr,5);
		if(ret == RT_EOK)
		{
			gps_analy(pstr);
			rt_free(pstr);
		}
/*维护链路*/
		rt_device_control(pdev_gsm,CTL_STATUS,&gsm_status);
	

#ifdef MULTI_PROCESS
/*多处理*/	
		{
			//msglist_foreach(list_jt808_tx,jt808_msg_tx_proc,RT_NULL);
			iter=list_jt808_tx->first;
			while(iter!=RT_NULL)
			{
				iter_next=iter->next;
				if(jt808_msg_tx_proc(RT_NULL,iter)==MSGLIST_RET_DELETE_NODE)/*该节点已被删除*/
				{
					iter=iter_next;
				}
			}
		}	
/*逐条处理*/
#else
		jt808_msg_tx_proc(RT_NULL,list_jt808_tx->first);
	
#endif		
		rt_thread_delay( RT_TICK_PER_SECOND / 20 );
	}

	msglist_destroy(list_jt808_tx);
}

/*jt808处理线程初始化*/
void jt808_init( void )
{
	rt_mb_init( &mb_gprsdata, "gprsdata", &mb_gprsdata_pool, MB_GPRSDATA_POOL_SIZE / 4, RT_IPC_FLAG_FIFO );
	rt_mb_init( &mb_gpsdata, "gpsdata", &mb_gpsdata_pool, MB_GPSDATA_POOL_SIZE / 4, RT_IPC_FLAG_FIFO );

	rt_thread_init( &thread_jt808,
	                "jt808",
	                rt_thread_entry_jt808,
	                RT_NULL,
	                &thread_jt808_stack[0],
	                sizeof( thread_jt808_stack ), 10, 5 );
	rt_thread_startup( &thread_jt808 );
}

/*gps接收处理*/
void gps_rx(uint8_t *pinfo,uint16_t length)
{
	uint8_t *pmsg;
	pmsg= rt_malloc(length+2);
	if(pmsg!=RT_NULL)
	{
		pmsg[0] = length >>8;
		pmsg[1] = length &0xff;
		memcpy(pmsg+2,pinfo,length);
		rt_mb_send(&mb_gpsdata,(rt_uint32_t)pmsg);
	}
}

/*gprs接收处理*/
void gprs_rx( uint8_t linkno,uint8_t * pinfo, uint16_t length )
{
	uint8_t *pmsg;
	pmsg = rt_malloc( length + 3 );      /*包含长度信息*/
	if( pmsg != RT_NULL )
	{
		pmsg[0] = linkno;
		pmsg[1] = length >> 8;
		pmsg[2] = length & 0xff;
		memcpy( pmsg + 3, pinfo, length );
		rt_mb_send( &mb_gprsdata, (rt_uint32_t)pmsg );
	}
}

/************************************** The End Of File **************************************/
