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
#include <finsh.h>

#include "stm32f4xx.h"
#include "gsm.h"
#include "jt808.h"
#include "msglist.h"

#define MULTI_PROCESS




#define ByteSwap2(val)				\
  (((val & 0xff) << 8) |			\
    ((val & 0xff00) >> 8))

#define ByteSwap4(val)				\
  (((val & 0xff) << 24) |			\
    ((val & 0xff00) << 8) |		\
      ((val & 0xff0000) >> 8) |		\
        ((val & 0xff000000) >> 24))



typedef struct
{
	uint16_t id;
	int ( *func )( JT808_RX_MSG_NODEDATA* nodedata );
}HANDLE_JT808_RX_MSG;






static struct rt_mailbox	mb_gprsdata;
#define MB_GPRSDATA_POOL_SIZE 32
static uint8_t				mb_gprsdata_pool[MB_GPRSDATA_POOL_SIZE];

static struct rt_mailbox	mb_gpsdata;
#define MB_GPSDATA_POOL_SIZE 32
static uint8_t				mb_gpsdata_pool[MB_GPSDATA_POOL_SIZE];

uint32_t					jt808_alarm		= 0x0;
uint32_t					jt808_status	= 0x0;

static rt_device_t			pdev_gsm = RT_NULL;

/*ϵͳ����*/
JT808_PARAM jt808_param;

/*������Ϣ�б�*/
MsgList* list_jt808_tx;

/*������Ϣ�б�*/
MsgList* list_jt808_rx;



T_GPSINFO	gpsinfo;


/*
   $GNRMC,074001.00,A,3905.291037,N,11733.138255,E,0.1,,171212,,,A*655220.9*3F0E
   $GNTXT,01,01,01,ANTENNA OK*2B7,N,11733.138255,E,0.1,,171212,,,A*655220.9*3F0E
   $GNGGA,074002.00,3905.291085,N,11733.138264,E,1,11,0.9,8.2,M,-1.6,M,,,1.4*68E
   $GNGLL,3905.291085,N,11733.138264,E,074002.00,A,0*02.9,8.2,M,-1.6,M,,,1.4*68E
   $GPGSA,A,3,18,05,08,02,26,29,15,,,,,,,,,,,,,,,,,,,,,,,,,,1.6,0.9,1.4,0.9*3F8E
   $BDGSA,A,3,04,03,01,07,,,,,,,,,,,,,,,,,,,,,,,,,,,,,1.6,0.9,1.4,0.9*220.9*3F8E
   $GPGSV,2,1,7,18,10,278,29,05,51,063,32,08,21,052,24,02,24,140,45*4C220.9*3F8E
   $GPGSV,2,2,7,26,72,055,24,29,35,244,37,15,66,224,37*76,24,140,45*4C220.9*3F8E
   $BDGSV,1,1,4,04,27,124,38,03,42,190,34,01,38,146,37,07,34,173,35*55220.9*3F8E

   ���ش�����ֶ����������ȷ�Ļ�
 */
