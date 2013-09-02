/************************************************************
 * Copyright (C), 2008-2012,
 * FileName:		//  jt808_sms.c
 * Author:			//  baiyangmin
 * Date:			//  2013-07-08
 * Description:		//  ���Ŵ������ͣ����գ��޸Ĳ����ȹ���
 * Version:			//  V0.01
 * Function List:	//  ��Ҫ�������书��
 *     1. -------
 * History:			//  ��ʷ�޸ļ�¼
 *     <author>  <time>   <version >   <desc>
 *     David    96/10/12     1.0     build this moudle
 ***********************************************************/

#include <rtthread.h>
#include <rthw.h>
#include "stm32f4xx.h"
#include <finsh.h>

#include  <stdlib.h> //����ת�����ַ���
#include  <stdio.h>
#include  <string.h>
#include  "jt808_sms.h"
#include "jt808.h"
#include "jt808_param.h"
#include "jt808_gps.h"
#include "m66.h"
#include "gps.h"

#define HALF_BYTE_SWAP( a ) ( ( ( ( a ) & 0x0f ) << 4 ) | ( ( a ) >> 4 ) )
#define GSM_7BIT	0x00
#define GSM_UCS2	0x08

static const char	sms_gb_data[]	= "������������ش�������������ɽ��ʹ����������³���������ԥ";
static uint16_t		sms_ucs2[31]	= { 0x5180, 0x4EAC, 0x6D25, 0x6CAA, 0x5B81, 0x6E1D, 0x743C, 0x85CF, 0x5DDD, 0x7CA4, 0x9752,
	                                    0x8D35,		   0x95FD, 0x5409, 0x9655, 0x8499, 0x664B, 0x7518, 0x6842, 0x9102, 0x8D63,
	                                    0x6D59,		   0x82CF, 0x65B0, 0x9C81, 0x7696, 0x6E58, 0x9ED1, 0x8FBD, 0x4E91, 0x8C6B, };

static char			sender[32];
static char			smsc[32];

/**/
u16  gsmencode8bit( const char *pSrc, char *pDst, u16 nSrcLength )
{
	u16 m;
	// �򵥸���
	for( m = 0; m < nSrcLength; m++ )
	{
		*pDst++ = *pSrc++;
	}

	return nSrcLength;
}

/**/
static uint16_t ucs2_to_gb2312( uint16_t uni )
{
	uint8_t i;
	for( i = 0; i < 31; i++ )
	{
		if( uni == sms_ucs2[i] )
		{
			return ( sms_gb_data[i * 2] << 8 ) | sms_gb_data[i * 2 + 1];
		}
	}
	return 0xFF20;
}

/**/
static uint16_t gb2312_to_ucs2( uint16_t gb )
{
	uint8_t i;
	for( i = 0; i < 31; i++ )
	{
		if( gb == ( ( sms_gb_data[i * 2] << 8 ) | sms_gb_data[i * 2 + 1] ) )
		{
			return sms_ucs2[i];
		}
	}
	return 0;
}

/*ucs2�����ı���*/
u16 gsmdecodeucs2( char* pSrc, char* pDst, u16 nSrcLength )
{
	u16			nDstLength = 0; // UNICODE���ַ���Ŀ
	u16			i;
	uint16_t	uni, gb;
	char		* src	= pSrc;
	char		* dst	= pDst;

	for( i = 0; i < nSrcLength; i += 2 )
	{
		uni = ( src[i] << 8 ) | src[i + 1];
		if( uni > 0x80 ) /**/
		{
			gb			= ucs2_to_gb2312( uni );
			*dst++		= ( gb >> 8 );
			*dst++		= ( gb & 0xFF );
			nDstLength	+= 2;
		}else /*Ҳ��˫�ֽڵ� ���� '1' ==> 0031*/
		{
			*dst++ = uni;
			nDstLength++;
		}
	}
	return nDstLength;
}

