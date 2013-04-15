/*SLE4442 初始化硬件*/
#ifndef _SLE4442_H_
#define _SLE4442_H_

#include <rtthread.h> 
#include <rthw.h>
#include "stm32f4xx.h"
#include "usart.h"
#include "board.h"
#include <serial.h>

#include  <stdlib.h>//数字转换成字符串
#include  <stdio.h>
#include  <string.h>
#include  <ctype.h>
#include  <stdlib.h>
#include  <stdarg.h>


#define  C_50Ms     1638 //50ms时钟中断

#define  TRUE   1
#define  FALSE  0

#define IDENTIFY1   0xA2     
#define IDENTIFY2   0x13  
#define IDENTIFY3   0x10
#define IDENTIFY4   0x91     

#define _CardSetRST_HIGH    GPIO_SetBits(GPIOD,GPIO_Pin_7);//P6OUT|=BIT2
#define _CardSetRST_LOW     GPIO_ResetBits(GPIOD,GPIO_Pin_7);//P6OUT&=~BIT2

#define _CardSetCLK_HIGH    GPIO_SetBits(GPIOB,GPIO_Pin_12);//P6OUT|=BIT0
#define _CardSetCLK_LOW     GPIO_ResetBits(GPIOB,GPIO_Pin_12);//P6OUT&=~BIT0


#define _CardCMDVCC_HIGH     GPIO_SetBits(GPIOC,GPIO_Pin_6);//P6OUT&=~BIT4
#define _CardCMDVCC_LOW      GPIO_ResetBits(GPIOC,GPIO_Pin_6);//P6OUT|=BIT4

unsigned char _CardReadIO(void);

#define _CardSetPower_HIGH  GPIO_SetBits(GPIOB,GPIO_Pin_0);//{P1OUT &= ~BIT7; P2OUT |= BIT1;}
#define _CardSetPower_LOW   GPIO_ResetBits(GPIOB,GPIO_Pin_0);//{P2OUT &= ~BIT1; P1OUT |= BIT7;}


#define MAM 0 /*定义主存储器代号*/
#define SCM 1 /*定义加密存储器代号*/
#define PRM 2 /*定义保护存储器代号*/


extern unsigned int R_Flag;

void _CardPutIO_HIGH(void);
void _CardPutIO_LOW(void);
void CardInsertCheck(void);
unsigned char Rx_4442(unsigned char addr,unsigned char num,unsigned char *buf);
unsigned char Tx_4442(unsigned char addr,unsigned char num,unsigned char *buf);


extern void Init_4442(void);

void IC_Operation(void);

void Start_COM(void);
void Stop_COM(void);
void SendByte(unsigned char c);
unsigned char RcvByte(void);
unsigned char AnRst(void);
void WrmOption(void);
void BreakN(void);
void SendCOM(unsigned char com1,unsigned char com2,unsigned char com3);
unsigned char IRcvdat_4442(unsigned char area,unsigned char addr,unsigned char num,unsigned char *buf);
unsigned char ISenddat_4442(unsigned char area,unsigned char addr,unsigned char num,unsigned char *buf);
unsigned char IChkpsw_4442(void);


extern void  DelayTime(unsigned int Time);
extern void _Nop(void);
extern void DELAY5us(void);



extern unsigned char IC_CardInsert;//1:IC卡插入正确  2:IC卡插入错误
extern unsigned char IC_Check_Count;
extern unsigned char administrator_card; 


void CheckICInsert(void);
void KeyBuzzer(unsigned char num); 



#endif
