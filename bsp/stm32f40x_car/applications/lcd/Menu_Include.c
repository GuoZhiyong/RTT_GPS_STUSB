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

#include "Menu_Include.h"
#include <string.h>
#include "sed1520.h"
#include "stm32f4xx.h"

#if 0

unsigned int		tzxs_value = 6000;
unsigned char		send_data[10];
MB_SendDataType		mb_senddata;

unsigned int		CounterBack = 0;
unsigned char		UpAndDown	= 1;                                //�����������˵�ѡ�����

unsigned char		Dis_date[22] = { "2000/00/00  00:00:00" };      //20
unsigned char		Dis_speDer[20] = { "000 km/h    000 ��" };

unsigned char		GPS_Flag = 0, Gprs_Online_Flag = 0;             //��¼gps gprs״̬�ı�־

unsigned char		speed_time_rec[15][6];                          //��  ��  ��  ʱ  ��  �ٶ�
unsigned char		DriverName[22], DriverCardNUM[20];              //��IC��������ļ�ʻԱ�����ͼ�ʻ֤����
unsigned char		ServiceNum[13];                                 //�豸��Ψһ�Ա���,IMSI����ĺ�12λ

unsigned char		ErrorRecord = 0;                                //ƣ�ͳ��ټ�¼   ƣ��ʱ�����Ϊ1����ʱ�����Ϊ2,���������0
PilaoRecord			PilaoJilu[12];
ChaosuRecord		ChaosuJilu[20];

unsigned char		StartDisTiredExpspeed	= 0;                    //��ʼ��ʾƣ�ͻ��߳��ټ�ʻ�ļ�¼,���ж���ʾʱ����Ϣ����ʱ��
unsigned char		tire_Flag				= 0, expsp_Flag = 0;    //ƣ�ͼ�ʻ/���ټ�ʻ  �м�¼Ϊ1(��ʾ�м�����¼)���޼�¼Ϊ2���鿴��¼��Ϊ3(��ʾ��down�����鿴)
unsigned char		pilaoCounter			= 0, chaosuCounter = 0; //��¼����ƣ�ͼ�ʻ�ͳ��ټ�ʻ������
unsigned char		pilaoCouAscii[2], chaosuCouAscii[2];
DispMailBoxInfor	LCD_Post, GPStoLCD, OtherToLCD, PiLaoLCD, ChaoSuLCD;

unsigned char		SetVIN_NUM	= 1;                                // :���ó��ƺ���  2:����VIN
unsigned char		OK_Counter	= 0;                                //��¼�ڿ�ݲ˵���ok�����µĴ���
unsigned char		Screen_In	= 0, Screen_in0Z = 0;               //��¼��ѡ����ѡ�еĺ���

unsigned char		OKorCancel	= 1, OKorCancel2 = 1, OKorCancelFlag = 1;
unsigned char		SetTZXSFlag = 0, SetTZXSCounter = 0;            //SetTZXSFlag  1:У׼��������ϵ����ʼ  2:У׼��������ϵ������
//    1���ݵ���(... .../���)  2:usb�豸�γ�
unsigned char		OUT_DataCounter		= 0;                        //ָ��������������  1��2��3
unsigned char		DataOutStartFlag	= 0;                        //���ݵ�����־
unsigned char		DataOutOK			= 0;

unsigned char		Rx_TZXS_Flag = 0;

unsigned char		battery_flag	= 0, tz_flag = 0;
unsigned char		USB_insertFlag	= 1;

unsigned char		BuzzerFlag = 0;     //=1��1��  ��11��2��

unsigned char		DaYin		= 0;    //�������´�ӡ��Ϊ101��2s��Ϊ1����ʼ��ӡ(�ж��Ƿ�ȡ�����ݣ�û����ʾ�����������ݴ�ӡ)
unsigned char		DaYinDelay	= 0;

unsigned char		FileName_zk[11];

//==============12*12========���ֿ��к��ֵĵ���==========
unsigned char	test_00[24], Read_ZK[24];
unsigned char	DisComFlag	= 0;
unsigned char	ICcard_flag = 0;

unsigned char	DisInfor_Menu[8][20];
unsigned char	DisInfor_Affair[8][20];