/**/
u16 gsmencodeucs2( u8* pSrc, u8* pDst, u16 nSrcLength )
{
	uint16_t	len = nSrcLength;
	uint16_t	gb, ucs2;
	uint8_t		*src	= pSrc;
	uint8_t		*dst	= pDst;

	while( len )
	{
		gb = *src++;
		if( gb > 0x7F )
		{
			gb		|= *src++;
			ucs2	= gb2312_to_ucs2( gb ); /*�п���û���ҵ�����00*/
			*dst++	= ( ucs2 >> 8 );
			*dst++	= ( ucs2 & 0xFF );
			len		-= 2;
		}else
		{
			*dst++	= 0x00;
			*dst++	= ( gb & 0xFF );
			len--;
		}
	}
	// ����Ŀ����봮����
	return ( nSrcLength << 1 );
}

/**/
u16 gsmdecode7bit( char* pSrc, char* pDst, u16 nSrcLength )
{
	u16 nSrc;   // Դ�ַ����ļ���ֵ
	u16 nDst;   // Ŀ����봮�ļ���ֵ
	u16 nByte;  // ��ǰ���ڴ���������ֽڵ���ţ���Χ��0-6
	u8	nLeft;  // ��һ�ֽڲ��������

	// ����ֵ��ʼ��
	nSrc	= 0;
	nDst	= 0;

	// �����ֽ���źͲ������ݳ�ʼ��
	nByte	= 0;
	nLeft	= 0;

	// ��Դ����ÿ7���ֽڷ�Ϊһ�飬��ѹ����8���ֽ�
	// ѭ���ô�����̣�ֱ��Դ���ݱ�������
	// ������鲻��7�ֽڣ�Ҳ����ȷ����
	while( nSrc < nSrcLength )
	{
		*pDst	= ( ( *pSrc << nByte ) | nLeft ) & 0x7f;    // ��Դ�ֽ��ұ߲��������������ӣ�ȥ�����λ���õ�һ��Ŀ������ֽ�
		nLeft	= *pSrc >> ( 7 - nByte );                   // �����ֽ�ʣ�µ���߲��֣���Ϊ�������ݱ�������
		pDst++;                                             // �޸�Ŀ�괮��ָ��ͼ���ֵ
		nDst++;
		nByte++;                                            // �޸��ֽڼ���ֵ
		if( nByte == 7 )                                    // ����һ������һ���ֽ�
		{
			*pDst = nLeft;                                  // ����õ�һ��Ŀ������ֽ�
			pDst++;                                         // �޸�Ŀ�괮��ָ��ͼ���ֵ
			nDst++;
			nByte	= 0;                                    // �����ֽ���źͲ������ݳ�ʼ��
			nLeft	= 0;
		}
		pSrc++;                                             // �޸�Դ����ָ��ͼ���ֵ
		nSrc++;
	}
	*pDst = 0;
	return nDst;                                            // ����Ŀ�괮����
}


/*��Ϣ����Ϊ7bit pduģʽ,������*/


/***********************************************************
* Function:
* Description:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
void sms_encode_pdu_7bit( char* info )
{
#define BITMASK_7BITS 0x7F

	char			buf[200];
	uint16_t		len;
	char			*pSrc, *pDst;
	unsigned char	c;
	char			ASC[]	= "0123456789ABCDEF";
	uint8_t			count	= 0;

	unsigned char	nSrc	= 0, nDst = 0, nChar = 0;
	uint8_t			nLeft	= 0;

	len = strlen( info );
	rt_kprintf( "\nSMS>ENCODE(%d):%s", len, info );

	//strcpy( buf, "0891683108200205F01100" );
	strcpy( buf, "001100" );
	count = strlen( sender );
	strcat( buf, sender );
	strcat( buf, "000000" );                                //	7bit����

	pSrc	= info;
	pDst	= buf + ( count + 12 + 2 );                     //sender����+12ASC+2Ԥ��TP����

	while( nSrc < ( len + 1 ) )
	{
		nChar = nSrc & 0x07;                                // ȡԴ�ַ����ļ���ֵ�����3λ
		if( nChar == 0 )                                    // ����Դ����ÿ���ֽ�
		{
			nLeft = *pSrc;                                  // ���ڵ�һ���ֽڣ�ֻ�Ǳ�����������������һ���ֽ�ʱʹ��
		}else
		{
			c		= ( *pSrc << ( 8 - nChar ) ) | nLeft;   // ���������ֽڣ������ұ߲��������������ӣ��õ�һ��Ŀ������ֽ�
			*pDst++ = ASC[c >> 4];
			*pDst++ = ASC[c & 0x0f];
			nLeft	= *pSrc >> nChar;                       // �����ֽ�ʣ�µ���߲��֣���Ϊ�������ݱ�������
			nDst++;
		}
		pSrc++;
		nSrc++;                                             // �޸�Դ����ָ��ͼ���ֵ
	}
	*pDst = 0;
	//buf[count + 28] = ASC[len >> 4];
	//buf[count + 29] = ASC[len & 0x0F];
	buf[count + 12] = ASC[len >> 4];
	buf[count + 13] = ASC[len & 0x0F];

	rt_kprintf( "\nSMS>encode 7BIT:%s", buf );
	//sms_tx( buf, ( strlen( buf ) - 18 ) >> 1 );
	sms_tx( buf, ( strlen( buf ) - 2 ) >> 1 );
}

/*��Ϣ����Ϊuscs2 pduģʽ,������*/

