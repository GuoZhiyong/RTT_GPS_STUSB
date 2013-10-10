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
unsigned char		UpAndDown	= 1;                                //参数设置主菜单选择序号

unsigned char		Dis_date[22] = { "2000/00/00  00:00:00" };      //20
unsigned char		Dis_speDer[20] = { "000 km/h    000 度" };

unsigned char		GPS_Flag = 0, Gprs_Online_Flag = 0;             //记录gps gprs状态的标志

unsigned char		speed_time_rec[15][6];                          //年  月  日  时  分  速度
unsigned char		DriverName[22], DriverCardNUM[20];              //从IC卡里读出的驾驶员姓名和驾驶证号码
unsigned char		ServiceNum[13];                                 //设备的唯一性编码,IMSI号码的后12位

unsigned char		ErrorRecord = 0;                                //疲劳超速记录   疲劳时间错误为1超速时间错误为2,按任意键清0
PilaoRecord			PilaoJilu[12];
ChaosuRecord		ChaosuJilu[20];

unsigned char		StartDisTiredExpspeed	= 0;                    //开始显示疲劳或者超速驾驶的记录,再判断提示时间信息错误时用
unsigned char		tire_Flag				= 0, expsp_Flag = 0;    //疲劳驾驶/超速驾驶  有记录为1(显示有几条记录)，无记录为2，查看记录变为3(显示按down逐条查看)
unsigned char		pilaoCounter			= 0, chaosuCounter = 0; //记录返回疲劳驾驶和超速驾驶的条数
unsigned char		pilaoCouAscii[2], chaosuCouAscii[2];
DispMailBoxInfor	LCD_Post, GPStoLCD, OtherToLCD, PiLaoLCD, ChaoSuLCD;

unsigned char		SetVIN_NUM	= 1;                                // :设置车牌号码  2:设置VIN
unsigned char		OK_Counter	= 0;                                //记录在快捷菜单下ok键按下的次数
unsigned char		Screen_In	= 0, Screen_in0Z = 0;               //记录备选屏内选中的汉字

unsigned char		OKorCancel	= 1, OKorCancel2 = 1, OKorCancelFlag = 1;
unsigned char		SetTZXSFlag = 0, SetTZXSCounter = 0;            //SetTZXSFlag  1:校准车辆特征系数开始  2:校准车辆特征系数结束
//    1数据导出(... .../完成)  2:usb设备拔出
unsigned char		OUT_DataCounter		= 0;                        //指定导出数据类型  1、2、3
unsigned char		DataOutStartFlag	= 0;                        //数据导出标志
unsigned char		DataOutOK			= 0;

unsigned char		Rx_TZXS_Flag = 0;

unsigned char		battery_flag	= 0, tz_flag = 0;
unsigned char		USB_insertFlag	= 1;

unsigned char		BuzzerFlag = 0;     //=1响1声  ＝11响2声

unsigned char		DaYin		= 0;    //待机按下打印键为101，2s后为1，开始打印(判断是否取到数据，没有提示错误，渠道数据打印)
unsigned char		DaYinDelay	= 0;

unsigned char		FileName_zk[11];

//==============12*12========读字库中汉字的点阵==========
unsigned char	test_00[24], Read_ZK[24];
unsigned char	DisComFlag	= 0;
unsigned char	ICcard_flag = 0;

unsigned char	DisInfor_Menu[8][20];
unsigned char	DisInfor_Affair[8][20];

unsigned char	UpAndDown_nm[4] = { 0xA1, 0xFC, 0xA1, 0xFD };   //↑ ↓

//========================================================================
unsigned char	UpdataDisp[8] = { "001/000" };                  //北斗升级进度
unsigned char	BD_updata_flag		= 0;                        //北斗升级度u盘文件的标志
unsigned int	FilePageBD_Sum		= 0;                        //记录文件大小，读文件大小/514
unsigned int	bd_file_exist		= 0;                        //读出存在要升级的文件
unsigned char	device_version[30]	= { "主机版本:V BD 1.00" };
unsigned char	bd_version[20] = { "模块版本:V 00.00.000" };

unsigned char	ISP_Updata_Flag = 0;                            //远程升级主机程序进度显示标志   1:开始升级  2:升级完成

unsigned char	data_tirexps[120];
unsigned char	OneKeyCallFlag		= 0;                        //  一键拨号
unsigned char	BD_upgrad_contr		= 0;                        //  北斗升级控制

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


/*检查是否回到主界面*/
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




/*内容分隔为行，记录行首、行尾地址*/
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
		if( *p < 0x20 ) /*控制字符，换行*/
		{
			if( count ) /*有数据*/
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
			if( *p > 0x7F ) /*汉字*/
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
				pos		+= 2; /*需要增加两个*/
				p		+= 2;
				count	+= 2;
			}else
			{
				count++;
				pos++;
				p++;
			}
			if( count == bytes_per_row) /*正好*/
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

/*拷贝n个字符到dst中，遇到\0 和不可见字符结束*/
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
