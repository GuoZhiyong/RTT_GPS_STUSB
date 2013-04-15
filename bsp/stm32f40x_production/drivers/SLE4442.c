/*SLE��ز�������*/

#include <rtthread.h>
#include <rthw.h>
#include "stm32f4xx.h"
#include "usart.h"
#include "board.h"
#include <serial.h>

#include  <stdlib.h> //����ת�����ַ���
#include  <stdio.h>
#include  <string.h>
#include  <ctype.h>
#include  <stdlib.h>
#include  <stdarg.h>

#include "SLE4442.h"

unsigned int	R_Flag		= 0;
unsigned char	sle_pass[3] = { 0xFF, 0xFF, 0xFF }; //����//{0x23,0x10,0x91};
unsigned int	delayint;

//#define _Nop() for(i=0;i<13;i++){__NOP;}//2us
//#define DELAY5us() for(i=0;i<35;i++){__NOP;}//5.2us

void _Nop( void )
{
	u8 i = 0;
	for( i = 0; i < 13; i++ )
	{
		; //2us
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
void DELAY5us( void )
{
	u8 i = 25;
	for( i = 0; i < 25; i++ )
	{
		;               //5.2us
	}
}

/*******************************************************************
   �����ߺ���
   ����ԭ��: void Start_COM();
   ����: ��������������ʼ����.
 ********************************************************************/
void Start_COM( void )
{
	_CardPutIO_HIGH( ); /*������ʼ�����������ź�*/
	_CardSetCLK_LOW;
	_Nop( );
	_Nop( );
	_Nop( );
	_CardSetCLK_HIGH;
	DELAY5us( );        /*��ʼ��������ʱ�����4.7us,��ʱ*/
	_CardPutIO_LOW( );  /*������ʼ�ź�*/
	DELAY5us( );        /*��ʼ��������ʱ�����4 s*/
	_CardSetCLK_LOW;    /*ǯס����׼�����ͻ�������� */
	_Nop( );
	_Nop( );
}

/*******************************************************************
   �������ߺ���
   ����ԭ��: void Stop_COM();
   ����: ����ͽ����ź�
 ********************************************************************/
void Stop_COM( void )
{
	_CardSetCLK_LOW;
	_CardPutIO_LOW( );  /*���ͽ��������������ź�*/
	_Nop( );            /*���ͽ���������ʱ���ź�*/
	_Nop( );
	_Nop( );
	_CardSetCLK_HIGH;   /*������������ʱ�����4 s*/
	DELAY5us( );
	_CardPutIO_HIGH( ); /*�������߽����ź�*/
	_Nop( );
	_Nop( );
}

/*******************************************************************
   �ֽ����ݴ��ͺ���
   ����ԭ��: void SendByte(unsigned char c);
   ����: ������c ���ͳ�ȥ,����������,Ҳ����������
 ********************************************************************/
void SendByte( unsigned char c )
{
	unsigned char BitCnt;
	for( BitCnt = 0; BitCnt < 8; BitCnt++ ) /*Ҫ���͵����ݳ���Ϊ8 λ*/
	{
		if( ( c >> BitCnt ) & 0x01 )
		{
			_CardPutIO_HIGH( );             /*�жϷ���λ*/
		}else
		{
			_CardPutIO_LOW( );
		}
		_Nop( ); _Nop( );
		_CardSetCLK_HIGH;                   /*��ʱ����Ϊ��֪ͨ��������ʼ��������λ*/
		DELAY5us( );                        /*��֤ʱ�Ӹߵ�ƽ���ڴ���4 s*/
		_CardSetCLK_LOW;
	}
}

/*******************************************************************
   �ֽ����ݽ��պ���
   ����ԭ��: unsigned char RcvByte();
   ����: �������մӿ�����������
 ********************************************************************/
unsigned char RcvByte( void )
{
	unsigned char		retc = 0;
	unsigned char		BitCnt;

	GPIO_InitTypeDef	GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin		= GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_IN;
	GPIO_Init( GPIOD, &GPIO_InitStructure );

	for( BitCnt = 0; BitCnt < 8; BitCnt++ )
	{
		_CardSetCLK_LOW;    /*��ʱ����Ϊ��׼����������λ*/
		DELAY5us( );
		DELAY5us( );        /*ʱ�ӵ͵�ƽ���ڴ���4.7 s*/
		_CardSetCLK_HIGH;   /*��ʱ����Ϊ��ʹ��������������Ч*/
		DELAY5us( );
		retc = retc >> 1;
		if( _CardReadIO( ) )
		{
			retc |= 0x80;   /*������λ,���յ�����λ����retc �� */
		}
		DELAY5us( );
	}
	_CardSetCLK_LOW;
	_Nop( ); _Nop( );
	return ( retc );
}

/*******************************************************************
   ��λ�͸�λ��Ӧ����
   ����ԭ�� void AnRst();
   ���� ��λIC ����������Ӧ�ֽ�
*******************************************************************/
unsigned char AnRst( void )
{
	unsigned char buffer[4], ii = 0;

	_CardSetRST_LOW;
	_CardSetCLK_LOW;
	DELAY5us( );
	DELAY5us( );
	_CardSetRST_HIGH;
	DELAY5us( );
	_CardSetCLK_HIGH;
	DELAY5us( );
	DELAY5us( );
	DELAY5us( );
	_CardSetCLK_LOW;
	DELAY5us( );
	_CardSetRST_LOW;
	_Nop( );

	for( ii = 0; ii < 4; ii++ )
	{
		buffer[ii] = RcvByte( );
	}
	//rt_kprintf("buffer:%X,%X,%X,%X\r\n",buffer[0],buffer[1],buffer[2],buffer[3]);
	if( ( buffer[0] == IDENTIFY1 ) && ( buffer[1] == IDENTIFY2 ) && ( buffer[2] == IDENTIFY3 ) && ( buffer[3] == IDENTIFY4 ) )
	{
		return 0;   //��λֵ��ȷ,���ظ�λ�ɹ�
	}else
	{
		return 1;   //��λֵ����,���ظ�λʧ��
	}
}

/*******************************************************************
   ����4442 �������庯��
   ����ԭ�� void WrmOption();
   ���� ���ʹ���ģʽָ���Ҫ���ô˳���������
*******************************************************************/
//#if 0
void WrmOption( void )
{
	while( 1 )
	{
		_CardSetCLK_LOW;
		_Nop( ); _Nop( ); _Nop( ); _Nop( ); _Nop( ); _Nop( );

		_Nop( ); _Nop( );
		if( _CardReadIO( ) )
		{
			break; /*û�д������������������*/
		}
		_CardSetCLK_HIGH;
		_Nop( ); _Nop( ); _Nop( ); _Nop( ); _Nop( ); _Nop( );
	}
}

//#endif


/*void WrmOption(void)
   {
   _CardSetCLK_LOW;				//����һ������,��Ϊ����Ĺ�ʼ
   DELAY5us();
   DELAY5us();
   _CardSetCLK_HIGH;
   DELAY5us();
   DELAY5us();

   do
   {
   _CardSetCLK_LOW;
   DELAY5us();
   DELAY5us();
   _CardSetCLK_HIGH;
   DELAY5us();
   DELAY5us();
   }while(_CardPutIO_LOW); //���ϲ�����������,����IC���ڲ�����,ֱ��IC����
   }*/


/*******************************************************************
   ��ֹ��������
   ����ԭ�� void BreakN();
   ���� ��ֹ��ǰ����
*******************************************************************/
void BreakN( void )
{
	_CardSetCLK_LOW;
	DELAY5us( );
	_CardSetRST_HIGH; /*������ֹ������ʱ��*/
	DELAY5us( );
	_CardSetRST_LOW;
}

/*******************************************************************
   ����ͺ���
   ����ԭ�� void SendCOM(unsigned char com1,unsigned char com2,unsigned char com3);
   ���� �����������3 �ֽ�������
   ��������
*******************************************************************/
void SendCOM( unsigned char com1, unsigned char com2, unsigned char com3 )
{
	Start_COM( );
	SendByte( com1 ); /*��������3 �ֽ�ָ��*/
	SendByte( com2 );
	SendByte( com3 );
	Stop_COM( );
}

/*******************************************************************
   SLE4442 �������ݺ���
   ����ԭ��: unsigned char IRcvdat_4442(unsigned char area,unsigned char addr,unsigned char num,unsigned char *buf);
   ����: ��SLE4442 �����ж�����area Ϊ�洢������addr Ϊ��ʼ��ַ
   num Ϊ��ȡ�����ֽ���buf[]Ϊ���ݻ�����ָ��
   ˵�� �����ɹ�����1 ����area ���󷵻�0 ʹ��ǰ���жϿ����û��
 ********************************************************************/
unsigned char IRcvdat_4442( unsigned char area, unsigned char addr, unsigned char num, unsigned char *buf )
{
	unsigned char i;

	switch( area )
	{
		case MAM:
			if( AnRst( ) == 1 )             /*��λSLE4442 �����ո�λ��Ӧ*/
			{
				return 0;
			}
			SendCOM( 0x30, addr, 0x00 );    /*�����洢��*/
			for( i = 0; i < num; i++ )
			{
				*buf = RcvByte( );
				buf++;
			}
			BreakN( );
			break;
		case SCM:
			if( AnRst( ) == 1 )
			{
				return 0;
			}
			SendCOM( 0x31, 0x00, 0x00 );
			for( i = 0; i < num; i++ )
			{
				*buf = RcvByte( );
				buf++;
			}
			BreakN( );
			break;
		case PRM:
			AnRst( );
			SendCOM( 0x34, 0x00, 0x00 );
			for( i = 0; i < num; i++ )
			{
				*buf = RcvByte( );
				buf++;
			}
			BreakN( );
			break;
		default:
			return ( 0 );
	}
	return ( 1 );
}

/*******************************************************************
   SLE4442 ��д���ݺ���
   ����ԭ��: unsigned char ISenddat_4442(unsigned char area,unsigned char addr,unsigned char num,unsigned char *buf);
   ����: ��SLE4442 ������д����area Ϊ�洢������addr Ϊ��ʼ��ַ
   num Ϊ��ȡ�����ֽ���buf[]Ϊ���ݻ�����ָ��
   ˵�� �����ɹ�����1 ����area ���󷵻�0 ʹ��ǰ���жϿ����û��
 ********************************************************************/
unsigned char ISenddat_4442( unsigned char area, unsigned char addr, unsigned char num, unsigned char *buf )
{
	unsigned char i;

	switch( area )
	{
		case MAM:
			if( AnRst( ) == 1 )
			{
				return 0x00;
			}
			for( i = 0; i < num; i++ )
			{
				SendCOM( 0x38, addr + i, *buf );    /*д���洢��*/
				buf++;
				WrmOption( );                       /*���Ͳ�������*/
			}
			break;
		case SCM:
			AnRst( );
			for( i = 0; i < num; i++ )
			{
				SendCOM( 0x39, addr + i, *buf );
				buf++;
				WrmOption( );
			}
			break;
		case PRM:
			AnRst( );
			for( i = 0; i < num; i++ )
			{
				SendCOM( 0x3c, addr + i, *buf );
				buf++;
				WrmOption( );
			}
			break;
		default:
			return ( 0 );
	}
	return ( 1 );
}

/*******************************************************************
   SLE4442 ��У�����뺯��
   ����ԭ��: unsigned char IChkpsw_4442(void);
   ����: ����SLE4442 ����������˶Ժ˶Ժ��ܽ���д����
   ˵�� �����ɹ�����0x00 ����Ч���𻵷���0x01,�������
   ��0x02 ��ֻʣ1 �λ��᷵��0x03.
 ********************************************************************/
unsigned char IChkpsw_4442( void )
{
	unsigned char ec;
	if( IRcvdat_4442( SCM, 0x00, 1, &ec ) == 0 )
	{
		return 0x01;
	}
	switch( ec & 0x7 )
	{
		case 1:
		case 2:
		case 4: return 0x3;
		case 3:
		case 5: ec	= 0x1; break;
		case 6: ec	= 0x2; break;
		case 7: ec	= 0x3; break;
		default: return 0x1;
	}
	if( AnRst( ) == 1 )
	{
		return 0x01;
	}
	SendCOM( 0x39, 0x00, ec ); //��дEC �ֽ�
	WrmOption( );
	SendCOM( 0x33, 0x01, sle_pass[0] );
	WrmOption( );
	SendCOM( 0x33, 0x02, sle_pass[1] );
	WrmOption( );
	SendCOM( 0x33, 0x03, sle_pass[2] );
	WrmOption( );
	SendCOM( 0x39, 0x00, 0xff ); //�޸�ECֵ
	WrmOption( );
	ec = 0;
	if( IRcvdat_4442( SCM, 0x00, 1, &ec ) == 0 )
	{
		return 0x01;
	}

	if( ( ec & 0x07 ) != 0x07 )
	{
		return ( 0x02 );
	}
	return ( 0x00 );
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
unsigned char _CardReadIO( void )
{
	unsigned char i;
	//_CardPutIO_HIGH;
	i = GPIO_ReadInputDataBit( GPIOD, GPIO_Pin_6 ); //((P6IN&BIT5)>>5);
	return i;
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
void Init_4442( void )
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOA, ENABLE );
	RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOB, ENABLE );
	RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOC, ENABLE );
	RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOD, ENABLE );
	RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOE, ENABLE );

	GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed	= GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_OType	= GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd	= GPIO_PuPd_NOPULL;

	GPIO_InitStructure.GPIO_Pin		= GPIO_Pin_0 | GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_OUT; //GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed	= GPIO_Speed_50MHz;
	GPIO_Init( GPIOB, &GPIO_InitStructure );

	GPIO_InitStructure.GPIO_Pin		= GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_Speed	= GPIO_Speed_50MHz;
	GPIO_Init( GPIOD, &GPIO_InitStructure );

	GPIO_InitStructure.GPIO_Pin		= GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_Speed	= GPIO_Speed_50MHz;
	GPIO_Init( GPIOD, &GPIO_InitStructure );
	GPIO_SetBits( GPIOD, GPIO_Pin_6 );

	GPIO_InitStructure.GPIO_Pin		= GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_Speed	= GPIO_Speed_50MHz;
	GPIO_Init( GPIOC, &GPIO_InitStructure );
	GPIO_SetBits( GPIOC, GPIO_Pin_6 );

	GPIO_InitStructure.GPIO_Pin		= GPIO_Pin_7; // IC  ������ָʾ
	GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_IN;
	GPIO_Init( GPIOC, &GPIO_InitStructure );
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
void _CardPutIO_HIGH( void )
{
	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Pin		= GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_Speed	= GPIO_Speed_2MHz; //GPIO_Speed_50MHz;
	GPIO_Init( GPIOD, &GPIO_InitStructure );

	GPIO_SetBits( GPIOD, GPIO_Pin_6 );
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
void _CardPutIO_LOW( void )
{
	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Pin		= GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_Speed	= GPIO_Speed_2MHz;
	GPIO_Init( GPIOD, &GPIO_InitStructure );

	GPIO_ResetBits( GPIOD, GPIO_Pin_6 );
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
unsigned char Rx_4442( unsigned char addr, unsigned char num, unsigned char *buf )
{
	if( addr < 32 )
	{
		return 0x03;
	}
	if( ( GPIO_ReadInputDataBit( GPIOC, GPIO_Pin_7 ) ) == 0 )
	{
		return 0x01;
	}
	if( IRcvdat_4442( MAM, addr, num, buf ) == 0 )
	{
		return 0x02;
	}
	return 0x00;
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
unsigned char Tx_4442( unsigned char addr, unsigned char num, unsigned char *buf )
{
	if( addr < 32 )
	{
		return 0x03;
	}
	if( ( GPIO_ReadInputDataBit( GPIOC, GPIO_Pin_7 ) ) == 0 )
	{
		return 0x01;
	}
	if( IChkpsw_4442( ) == 0 )
	{
		if( ISenddat_4442( MAM, addr, num, buf ) == 0 )
		{
			return 0x02;
		}
	}else
	{
		return 0x02;
	}
	return 0x00;
}

/*
       IC  card
 */

unsigned char	IC_CardInsert	= 0; //1:IC��������ȷ  2:IC���������
unsigned char	IC_Check_Count	= 0;
unsigned int	read_counter	= 0, flag_8024off = 1;
unsigned char	Init8024Flag	= 0;
unsigned int	DelayCheckIc	= 0;
unsigned char	institution[45];
unsigned char	administrator_card = 0;

uint8_t			BuzzerFlag = 0;

#define b_CardEdge 0x0001


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
void KeyBuzzer( unsigned char num )
{
	if( num == 1 )
	{
		BuzzerFlag++;
		if( BuzzerFlag == 2 )
		{
			GPIO_SetBits( GPIOB, GPIO_Pin_6 );
		}
		if( BuzzerFlag == 4 )
		{
			GPIO_ResetBits( GPIOB, GPIO_Pin_6 );
			BuzzerFlag		= 0;
			IC_CardInsert	= 0;
		}
	}else if( num == 2 )
	{
		BuzzerFlag++;
		if( ( BuzzerFlag == 12 ) || ( BuzzerFlag == 16 ) )
		{
			GPIO_SetBits( GPIOB, GPIO_Pin_6 );
		}
		if( ( BuzzerFlag == 14 ) || ( BuzzerFlag == 18 ) )
		{
			GPIO_ResetBits( GPIOB, GPIO_Pin_6 );
		}
		if( BuzzerFlag == 18 )
		{
			BuzzerFlag = 0; IC_CardInsert = 0;
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



void CheckICInsert( void )
{
	unsigned char	write_flag	= 0;
	u8				result0		= 0, result1 = 0, result2 = 0, result3 = 0, result4 = 0, result5 = 0;   //i=0;
	u8				FLagx		= 0;                                                                    //,len=0;
	unsigned char	reg_record[32];

	uint8_t			buf[128];

	if( GPIO_ReadInputDataBit( GPIOC, GPIO_Pin_7 ) )
	{
		IC_Check_Count++;
		if( IC_Check_Count >= 10 )
		{
			IC_Check_Count = 0;
			//�����ϵ翪8024�ĵ�
			if( flag_8024off == 1 )
			{
				R_Flag			|= b_CardEdge;
				Init8024Flag	= 2;
				flag_8024off	= 0;
			}
			//8024��off�ӵͱ��
			if( Init8024Flag == 1 )
			{
				Init8024Flag	= 2;
				R_Flag			|= b_CardEdge;
				//rt_kprintf("pc7  Ϊ �ߣ�R_Flag��1\r\n");
			}
			//��⵽�����ʼ��ic��
			if( ( R_Flag & b_CardEdge ) && ( Init8024Flag == 2 ) )
			{
				Init8024Flag = 3;
				_CardCMDVCC_LOW;
				for( DelayCheckIc = 0; DelayCheckIc < 500; DelayCheckIc++ )
				{
					DELAY5us( );
				}
				_CardSetPower_HIGH;
				_CardSetRST_LOW;
				for( DelayCheckIc = 0; DelayCheckIc < 15; DelayCheckIc++ )
				{
					_CardSetCLK_LOW;
					DELAY5us( ); DELAY5us( ); DELAY5us( );
					_CardSetCLK_HIGH;
					DELAY5us( ); DELAY5us( ); DELAY5us( );
					_CardSetCLK_LOW;
				}
				R_Flag		&= ~b_CardEdge;
				write_flag	= 1;
				//rt_kprintf("ִֻ��1��\r\n");
			}
		}
	}else
	{
		IC_Check_Count = 0;
		_CardSetRST_HIGH;
		_CardSetPower_LOW;
		_CardCMDVCC_HIGH;
		if( Init8024Flag == 0 )
		{
			Init8024Flag = 1;
			//rt_kprintf("pc7  Ϊ ��\r\n");
		}
	}
	if( write_flag == 1 )
	{
		write_flag = 0;
		Rx_4442( 241, 13, reg_record );                                                                                                 //����Ա��
		if( strncmp( (char*)reg_record, "administrator", 13 ) == 0 )
		{
			rt_kprintf( "\r\n����Ա��" );
			administrator_card = 1;
		}else
		{
			result0 = Rx_4442( 70, 10, (unsigned char*)buf );                                                                           //����ʻԱ����
			rt_kprintf( "\r\n��ʻԱ����:%s,result0=%d", buf, result0 );

			result1 = Rx_4442( 52, 18, (unsigned char*)buf );                                                                           //����ʻ֤����
			rt_kprintf( "\r\n��ʻ֤����:%s,result1=%d", buf, result1 );

			result2 = Rx_4442( 49, 3, (unsigned char*)buf );                                                                            //����ʻԱ����
			rt_kprintf( "\r\n��ʻԱ����:%s,result2=%d", buf, result2 );

			result3 = Rx_4442( 80, 20, (unsigned char*)buf );                                                                           //���֤����
			rt_kprintf( "\r\n���֤����:%s,result3=%d", buf, result3 );

			result4 = Rx_4442( 100, 40, (unsigned char*)buf );                                                                          //��ҵ�ʸ�֤
			rt_kprintf( "\r\n��ҵ�ʸ�֤:%s,result4=%d", buf, result4 );

			result5 = Rx_4442( 140, 41, (unsigned char*)buf );                                                                          //��֤����
			rt_kprintf( "\r\n��֤����:%s,result5=%d", buf, result5 );

			if( ( result0 == 0 ) && ( result1 == 0 ) && ( result2 == 0 ) && ( result3 == 0 ) && ( result4 == 0 ) && ( result5 == 0 ) )  //�������ȷ
			{
				IC_CardInsert	= 1;                                                                                                    //IC	��������ȷ
				FLagx			= 0;
				BuzzerFlag		= 1;                                                                                                    //��һ����ʾ
			}else
			{
				BuzzerFlag		= 11;                                                                                                   //��һ����ʾ
				IC_CardInsert	= 2;                                                                                                    //IC	���������
			}
		}
		Init8024Flag = 0;
	}

//===================����IC����д���==================================================
}

/************************************** The End Of File **************************************/
