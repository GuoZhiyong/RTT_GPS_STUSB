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

#include <finsh.h>
#include "sst25.h"

#define DF_CS_0 GPIO_ResetBits( GPIOD, GPIO_Pin_14 )
#define DF_CS_1 GPIO_SetBits( GPIOD, GPIO_Pin_14 )

#define DF_WP_1 GPIO_SetBits( GPIOD, GPIO_Pin_15 )
#define DF_WP_0 GPIO_SetBits( GPIOD, GPIO_Pin_15 )

#define DBSY 0X80

#define SST25_AAI		0xAD
#define SST25_BP		0x02
#define SST25_RDSR		0x05
#define SST25_WREN		0x06    //Write Enable
#define SST25_WRDI		0x04
#define SectorErace_4KB 0x20    //��������
#define SST25_READ		0x03


struct rt_semaphore sem_dataflash;

/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
uint8_t spi_tx( uint8_t data )
{
	while( SPI_I2S_GetFlagStatus( SPI2, SPI_I2S_FLAG_TXE ) == RESET )
	{
		;
	}
	SPI_I2S_SendData( SPI2, data );
	while( SPI_I2S_GetFlagStatus( SPI2, SPI_I2S_FLAG_RXNE ) == RESET )
	{
		;
	}
	return SPI_I2S_ReceiveData( SPI2 );
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
void sst25_init( void )
{
	GPIO_InitTypeDef	GPIO_InitStructure;
	SPI_InitTypeDef		SPI_InitStructure;

	/* Enable DF_SPI Periph clock */
	RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOB, ENABLE );
	RCC_APB1PeriphClockCmd( RCC_APB1Periph_SPI2, ENABLE );
	GPIO_PinAFConfig( GPIOB, GPIO_PinSource13, GPIO_AF_SPI2 );
	GPIO_PinAFConfig( GPIOB, GPIO_PinSource14, GPIO_AF_SPI2 );
	GPIO_PinAFConfig( GPIOB, GPIO_PinSource15, GPIO_AF_SPI2 );
	RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOD, ENABLE );

	GPIO_InitStructure.GPIO_Speed	= GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType	= GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd	= GPIO_PuPd_DOWN;

	/*!< SPI SCK pin configuration */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
	GPIO_Init( GPIOB, &GPIO_InitStructure );

	/*!< SPI MOSI pin configuration */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14;
	GPIO_Init( GPIOB, &GPIO_InitStructure );

	/*!< SPI MISO pin configuration */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
	GPIO_Init( GPIOB, &GPIO_InitStructure );

	//-------  CS _pin
	GPIO_InitStructure.GPIO_Pin		= GPIO_Pin_14;
	GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType	= GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed	= GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd	= GPIO_PuPd_NOPULL;
	GPIO_Init( GPIOD, &GPIO_InitStructure );

	//-------  WP  _pin
	GPIO_InitStructure.GPIO_Pin		= GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType	= GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed	= GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd	= GPIO_PuPd_NOPULL;
	GPIO_Init( GPIOD, &GPIO_InitStructure );

	/*------------------------ DF_SPI configuration ------------------------*/
	SPI_InitStructure.SPI_Direction			= SPI_Direction_2Lines_FullDuplex;  //SPI_Direction_1Line_Tx;
	SPI_InitStructure.SPI_Mode				= SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize			= SPI_DataSize_8b;
	SPI_InitStructure.SPI_CPOL				= SPI_CPOL_Low;
	SPI_InitStructure.SPI_CPHA				= SPI_CPHA_1Edge;
	SPI_InitStructure.SPI_NSS				= SPI_NSS_Soft;
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;          /* 72M/64=1.125M */
	SPI_InitStructure.SPI_FirstBit			= SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_CRCPolynomial		= 7;

	//SPI1->CR2=0x04;                                   //NSS ---SSOE
	//   SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	//SPI_InitStructure.SPI_Mode = SPI_Mode_Master;       //MSTR

	SPI_I2S_DeInit( SPI2 );
	SPI_Init( SPI2, &SPI_InitStructure );

	/* Enable SPI_MASTER */
	SPI_Cmd( SPI2, ENABLE );
	SPI_CalculateCRC( SPI2, DISABLE );

	DF_CS_1;
	DF_WP_0;

	DF_CS_0;
	spi_tx( 0x50 );
	DF_CS_1;

	DF_CS_0;
	spi_tx( 0x01 );
	spi_tx( 0x00 );
	DF_CS_1;

	DF_CS_0;
	spi_tx( DBSY );
	DF_CS_1;

	rt_sem_init(&sem_dataflash,"sem_df",0, RT_IPC_FLAG_FIFO );
	rt_sem_release(&sem_dataflash);
	
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
void sst25_read( uint32_t addr, uint8_t *p, uint16_t len )
{
	uint8_t		*pdata		= p;
	uint32_t	readaddr	= addr;
	DF_CS_0;
	spi_tx( SST25_READ );
	spi_tx( ( readaddr & 0xFF0000 ) >> 16 );
	spi_tx( ( readaddr & 0xFF00 ) >> 8 );
	spi_tx( readaddr & 0xFF );
	while( len-- )
	{
		*pdata = spi_tx( 0xA5 );
		pdata++;
	}
	DF_CS_1;
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
static uint8_t sst25_busy( void )
{
	uint8_t st;
	DF_CS_0;
	spi_tx( 0x05 );
	st = spi_tx( 0xff );
	DF_CS_1;
	return st & 0x01;
}

/*���һ������������Ҫ����ɾ��*/
void sst25_erase_4k( uint32_t addr )
{
	DF_CS_0;
	spi_tx( SST25_WREN );
	DF_CS_1;
	DF_CS_0;
	spi_tx( SectorErace_4KB );
	spi_tx( ( addr & 0xffFFFF ) >> 16 );
	spi_tx( ( addr & 0xffFF ) >> 8 );
	spi_tx( addr & 0xFF );
	DF_CS_1;
	while( sst25_busy( ) )
	{
		;
	}
}

FINSH_FUNCTION_EXPORT( sst25_erase_4k, erase );


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/

static uint8_t buf[4096];


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static void sst25_bytewrite( uint32_t addr, uint8_t data )
{
	volatile uint8_t st;
	DF_CS_0;
	spi_tx( SST25_WREN );
	DF_CS_1;

	DF_CS_0;
	spi_tx( SST25_BP );
	spi_tx( ( addr & 0xffffff ) >> 16 );
	spi_tx( ( addr & 0xff00 ) >> 8 );
	spi_tx( addr & 0xff );
	spi_tx( data );
	DF_CS_1;
	while( sst25_busy( ) )
	{
		;
	}
#if 0
	DF_CS_0;
	spi_tx( SST25_RDSR );
	do
	{
		st = spi_tx( 0xA5 );
	}
	while( st & 0x01 );
	DF_CS_1;
#endif
}

/*
   ���ж��Ƿ�Ҫɾ����ֱ��д
   �����4kд
 */

void sst25_write_through( uint32_t addr, uint8_t *p, uint16_t len )
{
	volatile uint8_t	st = 0;
	uint32_t			i, wr_addr;
	uint8_t				*pdata	= p;
	uint16_t			remain	= len;
	wr_addr = addr;
/*�ֽ�д�ķ�ʽ*/
#if 0
	while( remain-- )
	{
		sst25_bytewrite( wr_addr, *pdata++ );
		wr_addr++;
	}
#else
/*AAI��ʽд*/
	if( len == 1 )
	{
		sst25_bytewrite( wr_addr, *pdata );
		return;
	}

	DF_CS_0;
	spi_tx( SST25_WREN );
	DF_CS_1;
	DF_CS_0;
	spi_tx( SST25_AAI ); /**/
	spi_tx( wr_addr >> 16 );
	spi_tx( wr_addr >> 8 );
	spi_tx( wr_addr & 0xff );
	spi_tx( *pdata++ );
	spi_tx( *pdata++ );
	DF_CS_1;
	while( sst25_busy( ) )
	{
		;                       //��æ
	}
	remain -= 2;
	while( remain > 1 )
	{
		DF_CS_0;
		spi_tx( SST25_AAI );    /**/
		spi_tx( *pdata++ );
		spi_tx( *pdata++ );
		DF_CS_1;
		while( sst25_busy( ) )
		{
			;                   //��æ
		}
		remain -= 2;
	}

	DF_CS_0;
	spi_tx( SST25_WRDI );       //WRDI�����˳�AAIдģʽ ��νAAI ���ǵ�ַ�Զ���
	DF_CS_1;
	if( remain )
	{
		sst25_bytewrite( wr_addr + len - 1, *pdata );
	}
#endif
}

/*
   ��д���������ݣ�
   ����д���С�����ܿ�������
 */
void sst25_write_back( uint32_t addr, uint8_t *p, uint16_t len )
{
	uint32_t	i, wr_addr;
	uint8_t		*pdata = p;
	uint16_t	offset;
	uint8_t		*pbuf;

	uint16_t	remain = len;

/*�ҵ���ǰ��ַ��Ӧ��4k=0x1000�߽�*/
	wr_addr = addr & 0xFFFFF000;            /*���뵽4k�߽�*/

	offset = addr & 0xfff;                  /*4k��ƫ��*/

	while( remain )
	{
		sst25_read( wr_addr, buf, 4096 );   /*��������*/
		sst25_erase_4k( wr_addr );          /*�������*/

		if( remain + offset < 4096 )        /*��һ��������*/
		{
			memcpy( buf + offset, pdata, remain );
			remain = 0;
		}else
		{
			memcpy( buf + offset, pdata, 4096 - offset );
			remain = remain - ( 4096 - offset );
		}

		pbuf = buf;
		DF_CS_0;
		spi_tx( SST25_WREN );
		DF_CS_1;
		DF_CS_0;
		spi_tx( SST25_AAI ); /**/
		spi_tx( wr_addr >> 16 );
		spi_tx( wr_addr >> 8 );
		spi_tx( wr_addr & 0xff );
		spi_tx( *pbuf++ );
		spi_tx( *pbuf++ );
		DF_CS_1;
		while( sst25_busy( ) )
		{
			;                       //��æ
		}
		for( i = 0; i < 4094; i += 2 )
		{
			DF_CS_0;
			spi_tx( SST25_AAI );    /**/
			spi_tx( *pbuf++ );
			spi_tx( *pbuf++ );
			DF_CS_1;
			while( sst25_busy( ) )
			{
				;                   //��æ
			}
		}
		DF_CS_0;
		spi_tx( SST25_WRDI );       //WRDI�����˳�AAIдģʽ ��νAAI ���ǵ�ַ�Զ���
		DF_CS_1;

		offset	= 0;                /*�����4k�Ļ�����0��ʼ*/
		wr_addr += 4096;
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
int df_read( uint32_t addr, uint16_t size )
{
	uint8_t tbl[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
	int		i, count = 0;
	uint8_t c;
	uint8_t *p		= RT_NULL;
	uint8_t *pdata	= RT_NULL;
	uint8_t printbuf[100];
	int32_t len = size;
	uint32_t pos=addr;

	p = rt_malloc( len );
	if( p == RT_NULL )
	{
		return 0;
	}
	pdata = p;
	sst25_read( addr, pdata, len );

	while( len > 0 )
	{
		count = ( len < 16 ) ? len : 16;
		memset( printbuf, 0x20, 70 );
		for( i = 0; i < count; i++ )
		{
			c					= *pdata;
			printbuf[i * 3]		= tbl[c >> 4];
			printbuf[i * 3 + 1] = tbl[c & 0x0f];
			if( c < 0x20 )
			{
				c = '.';
			}
			if( c > 0x7f )
			{
				c = '.';
			}
			printbuf[50 + i] = c;
			pdata++;
		}
		printbuf[69] = 0;
		rt_kprintf( "%08x %s\r\n",pos,printbuf );
		len -= count;
		pos+=count;
	}
	if( p != RT_NULL )
	{
		rt_free( p );
		p=RT_NULL;
	}
	return size;
}

FINSH_FUNCTION_EXPORT( df_read, read from serial flash );

/////////////////////////////////////////////////////////////////////////////////////////////////////