void sms_encode_pdu_ucs2( char* info )
{
	char		buf[400];
	uint16_t	len, gb, ucs2;
	char		*pSrc, *pDst;
	char		ASC[]	= "0123456789ABCDEF";
	uint8_t		count	= 0;
	uint8_t		TP_UL	= 0;

	len = strlen( info );
	rt_kprintf( "\nSMS>ENCODE(%d):%s", len, info );

//	strcpy(buf,smsc,strlen(smsc));
	strcpy( buf, "001100" );
	count = strlen( sender );
	strcat( buf, sender );
	strcat( buf, "00088F" );
//	ucs2bit����
	pSrc	= info;
	pDst	= buf + ( count + 12 + 2 ); //sender����+12ASC+2Ԥ��TP����
	while( *pSrc )
	{
		gb = *pSrc++;
		if( gb > 0x7F )
		{
			gb		<<= 8;
			gb		|= *pSrc++;
			ucs2	= gb2312_to_ucs2( gb ); /*�п���û���ҵ�����00*/
			*pDst++ = ASC[( ucs2 & 0xF000 ) >> 12];
			*pDst++ = ASC[( ucs2 & 0x0F00 ) >> 8];
			*pDst++ = ASC[( ucs2 & 0x00F0 ) >> 4];
			*pDst++ = ASC[( ucs2 & 0x000F )];
		}else
		{
			*pDst++ = '0';
			*pDst++ = '0';
			*pDst++ = ASC[gb >> 4];
			*pDst++ = ASC[gb & 0x0F];
		}
		TP_UL += 2;
	}
	*pDst			= 0;
	buf[count + 12] = ASC[TP_UL >> 4];
	buf[count + 13] = ASC[TP_UL & 0x0F];

	//rt_kprintf( "\nSMS>encode UCS2:%s", buf );
	sms_tx( buf, ( strlen( buf ) - 2 ) >> 1 );
}

/*��������,��Ҫ�󷵻�
   ���ڲ�ѯ����ֱ�ӷ��� ����0������ҪӦ����ڲ���Ӧ��
   ���������1,���ɵ��÷�����,��Ҫͨ��Ӧ��


 */
