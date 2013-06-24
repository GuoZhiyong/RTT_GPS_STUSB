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
#ifndef _JT808_GPS_H_
#define _JT808_GPS_H_

#include <rtthread.h>
#include "stm32f4xx.h"

#define BIT_STATUS_ACC	0x01
#define BIT_STATUS_GPS	0x02
#define BIT_STATUS_NS	0x04
#define BIT_STATUS_EW	0x08

#define BIT_ALARM_EMG 0x01

/*����λ����Ϣ,��Ϊ�ֽڶ���ķ�ʽ������ʹ�����鷽��*/
typedef __packed struct _gps_baseinfo
{
	uint32_t	alarm;
	uint32_t	status;
	uint32_t	latitude;       /*γ��*/
	uint32_t	longitude;      /*����*/
	uint16_t	altitude;
	uint16_t	speed_10x;      /*�Ե��ٶ� 0.1KMH*/
	uint16_t	cog;            /*�ԵؽǶ�*/
	uint8_t		datetime[6];    /*BCD��ʽ*/
}GPS_BASEINFO;

enum BDGPS_MODE
{
	MODE_GET=0,                 /*��ѯ*/
	MODE_BD =1,
	MODE_GPS,
	MODE_BDGPS,
};

typedef  struct  _gps_status
{
	enum BDGPS_MODE Position_Moule_Status;  /* 1: BD   2:  GPS   3: BD+GPS    ��λģ���״̬*/
	uint8_t			Antenna_Flag;           //��ʾ��ʾ��·
	uint8_t			Raw_Output;             //  ԭʼ�������
	uint8_t			NoSV;
}GPS_STATUS;

extern GPS_STATUS	gps_status;

extern GPS_BASEINFO gps_baseinfo;

extern uint32_t		gps_lati;           /*�ڲ���С�� γ�� 10-E6��*/
extern uint32_t		gps_longi;          /*�ڲ���С�� ���� 10-E6��*/
extern uint16_t		gps_speed;          /*�ٶ� kmh*/
extern uint16_t		gps_cog;            /*�Եط����*/
extern uint16_t		gps_alti;           /*�߶�*/
extern uint8_t		gps_datetime[6];    /*����ʱ�� hex��ʽ*/

/*�澯��״̬��Ϣ*/
extern uint32_t jt808_alarm;
extern uint32_t jt808_status;

extern uint32_t	gps_sec_count;		/*gps���������*/

void gps_rx( uint8_t * pinfo, uint16_t length );


void jt808_gps_init( void );


#endif
/************************************** The End Of File **************************************/