unsigned char	UpAndDown_nm[4] = { 0xA1, 0xFC, 0xA1, 0xFD };   //�� ��

//========================================================================
unsigned char	UpdataDisp[8] = { "001/000" };                  //������������
unsigned char	BD_updata_flag		= 0;                        //����������u���ļ��ı�־
unsigned int	FilePageBD_Sum		= 0;                        //��¼�ļ���С�����ļ���С/514
unsigned int	bd_file_exist		= 0;                        //��������Ҫ�������ļ�
unsigned char	device_version[30]	= { "�����汾:V BD 1.00" };
unsigned char	bd_version[20] = { "ģ��汾:V 00.00.000" };

unsigned char	ISP_Updata_Flag = 0;                            //Զ�������������������ʾ��־   1:��ʼ����  2:�������

unsigned char	data_tirexps[120];
unsigned char	OneKeyCallFlag		= 0;                        //  һ������
unsigned char	BD_upgrad_contr		= 0;                        //  ������������

u8				CarSet_0_counter;



#endif








































MENUITEM		*pMenuItem;

/*add by bitter*/
#include "jt808.h"
#include "jt808_gps.h"

uint32_t		hmi_status;


HMI_15MIN_SPEED hmi_15min_speed[15];
uint8_t			hmi_15min_speed_curr = 0;

unsigned char	arrow_left[] = {0x01,0x03,0x07,0x0f,0x0f,0x07,0x03,0x01};
unsigned char	arrow_right[] = {0x80,0xc0,0xe0,0xf0,0xf0,0xe0,0xc0,0x80};


DECL_BMP( 8, 8, arrow_left );
DECL_BMP( 8, 8, arrow_right ); 


/*����Ƿ�ص�������*/
void timetick_default( unsigned int tick )
{
	MENUITEM *tmp;
	if( ( tick - pMenuItem->tick ) >= 100 * 30 )
	{
		
		if( pMenuItem->parent != (void*)0 )
		{
			tmp=pMenuItem->parent;
			pMenuItem->parent=(void*)0;
			pMenuItem=tmp;
		}else
		{
			pMenuItem = &Menu_1_Idle;
		}
		pMenuItem->show( );
	}
}




/*���ݷָ�Ϊ�У���¼���ס���β��ַ*/
uint8_t split_content( uint8_t *pinfo,uint16_t len,DISP_ROW *display_rows,uint8_t bytes_per_row)
{
	uint8_t count;
	uint8_t pos = 0;
	uint8_t * p;
	uint8_t row = 0;

	uint8_t start = 0;

	DISP_ROW* disp_row=display_rows;

	p		= pinfo;
	count	= 0;
	pos		= 0;

	while( pos < len )
	{
		if( *p < 0x20 ) /*�����ַ�������*/
		{
			if( count ) /*������*/
			{
				disp_row->start = start;
				disp_row->count = count;
				count				= 0;
				disp_row++;
				row++;
			}
			pos++;
			p++;
		}else
		{
			if( count == 0 )
			{
				start = pos;
			}
			if( *p > 0x7F ) /*����*/
			{
				if( count == (bytes_per_row-1))
				{
					disp_row->start = start;
					disp_row->count = count;
					count				= 0;
					disp_row++;
					row++;
					start = pos;
				}
				pos		+= 2; /*��Ҫ��������*/
				p		+= 2;
				count	+= 2;
			}else
			{
				count++;
				pos++;
				p++;
			}
			if( count == bytes_per_row) /*����*/
			{
				disp_row->start = start;
				disp_row->count = count;
				row++;
				disp_row++;
				count = 0;
			}
		}
		if( row > 31 )
		{
			break;
		}
	}

	if( count )
	{
		disp_row->start = start;
		disp_row->count = count;
		row++;
	}
	return row;
}

/*����n���ַ���dst�У�����\0 �Ͳ��ɼ��ַ�����*/
uint8_t my_strncpy(char* dst,char* src,uint8_t len)
{
	char* pdst=dst;
	char* psrc=src;
	uint8_t count=0;
	while(len--)
	{
		if(*psrc<0x20) break;
		*pdst++=*psrc++;
		count++;
	}
	return count;
}




/************************************** The End Of File **************************************/