uint8_t analy_param( char* cmd, char*value )
{
	char buf[160];
	char *ip;
	int port;
	
	if( strlen( cmd ) == 0 )
	{
		return 0;
	}
	rt_kprintf( "\nSMS>cmd=%s,value=%s", cmd, value );
/*���Ҳ���������*/
	if( strncmp( cmd, "TIREDCLEAR", 10 ) == 0 )
	{
		return 1;
	}
	if( strncmp( cmd, "DISCLEAR", 8 ) == 0 )
	{
		return 1;
	}
	if( strncmp( cmd, "RESET", 5 ) == 0 )               /*Ҫ��λ,������ɺ�λ�������?*/
	{
		reset(0xaa);
		return 1;
	}else if( strncmp( cmd, "CLEARREGIST", 11 ) == 0 )  /*���ע��*/
	{
		memset( jt808_param.id_0xF003, 0, 32 );         /*�����Ȩ��*/
		return 1;
	}


/*�������ģ�������û�в���*/
	if( strlen( value ) == 0 )
	{
		return 0;
	}
/*���Ҷ�Ӧ�Ĳ���*/
	if( strncmp( cmd, "DNSR", 4 ) == 0 )
	{
		if( cmd[4] == '1' )
		{
			strcpy( jt808_param.id_0x0013, value );
		}
		if( cmd[4] == '2' )
		{
			strcpy( jt808_param.id_0x0017, value );
		}
		return 1;
	}
	if( strncmp( cmd, "PORT", 4 ) == 0 )
	{
		if( cmd[4] == '1' )
		{
			jt808_param.id_0x0018=atoi( value );
		}
		if( cmd[4] == '2' )
		{
			jt808_param.id_0x0019=atoi( value );
		}
		return 1;
	}
	if( strncmp( cmd, "IP", 2 ) == 0 )
	{
		if( cmd[2] == '1' )
		{
			strcpy( jt808_param.id_0x0013, value );
		}
		if( cmd[2] == '2' )
		{
			strcpy( jt808_param.id_0x0017, value );
		}
		return 1;
	}
	if( strncmp( cmd, "DUR", 3 ) == 0 )
	{
		jt808_param.id_0x0029=atoi(value);
		return 1;
	}
	if( strncmp( cmd, "SIMID", 6 ) == 0 )
	{
		strcpy( jt808_param.id_0xF006, value );
		return 1;
	}
	if( strncmp( cmd, "DEVICEID", 8 ) == 0 )
	{
		strcpy( jt808_param.id_0xF002, value ); /*�ն�ID ��д��ĸ+����*/
		return 1;
	}
	if( strncmp( cmd, "MODE", 4 ) == 0 )        ///6. ���ö�λģʽ
	{
		if( strncmp( value, "BD", 2 ) == 0 )
		{
			gps_status.mode = MODE_BD;
		}
		if( strncmp( value, "GP", 2 ) == 0 )
		{
			gps_status.mode = MODE_GPS;
		}
		if( strncmp( value, "GN", 2 ) == 0 )
		{
			gps_status.mode= MODE_BDGPS;
		}
		gps_mode( gps_status.mode );
		return 1;
	}
	if( strncmp( cmd, "VIN", 3 ) == 0 )
	{
		strcpy( jt808_param.id_0xF005, value ); /*VIN(TJSDA09876723424243214324)*/
		return 1;
	}
	/*	RELAY(0)�رռ̵��� �̵���ͨ RELAY(1) �Ͽ��̵��� �̵�����*/
	if( strncmp( cmd, "RELAY", 5 ) == 0 )
	{
		return 1;
	}
	if( strncmp( cmd, "TAKE", 4 ) == 0 )    /*���� 1.2.3.4·*/
	{
		return 1;
	}
	if( strncmp( cmd, "PLAY", 4 ) == 0 )    /*��������*/
	{
		tts_write( value, strlen( value ) );
		return 1;
	}
	if( strncmp( cmd, "QUERY", 5 ) == 0 )   /*0 ���������Ϣ 1 ����ϵͳ������Ϣ 2λ����Ϣ*/
	{
		/*ֱ�ӷ�����Ϣ*/
		if( *value == '0' )
		{
			sprintf( buf, "TCBTW703#%s#%s#%s#%d",
			         jt808_param.id_0x0083 + 2,
			         jt808_param.id_0xF002,
			         jt808_param.id_0xF005,
			         jt808_param.id_0x0029 );
			sms_encode_pdu_7bit( buf );
		}

		if( *value == '1' )
		{
			sprintf( buf, "TCBTW703#%s#%s#%s#%d",
			         jt808_param.id_0x0083 + 2,
			         jt808_param.id_0xF002,
			         jt808_param.id_0x0013,
			         jt808_param.id_0x0018);
			sms_encode_pdu_7bit( buf );
		}
		if( *value == '2' )
		{
			sprintf( buf, "TCBTW703#%s#%s#%s#%d",
			         jt808_param.id_0x0083 + 2,
			         jt808_param.id_0xF002,
			         jt808_param.id_0xF005,
			         jt808_param.id_0x0029 );
			sms_encode_pdu_7bit( buf );
		}

		return 0;
	}
	if( strncmp( cmd, "ISP", 3 ) == 0 )         /*	ISP(202.89.23.210��9000)*/
	{
		sscanf(value,"%s:%d",ip,&port);
		gsm_socket[2].index=2;
		strcpy(gsm_socket[2].ipstr,ip);
		gsm_socket[2].port=port;
		gsm_socket[2].active=1;
		return 1;
	}
	if( strncmp( cmd, "PLATENUM", 8 ) == 0 )    /*PLATENUM(��A8888)	*/
	{
		strcpy(jt808_param.id_0x0083,value);
		param_save();
		return 0x80;
	}
	if( strncmp( cmd, "COLOR", 5 ) == 0 )       /* COLOR(0) 1�� ��ɫ  2�� ��ɫ    3�� ��ɫ    4�� ��ɫ    9��  ����*/
	{
		jt808_param.id_0x0084=atoi(value);
		param_save();
		return 1;
	}
	if( strncmp( cmd, "CONNECT", 7 ) == 0 )     /*CONNECT(0)  0:  DNSR   ����     1��  MainIP   ���� */
	{
		return 1;
	}
	if( strncmp( cmd, "PASSWORD", 7 ) == 0 )    /*PASSWORD��0�� 1�� ͨ��   0�� ��ͨ��*/
	{
		return 1;
	}
	return 0;
}

