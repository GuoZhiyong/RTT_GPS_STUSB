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

/*��ӳ����ź�*/
__IO uint16_t	IC2Value	= 0;
__IO uint16_t	DutyCycle	= 0;
__IO uint32_t	Frequency	= 0;

/*����PA.0 ��Ϊ�ⲿ�������*/
void pulse_init( void )
{
	GPIO_InitTypeDef	GPIO_InitStructure;
	NVIC_InitTypeDef	NVIC_InitStructure;
	TIM_ICInitTypeDef	TIM_ICInitStructure;

	/* TIM5 clock enable */
	RCC_APB1PeriphClockCmd( RCC_APB1Periph_TIM5, ENABLE );

	/* GPIOA clock enable */
	RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOA, ENABLE );

	/* TIM5 chennel1 configuration : PA.0 */
	GPIO_InitStructure.GPIO_Pin		= GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed	= GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_OType	= GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd	= GPIO_PuPd_UP;
	GPIO_Init( GPIOA, &GPIO_InitStructure );

	/* Connect TIM pin to AF0 */
	GPIO_PinAFConfig( GPIOA, GPIO_PinSource0, GPIO_AF_TIM5 );

	/* Enable the TIM5 global Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel						= TIM5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority	= 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority			= 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd					= ENABLE;
	NVIC_Init( &NVIC_InitStructure );

	TIM_ICInitStructure.TIM_Channel		= TIM_Channel_1;
	TIM_ICInitStructure.TIM_ICPolarity	= TIM_ICPolarity_Rising;
	TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
	TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
	TIM_ICInitStructure.TIM_ICFilter	= 0x0;

	TIM_PWMIConfig( TIM5, &TIM_ICInitStructure );

	/* Select the TIM5 Input Trigger: TI1FP1 */
	TIM_SelectInputTrigger( TIM5, TIM_TS_TI1FP1 );

	/* Select the slave Mode: Reset Mode */
	TIM_SelectSlaveMode( TIM5, TIM_SlaveMode_Reset );
	TIM_SelectMasterSlaveMode( TIM5, TIM_MasterSlaveMode_Enable );

	/* TIM enable counter */
	TIM_Cmd( TIM5, ENABLE );

	/* Enable the CC2 Interrupt Request */
	TIM_ITConfig( TIM5, TIM_IT_CC2, ENABLE );
}

/*TIM5_CH1*/
void TIM5_IRQHandler( void )
{
	RCC_ClocksTypeDef RCC_Clocks;
	RCC_GetClocksFreq( &RCC_Clocks );

	TIM_ClearITPendingBit( TIM5, TIM_IT_CC2 );

	/* Get the Input Capture value */
	IC2Value = TIM_GetCapture2( TIM5 );

	if( IC2Value != 0 )
	{
		/* Duty cycle computation */
		//DutyCycle = ( TIM_GetCapture1( TIM5 ) * 100 ) / IC2Value;
		/* Frequency computation   TIM4 counter clock = (RCC_Clocks.HCLK_Frequency)/2 */
		//Frequency = (RCC_Clocks.HCLK_Frequency)/2 / IC2Value;
/*�ǲ��Ƿ����·?*/
		DutyCycle	= ( IC2Value * 100 ) / TIM_GetCapture1( TIM5 );
		Frequency	= ( RCC_Clocks.HCLK_Frequency ) / 2 / TIM_GetCapture1( TIM5 );
	}else
	{
		DutyCycle	= 0;
		Frequency	= 0;
	}
}

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

	pulse_init( ); /*���������*/

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
