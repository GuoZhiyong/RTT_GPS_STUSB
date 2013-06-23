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

#include "jt808_gps.h"
#include "stm32f4xx.h"
#include <rtthread.h>

/*�ⲿIO��*/
typedef struct _auxio_out
{
	GPIO_TypeDef* port;
	uint16_t	pin;
	uint8_t		value;                      /*��ǰ�˿�ֵ*/
	void ( *onchange )( uint8_t value );    /*�˿ڱ仯������*/
}AUX_OUT;

/*��������Ĵ���*/
void onemg( uint8_t value )
{
	if( value )
	{
		jt808_alarm |= BIT_ALARM_EMG;
	} else
	{
		jt808_alarm &= ~BIT_ALARM_EMG;
	}
}

/*ACC״̬�仯*/
void onacc( uint8_t value )
{
	if( value )
	{
		jt808_status |= BIT_STATUS_ACC;
	}else
	{
		jt808_status &= ~BIT_STATUS_ACC;
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
void ondefault( uint8_t value )
{


}

typedef struct _auxio_in
{
	GPIO_TypeDef* port;
	uint16_t	pin;
	uint8_t		value;                              /*��ǰ�˿�ֵ*/
	uint32_t	dithering_threshold;                /*���ޣ�����ȥ������50ms���һ��*/
	uint8_t		dithering_count;                    /*��������*/
	uint32_t	duration;                           /*��ǰ״̬�ĳ���ʱ��*/
	void ( *onchange )( uint8_t );                  /*�˿ڱ仯������*/
}AUX_IN;

AUX_IN	PIN_IN[10] = {
	{ GPIOE, GPIO_Pin_8,  0, 40, 0, 0, onemg	 }, /*������ť*/
	{ GPIOE, GPIO_Pin_9,  0, 40, 0, 0, onacc	 }, /*ACC*/
	{ GPIOE, GPIO_Pin_7,  0, 0,	 0, 0, ondefault }, /*����*/
	{ GPIOC, GPIO_Pin_0,  0, 0,	 0, 0, ondefault }, /*4.Զ��*/
	{ GPIOC, GPIO_Pin_1,  0, 0,	 0, 0, ondefault }, /*5.����*/
	{ GPIOA, GPIO_Pin_1,  0, 0,	 0, 0, ondefault }, /*6.���� ����ΪAD����*/
	{ GPIOC, GPIO_Pin_3,  0, 0,	 0, 0, ondefault }, /*7.��ת ����ΪAD����*/
	{ GPIOC, GPIO_Pin_2,  0, 0,	 0, 0, ondefault }, /*8.��ת*/
	{ GPIOE, GPIO_Pin_11, 0, 0,	 0, 0, ondefault }, /*9.ɲ��*/
	{ GPIOE, GPIO_Pin_10, 0, 0,	 0, 0, ondefault }, /*10.��ˢ*/
};

AUX_OUT PIN_OUT[] = {
	{ GPIOB, GPIO_Pin_1, 0, 0 },                    /*�̵���*/
	{ GPIOB, GPIO_Pin_6, 0, 0 },                    /*������*/
};


/*
   ��ȡ�����״̬
   ����ȥ��

   Ҫ��Ҫ���ݽ���tickֵ?

 */
void auxio_input_check( void)
{
	int		i;
	uint8_t st;
	for( i = 0; i < sizeof( PIN_IN ) / sizeof( AUX_IN ); i++ )
	{
		st = GPIO_ReadInputDataBit( PIN_IN[i].port, PIN_IN[i].pin );
		if( st ^ PIN_IN[i].value )                      /*ֵ��ͬ,�б仯*/
		{
			if( PIN_IN[i].dithering_threshold == 0 )    /*��������*/
			{
				PIN_IN[i].value = st;
				PIN_IN[i].onchange( st );               /*���ô�����*/
			}else
			{
				PIN_IN[i].dithering_count++;
				if( PIN_IN[i].dithering_count == PIN_IN[i].dithering_threshold )
				{
					PIN_IN[i].value = st;
					PIN_IN[i].onchange( st );           /*���ô�����*/
					PIN_IN[i].dithering_count = 0;
				}
			}
		}else
		{
			PIN_IN[i].duration++;
		}
	}
}

/*
   �����ⲿ�����������
   todo:�Ƿ��뵽�洢�У������������

 */
void auxio_init( void )
{
	GPIO_InitTypeDef	GPIO_InitStructure;
	int					i;



	RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOA, ENABLE );
	RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOB, ENABLE );
	RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOC, ENABLE );
	RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOD, ENABLE );
	RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOE, ENABLE );

	GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd	= GPIO_PuPd_UP;

	for( i = 0; i < sizeof( PIN_IN ) / sizeof( AUX_IN ); i++ )
	{
		GPIO_InitStructure.GPIO_Pin = PIN_IN[i].pin;
		GPIO_Init( PIN_IN[i].port, &GPIO_InitStructure );
	}
}

/************************************** The End Of File **************************************/