uint8_t process_rmc( uint8_t *pinfo )
{
	//�������������,ִ������ת��
	uint8_t		CommaCount = 0, iCount = 0;
	char		tmpinfo[16];
	uint8_t		year = 0, mon = 0, day = 0, hour = 0, min = 0, sec = 0, fDateModify = 0;
	uint32_t		degrees, minutes;
	uint16_t	i = 0, j = 0;
	uint32_t	t32;
	uint8_t count;

	uint8_t gps_time[10];
	uint8_t gps_av=0;
	uint8_t gps_ns=0;
	uint8_t gps_ew=0;
	uint8_t gps_latitude[16];
	uint8_t gps_longitude[16];
	uint8_t gps_speed[8];
	uint8_t gps_direct[8];
	uint8_t gps_date[8];

	uint8_t *psrc=pinfo+7; 	//ָ��ʼλ��
/*ʱ�䴦�� */
	count=0;
	while((*psrc!=',')&&(count<10))
	{
		gps_time[count++]=*psrc;
		gps_time[count]=0;
		psrc++;
	}
	if((count==0)||(count==10)) return 0;
	hour=(gps_time[0]-0x30)*10+(gps_time[1]-0x30)+8;
	min=(gps_time[2]-0x30)*10+(gps_time[3]-0x30)+8;
	sec=(gps_time[4]-0x30)*10+(gps_time[5]-0x30)+8;
	if(hour>23){
		fDateModify=1;
		hour-=24;
	}
	gpsinfo.datetime[3]=((hour/10)<<4)|(hour%10);
	gpsinfo.datetime[4]=((min/10)<<4)|(min%10);
	gpsinfo.datetime[5]=((sec/10)<<4)|(sec%10);
/*A_V����*/	
	psrc++;
	if((*psrc=='A')||(*psrc=='V')) gps_av=*psrc;
	else return 1;
	if(gps_av=='A') jt808_status&=~0x01;
	else  jt808_status|=0x01;
/*γ�ȴ���ddmm.mmmmmm*/
	psrc++;
	count=0;
	while((*psrc!=',')&&(count<11))
	{
		gps_latitude[count++]=*psrc;
		gps_latitude[count]=0;
		psrc++;
	}
	if(count==0) return 2;
	degrees=((gps_latitude[0]-0x30)*10+(gps_latitude[1]-0x30))*60*100000;
	minutes=(gps_latitude[2]-0x30)*1000000+
			(gps_latitude[3]-0x30)*100000+
			(gps_latitude[5]-0x30)*10000+
			(gps_latitude[6]-0x30)*1000+
			(gps_latitude[7]-0x30)*100+
			(gps_latitude[8]-0x30)*10+
			(gps_latitude[9]-0x30);

	gpsinfo.latitude=ByteSwap4(degrees+minutes);
	
/*N_S����*/	
	psrc++;
	if((*psrc=='N')||(*psrc=='S')) gps_ns=*psrc;
	else return 3;	
	if(gps_ns=='N') jt808_status&=~0x02;
	else jt808_status|=0x02;
	
/*���ȴ���*/
	psrc++;
	count=0;
	while((*psrc!=',')&&(count<12))
	{
		gps_longitude[count++]=*psrc;
		gps_longitude[count]=0;
		psrc++;
	}
	if(count==0) return 4;
	degrees=((gps_latitude[0]-0x30)*100+(gps_latitude[1]-0x30)*10+(gps_latitude[2]-0x30))*60*100000;
	minutes=(gps_latitude[3]-0x30)*1000000+
			(gps_latitude[4]-0x30)*100000+
			(gps_latitude[6]-0x30)*10000+
			(gps_latitude[7]-0x30)*1000+
			(gps_latitude[8]-0x30)*100+
			(gps_latitude[9]-0x30)*10+
			(gps_latitude[10]-0x30);
	gpsinfo.longitude=ByteSwap4(degrees+minutes);
/*N_S����*/ 
	psrc++;
	if((*psrc=='E')||(*psrc=='W')) gps_ew=*psrc;
	else return 5;	
	if(gps_ew=='E') jt808_status&=~0x04;
	else jt808_status|=0x04;

/*�ٶȴ���*/
	psrc++;
	count=0;
	while((*psrc!=',')&&(count<7))
	{
		gps_speed[count++]=*psrc;
		gps_speed[count]=0;
		psrc++;
	}
	if(count==0) return 6;
	
	
/*������*/
	psrc++;
	count=0;
	while((*psrc!=',')&&(count<7))
	{
		gps_direct[count++]=*psrc;
		gps_direct[count]=0;
		psrc++;
	}
	if(count==0) return 7;
		
/*���ڴ���*/
	psrc++;
	count=0;
	while((*psrc!=',')&&(count<7))
	{
		gps_date[count++]=*psrc;
		gps_date[count]=0;
		psrc++;
	}
	if(count==0) return 8;
	day = (( gps_date[0] - 0x30 ) * 10 ) + ( gps_date[1] - 0x30 );
	mon = (( gps_date[2] - 0x30 ) * 10 ) + ( gps_date[3] - 0x30 );
	year = (( gps_date[4] - 0x30 ) * 10 ) + ( gps_date[5] - 0x30 );
	
	if(fDateModify){
		day++;
		if(mon == 2){
			if ( ( year % 4 ) == 0 ){
				if ( day == 30 ){day = 1;mon++;}
			}else if ( day == 29 ){ day = 1;mon++;}
		}else if (( mon == 4 ) || ( mon == 6 ) || ( mon == 9 ) || ( mon == 11 )){
			if ( day == 31 ){mon++;day = 1;}
		}else{
			if ( day == 32 ){mon++;day = 1;}	
			if( mon == 13 ){mon = 1;year++; }
		}
	}
	gpsinfo.datetime[0]=((year/10)<<4)|(year%10);
	gpsinfo.datetime[1]=((mon/10)<<4)|(mon%10);
	gpsinfo.datetime[2]=((day/10)<<4)|(day%10);
	return 0;
}

