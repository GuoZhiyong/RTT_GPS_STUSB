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

/*ϵͳ����*/
JT808_PARAM jt808_param;

/*������Ϣ�б�*/
MsgList* list_jt808_tx;

/*������Ϣ�б�*/
MsgList* list_jt808_rx;

/*
   jt808��ʽ���ݽ����ж�
   <��ʶ0x7e><��Ϣͷ><��Ϣ��><У����><��ʶ0x7e>

   ������Ч�����ݳ���,Ϊ0 �����д�

 */
static uint16_t jt808_decode_fcs( uint8_t *pinfo, uint16_t length )
{
	uint8_t		*psrc, *pdst;
	uint16_t	count,len;
	uint8_t		fstuff	= 0; /*�Ƿ��ֽ����*/
	uint8_t		fcs		= 0;

	if( length < 5 )return 0;
	if( *pinfo != 0x7e )return 0;
	if( *( pinfo + length - 1 )!=0x7e )	return 0;
	psrc	= pinfo + 1; /*1byte��ʶ��Ϊ��ʽ��Ϣ*/
	pdst	= pinfo;
	count =0;	/*ת���ĳ���*/
	len =length-2;  /*ȥ����ʶλ�����ݳ���*/
	
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
   �����ݽ���FCS�����У��
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
   jt808��ʽ����FCS���㣬������
   ���ر��������ݳ���
 */
static uint16_t jt808_encode( uint8_t * pinfo, uint16_t length )
{
	return 1;
}

/*
   jt808�ն˷�����Ϣ
   ���������Ϣע�ᵽ������Ϣ�Ĵ����߳���
   ��Ҫ������ϢID,����Ϣ�壬��jt808_send�߳����
    ��Ϣ�����
    ���ͺ��ط�����
    ��ˮ��
    �ѷ���Ϣ�Ļ���free
   ���ݽ����ĸ�ʽ
   <msgid 2bytes><msg_len 2bytes><msgbody nbytes>

 */
static void jt808_send( void* parameter )
{
}

/*
�Ƚϲ���λnodeλ��,index
*/
static int jt808_msg_cmp(void* ctx, void* data)
{
//	JT808_MSG_NODE* psrc=(JT808_MSG_NODE*)ctx;
//	JT808_MSG_NODE* pdst=(JT808_MSG_NODE*)data;
//	if(psrc->msg_sn==pdst->msg_sn) return 0;
	return 1;

}


/*
����ÿ��Ҫ������Ϣ��״̬
���������д�����?
*/
static MsgListRet jt808_msg_tx_proc(void* ctx, void* data)
{
	MsgListNode* pnode=(MsgListNode*)data;
	JT808_TX_MSG_NODEDATA* pnodedata=(JT808_TX_MSG_NODEDATA*)(pnode->data);

	if(pnodedata->state==IDLE) /*���У�������Ϣ��ʱ��û������*/
	{
		if(pnodedata->retry==pnodedata->max_retry)	/*�Ѿ��ﵽ���Դ���*/
		{
			/*��ʾ����ʧ��*/
			rt_free(pnodedata->pmsg); /*ɾ���ڵ�����*/
			pnode->prev->next=pnode->next; /*ɾ���ڵ�*/
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
	uint16_t seq;	/*Ӧ����ˮ��*/
	uint16_t id;	/*Ӧ��ID*/
	uint8_t	result;	/*���*/
}JT808_MSG_ACK;



/*
���Ľ��յ���Ϣ�Ĵ�����Ҫ��Ӧ��Ĵ���
*ctx:���ݽ����յ�����Ϣ
*/
static MsgListRet jt808_msg_rx_ack_proc(void* ctx, void* data)
{
	MsgListNode* pnode=(MsgListNode*)data;
	JT808_RX_MSG_NODEDATA* pnodedata=(JT808_RX_MSG_NODEDATA*)(pnode->data);
	if(pnode==NULL) return MSGLIST_RET_OK;



	
}




/*ƽ̨ͨ��Ӧ��,�յ���Ϣ��ֹͣ����*/
static int handle_jt808_msg_0x8001(uint8_t *pinfo,uint16_t len)
{
	uint16_t msg_id;
	uint8_t *psrc=pinfo;
	MsgListNode* iter;
	MsgListNode* iter_next;
#ifdef MULTI_PROCESS
	/*�ദ��*/	
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
	/*��������*/
#else
			jt808_msg_rx_ack_proc(RT_NULL,list_jt808_tx->first);
		
#endif


	







	
	/* ��û�зְ�����Ļ�  ��Ϣͷ��12  ��0��ʼ�����12���ֽ�����Ϣ�������
	   13 14	��Ӧ���ն���Ϣ��ˮ��
	   15 16	��Ӧ�ն˵���Ϣ
	 */
	msg_id = ( *( psrc + 14 ) << 8 ) + *( psrc + 15 );
	
	switch( msg_id )	// �ж϶�Ӧ�ն���Ϣ��ID�����ִ���
	{
		case 0x0200:	//	��Ӧλ����Ϣ��Ӧ��
			rt_kprintf( "\r\nCentre ACK!\r\n" );
			break;
		case 0x0002:	//	��������Ӧ��
			rt_kprintf( "\r\n  Centre  Heart ACK!\r\n" );
			break;
		case 0x0101:	//	�ն�ע��Ӧ��
			break;
		case 0x0102:	//	�ն˼�Ȩ
			break;
		case 0x0800:	// ��ý���¼���Ϣ�ϴ�
			break;
		case 0x0702:
			rt_kprintf( "\r\n  ��ʻԱ��Ϣ�ϱ�---����Ӧ��!  \r\n" );
			break;
		case 0x0701:
			rt_kprintf( "\r\n	�����˵��ϱ�---����Ӧ��!  \r\n");
			break;
		default:
			break;
	}


}


/* ������Ķ��ն�ע����Ϣ��Ӧ��*/
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
	DECL_RX_MSG_HANDLE(0x8100),  //  ������Ķ��ն�ע����Ϣ��Ӧ��
	DECL_RX_MSG_HANDLE(0x8103),	//	�����ն˲���
	DECL_RX_MSG_HANDLE(0x8104),	//	��ѯ�ն˲���
	DECL_RX_MSG_HANDLE(0x8105),	// �ն˿���
	DECL_RX_MSG_HANDLE(0x8201),	// λ����Ϣ��ѯ    λ����Ϣ��ѯ��Ϣ��Ϊ��
	DECL_RX_MSG_HANDLE(0x8202),	// ��ʱλ�ø��ٿ���
	DECL_RX_MSG_HANDLE(0x8300),	//	�ı���Ϣ�·�
	DECL_RX_MSG_HANDLE(0x8301),	//	�¼�����
	DECL_RX_MSG_HANDLE(0x8302),	// �����·�
	DECL_RX_MSG_HANDLE(0x8303),	//	��Ϣ�㲥�˵�����
	DECL_RX_MSG_HANDLE(0x8304),	//	��Ϣ����
	DECL_RX_MSG_HANDLE(0x8400),	//	�绰�ز�
	DECL_RX_MSG_HANDLE(0x8401),	//	���õ绰��
	DECL_RX_MSG_HANDLE(0x8500),	//	��������
	DECL_RX_MSG_HANDLE(0x8600),	//	����Բ������
	DECL_RX_MSG_HANDLE(0x8601),	//	ɾ��Բ������
	DECL_RX_MSG_HANDLE(0x8602),	//	���þ�������
	DECL_RX_MSG_HANDLE(0x8603),	//	ɾ����������
	DECL_RX_MSG_HANDLE(0x8604),	//	���������
	DECL_RX_MSG_HANDLE(0x8605),	//	ɾ���������
	DECL_RX_MSG_HANDLE(0x8606),	//	����·��
	DECL_RX_MSG_HANDLE(0x8607),	//	ɾ��·��
	DECL_RX_MSG_HANDLE(0x8700),	//	�г���¼�����ݲɼ�����
	DECL_RX_MSG_HANDLE(0x8701),	//	��ʻ��¼�ǲ����´�����
	DECL_RX_MSG_HANDLE(0x8800),	//	��ý�������ϴ�Ӧ��
	DECL_RX_MSG_HANDLE(0x8801),	//	����ͷ��������
	DECL_RX_MSG_HANDLE(0x8802),	//	�洢��ý�����ݼ���
	DECL_RX_MSG_HANDLE(0x8803),	//	�洢��ý�������ϴ�����
	DECL_RX_MSG_HANDLE(0x8804),	//	¼����ʼ����
	DECL_RX_MSG_HANDLE(0x8805),	//	�����洢��ý�����ݼ����ϴ����� ---- ����Э��Ҫ��
	DECL_RX_MSG_HANDLE(0x8900),	//	��������͸��
	DECL_RX_MSG_HANDLE(0x8A00),	//	ƽ̨RSA��Կ 
	{0x0000,RT_NULL},

};


/*
   ����jt808��ʽ������
   <linkno><����2byte><��ʶ0x7e><��Ϣͷ><��Ϣ��><У����><��ʶ0x7e>

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
	if( len == 0 )	/*��ʽ����ȷ*/
	{	
		rt_free(pinfo);
		return 1;
	}
	nodedata=rt_malloc(sizeof(JT808_RX_MSG_NODEDATA));
	if(nodedata==RT_NULL) /*�޷��������Ϣ*/
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
/*�����µĽڵ�*/
	node=msglist_node_create((void*)nodedata);
	if(node==RT_NULL)
	{
		rt_free(nodedata);
		rt_free(pinfo);
		return 1;
	}

	if( nodedata->attr & 0x2000 )  /*�ְ�����*/
	{
		nodedata->packetcount= (*( psrc + 12 ) << 8) | *( psrc + 13 );
		nodedata->packetno = (*( psrc + 14 ) << 8) | *( psrc + 15 );
/*���ǲ��ǵ�һ���ְ�*/
		flag_find=0;
		iter=list_jt808_rx->first;
		while(iter!=NULL)
		{
			iterdata=(JT808_RX_MSG_NODEDATA*)(iter->data);
			if(iterdata->id==nodedata->id)  /*�жϵ���ϢIDһ��,�������зְ�����ID��һ�£�Ҳ��Ϊ���°�*/
			{
				flag_find=1;
				break;
			}
			iter=iter->next;
		}
/*�ҵ������ǵ�һ���ְ���Ҫ�����еķְ�����*/		
		if(flag_find) /*�ҵ��� iter����ʼ����sibling������*/
		{
			if(iterdata->packetno<nodedata->packetno) /*��������,�ı������ϵĽڵ�*/
			{
				node->prev=iter->prev;
				node->next=iter->next; /*�滻ԭ����λ��*/
				node->sibling_dn=iter;
				iter->sibling_up=node;
			}
			else /*�ڶ����Ϣ�ķ�֧��*/
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
				if(flag_find==0) /*��βҲû���ҵ�*/
				{
					node->sibling_up=iter;
					iter->sibling_dn=node;
				}
			}
		}
		else	/*û�ҵ���û��ʹ��msglist_append(list_jt808_rx,nodedata);*/
		{
			if(list_jt808_rx->first==NULL) /*�ǵ�һ���ڵ�*/
			{
				list_jt808_rx->first->data=nodedata;
			}
			else /*���нڵ㣬��ӵ����*/
			{
				iter=iter->prev;  /*��ʱiterΪNULL,Ӧָ��ǰһ����Чnode*/
				iter->next=node; 
				node->prev=iter;
			}
		}
	}
	else /*���Ƿְ��ڵ㣬��ӵ�������Ϣ��list��,û��ʹ��msglist_append(list_jt808_rx,nodedata);*/
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
   ����״̬ά��
   jt808Э�鴦��

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


	pdev_gsm=rt_device_find("gsm");  /*û�г�����,δ�ҵ���ô��*/

/*��ȡ������������*/	

	while( 1 )
	{
/*����gprs��Ϣ*/
		ret = rt_mb_recv( &mb_gprsdata, (rt_uint32_t*)&pstr, 5 );
		if( ret == RT_EOK )
		{
			jt808_rx_analy(pstr);
			//rt_free( pstr );  /*���ܹ�����ͷ�*/
		}
/*����gps��Ϣ*/		
		ret = rt_mb_recv(&mb_gpsdata,(rt_uint32_t*)&pstr,5);
		if(ret == RT_EOK)
		{
			gps_analy(pstr);
			rt_free(pstr);
		}
/*ά����·*/
		rt_device_control(pdev_gsm,CTL_STATUS,&gsm_status);
	

#ifdef MULTI_PROCESS
/*�ദ��*/	
		{
			//msglist_foreach(list_jt808_tx,jt808_msg_tx_proc,RT_NULL);
			iter=list_jt808_tx->first;
			while(iter!=RT_NULL)
			{
				iter_next=iter->next;
				if(jt808_msg_tx_proc(RT_NULL,iter)==MSGLIST_RET_DELETE_NODE)/*�ýڵ��ѱ�ɾ��*/
				{
					iter=iter_next;
				}
			}
		}	
/*��������*/
#else
		jt808_msg_tx_proc(RT_NULL,list_jt808_tx->first);
	
#endif		
		rt_thread_delay( RT_TICK_PER_SECOND / 20 );
	}

	msglist_destroy(list_jt808_tx);
}

/*jt808�����̳߳�ʼ��*/
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

/*gps���մ���*/
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

/*gprs���մ���*/
void gprs_rx( uint8_t linkno,uint8_t * pinfo, uint16_t length )
{
	uint8_t *pmsg;
	pmsg = rt_malloc( length + 3 );      /*����������Ϣ*/
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
