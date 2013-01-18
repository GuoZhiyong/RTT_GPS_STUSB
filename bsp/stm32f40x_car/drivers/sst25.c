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
#include "sst25vf.h"

#define DF_CS_0 GPIO_ResetBits( GPIOD, GPIO_Pin_14 )
#define DF_CS_1 GPIO_SetBits( GPIOD, GPIO_Pin_14 )

#define DF_WP_1 GPIO_SetBits( GPIOD, GPIO_Pin_15 )
#define DF_WP_0 GPIO_SetBits( GPIOD, GPIO_Pin_15 )


#define DBSY                          0X80


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
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;          /* 72M/64=1.125M */
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
	spi_tx(0x50);
	DF_CS_1;

	DF_CS_0;
	spi_tx(0x01);
	spi_tx(0x00);
	DF_CS_1;

	
	DF_CS_0;
	spi_tx(DBSY);
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
void sst25_read( uint32_t addr, uint8_t *p, uint16_t len )
{
#define SST25_READ 0x03
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
void sst25_erase( uint32_t addr, uint32_t len )
{
#define SST25_WREN		0x06    //Write Enable
#define SectorErace_4KB 0x20    //扇区擦除
#define SST25_RDSR		0x05

	volatile uint8_t	st;
	uint32_t			i ;
	i= addr&0xFFFFE000; /*对齐到4k边界*/;
	while( i < ( addr + len ) )
	{
		DF_CS_0;
		spi_tx( SST25_WREN );
		DF_CS_1;
		DF_CS_0;
		spi_tx( SectorErace_4KB );
		spi_tx( ( i & 0xff0000 ) >> 16 );
		spi_tx( ( i & 0xff00 ) >> 8 );
		spi_tx( i & 0xFF );
		DF_CS_1;
		DF_CS_0;
		do
		{
			spi_tx( SST25_RDSR );
			st = spi_tx( 0xA5 );
		}
		while( st & 0x01 );
		DF_CS_1;
		i += 4096;
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

static 	uint8_t		buf[4096];
void sst25_write( uint32_t addr, uint8_t *p, uint16_t len)
{
#define SST25_BP	0x02
#define SST25_RDSR	0x05
#define SST25_WREN	0x06 //Write Enable

	volatile uint8_t	st		= 0;
	volatile uint32_t	i,wr_addr;
	uint8_t		*pdata=p;
	uint16_t 	offset;

	int16_t		remain=len;
	

/*找到当前地址对应的4k=0x1000边界*/
	wr_addr=addr&0xFFFFE000; /*对齐到4k边界*/
	offset=addr&0xfff; /*4k内偏移*/
	while(remain>0)
	{
		sst25_read(wr_addr,buf,4096);/*读出数据*/
		sst25_erase(wr_addr,remain);/*清除扇区*/
		
		remain=remain-(4096-offset); /*判断能否在一个扇区内写完*/
		if(remain>0)
			memcpy(buf+offset,pdata,4096-offset);
		else
			memcpy(buf+offset,pdata,len); /*能在一个扇区内完成，就是数据长度*/
		for(i=0;i<4096;i++)
		{
			DF_CS_0;
			spi_tx( SST25_WREN );
			DF_CS_1;
			DF_CS_0;
			spi_tx( SST25_BP );
			spi_tx( (wr_addr &0xffffff) >> 16 );
			spi_tx( (wr_addr &0xff00)>> 8 );
			spi_tx( wr_addr & 0xff );
			spi_tx( buf[i] );
			DF_CS_1;
			DF_CS_0;
			
			do
			{
			st=spi_tx( SST25_RDSR );	
				//st = spi_tx( 0xA5 );
			}while(st & 0x01 );
			DF_CS_1;
			wr_addr++;
		}
		offset=0;	/*写完了整个扇区，下一个的偏移从0开始*/
	}	
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