void process_gga( uint8_t *pinfo )
{


}


/***********************************************************
* Function:
* Description:gps�յ���Ϣ��Ĵ���ͷ�����ֽ�Ϊ����
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
void gps_analy( uint8_t * pinfo )
{
	uint16_t	len;
	uint8_t		*psrc;
	len		= ( pinfo[0] << 8 ) | pinfo[1];
	psrc	= pinfo + 2;
	if( ( strncmp( psrc, "$GPRMC,", 7 ) == 0 ) || ( strncmp( psrc, "$GNRMC,", 7 ) == 0 )|| ( strncmp( psrc, "$BDRMC,", 7 ) == 0 ) )
	{
		process_rmc( psrc );
	}
	if( ( strncmp( psrc, "$GPGGA,", 7 ) == 0 ) || ( strncmp( psrc, "$GNGGA,", 7 ) == 0 )|| ( strncmp( psrc, "$BDGGA,", 7 ) == 0 ) )
	{
		process_gga( psrc );
	}
	
}





/*
   jt808��ʽ���ݽ����ж�
   <��ʶ0x7e><��Ϣͷ><��Ϣ��><У����><��ʶ0x7e>

   ������Ч�����ݳ���,Ϊ0 �����д�

 */
static uint16_t jt808_decode_fcs( uint8_t *pinfo, uint16_t length )
{
	uint8_t		*psrc, *pdst;
	uint16_t	count, len;
	uint8_t		fstuff	= 0; /*�Ƿ��ֽ����*/
	uint8_t		fcs		= 0;

	if( length < 5 )
	{
		return 0;
	}
	if( *pinfo != 0x7e )
	{
		return 0;
	}
	if( *( pinfo + length - 1 ) != 0x7e )
	{
		return 0;
	}
	psrc	= pinfo + 1;    /*1byte��ʶ��Ϊ��ʽ��Ϣ*/
	pdst	= pinfo;
	count	= 0;            /*ת���ĳ���*/
	len		= length - 2;   /*ȥ����ʶλ�����ݳ���*/

	while( len )
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
		}else
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
		rt_kprintf( "%s>fcs error\r\n", __func__ );
		return 0;
	}
	rt_kprintf( "count=%d\r\n",count);
	return count;
}


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

