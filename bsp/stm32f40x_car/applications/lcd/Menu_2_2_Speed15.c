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
#include "Menu_Include.h"
#include "sed1520.h"


//unsigned long AvrgSpdPerMin_write=0;//存储过整小时的速度，记录存储的条数
unsigned char			Speed_15min[120];       //读取 最近15分钟的速度信息

static unsigned char	Speed_15minFlag = 0;    //取速度正确或者错误 0:正确 1:错误
static unsigned char	CheckSpeedFlag	= 0;
static unsigned char	index_speednum;
static unsigned char	SpeedNumScreen	= 0;
static unsigned char	ReadSpeedFlag	= 0;    //进入读取一次，再次按下确认键不读


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
unsigned char Fetch_15minSpeed( unsigned char Num15 )
{
	unsigned char	rec_num = 0;
	unsigned char	i		= 0, j = 0, k = 0;
	unsigned char	Read_15minData[75];
	unsigned int	kk			= 0;
	u8				temp_data	= 0, temp_time = 0;

#if NEED_TODO	
//数据格式:15 1 xx xx xx xx xx sp 2 xx xx xx xx xx sp ... ...15 xx xx xx xx xx sp

	if( avgspd_Mint_Wr >= 15 )
	{
		avgspd_Mint_Wr--;
		for( i = 0; i < 15; i++ )
		{
			memcpy( &Speed_15min[2 + i * 7], Avrgspd_Mint.datetime, 3 );    //BCD
			Speed_15min[2 + i * 7 + 3]	= Temp_Gps_Gprs.Time[0];
			Speed_15min[2 + i * 7 + 4]	= avgspd_Mint_Wr - 14 + i;          //不是BCD码
			Speed_15min[2 + i * 7 + 5]	= Avrgspd_Mint.avgrspd[avgspd_Mint_Wr - 14 + i];
		}
#if 0
//==================================================
//rt_kprintf("\r\n******当前分钟数>15********",rec_num);
		rec_num = Api_DFdirectory_Query( spdpermin, 0 );
//rt_kprintf("\r\n已有整小时记录条数(%d 条)",rec_num);
		if( rec_num )
		{
			Api_DFdirectory_Read( spdpermin, Read_15minData, 70, 0, 0 ); // 从new-->old  读取


			/*rt_kprintf("\r\n(读取到)存储的最近一条整小时记录:");
			   rt_kprintf("\r\n %2x/%2x/%2x   %2x:%2x\r\n",Read_15minData[0],Read_15minData[1],Read_15minData[2],Read_15minData[3],Read_15minData[4]);
			   for(i=0;i<65;i++)
			   {
			   if(i%20==0)
			   rt_kprintf("\r\n");
			   rt_kprintf(" %2d",Read_15minData[i+5]);
			   }
			   rt_kprintf("\r\n存储的前一条数据读出完成\r\n");*/
			//判断读出的前一条记录是不是当前小时的前一个小时
			//if((Read_15minData[2]==Avrgspd_Mint.datetime[2])&&(Avrgspd_Mint.datetime[3]==(Read_15minData[3]+1)))
			temp_data	= ( Read_15minData[2] >> 4 ) * 10 + ( Read_15minData[2] & 0x0f );
			temp_time	= ( Read_15minData[3] >> 4 ) * 10 + ( Read_15minData[3] & 0x0f );
			//rt_kprintf("\r\n(读出)%d日，%d点\r\n",temp_data,temp_time);
			//rt_kprintf("\r\n(当前)%d日，%d点\r\n",Temp_Gps_Gprs.Date[2],Temp_Gps_Gprs.Time[0]);
		}
#endif
//==================================================
	}else
	{
		if( avgspd_Mint_Wr >= 1 )
		{
			avgspd_Mint_Wr--;
		}
		rec_num = Api_DFdirectory_Query( spdpermin, 0 );
		//rt_kprintf("\r\n已有整小时记录条数(%d 条)",rec_num);
		//rt_kprintf("\r\n当前分钟数<15,已有 %d 条,需要从前一小时内读取 %d 条",avgspd_Mint_Wr,(15-avgspd_Mint_Wr));
		if( rec_num )
		{
			Api_DFdirectory_Read( spdpermin, Read_15minData, 70, 0, 0 ); // 从new-->old  读取


			/*rt_kprintf("\r\n(读取到)存储的最近一条整小时记录:");
			   rt_kprintf("\r\n %2x/%2x/%2x   %2x:%2x\r\n",Read_15minData[0],Read_15minData[1],Read_15minData[2],Read_15minData[3],Read_15minData[4]);
			   for(i=0;i<65;i++)
			   {
			   if(i%10==0)
			   rt_kprintf("\r\n");
			   rt_kprintf(" %2d",Read_15minData[i+5]);
			   }
			   rt_kprintf("\r\n存储的前一条数据读出完成\r\n");*/
			//判断读出的前一条记录是不是当前小时的前一个小时
			//if((Read_15minData[2]==Avrgspd_Mint.datetime[2])&&(Avrgspd_Mint.datetime[3]==(Read_15minData[3]+1)))
			temp_data	= ( Read_15minData[2] >> 4 ) * 10 + ( Read_15minData[2] & 0x0f );
			temp_time	= ( Read_15minData[3] >> 4 ) * 10 + ( Read_15minData[3] & 0x0f );
			//
			//rt_kprintf("\r\n(读出)%2x日,%2x时",temp_data,temp_time);
			//rt_kprintf("\r\n(当前)%2x日,%2x时\r\n",Temp_Gps_Gprs.Date[2],Temp_Gps_Gprs.Time[0]);
			if( ( Temp_Gps_Gprs.Date[2] == temp_data ) && ( Temp_Gps_Gprs.Time[0] == ( temp_time + 1 ) ) )
			{
				//rt_kprintf("\r\n读出时间是前一小时，填充\r\n");
				for( i = 0; i < ( 14 - avgspd_Mint_Wr ); i++ )                                      //avgspd_Mint_Wr
				{
					rt_kprintf( " %d", Read_15minData[64 - ( 14 - avgspd_Mint_Wr - 1 ) + i] );      //不需要读取存储的数据
					memcpy( &Speed_15min[2 + i * 7], Read_15minData, 4 );                           //yymmddhh
					Speed_15min[2 + i * 7 + 4]	= 60 - ( 14 - avgspd_Mint_Wr ) + i;                 //mm
					Speed_15min[2 + i * 7 + 5]	= Read_15minData[64 - ( 13 - avgspd_Mint_Wr ) + i]; //speed
				}
				k = i;
			}else //读出的前一条记录不是当前小时的前一个小时的数据
			{
				//rt_kprintf("\r\n读出时间不是前一小时，补0\r\n");
				for( i = 0; i < ( 14 - avgspd_Mint_Wr ); i++ )                                      //avgspd_Mint_Wr
				{
					rt_kprintf( " %d", Avrgspd_Mint.avgrspd[i] );                                   //不需要读取存储的数据
					memcpy( &Speed_15min[2 + i * 7 + k * 7], Avrgspd_Mint.datetime, 3 );
					kk									= ( Avrgspd_Mint.datetime[3] >> 4 ) * 10 + ( Avrgspd_Mint.datetime[3] & 0x0f ) - 1;
					Speed_15min[2 + i * 7 + k * 7 + 3]	= ( ( kk / 10 ) << 4 ) + ( kk % 10 );       //
					Speed_15min[2 + i * 7 + k * 7 + 4]	= 60 - ( 14 - avgspd_Mint_Wr ) + i;;
					Speed_15min[2 + i * 7 + k * 7 + 5]	= 0;
				}
				k = i;
			}
			// rt_kprintf("\r\n当前小时速度:\r\n");
			for( i = 0; i <= avgspd_Mint_Wr; i++ )
			{
				rt_kprintf( " %d", Avrgspd_Mint.avgrspd[i] ); //不需要读取存储的数据
				memcpy( &Speed_15min[2 + i * 7 + k * 7], Avrgspd_Mint.datetime, 4 );
				Speed_15min[2 + i * 7 + k * 7 + 4]	= i;
				Speed_15min[2 + i * 7 + k * 7 + 5]	= Avrgspd_Mint.avgrspd[i];
			}
		}else //没有存过整小时的速度
		{
			rt_kprintf( "\r\n 没有整小时的数据存入 " );
			for( i = 0; i < ( 14 - avgspd_Mint_Wr ); i++ )                                  //avgspd_Mint_Wr
			{
				memcpy( &Speed_15min[2 + i * 7 + k * 7], Avrgspd_Mint.datetime, 3 );
				kk									= ( Avrgspd_Mint.datetime[3] >> 4 ) * 10 + ( Avrgspd_Mint.datetime[3] & 0x0f ) - 1;
				Speed_15min[2 + i * 7 + k * 7 + 3]	= ( ( kk / 10 ) << 4 ) + ( kk % 10 );   //
				Speed_15min[2 + i * 7 + k * 7 + 4]	= 60 - ( 14 - avgspd_Mint_Wr ) + i;;
				Speed_15min[2 + i * 7 + k * 7 + 5]	= 0;
			}
			k = i;
			//rt_kprintf("\r\n  %d  条人为补充数据,  已有分钟数条数=%d\r\n",k,avgspd_Mint_Wr+1);
			for( i = 0; i <= avgspd_Mint_Wr; i++ )
			{
				rt_kprintf( "\r\n i=%d   speed=%d", i, Avrgspd_Mint.avgrspd[i] ); //不需要读取存储的数据
				memcpy( &Speed_15min[2 + i * 7 + k * 7], Avrgspd_Mint.datetime, 4 );
				Speed_15min[2 + i * 7 + k * 7 + 4]	= i;
				Speed_15min[2 + i * 7 + k * 7 + 5]	= Avrgspd_Mint.avgrspd[i];
			}
		}
		//rt_kprintf("\r\n 停车前15分钟车速读完");
	}
	Speed_15min[0] = Num15; //总条数
	for( i = 0, j = 0; i < Num15; i++, j++ )
	{
		Speed_15min[1 + j * 7] = i + 1;
	}

	//转换填成需要的格式
	for( i = 0; i < Num15; i++ )
	{
		memcpy( speed_time_rec[i], &Speed_15min[2 + i * 7], 6 );
		//rt_kprintf("\r\ni=%d,yy=%x,mm=%x,dd=%x,hh=%x,mm=%d,speed=%d",i,Speed_15min[2+i*7],Speed_15min[2+i*7+1],Speed_15min[2+i*7+2],Speed_15min[2+i*7+3],Speed_15min[2+i*7+4],Speed_15min[2+i*7+5]);
		speed_time_rec[i][0]	= ( speed_time_rec[i][0] >> 4 ) * 10 + ( speed_time_rec[i][0] & 0x0F ); //yy
		speed_time_rec[i][1]	= ( speed_time_rec[i][1] >> 4 ) * 10 + ( speed_time_rec[i][1] & 0x0F ); //mm
		speed_time_rec[i][2]	= ( speed_time_rec[i][2] >> 4 ) * 10 + ( speed_time_rec[i][2] & 0x0F ); //dd

		speed_time_rec[i][3]	= ( speed_time_rec[i][3] >> 4 ) * 10 + ( speed_time_rec[i][3] & 0x0F ); //hh
		speed_time_rec[i][4]	= speed_time_rec[i][4];                                                 //mm
		speed_time_rec[i][5]	= speed_time_rec[i][5];                                                 //speed
		//rt_kprintf("\r\ni=%d, min=%d",i,speed_time_rec[i][4]);
	}
	//判断错误数据,分钟数全为0
	for( i = 0; i < ( Num15 - 1 ); i++ )
	{
		if( ( speed_time_rec[i][3] > 23 ) || ( speed_time_rec[i][4] > 59 ) )
		{
			rt_kprintf( "\r\n小时:%d, 分钟:%d,", speed_time_rec[i][3], speed_time_rec[i][4] );
			for( i = 0; i < 15; i++ )
			{
				speed_time_rec[i][3]	= 0;    //hh
				speed_time_rec[i][4]	= 0;    //mm
				speed_time_rec[i][5]	= 0;    //speed
			}
			//return 1;
		}
		if( speed_time_rec[i][4] == speed_time_rec[i + 1][4] )
		{
			rt_kprintf( "\r\n取到错误信息" );
			return 1;
		}
	}
#endif	
	return 0;
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
static void drawspeed( unsigned char num )
{
	unsigned char t[50];
	memset( t, 0, sizeof( t ) );
	lcd_fill( 0 );

	sprintf( (char*)t, "%02d %02d:%02d %02d", num, speed_time_rec[num - 1][3], speed_time_rec[num - 1][4], speed_time_rec[num - 1][5] );
	lcd_text12( 10, 0, (char*)t, 11, LCD_MODE_SET );

	sprintf( (char*)t, "%02d %02d:%02d %02d", num + 1, speed_time_rec[num][3], speed_time_rec[num][4], speed_time_rec[num][5] );
	lcd_text12( 10, 11, (char*)t, 11, LCD_MODE_SET );

	sprintf( (char*)t, "%02d %02d:%02d %02d", num + 2, speed_time_rec[num + 1][3], speed_time_rec[num + 1][4], speed_time_rec[num + 1][5] );
	lcd_text12( 10, 22, (char*)t, 11, LCD_MODE_SET );

	if( index_speednum <= 9 )
	{
		index_speednum += 3;
	}
	lcd_update_all( );
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
static void msg( void *p )
{
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
static void show( void )
{
	index_speednum	= 1;
	SpeedNumScreen	= 0; //显示速度第几屏的序号
	ReadSpeedFlag	= 1;


	/*memset(send_data,0,sizeof(send_data));
	   send_data[0]=0x0F;
	   send_data[1]=0xF4;
	   send_data[2]=0x00;
	   send_data[3]=0x00;
	   rt_mb_send(&mb_hmi, (rt_uint32_t)&send_data[0]);*/

	lcd_fill( 0 );
	lcd_text12( 10, 3, "停车前15分钟速度", 16, LCD_MODE_SET );
	lcd_text12( 24, 19, "按确认键查看", 12, LCD_MODE_SET );
	lcd_update_all( );
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
static void keypress( unsigned int key )
{
	switch( key )
	{
		case KEY_MENU:
			Speed_15minFlag = 0;
			CounterBack		= 0;
			CheckSpeedFlag	= 0;
			pMenuItem		= &Menu_2_InforCheck;
			pMenuItem->show( );
			ReadSpeedFlag = 0;
			break;
		case KEY_OK:
			if( ReadSpeedFlag == 1 )
			{
				ReadSpeedFlag	= 0;
				Speed_15minFlag = Fetch_15minSpeed( 15 );   //取最近15分钟的速度信息
				rt_kprintf( "\r\n读最近15分钟的速度信息结果= %d 0:right 1:error\r\n", Speed_15minFlag );
				drawspeed( 1 );                             //显示第1屏
				SpeedNumScreen	= 0;
				CheckSpeedFlag	= 1;
			}
			break;
		case KEY_UP:
			if( ( SpeedNumScreen >= 1 ) && ( CheckSpeedFlag == 1 ) )
			{
				SpeedNumScreen--;
				drawspeed( SpeedNumScreen * 3 + 1 );
			}
			break;
		case KEY_DOWN:
			if( ( SpeedNumScreen <= 3 ) && ( CheckSpeedFlag == 1 ) )
			{
				SpeedNumScreen++;
				drawspeed( SpeedNumScreen * 3 + 1 );
			}
			break;
	}
}

/**/
static void timetick( unsigned int systick )
{
	Cent_To_Disp( );
	CounterBack++;
	if( CounterBack != MaxBankIdleTime )
	{
		return;
	}
	pMenuItem = &Menu_1_Idle;
	pMenuItem->show( );
	CounterBack = 0;

	CheckSpeedFlag	= 0;
	Speed_15minFlag = 0;
	ReadSpeedFlag	= 0;
}

MENUITEM Menu_2_2_Speed15 =
{
	"停车前15min速度",
	15,
	&show,
	&keypress,
	&timetick,
	&msg,
	(void*)0
};

/************************************** The End Of File **************************************/