/*�յ�����Ϣ����*/
void jt808_sms_rx( char *info, uint16_t size )
{
	uint16_t	i;
	uint8_t		tbl[24] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0, 0, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf };
	char		*p, *pdst;
	char		c, pid;
	uint16_t	len;
	uint8_t		pdu_type, msg_len;
	uint8_t		dcs;

	char		decode_msg[180];    /*��������Ϣ*/
	uint16_t	decode_msg_len = 0;
	char		tmpbuf[180];        /*��������Ϣ*/

	char		cmd[64];
	char		value[64];
	uint8_t		count;
	char		*psave;

	memset( sender, 0, 64 );
	memset( smsc, 0, 64 );
	p = info;                       /*���¶�λ����ʼ*/
	/*SCA*/
	len = tbl[p[0] - '0'] << 4;
	len |= tbl[p[1] - '0'];         /*�������ĺ��� Length (����)+Tosca�������������ͣ�+Address����ַ��*/
	len <<= 1;                      /*ת��˫��*/
	for( i = 0; i < len + 2; i++ )  /*��������*/
	{
		smsc[i] = *p++;
	}
	/*pdu����*/
	pdu_type	= tbl[*p++ - '0'] << 4;
	pdu_type	|= tbl[*p++ - '0'];

	/*OA��Դ��ַ 05 A1 0180F6*/
	len = tbl[p[0] - '0'] << 4;
	len |= tbl[p[1] - '0']; /*�ֻ�����Դ��ַ Length (����,�����ָ���)+Tosca����ַ���ͣ�+Address����ַ��*/
	if( len & 0x01 )
	{
		len++;              /*����������λ��������10086��5λ �������ֽڱ�ʾ*/
	}

	for( i = 0; i < len + 4; i++ )
	{
		sender[i] = *p++;   /*����û�а��ֽڽ��������͵�ʱ�򻹵û�����*/
	}

	/*PID��־*/
	pid = tbl[*p++ - '0'] << 4;
	pid |= tbl[*p++ - '0'];
	/*DCS����*/
	dcs = tbl[*p++ - '0'] << 4;
	dcs |= tbl[*p++ - '0'];
	/*SCTS��������ʱ��� yymmddhhmmsszz ����ʱ��*/
	p += 14;
	/*��Ϣ����*/
	msg_len = tbl[*p++ - '0'] << 4;
	msg_len |= tbl[*p++ - '0'];
	/*�Ƿ�Ϊ����Ϣpdu_type ��bit6 UDHI=1*/
	if( pdu_type & 0x40 )           /*��������Ϣ��ͷ*/
	{
		len		= tbl[*p++ - '0'] << 4;
		len		|= tbl[*p++ - '0'];
		p		+= ( len >> 1 );    /*���ڻ���OCTģʽ*/
		msg_len -= ( len + 1 );     /*����ռһ���ֽ�*/
	}
	rt_kprintf( "\nSMS>PDU Type=%02x PID=%02x DCS=%02x TP-UDL=%d", pdu_type, pid, dcs, msg_len );