/*ƽ̨ͨ��Ӧ��,�յ���Ϣ��ֹͣ����*/
static int handle_jt808_msg_0x8001( JT808_RX_MSG_NODEDATA* nodedata )
{
	MsgListNode * iter;
	JT808_TX_MSG_NODEDATA *iterdata;
	MsgListNode * iter_tmp;
	
	uint8_t		* msg = nodedata->pmsg;
	uint16_t	id;
	uint16_t	seq;
	uint8_t		res;

	seq = ( *msg << 8 ) | *( msg + 1 );
	id	= ( *( msg + 2 ) << 8 ) | *( msg + 3 );
	res = *( msg + 4 );

	switch( id )        // �ж϶�Ӧ�ն���Ϣ��ID�����ִ���
	{
		case 0x0200:    //	��Ӧλ����Ϣ��Ӧ��
			rt_kprintf( "\r\nCentre ACK!\r\n" );
			break;
		case 0x0002:    //	��������Ӧ��
			rt_kprintf( "\r\n  Centre  Heart ACK!\r\n" );
			break;
		case 0x0101:    //	�ն�ע��Ӧ��
			break;
		case 0x0102:    //	�ն˼�Ȩ
			break;
		case 0x0800:    // ��ý���¼���Ϣ�ϴ�
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

#ifdef MULTI_PROCESS
	iter = list_jt808_tx->first;
	while( iter != RT_NULL )
	{
		iterdata=iter->data;

		if((iterdata->msg_id==id)&&(iterdata->msg_sn==seq))
		{
			iter->prev->next=iter->next;
			iter->next->prev=iter->prev;
			rt_free(iterdata->pmsg);
			rt_free(iterdata);
			rt_free(iter);
			break;
		}
	}
	/*��������*/
#else
	iter = list_jt808_tx->first;
	if((iterdata->msg_id==id)&&(iterdata->msg_sn==seq))
	{
		iter->prev->next=iter->next;
		iter->next->prev=iter->prev;
		rt_free(iterdata->pmsg);
		rt_free(iterdata);
		rt_free(iter);
	}

#endif
}

/* ������Ķ��ն�ע����Ϣ��Ӧ��*/
static int handle_jt808_msg_0x8100( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/*�����ն˲���*/
static int handle_jt808_msg_0x8103( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/*��ѯ�ն˲���*/
static int handle_jt808_msg_0x8104( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/*�ն˿���*/
static int handle_jt808_msg_0x8105( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/**/
static int handle_jt808_msg_0x8201( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/**/
static int handle_jt808_msg_0x8202( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/**/
static int handle_jt808_msg_0x8300( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/**/
static int handle_jt808_msg_0x8301( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/**/
static int handle_jt808_msg_0x8302( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/**/
static int handle_jt808_msg_0x8303( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/**/
static int handle_jt808_msg_0x8304( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/**/
static int handle_jt808_msg_0x8400( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/**/
static int handle_jt808_msg_0x8401( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/**/
static int handle_jt808_msg_0x8500( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/**/
static int handle_jt808_msg_0x8600( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/**/
static int handle_jt808_msg_0x8601( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/**/
static int handle_jt808_msg_0x8602( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/**/
static int handle_jt808_msg_0x8603( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/**/
static int handle_jt808_msg_0x8604( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/**/
static int handle_jt808_msg_0x8605( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/**/
static int handle_jt808_msg_0x8606( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/**/
static int handle_jt808_msg_0x8607( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/**/
static int handle_jt808_msg_0x8700( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/**/
static int handle_jt808_msg_0x8701( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/**/
static int handle_jt808_msg_0x8800( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/**/
static int handle_jt808_msg_0x8801( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/**/
static int handle_jt808_msg_0x8802( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/**/
static int handle_jt808_msg_0x8803( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/**/
static int handle_jt808_msg_0x8804( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/**/
static int handle_jt808_msg_0x8805( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/**/
static int handle_jt808_msg_0x8900( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

/**/
static int handle_jt808_msg_0x8A00( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
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
static int handle_jt808_msg_default( JT808_RX_MSG_NODEDATA* nodedata )
{
	return 1;
}

#define DECL_RX_MSG_HANDLE( a ) { a, handle_jt808_msg_ ## a }

HANDLE_JT808_RX_MSG handle_jt808_rx_msg[] =
{
	DECL_RX_MSG_HANDLE( 0x8001 ),   //	ͨ��Ӧ��
	DECL_RX_MSG_HANDLE( 0x8100 ),   //  ������Ķ��ն�ע����Ϣ��Ӧ��
	DECL_RX_MSG_HANDLE( 0x8103 ),   //	�����ն˲���
	DECL_RX_MSG_HANDLE( 0x8104 ),   //	��ѯ�ն˲���
	DECL_RX_MSG_HANDLE( 0x8105 ),   // �ն˿���
	DECL_RX_MSG_HANDLE( 0x8201 ),   // λ����Ϣ��ѯ    λ����Ϣ��ѯ��Ϣ��Ϊ��
	DECL_RX_MSG_HANDLE( 0x8202 ),   // ��ʱλ�ø��ٿ���
	DECL_RX_MSG_HANDLE( 0x8300 ),   //	�ı���Ϣ�·�
	DECL_RX_MSG_HANDLE( 0x8301 ),   //	�¼�����
	DECL_RX_MSG_HANDLE( 0x8302 ),   // �����·�
	DECL_RX_MSG_HANDLE( 0x8303 ),   //	��Ϣ�㲥�˵�����
	DECL_RX_MSG_HANDLE( 0x8304 ),   //	��Ϣ����
	DECL_RX_MSG_HANDLE( 0x8400 ),   //	�绰�ز�
	DECL_RX_MSG_HANDLE( 0x8401 ),   //	���õ绰��
	DECL_RX_MSG_HANDLE( 0x8500 ),   //	��������
	DECL_RX_MSG_HANDLE( 0x8600 ),   //	����Բ������
	DECL_RX_MSG_HANDLE( 0x8601 ),   //	ɾ��Բ������
	DECL_RX_MSG_HANDLE( 0x8602 ),   //	���þ�������
	DECL_RX_MSG_HANDLE( 0x8603 ),   //	ɾ����������
	DECL_RX_MSG_HANDLE( 0x8604 ),   //	���������
	DECL_RX_MSG_HANDLE( 0x8605 ),   //	ɾ���������
	DECL_RX_MSG_HANDLE( 0x8606 ),   //	����·��
	DECL_RX_MSG_HANDLE( 0x8607 ),   //	ɾ��·��
	DECL_RX_MSG_HANDLE( 0x8700 ),   //	�г���¼�����ݲɼ�����
	DECL_RX_MSG_HANDLE( 0x8701 ),   //	��ʻ��¼�ǲ����´�����
	DECL_RX_MSG_HANDLE( 0x8800 ),   //	��ý�������ϴ�Ӧ��
	DECL_RX_MSG_HANDLE( 0x8801 ),   //	����ͷ��������
	DECL_RX_MSG_HANDLE( 0x8802 ),   //	�洢��ý�����ݼ���
	DECL_RX_MSG_HANDLE( 0x8803 ),   //	�洢��ý�������ϴ�����
	DECL_RX_MSG_HANDLE( 0x8804 ),   //	¼����ʼ����
	DECL_RX_MSG_HANDLE( 0x8805 ),   //	�����洢��ý�����ݼ����ϴ����� ---- ����Э��Ҫ��
	DECL_RX_MSG_HANDLE( 0x8900 ),   //	��������͸��
	DECL_RX_MSG_HANDLE( 0x8A00 ),   //	ƽ̨RSA��Կ
};





/*
   ���մ���
   ����jt808��ʽ������
   <linkno><����2byte><��ʶ0x7e><��Ϣͷ><��Ϣ��><У����><��ʶ0x7e>

 */
uint16_t jt808_rx_proc( uint8_t * pinfo )
{
	uint8_t					*psrc;
	uint16_t				len;
	uint8_t					linkno;
	uint16_t				i;
	uint8_t					flag_find = 0;

	uint16_t				ret;

	MsgListNode				* node;
	JT808_RX_MSG_NODEDATA	* nodedata;

	MsgListNode				* iter;
	JT808_RX_MSG_NODEDATA	* iterdata;

	MsgListNode				* iter_tmp;
	JT808_RX_MSG_NODEDATA	* iterdata_tmp;

	linkno	= pinfo[0];
	len		= ( pinfo[1] << 8 ) | pinfo[2];

	len = jt808_decode_fcs( pinfo + 3, len );
	if( len == 0 )              /*��ʽ����ȷ*/
	{
		rt_free( pinfo );
		return 1;
	}
	
	nodedata = rt_malloc( sizeof( JT808_RX_MSG_NODEDATA ) );
	if( nodedata == RT_NULL )   /*�޷��������Ϣ*/
	{
		rt_free( pinfo );
		return 1;
	}
	
	psrc				= pinfo; /*ע�⿪ʼ��linkno len*/
	nodedata->linkno	= psrc[0];
	nodedata->id		= ( *( psrc + 3 ) << 8 ) | *( psrc + 4 );
	nodedata->attr		= ( *( psrc + 5 ) << 8 ) | *( psrc + 6 );
	memcpy( nodedata->mobileno, psrc + 7, 6 );
	nodedata->seq		= ( *( psrc + 13 ) << 8 ) | *( psrc + 14 );
	nodedata->pmsg		= pinfo;  /*��Ч����Ϣ������attr�ֶ�ָʾ*/
	nodedata->msg_len	= nodedata->attr&0x3ff;
	nodedata->tick		= rt_tick_get( ); /*�յ���ʱ��*/

	rt_kprintf("attr=%x\r\n",nodedata->attr);
	rt_kprintf("pinfo=\r\n");
	for(i=0;i<len;i++) rt_kprintf("%02x ",*(pinfo+i));
	rt_kprintf("\r\n");

/* �������ݴ���,����Ҫ����MsgNode */
	if( nodedata->attr & 0x2000 == 0 )
	{
		for( i = 0; i < sizeof( handle_jt808_rx_msg ) / sizeof( HANDLE_JT808_RX_MSG ); i++ )
		{
			if( nodedata->id == handle_jt808_rx_msg[i].id )
			{
				handle_jt808_rx_msg[i].func( nodedata );
				flag_find = 1;
			}
		}
		if( !flag_find )
		{
			handle_jt808_msg_default( nodedata );
		}
		rt_free( pinfo );
		rt_free( nodedata );
	}
/*����Ƿ��г�ʱû�д������Ϣ����Ҫ�Ƕ����Ϣ*/


	iter		= list_jt808_rx->first;
	flag_find	= 0;
	while( iter != NULL )
	{
		iterdata = iter->data;
		if( rt_tick_get( ) - iterdata->tick > RT_TICK_PER_SECOND * 10 ) /*����10��û�����ݰ�*/
		{
			/*����һ�£�׼��ɾ���ýڵ㴮*/
			if( iter->prev == NULL )                                    /*����*/
			{
				list_jt808_rx->first = iter->next;
			}else
			{
				iter->prev->next = iter->next;
				if( iter->next != RT_NULL )
				{
					iter->next->prev = iter->prev;
				}
			}

			while( iter != NULL )
			{
				iter_tmp	= iter->next;
				iterdata	= iter->data;
				rt_free( iterdata->pmsg );
				rt_free( iterdata );
				rt_free( iter );
				iter = iter_tmp;
			}
		}
	}
/*���Ƕ��,����*/
	if( nodedata->attr & 0x2000 == 0 )
	{
		return 0;
	}

/*�ְ�����,�����µĽڵ�*/
	node = msglist_node_create( (void*)nodedata );
	if( node == RT_NULL )
	{
		rt_free( nodedata );
		rt_free( pinfo );
		return 1;
	}
	nodedata->packetcount	= ( *( psrc + 12 ) << 8 ) | *( psrc + 13 );
	nodedata->packetno		= ( *( psrc + 14 ) << 8 ) | *( psrc + 15 );
/*���ǲ��ǵ�һ���ְ�*/
	flag_find	= 0;
	iter		= list_jt808_rx->first;
	while( iter != NULL )
	{
		iterdata = (JT808_RX_MSG_NODEDATA*)( iter->data );
		if( iterdata->id == nodedata->id ) /*�жϵ���ϢIDһ��,�������зְ�����ID��һ�£�Ҳ��Ϊ���°�*/
		{
			flag_find = 1;
			break;
		}
		iter = iter->next;
	}
/*�ҵ������ǵ�һ���ְ���Ҫ�����еķְ�����*/
	if( flag_find )                                     /*�ҵ��� iter����ʼ����sibling������*/
	{
		if( iterdata->packetno < nodedata->packetno )   /*��������,�ı������ϵĽڵ�*/
		{
			node->prev			= iter->prev;
			node->next			= iter->next;           /*�滻ԭ����λ��*/
			node->sibling_dn	= iter;
			iter->sibling_up	= node;
		}else /*�ڶ����Ϣ�ķ�֧��*/
		{
			flag_find = 0;
			while( iter->sibling_dn != NULL )
			{
				iter		= iter->sibling_dn;
				iterdata	= iter->data;
				if( iterdata->packetno < nodedata->packetno )
				{
					node->sibling_up	= iter->sibling_up;
					node->sibling_dn	= iter;
					iter->sibling_up	= node;
					flag_find			= 1;
					break;
				}
			}
			if( flag_find == 0 ) /*��βҲû���ҵ�*/
			{
				node->sibling_up	= iter;
				iter->sibling_dn	= node;
			}
		}
	}else /*û�ҵ���û��ʹ��msglist_append(list_jt808_rx,nodedata);*/
	{
		if( list_jt808_rx->first == NULL )  /*�ǵ�һ���ڵ�*/
		{
			list_jt808_rx->first->data = nodedata;
		}else /*���нڵ㣬��ӵ����*/
		{
			iter		= iter->prev;       /*��ʱiterΪNULL,Ӧָ��ǰһ����Чnode*/
			iter->next	= node;
			node->prev	= iter;
		}
	}
}

/*
��Ϣ���ͳ�ʱ
*/
static rt_err_t	jt808_tx_proc_timeout()
{

}


/*
   ����ÿ��Ҫ������Ϣ��״̬
   ���������д�����?
 */
static MsgListRet jt808_tx_proc( MsgListNode* node )
{
	MsgListNode				* pnode		= (MsgListNode*)node;
	JT808_TX_MSG_NODEDATA	* pnodedata = (JT808_TX_MSG_NODEDATA*)( pnode->data );

	if (node==NULL) return MSGLIST_RET_OK;
	if( pnodedata->state == IDLE )                      /*���У�������Ϣ��ʱ��û������*/
	{
		if( pnodedata->retry == pnodedata->max_retry )  /*�Ѿ��ﵽ���Դ���*/
		{
			/*��ʾ����ʧ��*/
			rt_free( pnodedata->pmsg );                 /*ɾ���ڵ�����*/
			pnode->prev->next	= pnode->next;          /*ɾ���ڵ�*/
			pnode->next->prev	= pnode->prev;
			msglist_node_destroy( pnode );
			return MSGLIST_RET_DELETE_NODE;
		}else
		{
			pnodedata->retry++;
			rt_device_write( pdev_gsm, 0, pnodedata->pmsg, pnodedata->msg_len );
			pnodedata->tick		= rt_tick_get( );
			pnodedata->timeout	= pnodedata->max_retry * pnodedata->timeout;
			pnodedata->state	= WAIT_ACK;
			rt_kprintf( "send retry=%d,timeout=%d\r\n", pnodedata->retry, pnodedata->timeout );
		}
		return MSGLIST_RET_OK;
	}

	if( pnodedata->state == WAIT_ACK )
	{
		if( rt_tick_get( ) - pnodedata->tick > pnodedata->timeout )
		{
			pnodedata->state = IDLE;
		}
	}

	if( pnodedata->state == ACK_OK)	/*�յ�ACK���ڽ�������λ*/
	{
		//pnodedata->
	}

	
	return MSGLIST_RET_OK;
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

	MsgListNode * iter;
	MsgListNode * iter_next;

	list_jt808_tx	= msglist_create( );
	list_jt808_rx	= msglist_create( );

	pdev_gsm = rt_device_find( "gsm" ); /*û�г�����,δ�ҵ���ô��*/

/*��ȡ������������*/

	while( 1 )
	{
/*����gps��Ϣ*/
		ret = rt_mb_recv( &mb_gpsdata, (rt_uint32_t*)&pstr, 5 );
		if( ret == RT_EOK )
		{
			gps_analy( pstr );
			rt_free( pstr );
		}
/*����gprs��Ϣ*/
		ret = rt_mb_recv( &mb_gprsdata, (rt_uint32_t*)&pstr, 5 );
		if( ret == RT_EOK )
		{
			jt808_rx_proc( pstr );
		}
/*ά����·*/
		rt_device_control( pdev_gsm, CTL_STATUS, &gsm_status );

#ifdef MULTI_PROCESS  /*�ദ��*/
		iter = list_jt808_tx->first;
		while( iter != RT_NULL )
		{
			iter_next = iter->next;                                     /*�ȱ���,�Է��ڵ㱻ɾ��*/
			if( jt808_tx_proc( iter ) == MSGLIST_RET_DELETE_NODE )  /*�ýڵ��ѱ�ɾ��*/
			{
				iter = iter_next;
			}
		}
#else  /*��������*/
		jt808_tx_proc( list_jt808_tx->first );

#endif
		rt_thread_delay( RT_TICK_PER_SECOND / 20 );
	}

	msglist_destroy( list_jt808_tx );
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
void gps_rx( uint8_t *pinfo, uint16_t length )
{
	uint8_t *pmsg;
	pmsg = rt_malloc( length + 2 );
	if( pmsg != RT_NULL )
	{
		pmsg[0] = length >> 8;
		pmsg[1] = length & 0xff;
		memcpy( pmsg + 2, pinfo, length );
		rt_mb_send( &mb_gpsdata, (rt_uint32_t)pmsg );
	}
}

/*gprs���մ���*/
rt_err_t gprs_rx( uint8_t linkno, uint8_t * pinfo, uint16_t length )
{
	uint8_t *pmsg;
	pmsg = rt_malloc( 20 + 3 ); /*����������Ϣ*/
	//pmsg = rt_malloc( length + 3 ); /*����������Ϣ*/
	if( pmsg != RT_NULL )
	{
		//pmsg[0] = linkno;
		//pmsg[1] = length >> 8;
		//pmsg[2] = length & 0xff;
		//memcpy( pmsg + 3, pinfo, length );
		
		pmsg[0] = 0;
		pmsg[1] = 0;
		pmsg[2] = 20;
		memcpy(pmsg+3,"\x7e\x80\x01\x00\x05\x01\x39\x20\x61\x41\x00\x00\x01\x00\x01\x80\x20\x00\x1c\x7e",20);
		rt_mb_send( &mb_gprsdata, (rt_uint32_t)pmsg );
		return 0;
	}
	return 1;
}

FINSH_FUNCTION_EXPORT( gprs_rx, simlute gprs rx );



rt_err_t gprs_tx( uint8_t linkno, uint8_t * pinfo, uint16_t length )
{
	uint8_t *pmsg;
	pmsg = rt_malloc( length + 3 ); /*����������Ϣ*/
	if( pmsg != RT_NULL )
	{
		pmsg[0] = linkno;
		pmsg[1] = length >> 8;
		pmsg[2] = length & 0xff;
		memcpy( pmsg + 3, pinfo, length );
		rt_mb_send( &mb_gprsdata, (rt_uint32_t)pmsg );
		return 0;
	}
	return 1;
}

FINSH_FUNCTION_EXPORT( gprs_tx, simlute gprs tx );




/************************************** The End Of File **************************************/