/*ֱ����info�ϲ�������?*/
	pdst = tmpbuf;
	/*��OCT����תΪHEX,�����ֽ�ƴ��һ��*/
	for( i = 0; i < msg_len; i++ )
	{
		c		= tbl[*p++ - '0'] << 4;
		c		|= tbl[*p++ - '0'];
		*pdst++ = c;
	}
	*pdst = 0;

	if( ( dcs & 0x0c ) == GSM_7BIT )
	{
		decode_msg_len = gsmdecode7bit( tmpbuf, decode_msg, msg_len );
	}else if( ( dcs & 0x0c ) == GSM_UCS2 )
	{
		decode_msg_len = gsmdecodeucs2( tmpbuf, decode_msg, msg_len );
	}else
	{
		memcpy( decode_msg, tmpbuf, msg_len );
		decode_msg_len = msg_len;   /*�򵥿�������*/
	}

	if( decode_msg_len == 0 )       /*���ڲ��ǺϷ��������ǲ���ֱ�ӷ���0*/
	{
		return;
	}
	decode_msg[decode_msg_len] = 0;

/*decode_msg�����������Ϣ*/
	p = decode_msg;
	rt_kprintf( "\nSMS(%dbytes)>%s", decode_msg_len, p );
	if( strncmp( p, "TW703", 5 ) != 0 )
	{
		return;
	}
	p += 5;
	memset( cmd, 0, 64 );
	memset( value, 0, 64 );
	count	= 0;
	psave	= cmd;
	i		= 0;
/*tmpbuf����Ӧ�����Ϣ TW703#*/
	while( *p != 0 )
	{
		switch( *p )
		{
			case '#':
				i += analy_param( cmd, value );
				memset( cmd, 0, 64 );
				memset( value, 0, 64 );
				psave	= cmd;      /*����cmd�����д�*/
				count	= 0;
				break;
			case '(':
				psave	= value;    /*��ȡֵ������*/
				count	= 0;
				break;
			case ')':               /*��ʾ����,������*/
				break;
			default:
				if( count < 64 )    /*����������*/
				{
					*psave++ = *p;
					count++;
				}
				break;
		}
		p++;
	}
	i += analy_param( cmd, value );
	
	if( i > 0x7F ) /*Ҫ������Ϣ,�г���*/
	{
		sprintf( tmpbuf, "%s#%-7s#%s", jt808_param.id_0x0083 + 2, jt808_param.id_0xF002, decode_msg + 6 );
		sms_encode_pdu_ucs2( tmpbuf );
		param_save();
	}else if( i > 0 )
	{
		sprintf( tmpbuf, "%s#%-7s#%s", jt808_param.id_0x0083 + 2, jt808_param.id_0xF002, decode_msg + 6 );
		sms_encode_pdu_7bit( tmpbuf );
		param_save();		/*�޸Ĳ���*/
	}
}

/*
   0891683108200205F0000D91683106029691F10008318011216584232C0054005700370030003300230050004C004100540045004E0055004D00286D25005400540036003500320029
   0891683108200205F0000D91683106029691F100003180112175222332D4EB0D361B0D9F4E67714845C15223A2732A8DA1D4F498EB7C46E7E17497BB4C4F8DA04F293586BAC160B814

   ������֤

   http://www.diafaan.com/sms-tutorials/gsm-modem-tutorial/online-sms-submit-pdu-decoder/


 */


#if 1

void sms_test( uint8_t index )
{
	char *s[2] =
	{ "0891683108200205F0000D91683106029691F10008318011216584232C0054005700370030003300230050004C004100540045004E0055004D00286D25005400540036003500320029",
	  "0891683108200205F0000D91683106029691F100003180112175222332D4EB0D361B0D9F4E67714845C15223A2732A8DA1D4F498EB7C46E7E17497BB4C4F8DA04F293586BAC160B814" };
	jt808_sms_rx( s[index], strlen( s[index] ) );
}

FINSH_FUNCTION_EXPORT( sms_test, test sms );


void oiap_test(char *ip,uint16_t port)
{
	gsm_socket[2].index=2;
	strcpy(gsm_socket[2].ipstr,ip);
	gsm_socket[2].port=port;
	gsm_socket[2].active=1;
}
FINSH_FUNCTION_EXPORT( oiap_test, conn onair iap );

#endif

/************************************** The End Of File **************************************/
