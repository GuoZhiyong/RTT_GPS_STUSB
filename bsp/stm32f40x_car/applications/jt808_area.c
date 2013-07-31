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
#include <rtthread.h>
#include <rtdevice.h>
#include <dfs_posix.h>

#include "stm32f4xx.h"
#include "jt808.h"
#include <finsh.h>
#include "sst25.h"
#include "math.h"
#include "jt808_gps.h"
#include "camera.h"


#define	DF_LineAddress_Start	0xF2000			///路线数据存储开始位置
#define DF_LineAddress_End		0x106000			///路线数据存储结束位置
#define DF_LINESaveSect			0x1000			///路线数据存储最小间隔


#define	DF_AreaAddress_Start	0x106000			///电子围栏数据存储开始位置
#define DF_AreaAddress_End		0x108000			///电子围栏数据存储结束位置
#define DF_AreaSaveSect			0x40			///电子围栏数据存储最小间隔





#define AREANUM			24

#define LINENUM			20

#define AREA_BUF_SIZE	4096
#define LINE_BUF_SIZE	4096

#define WLENGH			111200			///每个纬度的距离，该值为固定值单位为米

#define JLENGH			111320			///赤道上每个经度的距离，单位为米



#ifndef BIT
#define BIT(i) ((unsigned long)(1<<i))
#endif


typedef enum
{
	AREA_None=0,				///无数据
	AREA_Circular,				///圆形
	AREA_Rectangle,				///矩形
	AREA_Polygon,				///多边形
	AREA_Line,					///线路
}ENUM_AREA;


typedef __packed struct				///定义了一个坐标
{
	u32				Lati;				///地球坐标纬度
	u32 			Longi;				///地球坐标经度
}TypeStruct_Coor;					


typedef __packed struct
{
	u8			alarm_bit;				///报警标志位取值为0~31
	u8			attri_id;				///附加信息 ID 
	u8 			len;					///附加信息长度
	u8			data[7];				///报警附加消息体
}Type_AREA_ALARM;


typedef __packed struct
{
	char 		Head[4];			///幻数部分，表示当前数据区域为某固定数据开始
	u16			Len;				///数据长度，包括数据头部分内容
	ENUM_AREA	State;				///区域类型，如果为AREA_None表示无数据
	TypeStruct_Coor	Rect_left;		///区域外围矩形->左上角经纬度
	TypeStruct_Coor	Rect_right;		///区域外围矩形->右下角经纬度
	u32			ID;					///区域ID
	u8			Data[1];			///区域
}TypeDF_AreaHead;

typedef __packed struct
{
	TypeDF_AreaHead *area_data;		///电子围栏数据指针
	u8 			in_area;			///在区域里面的内部标记,0表示不在该区域,1表示在
	u32			in_tick;			///进入该区域的时间，单位为tick
	u32			speed_tick;			///超速的时间，单位为tick
	u8			speed;				///超速标记，为0表示没有超速，为1表示超速
}Type_AreaInfo;

typedef __packed struct
{
	TypeDF_AreaHead line_head;		///线路数据包头
	u8			*line_data;			///线路数据指针
	u8 			in_area;			///在区域里面的内部标记0表示不在该区域
	u32			in_tick;			///进入该区域的时间，单位为tick
	u32			road_speed_tick;	///超时的时间，单位为tick
	u32			road_id;			///当前所在的路段ID
	u32			road_in_tick;		///进入该路段的时间，单位为tick
	u32 		road_timer_max;		///路段行驶过长阈值
	u32 		road_timer_min;		///路段行驶不足阈值
	u8			deviate;			///路段偏离，为0表示没有偏离，为1表示偏离
	u8			speed;				///超速标记，为0表示没有超速，为1表示超速
}Type_LineInfo;

typedef __packed struct
{

 u16				area_len;			///电子围栏数据使用长度
 u16				line_len;			///线路数据使用长度
 u8 				area_num;			///电子围栏数量
 u8 				line_num;			///线路数量
 Type_AreaInfo	 	area_info[AREANUM];	///电子围栏参数及数据
 Type_LineInfo		line_info[LINENUM];	///线路参数及数据
}Type_AreaPara;

static u8	area_buffer[AREA_BUF_SIZE];    ///存储电子围栏的数据区域
static u8	line_buffer[LINE_BUF_SIZE];    ///存储线路的数据区域

static const char	AREA_HEAD[]={"AREA"};
Type_AreaPara		Area_Para;

/* 消息队列控制块，用于电子围栏和线路报警事件的处理*/
struct rt_messagequeue mq_area;
/* 消息队列中用到的放置消息的内存池*/
static char msgpool_area[256];

static u16 test_speed;

//*********************************************************************************
//声明外部函数
//*********************************************************************************
extern u32 Times_To_LongInt(T_TIMES *T);
extern uint32_t buf_to_data( uint8_t * psrc, uint8_t width );
extern uint16_t data_to_buf( uint8_t * pdest, uint32_t data, uint8_t width );
extern void printer_data_hex(u8 *pSrc,u16 nSrcLength);


/*********************************************************************************
*函数名称:rt_err_t area_jt808_0x8600(uint16_t fram_num,uint8_t *pmsg,u16 msg_len)
*功能描述:计算指定的纬度上每个经度的长度.
*输	入:	latitude:当前纬度值，单位为百万分之一度
*输	出:none
*返 回 值:double 	:返回实际每个经度的长度，单位为米
*作	者:白养民
*创建日期:2013-06-16
*---------------------------------------------------------------------------------
*修 改 人:
*修改日期:
*修改描述:
*********************************************************************************/
double Cal_Longi_Distance(u32 latitude)
{
 double dx,dy;
 double	tempd;
 //u8 tempbuf[128];
 double pi=3.1415926;
 
 tempd = latitude;
 dy = (tempd/1000000/180) *pi ;
 dx = JLENGH * cos(dy);
 //sprintf(tempbuf,"\r\n dx=%f\r\n",dx);
 //rt_kprintf(tempbuf);
 return dx;
}


/*********************************************************************************
*函数名称:rt_err_t area_jt808_0x8600(uint16_t fram_num,uint8_t *pmsg,u16 msg_len)
*功能描述:计算指定的纬度上每个经度的长度，然后根据用户输入的距离计算对应的经度数，单位为
			以度为单位的经度值乘以 10 的 6 次方，精确到百万分之一度
*输	入:	latitude:当前纬度值，单位为百万分之一度
		distance:用户输入的距离
*输	出:none
*返 回 值:u32 	:返回实际的经度数
*作	者:白养民
*创建日期:2013-06-16
*---------------------------------------------------------------------------------
*修 改 人:
*修改日期:
*修改描述:
*********************************************************************************/
u32 dis_to_Longi(u32 latitude,u32 distance)
{
 u32	tempu32data;
 double	tempd;
 //u8 tempbuf[128];
 
 tempd = distance;
 tempu32data = tempd*1000000/Cal_Longi_Distance(latitude);
 tempu32data ++;
 //sprintf(tempbuf,"\r\n longitude=%d\r\n",tempu32data);
 //rt_kprintf(tempbuf);
 return tempu32data ;
}


/*********************************************************************************
*函数名称:u32 AnglPoint2Point(s32 Lati_0,s32 Longi_0,s32 Lati_1,s32 Longi_1)
*功能描述:计算两点之间的距离，
*输	入:	Lati_1	:第一个点的纬度，单位为百万分之一度
		Longi_1	:第一个点的经度，单位为百万分之一度
		Lati_2	:第二个点的纬度，单位为百万分之一度
		Longi_2	:第二个点的经度，单位为百万分之一度
*输	出:none
*返 回 值:u32 	:返回实际的距离，单位为米
*作	者:白养民
*创建日期:2013-06-16
*---------------------------------------------------------------------------------
*修 改 人:
*修改日期:
*修改描述:
*********************************************************************************/
u32 AnglPoint2Point(s32 Lati_0,s32 Longi_0,s32 Lati_1,s32 Longi_1)
{
 s32	temps32data;
 double	tempd1;
 double	x,y;
 //u8 tempbuf[128];

 x	= Longi_1 - Longi_0;
 y	= Lati_1  - Lati_0;
 if( y == 0)
 	{
	if( x >= 0 )
		temps32data = 0;
	else
		temps32data = 180;
 	}
 else if( x == 0 )
 	{
	if( y > 0 )
		temps32data = 90;
	else
		temps32data = 270;
 	}
 else
 	{
	tempd1 = atan2(y,x);
	temps32data = tempd1 * 180 / 3.1415926;
	if(temps32data < 0)
	 	{
	 	temps32data += 360;
	 	}
 	}
 //rt_kprintf("Angle = %d",temps32data);
 return temps32data;
}

FINSH_FUNCTION_EXPORT( AnglPoint2Point, AnglPoint2Point);


/*********************************************************************************
*函数名称:u32 dis_Point2Point(s32 Lati_1,s32 Longi_1,s32 Lati_2,s32 Longi_2)
*功能描述:计算两点之间的距离，
*输	入:	Lati_1	:第一个点的纬度，单位为百万分之一度
		Longi_1	:第一个点的经度，单位为百万分之一度
		Lati_2	:第二个点的纬度，单位为百万分之一度
		Longi_2	:第二个点的经度，单位为百万分之一度
*输	出:none
*返 回 值:u32 	:返回实际的距离，单位为米
*作	者:白养民
*创建日期:2013-06-16
*---------------------------------------------------------------------------------
*修 改 人:
*修改日期:
*修改描述:
*********************************************************************************/
u32 dis_Point2Point(s32 Lati_1,s32 Longi_1,s32 Lati_2,s32 Longi_2)
{
 s32	temps32data;
 double	tempd1,tempd2;
 
 //rt_kprintf("\r\n Lati_1= %d,Longi_1= %d,Lati_2= %d,Longi_2= %d",Lati_1,Longi_1,Lati_2,Longi_2);
 
 temps32data	= abs(Lati_1 + Lati_2);
 tempd1 = Cal_Longi_Distance( temps32data/2 );
 tempd1 *= abs( Longi_1 - Longi_2 );
 tempd1 /= 1000000.0;
 tempd2 = WLENGH;
 tempd2 *= abs( Lati_1 - Lati_2 );
 tempd2 /= 1000000;
 
 tempd2 *= tempd2;
 tempd1 *= tempd1;
 
 temps32data = sqrt( tempd1 + tempd2);
 //rt_kprintf("dis = %d",temps32data);
 return (u32)temps32data ;
}

FINSH_FUNCTION_EXPORT( dis_Point2Point, dis_Point2Point);



/*********************************************************************************
*函数名称:u32 dis_Point2Line(u32 Cur_Lati, u32 Cur_Longi,u32 Lati_1,u32 Longi_1,u32 Lati_2,u32 Longi_2)
*功能描述:计算点到线之间的距离，
*输	入:	Cur_Lati	:当前点的纬度，单位为百万分之一度
		Cur_Longi	:当前点的经度，单位为百万分之一度
		Lati_1		:第一个点的纬度，单位为百万分之一度
		Longti_1	:第一个点的经度，单位为百万分之一度
		Lati_2		:第二个点的纬度，单位为百万分之一度
		Longti_2	:第二个点的经度，单位为百万分之一度
*输	出:none
*返 回 值:u32 	:返回实际的距离，单位为米
*作	者:白养民
*创建日期:2013-06-16
*数学原形:	1、当前处理的线条的函数为		y = a*x;
			2、和这个线条垂直的线条的函数为	y1= -(1/a)*x1 + b;
			3、首先计算出:  a,b及1/a，然后根据这三个值计算出两条线条相交的点的X轴坐标
				有公式	a*x	= -(1/a)*x + b;   因此  x2 = b / (a + 1/a),
			4、根据x2坐标计算y2坐标，y = a * x,然后计算这个点到当前点的距离，同时计算这个点
				是否在两点组成的线中间，是则返回正数，否则返回负数
			5、注意，所有的点的计算都以Lati_1和Longi_1作为原点,经度为x轴，纬度为y轴
*---------------------------------------------------------------------------------
*修 改 人:
*修改日期:
*修改描述:
*********************************************************************************/
u32 dis_Point2Line(s32 Cur_Lati, s32 Cur_Longi,s32 Lati_1,s32 Longi_1,s32 Lati_2,s32 Longi_2)
{
 double	x,y,x1,y1,x2,y2;
 s32	temp_lati,temp_longi;
 double a,a1,b;
 u32	tempu32data;
 //u8 tempbuf[128];
 


 y1 	= Cur_Lati - Lati_1;
 x1		= Cur_Longi - Longi_1;
 y		= Lati_2 - Lati_1;
 x		= Longi_2 - Longi_1;

 //sprintf(tempbuf,"\r\n x=%f,y=%f,x1=%f,y1=%f ",x,y,x1,y1);
 //rt_kprintf(tempbuf);

 if(x==0)
 	{
 	 x2	= 0;
	 y2	= y1;
 	}
 else if(y==0)
 	{
 	 x2	= x1;
	 y2	= 0;
 	}
 else
 	{
	 a 	= y / x;
	 a1	= x / y;
	 b	= y1 + a1 * x1;
	 x2	= b / ( a + a1 );
	 y2 = a * x2;
 	}

 temp_lati 	= y2 + Lati_1;
 temp_longi	= x2 + Longi_1;
 
 //sprintf(tempbuf,"\r\n x2=%f,y2=%f,temp_longi=%d,temp_lati=%d ",x2,y2,temp_longi,temp_lati);
 //rt_kprintf(tempbuf);
 if(((x >= 0 && x2 >= 0)||(x <= 0 && x2 <= 0))&&((y >= 0 && y2 >= 0)||(y <= 0 && y2 <= 0)))
 	{
	if((abs(x2) <= abs(x))&&(abs(y2) <= abs(y)))
	 	{
	 	tempu32data = dis_Point2Point(temp_lati,temp_longi,Cur_Lati,Cur_Longi);
		//rt_kprintf("dis = %d",tempu32data);
	 	return tempu32data;
	 	}
 	}
 return 0xFFFFFFFF;
}
FINSH_FUNCTION_EXPORT( dis_Point2Line, dis_Point2Line);


/*********************************************************************************
*函数名称:u8 Check_CooisInRect(TypeStruct_Coor *pCoo, TypeDF_AreaHead *pHead )
*功能描述:判断当前位置是否在该矩形区域里面
*输	入:	pCoo	:当前位置坐标
		pHead	:当前矩形区域
*输	出:none
*返 回 值:u8	1:表示在该区域，	0:表示不在该区域
*作	者:白养民
*创建日期:2013-07-01
*---------------------------------------------------------------------------------
*修 改 人:
*修改日期:
*修改描述:
*********************************************************************************/
u8 Check_CooisInRect(TypeStruct_Coor *pCoo, TypeDF_AreaHead *pHead )
{
 if((pCoo->Lati > pHead->Rect_left.Lati)||(pCoo->Longi < pHead->Rect_left.Longi))
 	{
 	return 0;
 	}
 if((pCoo->Lati < pHead->Rect_right.Lati)||(pCoo->Longi > pHead->Rect_right.Longi))
	{
	return 0;
 	}
 return 1;
}


/*********************************************************************************
*函数名称:u8 Check_Area_Time(u8 * StartTime,u8 *EndTime)
*功能描述:判断当前时间是否在指定时间段里面，用于808协议的电子围栏和线路时间比较
*输	入:	StartTime	:开始时间
		EndTime		:结束时间
*输	出:none
*返 回 值:u8	1:表示在该时间段，	0:表示不在该时间段
*作	者:白养民
*创建日期:2013-07-01
*---------------------------------------------------------------------------------
*修 改 人:
*修改日期:
*修改描述:
*********************************************************************************/
u8 Check_Area_Time(u8 * StartTime,u8 *EndTime)
{
 u8 i;
 u8 T0[6];
 u8 T1[6];
 u8 T2[6];
 u32 temp0,temp1,temp2;
 memset(T0 ,0 ,6);
 memset(T1 ,0 ,6);
 memset(T2 ,0 ,6);
 
 for(i=0;i<6;i++)
 	{
 	if( StartTime[i] )
 		{
		break;
 		}
 	}
 memcpy(T0+i, gps_datetime+i, 6-i);
 memcpy(T1+i, StartTime+i, 6-i);
 memcpy(T2+i, EndTime+i, 6-i);
 
 temp0 = Times_To_LongInt((T_TIMES *)T0);
 temp1 = Times_To_LongInt((T_TIMES *)T1);
 temp2 = Times_To_LongInt((T_TIMES *)T2);

 if((temp0 >=  temp1)&&(temp0 <  temp2))
 	{
 	return 1;
 	}
 return 0;
}


/*********************************************************************************
*函数名称:u8 Check_Angle(u32 angle,u32 angle_start,u32 angle_end )
*功能描述:判断当前角度是否在指定的角度里面，指定的角度是按照逆时针方式旋转的
*输	入:	angle		:当前的角度
		StartTime	:起始角度值
		EndTime		:结束角度值
*输	出:none
*返 回 值:u8	1:表示在该角度范围	0:表示不在该角度范围
*作	者:白养民
*创建日期:2013-07-01
*---------------------------------------------------------------------------------
*修 改 人:
*修改日期:
*修改描述:
*********************************************************************************/
u8 Check_Angle(u32 angle,u32 angle_start,u32 angle_end )
{
 if(angle_start > angle_end)
 	{
 	if((angle > angle_end)&&(angle < angle_start))
 		{
 		return 0;
 		}
	else
		{
		return 1;
		}
 	}
 else
 	{
 	if((angle >= angle_start)&&(angle <= angle_end))
 		{
 		return 1;
 		}
	else
		{
		return 0;
		}
 	}
}

/*********************************************************************************
*函数名称:u16 area_flash_read_area( u8 *pdest,u16 maxlen)
*功能描述:读取flash中的电子围栏到全局变量参数 Area_Para中
*输	入:	pdatabuf	:将要保存电子围栏数据的ram区域
		maxlen		:pdatabuf的长度，告诉函数防止溢出
*输	出:	none
*返 回 值:u16	0:发生了溢出	1:正常返回
*作	者:白养民
*创建日期:2013-06-25
*---------------------------------------------------------------------------------
*修 改 人:
*修改日期:
*修改描述:
*********************************************************************************/
u16 area_flash_read_area( u8 *pdatabuf,u16 maxlen)
{
 u8 *ptempbuf = pdatabuf;
 u32 TempAddress;
 TypeDF_AreaHead TempAreaHead;
 u16 ret = 0;
 
 Area_Para.area_len		= 0;
 Area_Para.area_num		= 0;
 memset(pdatabuf, 0 , maxlen);
 memset((void *)(Area_Para.area_info),0,sizeof(Area_Para.area_info));
 
 for(TempAddress=DF_AreaAddress_Start;TempAddress<DF_AreaAddress_End;)
 	{
 	sst25_read(TempAddress,(u8 *)&TempAreaHead,sizeof(TypeDF_AreaHead));
	if((strncmp(TempAreaHead.Head,AREA_HEAD,strlen(AREA_HEAD))==0)&&(TempAreaHead.State))
		{
		///如果buf长度不够或者已经读取到得区域总数量超过AREANUM，则直接返回0，表示溢出
		if((TempAreaHead.Len > maxlen - Area_Para.area_len)||( Area_Para.area_num >= AREANUM ))
			{
			ret = 0; goto FUNC_RET;
			}
		sst25_read(TempAddress,ptempbuf + Area_Para.area_len, TempAreaHead.Len);
		Area_Para.area_info[Area_Para.area_num++].area_data = (TypeDF_AreaHead*)(ptempbuf + Area_Para.area_len);
		Area_Para.area_len += TempAreaHead.Len;
		TempAddress+=(TempAreaHead.Len+DF_AreaSaveSect-1)/DF_AreaSaveSect*DF_AreaSaveSect;
		}
	else
		{
		TempAddress+=DF_AreaSaveSect;
		}
 	}
 ret = 1;
 FUNC_RET:
 return ret;
}


/*********************************************************************************
*函数名称:u16 area_flash_read_line( u8 *pdest,u16 maxlen)
*功能描述:读取flash中的线路参数到全局变量参数 Area_Para中
*输	入:	pdatabuf	:将要保存线路数据的ram区域
		maxlen		:pdatabuf的长度，告诉函数防止溢出
*输	出:	none
*返 回 值:u16	0:发生了溢出	1:正常返回
*作	者:白养民
*创建日期:2013-06-25
*---------------------------------------------------------------------------------
*修 改 人:
*修改日期:
*修改描述:
*********************************************************************************/
u16 area_flash_read_line( u8 *pdatabuf,u16 maxlen)
{
 u8 *ptempbuf = pdatabuf;
 u32 TempAddress;
 u16 sublen;		///flash存储增加的包头部分的长度
 TypeDF_AreaHead TempAreaHead;
 u16 ret = 0;
 
 sublen	= sizeof(TypeDF_AreaHead) - 5;
 
 Area_Para.line_len		= 0;
 Area_Para.line_num		= 0;
 memset(pdatabuf, 0 , maxlen);
 memset(&Area_Para.line_info,0,sizeof(Area_Para.line_info));
 
 for(TempAddress=DF_LineAddress_Start;TempAddress<DF_LineAddress_End;)
 	{
 	sst25_read(TempAddress,(u8 *)&TempAreaHead,sizeof(TypeDF_AreaHead));
	if((strncmp(TempAreaHead.Head,AREA_HEAD,strlen(AREA_HEAD))==0)&&(TempAreaHead.State == AREA_Line))
		{
		///如果已经读取到得区域总数量超过LINENUM，则直接返回0，表示溢出
		if( Area_Para.line_num >= LINENUM )
			{
			ret = 0; goto FUNC_RET;
			}
		memcpy(&Area_Para.line_info[Area_Para.line_num].line_head,&TempAreaHead,sizeof(TypeDF_AreaHead));
		///如果buf长度不够则不填充数据到数据buf中
		if(TempAreaHead.Len - sublen <= maxlen - Area_Para.line_len)
			{
			sst25_read(TempAddress + sublen,ptempbuf + Area_Para.line_len, TempAreaHead.Len - sublen);
			Area_Para.line_info[Area_Para.line_num].line_data = ptempbuf + Area_Para.line_len;
			Area_Para.line_len += TempAreaHead.Len - sublen;
			}
		Area_Para.line_num++;
		}
	TempAddress+=DF_LINESaveSect;
 	}
 
 ret = 1;
 FUNC_RET:
 return ret;
}


/*********************************************************************************
*函数名称:u16 area_flash_get_line_data( u8 *pdatabuf, u16 maxlen, Type_LineInfo *AreaInfo)
*功能描述:读取flash中的线路数据到AreaInfo中，该函数也会修改全局参数 Area_Para.line_info 的数据指针line_data
*输	入:	pdatabuf	:将要保存线路数据的ram区域
		maxlen		:pdatabuf的长度，告诉函数防止溢出
		AreaInfo	:
*输	出:	none
*返 回 值:u16	0:发生了溢出	1:正常返回
*作	者:白养民
*创建日期:2013-06-25
*---------------------------------------------------------------------------------
*修 改 人:
*修改日期:
*修改描述:
*********************************************************************************/
u16 area_flash_get_line_data( u8 *pdatabuf, u16 maxlen, Type_LineInfo *AreaInfo)
{
 u32 i;
 u8 *ptempbuf = pdatabuf;
 u32 TempAddress;
 u16 sublen;		///flash存储增加的包头部分的长度
 TypeDF_AreaHead TempAreaHead;
 u16 ret = 0;
 
 sublen	= sizeof(TypeDF_AreaHead) - 5;
 
 ///如果buf长度不够则将buf清空，并且将全局参数Area_Para.line_info[i].line_data 全部清空
 if(TempAreaHead.Len - sublen > maxlen - Area_Para.line_len)
	{
	Area_Para.line_len	= 0;
	memset(pdatabuf,0,maxlen);
	for(i=0;i<LINENUM;i++)
		{
		Area_Para.line_info[i].line_data = RT_NULL;
		}
	}
 rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
 for(TempAddress=DF_LineAddress_Start;TempAddress<DF_LineAddress_End;)
 	{
 	sst25_read(TempAddress,(u8 *)&TempAreaHead,sizeof(TypeDF_AreaHead));
	if((strncmp(TempAreaHead.Head,AREA_HEAD,strlen(AREA_HEAD))==0)&&(TempAreaHead.State == AREA_Line))
		{
		///如果已经读取到得区域总数量超过LINENUM，则直接返回0，表示溢出
		if( Area_Para.line_num >= LINENUM )
			{
			ret = 0; goto FUNC_RET;
			}
		///如果当前读取到得ID和要处理的ID相同
		if(TempAreaHead.ID == AreaInfo->line_head.ID)
			{
			//如果buf长度够则填充数据到数据buf中
			if(TempAreaHead.Len - sublen <= maxlen - Area_Para.line_len)
				{
				sst25_read(TempAddress + sublen,ptempbuf + Area_Para.line_len, TempAreaHead.Len - sublen);
				Area_Para.line_info[Area_Para.line_num].line_data = ptempbuf + Area_Para.line_len;
				Area_Para.line_len += TempAreaHead.Len - sublen;
				ret = 1;
				}
			else
				{
				ret = 0; 
				}
			goto FUNC_RET;
			}
		Area_Para.line_num++;
		}
	TempAddress+=DF_LINESaveSect;
 	}
 ret = 0;
FUNC_RET:
 rt_sem_release(&sem_dataflash);
 return ret;
}


/*********************************************************************************
*函数名称:u16 area_flash_write_area( u8 *pdatabuf,u16 maxlen,TypeDF_AreaHead *pData)
*功能描述:读取flash中的电子围栏到全局变量参数 Area_Para中
*输	入:	pdatabuf	:将要保存电子围栏数据的ram区域
		maxlen		:pdatabuf的长度，告诉函数防止溢出
		pData		:将要存入的电子围栏数据
*输	出:	none
*返 回 值:u16	0:发生了溢出	1:正常返回
*作	者:白养民
*创建日期:2013-06-25
*---------------------------------------------------------------------------------
*修 改 人:
*修改日期:
*修改描述:
*********************************************************************************/
u16 area_flash_write_area( u8 *pdatabuf,u16 maxlen,TypeDF_AreaHead *pData)
{
 u8 i;
 u8 *ptempbuf = pdatabuf;
 u32 TempAddress;
 TypeDF_AreaHead TempAreaHead;
 u16 ret = 0;

 if((pData->State==0)||(pData->State>AREA_Polygon))
 	{
 	return 0;
 	}
 if((pData->Len > maxlen - Area_Para.area_len)||( Area_Para.area_num >= AREANUM ))
 	{
 	return 0;
 	}
 rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
 ///处理修改电子围栏区域命令
 for(TempAddress=DF_AreaAddress_Start;TempAddress<DF_AreaAddress_End;)
 	{
 	sst25_read(TempAddress,(u8 *)&TempAreaHead,sizeof(TypeDF_AreaHead));
	if((strncmp(TempAreaHead.Head,AREA_HEAD,strlen(AREA_HEAD))==0)&&(TempAreaHead.State))
		{
		if((pData->Len== TempAreaHead.Len)&&(pData->ID == TempAreaHead.ID))
			{
			sst25_write_back(TempAddress,(u8 *)pData,pData->Len);
			area_flash_read_area(area_buffer,AREA_BUF_SIZE);
			ret = 1; goto FUNC_RET;
			}
		TempAddress+=(TempAreaHead.Len+DF_AreaSaveSect-1)/DF_AreaSaveSect*DF_AreaSaveSect;
		}
	else
		{
		TempAddress+=DF_AreaSaveSect;
		}
 	}
 ///处理增加电子围栏区域
 memcpy(ptempbuf + Area_Para.area_len,pData,pData->Len);
 Area_Para.area_info[Area_Para.area_num++].area_data = (TypeDF_AreaHead*)(ptempbuf + Area_Para.area_len);
 Area_Para.area_len += pData->Len;

 ///擦除电子围栏区域flash
 for(TempAddress=DF_AreaAddress_Start;TempAddress<DF_AreaAddress_End;TempAddress+=0x1000)
 	{
 	sst25_erase_4k(TempAddress);
 	}
 TempAddress = DF_AreaAddress_Start;
 for( i=0; i < Area_Para.area_num; i++ )
 	{
 	if(TempAddress + Area_Para.area_info[i].area_data->Len >  DF_AreaAddress_End)
 		{
 		ret = 0; goto FUNC_RET;
 		}
 	sst25_write_through(TempAddress,(u8 *)Area_Para.area_info[i].area_data,Area_Para.area_info[i].area_data->Len);
	TempAddress += (Area_Para.area_info[i].area_data->Len+DF_AreaSaveSect-1)/DF_AreaSaveSect*DF_AreaSaveSect;
 	}
 
 ret = 1;
 FUNC_RET:
 rt_sem_release(&sem_dataflash);
 return ret;
}


/*********************************************************************************
*函数名称:u16 area_flash_write_line( u8 *pdatabuf,u16 maxlen,TypeDF_AreaHead *pData)
*功能描述:向flash中存入线路
*输	入:	pdatabuf	:将要保存电子围栏数据的ram区域
		maxlen		:pdatabuf的长度，告诉函数防止溢出
		pData		:将要存入的线路数据
*输	出:	none
*返 回 值:u16	0:发生了溢出	1:正常返回
*作	者:白养民
*创建日期:2013-06-25
*---------------------------------------------------------------------------------
*修 改 人:
*修改日期:
*修改描述:
*********************************************************************************/
u16 area_flash_write_line( u8 *pdatabuf,u16 maxlen,TypeDF_AreaHead *pData)
{
 u8 i;
 u32 TempAddress;
 TypeDF_AreaHead TempAreaHead;

 if((pData->State==0)||(pData->State != AREA_Line))
 	{
 	return 0;
 	}
 if( Area_Para.line_num >= LINENUM )
 	{
 	return 0;
 	}
 rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
 TempAddress = DF_LineAddress_Start;
 for( i = 0; i < LINENUM; i++ )
 	{
 	sst25_read(TempAddress,(u8 *)&TempAreaHead,sizeof(TypeDF_AreaHead));
	if((strncmp(TempAreaHead.Head,AREA_HEAD,strlen(AREA_HEAD)))||(TempAreaHead.State == 0))
		{
		sst25_write_back(TempAddress,(u8 *)pData,pData->Len);
		break;
		}
	
	TempAddress += DF_LINESaveSect;
	}
 
 area_flash_read_line(line_buffer,LINE_BUF_SIZE);
 
 rt_sem_release(&sem_dataflash);
 return 1;
}


/*********************************************************************************
*函数名称:void area_flash_del_area( u32 del_id , ENUM_AREA	del_State)
*功能描述:删除指定类型的电子围栏
*输	入:	del_id		:将要删除的电子围栏ID，如果该值为0表示删除所有指定类型的电子围栏
		del_State	:将要删除的电子围栏类型
*输	出:	none
*返 回 值:none
*作	者:白养民
*创建日期:2013-06-25
*---------------------------------------------------------------------------------
*修 改 人:
*修改日期:
*修改描述:
*********************************************************************************/
void area_flash_del_area( u32 del_id, ENUM_AREA	del_State)
{
 u32 TempAddress;
 TypeDF_AreaHead TempAreaHead;
 
 rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
 for(TempAddress=DF_AreaAddress_Start;TempAddress<DF_AreaAddress_End;)
 	{
 	sst25_read(TempAddress,(u8 *)&TempAreaHead,sizeof(TypeDF_AreaHead));
	if((strncmp(TempAreaHead.Head,AREA_HEAD,strlen(AREA_HEAD))==0)&&(TempAreaHead.State))
		{
		if(((TempAreaHead.ID == del_id)||(0 == del_id))&&(del_State == TempAreaHead.State))
			{
			TempAreaHead.State = AREA_None;
			sst25_write_through(TempAddress,(u8 *)&TempAreaHead,sizeof(TypeDF_AreaHead));
			}
		TempAddress+=(TempAreaHead.Len+DF_AreaSaveSect-1)/DF_AreaSaveSect*DF_AreaSaveSect;
		}
	else
		{
		TempAddress+=DF_AreaSaveSect;
		}
 	}
 area_flash_read_area(area_buffer,AREA_BUF_SIZE);
 rt_sem_release(&sem_dataflash);
}

/*********************************************************************************
*函数名称:void area_flash_del_line( u32 del_id )
*功能描述:删除线路
*输	入:	del_id	:将要删除的线路ID，如果该值为0表示删除所有线路
*输	出:	none
*返 回 值:none
*作	者:白养民
*创建日期:2013-06-25
*---------------------------------------------------------------------------------
*修 改 人:
*修改日期:
*修改描述:
*********************************************************************************/
void area_flash_del_line( u32 del_id )
{
 u32 TempAddress;
 TypeDF_AreaHead TempAreaHead;
 rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
 for(TempAddress=DF_LineAddress_Start;TempAddress<DF_LineAddress_End;)
 	{
 	sst25_read(TempAddress,(u8 *)&TempAreaHead,sizeof(TypeDF_AreaHead));
	if((strncmp(TempAreaHead.Head,AREA_HEAD,strlen(AREA_HEAD))==0)&&(TempAreaHead.State))
		{
		if((TempAreaHead.ID == del_id)||(0 == del_id))
			{
			TempAreaHead.State = AREA_None;
			sst25_write_through(TempAddress,(u8 *)&TempAreaHead,sizeof(TypeDF_AreaHead));
			}
		}
	TempAddress+=DF_LINESaveSect;
 	}
 area_flash_read_line(line_buffer,LINE_BUF_SIZE);
 rt_sem_release(&sem_dataflash);
}


/*********************************************************************************
*函数名称:void area_jt808_commit_ack(u16 fram_num,u16 cmd_id,u8 isFalse)
*功能描述:通用应答，单包OK应答
*输	入:	fram_num:应答流水号
		cmd_id	:接收时的808命令
		statue 	:表示状态，0:表示OK	1:表示失败	2:表示消息有误	3:表示不支持
*输	出:	none
*返 回 值:rt_err_t
*作	者:白养民
*创建日期:2013-06-24
*---------------------------------------------------------------------------------
*修 改 人:
*修改日期:
*修改描述:
*********************************************************************************/
void area_jt808_commit_ack(u16 fram_num,u16 cmd_id,u8 isFalse)
{
	u8 pbuf[8];
	data_to_buf(pbuf, fram_num, 2);
	data_to_buf(pbuf+2, cmd_id, 2);
	pbuf[4] = isFalse;
	jt808_tx_ack(0x0001,pbuf,2);
}


/*********************************************************************************
*函数名称:rt_err_t area_jt808_del(uint8_t linkno,uint8_t *pmsg, ENUM_AREA  del_State))
*功能描述:808删除圆形区域处理函数
*输	入:	pmsg		:808消息体数据
		msg_len		:808消息体长度
		del_State	:表示要删除的类型
*输	出:none
*返 回 值:rt_err_t
*作	者:白养民
*创建日期:2013-06-16
*---------------------------------------------------------------------------------
*修 改 人:
*修改日期:
*修改描述:
*********************************************************************************/
rt_err_t area_jt808_del(uint8_t linkno,uint8_t *pmsg, ENUM_AREA  del_State)
{
 u16 i;
 u32 tempu32data;
 u16 cmd_id;
 u8 *msg;
 //u16 msg_len;
 u16 fram_num;

 cmd_id		= buf_to_data( pmsg , 2 );
 //msg_len	= buf_to_data( pmsg + 2, 2 ) & 0x3FF;
 fram_num	= buf_to_data( pmsg + 10, 2 );
 pmsg 		+= 12;
 msg		= pmsg;
 if( pmsg[0] == 0)
 	{
 	if(AREA_Line== del_State)
 		{
 		area_flash_del_line( 0 );
 		}
	else
		{
		area_flash_del_area( 0 , del_State );
		}
 	}
 else
 	{
	msg++;
	for(i=0;i<pmsg[0];i++)
	 	{
	 	tempu32data = buf_to_data(msg, 2);
		if(AREA_Line== del_State)
	 		{
	 		area_flash_del_line( tempu32data );
	 		}
		else
			{
			area_flash_del_area( tempu32data , del_State );
			}
		msg += 4;
	 	}
 	}
 area_jt808_commit_ack(fram_num,cmd_id,0);
 return RT_EOK;
}


/*********************************************************************************
*函数名称:rt_err_t area_jt808_0x8600(uint8_t linkno,uint8_t *pmsg)
*功能描述:808设置圆形区域处理函数
*输	入:	pmsg	:808消息体数据
		msg_len	:808消息体长度
*输	出:none
*返 回 值:rt_err_t
*作	者:白养民
*创建日期:2013-06-16
*---------------------------------------------------------------------------------
*修 改 人:
*修改日期:
*修改描述:
*********************************************************************************/
rt_err_t area_jt808_0x8600(uint8_t linkno,uint8_t *pmsg)
{
 u16 i;
 u16 datalen;
 u16 tempu16data;
 u32 tempu32data;
 u32 Longi,Lati;		///经度和纬度
 TypeDF_AreaHead *pTempHead;
 u8 tempbuf[64];
 double temp_d;
 
 u8 *msg;
 //u16 msg_len;
 u16 fram_num;
 
 //msg_len	= buf_to_data( pmsg + 2, 2 ) & 0x3FF;
 fram_num	= buf_to_data( pmsg + 10, 2 );
 pmsg 		+= 12;
 msg	= pmsg;

 pTempHead = (TypeDF_AreaHead *)tempbuf;
 ///808消息体部分
 msg+=2;	///设置属性 1byte,区域总数 1byte
 
 for(i=0;i<pmsg[1];i++)
 	{
 	datalen = 23;
 	datalen += 18;
 	tempu16data = buf_to_data(msg+4, 2);
	if(tempu16data & BIT(0))
		{
		datalen += 12;
		}
	if(tempu16data & BIT(1))
		{
		datalen += 3;
		}
	memcpy(pTempHead->Head, AREA_HEAD, 4);
	pTempHead->Len	= datalen;
	pTempHead->State= AREA_Circular;

	///计算该形状的外框矩形
	Lati	= buf_to_data(msg+6, 4);
	Longi	= buf_to_data(msg+10, 4);
		///计算圆半径对应的经度数值
	tempu32data=dis_to_Longi(Lati,buf_to_data(msg+14, 4));
	
	pTempHead->Rect_left.Longi	= Longi - tempu32data;
	pTempHead->Rect_right.Longi	= Longi + tempu32data;
		///计算圆半径对应的纬度数值
	temp_d = buf_to_data(msg+14, 4);
	tempu32data = temp_d*1000000/WLENGH;
	pTempHead->Rect_left.Lati	= Lati + tempu32data;
	pTempHead->Rect_right.Lati	= Lati - tempu32data;
	///复制电子围栏数据到将要写入的结构体中
	memcpy((void *)&pTempHead->ID,msg,datalen);
	area_flash_write_area(area_buffer,sizeof(area_buffer),pTempHead);
 	}
 area_jt808_commit_ack(fram_num,0x8600,0);
 return RT_EOK;
}


/*********************************************************************************
*函数名称:rt_err_t area_jt808_0x8601(uint8_t linkno,uint8_t *pmsg)
*功能描述:808删除圆形区域处理函数
*输	入:	pmsg	:808消息体数据
		msg_len	:808消息体长度
*输	出:none
*返 回 值:rt_err_t
*作	者:白养民
*创建日期:2013-06-16
*---------------------------------------------------------------------------------
*修 改 人:
*修改日期:
*修改描述:
*********************************************************************************/
rt_err_t area_jt808_0x8601(uint8_t linkno,uint8_t *pmsg)
{
 return area_jt808_del(linkno, pmsg, AREA_Circular);
}


/*********************************************************************************
*函数名称:rt_err_t area_jt808_0x8602(uint8_t linkno,uint8_t *pmsg)
*功能描述:808设置矩形区域处理函数
*输	入:	pmsg	:808消息体数据
		msg_len	:808消息体长度
*输	出:none
*返 回 值:rt_err_t
*作	者:白养民
*创建日期:2013-06-16
*---------------------------------------------------------------------------------
*修 改 人:
*修改日期:
*修改描述:
*********************************************************************************/
rt_err_t area_jt808_0x8602(uint8_t linkno,uint8_t *pmsg)
{
 u16 i;
 u16 datalen;
 u16 tempu16data;
 TypeDF_AreaHead *pTempHead;
 u8 tempbuf[64];
 
 u8 *msg;
// u16 msg_len;
 u16 fram_num;
 
 //msg_len	= buf_to_data( pmsg + 2, 2 ) & 0x3FF;
 fram_num	= buf_to_data( pmsg + 10, 2 );
 pmsg 		+= 12;
 msg	= pmsg;

 pTempHead = (TypeDF_AreaHead *)tempbuf;
 ///808消息体部分
 msg+=2;	///设置属性 1byte,区域总数 1byte
 
 for(i=0;i<pmsg[1];i++)
 	{
 	datalen = 23;
 	datalen += 22;
 	tempu16data = buf_to_data(msg+4, 2);
	if(tempu16data & BIT(0))
		{
		datalen += 12;
		}
	if(tempu16data & BIT(1))
		{
		datalen += 3;
		}
	memcpy(pTempHead->Head, AREA_HEAD, 4);
	pTempHead->Len	= datalen;
	pTempHead->State= AREA_Rectangle;

	///计算该形状的外框矩形
	pTempHead->Rect_left.Longi	= buf_to_data(msg+10, 4);
	pTempHead->Rect_right.Longi	= buf_to_data(msg+18, 4);
	pTempHead->Rect_left.Lati	= buf_to_data(msg+6, 4);
	pTempHead->Rect_right.Lati	= buf_to_data(msg+14, 4);
	
	///复制电子围栏数据到将要写入的结构体中
	memcpy((void *)&pTempHead->ID,msg,datalen);
	area_flash_write_area(area_buffer,sizeof(area_buffer),pTempHead);
 	}
 area_jt808_commit_ack(fram_num,0x8602,0);
 return RT_EOK;
}


/*********************************************************************************
*函数名称:rt_err_t area_jt808_0x8603(uint8_t linkno,uint8_t *pmsg)
*功能描述:808删除矩形区域处理函数
*输	入:	pmsg	:808消息体数据
		msg_len	:808消息体长度
*输	出:none
*返 回 值:rt_err_t
*作	者:白养民
*创建日期:2013-06-16
*---------------------------------------------------------------------------------
*修 改 人:
*修改日期:
*修改描述:
*********************************************************************************/
rt_err_t area_jt808_0x8603(uint8_t linkno,uint8_t *pmsg)
{
 return area_jt808_del(linkno, pmsg, AREA_Rectangle);
}


/*********************************************************************************
*函数名称:rt_err_t area_jt808_0x8604(uint8_t linkno,uint8_t *pmsg)
*功能描述:808设置多边形区域处理函数
*输	入:	pmsg	:808消息体数据
		msg_len	:808消息体长度
*输	出:none
*返 回 值:rt_err_t
*作	者:白养民
*创建日期:2013-06-16
*---------------------------------------------------------------------------------
*修 改 人:
*修改日期:
*修改描述:
*********************************************************************************/
rt_err_t area_jt808_0x8604(uint8_t linkno,uint8_t *pmsg)
{
 u16 i;
 u16 datalen;
 u16 tempu16data;
 u32 Longi,Lati;		///经度和纬度
 TypeDF_AreaHead *pTempHead = NULL;
 
 u8 *msg;
// u16 msg_len;
 u16 fram_num;
 
 //msg_len	= buf_to_data( pmsg + 2, 2 ) & 0x3FF;
 fram_num	= buf_to_data( pmsg + 10, 2 );
 pmsg 		+= 12;
 msg	= pmsg;

 ///808消息体部分
 
 datalen = 23;
 datalen += 6;
 tempu16data = buf_to_data(msg+4, 2);
 if(tempu16data & BIT(0))
	 {
	 datalen += 12;
	 }
 if(tempu16data & BIT(1))
	 {
	 datalen += 3;
	 }
 ///顶点数量
 tempu16data = buf_to_data(msg+datalen, 2);
 if(tempu16data > 64)			///限定最大顶点数为64
 	tempu16data = 64;
 datalen += 2;
 
 pTempHead = (TypeDF_AreaHead *)rt_malloc( datalen + tempu16data * 8 );
 if( pTempHead )
 	{
 	///计算该形状的外框矩形
	for( i=0; i< tempu16data; i++)
	 	{
	 	Lati	= buf_to_data(msg + datalen + i*8, 4);
	 	Longi	= buf_to_data(msg + datalen + i*8 + 4, 4);
	 	if(i == 0)
	 		{
			pTempHead->Rect_left.Lati	= Lati;
			pTempHead->Rect_right.Lati	= Lati;
			pTempHead->Rect_left.Longi = Longi;
			pTempHead->Rect_right.Longi = Longi;
	 		}
		else
			{
			if(pTempHead->Rect_left.Lati < Lati )
				pTempHead->Rect_left.Lati = Lati;
			if(pTempHead->Rect_right.Lati > Lati )
				pTempHead->Rect_right.Lati = Lati;
			
			if(pTempHead->Rect_left.Longi > Longi )
				pTempHead->Rect_left.Longi = Longi;
			if(pTempHead->Rect_right.Longi < Longi )
				pTempHead->Rect_right.Longi = Longi;
			}
	 	}
	datalen += tempu16data * 8;
	
	memcpy(pTempHead->Head, AREA_HEAD, 4);
	pTempHead->Len	= datalen;
	pTempHead->State= AREA_Polygon;

	///复制电子围栏数据到将要写入的结构体中
	memcpy((void *)&pTempHead->ID,msg,datalen);
	area_flash_write_area(area_buffer,sizeof(area_buffer),pTempHead);
	area_jt808_commit_ack(fram_num,0x8604,0);
	rt_free( pTempHead );
 	}
 else
 	{
 	area_jt808_commit_ack(fram_num,0x8604,3);
 	}
 return RT_EOK;
}


/*********************************************************************************
*函数名称:rt_err_t area_jt808_0x8605(uint8_t linkno,uint8_t *pmsg)
*功能描述:808删除多边形区域处理函数
*输	入:	pmsg	:808消息体数据
		msg_len	:808消息体长度
*输	出:none
*返 回 值:rt_err_t
*作	者:白养民
*创建日期:2013-06-16
*---------------------------------------------------------------------------------
*修 改 人:
*修改日期:
*修改描述:
*********************************************************************************/
rt_err_t area_jt808_0x8605(uint8_t linkno,uint8_t *pmsg)
{
 return area_jt808_del(linkno, pmsg, AREA_Polygon);
}


/*********************************************************************************
*函数名称:rt_err_t area_jt808_0x8606(uint8_t linkno,uint8_t *pmsg)
*功能描述:808设置线路处理函数
*输	入:	pmsg	:808消息体数据
		msg_len	:808消息体长度
*输	出:none
*返 回 值:rt_err_t
*作	者:白养民
*创建日期:2013-06-16
*---------------------------------------------------------------------------------
*修 改 人:
*修改日期:
*修改描述:
*********************************************************************************/
rt_err_t area_jt808_0x8606(uint8_t linkno,uint8_t *pmsg)
{
 u16 i;
 u16 datalen;
 u16 tempu16data;
 u32 Longi,Lati;		///经度和纬度
 TypeDF_AreaHead *pTempHead = NULL;
 
 u8 *msg;
 u16 msg_len;
 u16 fram_num;
 
 msg_len	= buf_to_data( pmsg + 2, 2 ) & 0x3FF;
 fram_num	= buf_to_data( pmsg + 10, 2 );

 ///处理多包的情况，暂时不支持多包
 if((pmsg[2] & 0x20) == 0)
 	{
 	tempu16data	= buf_to_data( pmsg + 14, 2 );
	if( tempu16data )
		{
		area_jt808_commit_ack(fram_num,0x8606,3);
 		return RT_ERROR;
		}
 	pmsg 		+= 16;
 	}
 else
 	{
	pmsg 		+= 12;
 	}
 msg	= pmsg;

 ///808消息体部分
 datalen = 23;
 datalen += 6;
 tempu16data = buf_to_data(msg+4, 2);
 if(tempu16data & BIT(0))
	 {
	 datalen += 12;
	 }
 ///顶点数量
 tempu16data = buf_to_data(msg+datalen, 2);
 if(tempu16data > 32)			///限定最大顶点数为64
 	tempu16data = 32;
 datalen += 2;
 
 if( tempu16data > (msg_len - datalen) / 25 )
 	{
 	tempu16data = (msg_len - datalen) / 25 ;
 	}
 
 pTempHead = (TypeDF_AreaHead *)rt_malloc( datalen + tempu16data * 25 );
 if( pTempHead )
 	{
 	///计算该形状的外框矩形
	for( i=0; i< tempu16data; i++)
	 	{
	 	Lati	= buf_to_data(msg + datalen + i*25 + 8, 4);
	 	Longi	= buf_to_data(msg + datalen + i*25 + 12, 4);
	 	if(i == 0)
	 		{
			pTempHead->Rect_left.Lati	= Lati;
			pTempHead->Rect_right.Lati	= Lati;
			pTempHead->Rect_left.Longi = Longi;
			pTempHead->Rect_right.Longi = Longi;
	 		}
		else
			{
			if(pTempHead->Rect_left.Lati < Lati )
				pTempHead->Rect_left.Lati = Lati;
			if(pTempHead->Rect_right.Lati > Lati )
				pTempHead->Rect_right.Lati = Lati;
			
			if(pTempHead->Rect_left.Longi > Longi )
				pTempHead->Rect_left.Longi = Longi;
			if(pTempHead->Rect_right.Longi < Longi )
				pTempHead->Rect_right.Longi = Longi;
			}
	 	}
	datalen += tempu16data * 25;
	
	memcpy(pTempHead->Head, AREA_HEAD, 4);
	pTempHead->Len	= datalen;
	pTempHead->State= AREA_Line;

	///复制电子围栏数据到将要写入的结构体中
	memcpy((void *)&pTempHead->ID,msg,datalen);
	area_flash_write_line(line_buffer,sizeof(line_buffer),pTempHead);
	area_jt808_commit_ack(fram_num,0x8606,0);
	rt_free( pTempHead );
 	}
 else
 	{
 	area_jt808_commit_ack(fram_num,0x8604,3);
 	}
 return RT_EOK;
}


/*********************************************************************************
*函数名称:rt_err_t area_jt808_0x8607(uint8_t linkno,uint8_t *pmsg)
*功能描述:808删除线路区域处理函数
*输	入:	pmsg	:808消息体数据
		msg_len	:808消息体长度
*输	出:none
*返 回 值:rt_err_t
*作	者:白养民
*创建日期:2013-06-16
*---------------------------------------------------------------------------------
*修 改 人:
*修改日期:
*修改描述:
*********************************************************************************/
rt_err_t area_jt808_0x8607(uint8_t linkno,uint8_t *pmsg)
{
 return area_jt808_del(linkno, pmsg, AREA_Line);
}


/*********************************************************************************
*函数名称:void area_alarm_enter( Type_AreaInfo *AreaInfo )
*功能描述:电子围栏进入报警
*输	入:	AreaInfo	:当前电子围栏信息
*输	出:none
*返 回 值:none
*作	者:白养民
*创建日期:2013-07-01
*---------------------------------------------------------------------------------
*修 改 人:
*修改日期:
*修改描述:
*********************************************************************************/
void area_alarm_enter( Type_AreaInfo *AreaInfo )
{
 Type_AREA_ALARM area_alarm;
 memset(&area_alarm, 0, sizeof(Type_AREA_ALARM));
 area_alarm.alarm_bit	= 20;
 area_alarm.attri_id	= 0x12;
 area_alarm.len			= 6;
 area_alarm.data[0]		= AreaInfo->area_data->State;
 data_to_buf(area_alarm.data+1, AreaInfo->area_data->ID, 4);
 area_alarm.data[5]		= 0;
 rt_mq_send(&mq_area,(void *)&area_alarm,sizeof(Type_AREA_ALARM));
 rt_kprintf("\n 进入电子围栏:ID = %d",AreaInfo->area_data->ID);
}


/*********************************************************************************
*函数名称:void area_alarm_leave( Type_AreaInfo *AreaInfo )
*功能描述:电子围栏离开报警
*输	入:	AreaInfo	:当前电子围栏信息
*输	出:none
*返 回 值:none
*作	者:白养民
*创建日期:2013-07-01
*---------------------------------------------------------------------------------
*修 改 人:
*修改日期:
*修改描述:
*********************************************************************************/
void area_alarm_leave( Type_AreaInfo *AreaInfo )
{
 Type_AREA_ALARM area_alarm;
 memset(&area_alarm, 0, sizeof(Type_AREA_ALARM));
 area_alarm.alarm_bit	= 20;
 area_alarm.attri_id	= 0x12;
 area_alarm.len			= 6;
 area_alarm.data[0]		= AreaInfo->area_data->State;
 data_to_buf(area_alarm.data+1, AreaInfo->area_data->ID, 4);
 area_alarm.data[5]		= 1;
 rt_mq_send(&mq_area,(void *)&area_alarm,sizeof(Type_AREA_ALARM));
 rt_kprintf("\n 离开电子围栏:ID = %d",AreaInfo->area_data->ID);
}


/*********************************************************************************
*函数名称:void area_alarm_speed( Type_AreaInfo *AreaInfo )
*功能描述:电子围栏超速报警
*输	入:	AreaInfo	:当前电子围栏信息
*输	出: none
*返 回 值:none
*作	者:白养民
*创建日期:2013-07-01
*---------------------------------------------------------------------------------
*修 改 人:
*修改日期:
*修改描述:
*********************************************************************************/
void area_alarm_speed( Type_AreaInfo *AreaInfo )
{
 AreaInfo->speed	= 1;
 rt_kprintf("\n 超速报警:ID = %d",AreaInfo->area_data->ID);
}


/*********************************************************************************
*函数名称:void area_alarm_enter_line( Type_LineInfo *AreaInfo )
*功能描述:路线进入报警
*输	入:	AreaInfo	:当前线路信息
*输	出:none
*返 回 值:none
*作	者:白养民
*创建日期:2013-07-01
*---------------------------------------------------------------------------------
*修 改 人:
*修改日期:
*修改描述:
*********************************************************************************/
void area_alarm_enter_line( Type_LineInfo *AreaInfo )
{
 Type_AREA_ALARM area_alarm;
 memset(&area_alarm, 0, sizeof(Type_AREA_ALARM));
 area_alarm.alarm_bit	= 21;
 area_alarm.attri_id	= 0x12;
 area_alarm.len			= 6;
 area_alarm.data[0]		= AreaInfo->line_head.State;
 data_to_buf(area_alarm.data+1, AreaInfo->line_head.ID, 4);
 area_alarm.data[5]		= 0;
 rt_mq_send(&mq_area,(void *)&area_alarm,sizeof(Type_AREA_ALARM));
 rt_kprintf("\n 进入线路:ID = %d",AreaInfo->line_head.ID);
}


/*********************************************************************************
*函数名称:void area_alarm_leave_line( Type_LineInfo *AreaInfo )
*功能描述:线路离开报警
*输	入:	AreaInfo	:当前线路信息
*输	出:none
*返 回 值:none
*作	者:白养民
*创建日期:2013-07-01
*---------------------------------------------------------------------------------
*修 改 人:
*修改日期:
*修改描述:
*********************************************************************************/
void area_alarm_leave_line( Type_LineInfo *AreaInfo )
{
 Type_AREA_ALARM area_alarm;
 memset(&area_alarm, 0, sizeof(Type_AREA_ALARM));
 area_alarm.alarm_bit	= 21;
 area_alarm.attri_id	= 0x12;
 area_alarm.len			= 6;
 area_alarm.data[0]		= AreaInfo->line_head.State;
 data_to_buf(area_alarm.data+1, AreaInfo->line_head.ID, 4);
 area_alarm.data[5]		= 1;
 rt_mq_send(&mq_area,(void *)&area_alarm,sizeof(Type_AREA_ALARM));
 rt_kprintf("\n 离开线路:ID = %d",AreaInfo->line_head.ID);
}


/*********************************************************************************
*函数名称:void area_alarm_speed_road( Type_LineInfo *AreaInfo ,u32 road_id)
*功能描述:电子围栏超速报警
*输	入:	AreaInfo	:当前线路信息
		road_id		:路段ID
*输	出:none
*返 回 值:none
*作	者:白养民
*创建日期:2013-07-01
*---------------------------------------------------------------------------------
*修 改 人:
*修改日期:
*修改描述:
*********************************************************************************/
void area_alarm_road_speed( Type_LineInfo *AreaInfo ,u32 road_id)
{
	AreaInfo->speed	= 1;
}

/*********************************************************************************
*函数名称:void area_alarm_road_deviate( Type_LineInfo *AreaInfo,u32 road_id )
*功能描述:线路路段偏离报警
*输	入:	AreaInfo	:当前线路信息
		road_id		:路段ID
*输	出:none
*返 回 值:none
*作	者:白养民
*创建日期:2013-07-03
*---------------------------------------------------------------------------------
*修 改 人:
*修改日期:
*修改描述:
*********************************************************************************/
void area_alarm_road_deviate( Type_LineInfo *AreaInfo,u32 road_id )
{
}


/*********************************************************************************
*函数名称:void area_alarm_road_timer( u16 road_time,u32 road_id, u8 state)
*功能描述:线路路段偏离报警
*输	入:	road_time	:当前路段行驶时长
		road_id		:路段ID
		state		:行驶时间过长或不足报警，0表示时间不足，1表示时间太长
*输	出:none
*返 回 值:none
*作	者:白养民
*创建日期:2013-07-03
*---------------------------------------------------------------------------------
*修 改 人:
*修改日期:
*修改描述:
*********************************************************************************/
void area_alarm_road_timer( u16 road_time,u32 road_id, u8 state)
{
 Type_AREA_ALARM area_alarm;
 memset(&area_alarm, 0, sizeof(Type_AREA_ALARM));
 area_alarm.alarm_bit   = 22;
 area_alarm.attri_id    = 0x13;
 area_alarm.len		   = 6;
 data_to_buf(area_alarm.data, road_id, 4);
 data_to_buf(area_alarm.data+4, road_time, 2);
 area_alarm.data[6]	   = state;
 rt_mq_send(&mq_area,(void *)&area_alarm,sizeof(Type_AREA_ALARM));
}


/*********************************************************************************
*函数名称:u8 area_process_circular(TypeStruct_Coor *pCoo, Type_AreaInfo *AreaInfo )
*功能描述:判断当前位置是否在该圆形区域里面
*输	入:	pCoo	:当前位置坐标
		AreaInfo:当前圆形区域
*输	出:none
*返 回 值:u8	1:表示在该区域，	0:表示不在该区域
*作	者:白养民
*创建日期:2013-07-01
*---------------------------------------------------------------------------------
*修 改 人:
*修改日期:
*修改描述:
*********************************************************************************/
u8 area_process_circular(TypeStruct_Coor *pCoo, Type_AreaInfo *AreaInfo )
{
 u16 datalen;
 u16 attri;					///区域属性
 u32 lati,longi;			///区域中心点坐标
 u32 r;						///半径
 u32 d;						///距离
 u32 curspeed;				///当前速度
 u32 speed=0xFFFFFFFF;		///最高速度
 u32 speedtime=0xFFFFFFFF;	///超速时间
 rt_kprintf("\n 圆形区域判断");
 if( Check_CooisInRect( pCoo, AreaInfo->area_data) == 0)
	{
	rt_kprintf("\n 没在矩形里面");
 	goto AREA_OUT;
 	}
 attri	= buf_to_data(AreaInfo->area_data->Data, 2);
 datalen = 14;
 if(attri & BIT(0))		///根据时间
 	{
 	if(Check_Area_Time(&AreaInfo->area_data->Data[datalen],&AreaInfo->area_data->Data[6+datalen]) == 0)
 		{
 		rt_kprintf("\n 没在时间范围");
 		goto AREA_OUT;
 		}
	datalen += 12;
 	}
 if(attri & BIT(1))		///限速
 	{
 	speed = buf_to_data(AreaInfo->area_data->Data+datalen, 2);
 	speedtime = buf_to_data(AreaInfo->area_data->Data+datalen+2, 1);
	speedtime *= RT_TICK_PER_SECOND;
	datalen += 3;
 	}
 lati 	= buf_to_data(AreaInfo->area_data->Data+2, 4);
 longi	= buf_to_data(AreaInfo->area_data->Data+6, 4);
 r		= buf_to_data(AreaInfo->area_data->Data+10, 4);
 d		= dis_Point2Point(lati, longi, pCoo->Lati, pCoo->Longi );	///当前点到中心点的距离
 //rt_kprintf("\r\n circular_dis= %d,   r=%d",d,r);
 if( d <= r )
 	{
 	goto AREA_IN;		///进区域
 	}
 else
 	{
 	goto AREA_OUT;		///出区域
 	}
 
 AREA_IN:				///进区域
 //curspeed	= gps_speed;
 curspeed	= test_speed;
 if( AreaInfo->in_area == 0)
	{
 	AreaInfo->speed	  = 0;
	AreaInfo->in_area = 1;
	AreaInfo->in_tick = rt_tick_get();
	AreaInfo->speed_tick = rt_tick_get();
	area_alarm_enter(AreaInfo);
	}
 if(attri & BIT(1))		///限速
	{
	if( curspeed < speed )
		{
		AreaInfo->speed_tick = rt_tick_get();
		}
	if(rt_tick_get() - AreaInfo->speed_tick > speedtime)
		{
		area_alarm_speed(AreaInfo);
		}
	else
		{
	 	AreaInfo->speed		= 0;
		}
	}
 return 1;
 AREA_OUT:				///出区域
 if( AreaInfo->in_area == 1)
	{
	AreaInfo->speed		= 0;
	AreaInfo->in_area = 0;
	AreaInfo->in_tick = rt_tick_get();
	AreaInfo->speed_tick = rt_tick_get();
	area_alarm_leave(AreaInfo);
	}
 return 0;
}


/*********************************************************************************
*函数名称:u8 area_process_rectangle(TypeStruct_Coor *pCoo, Type_AreaInfo *AreaInfo )
*功能描述:判断当前位置是否在该矩形区域里面
*输	入:	pCoo	:当前位置坐标
		AreaInfo:当前矩形区域
*输	出:none
*返 回 值:u8	1:表示在该区域，	0:表示不在该区域
*作	者:白养民
*创建日期:2013-07-01
*---------------------------------------------------------------------------------
*修 改 人:
*修改日期:
*修改描述:
*********************************************************************************/
u8 area_process_rectangle(TypeStruct_Coor *pCoo, Type_AreaInfo *AreaInfo )
{
 u16 datalen;
 u16 attri;					///区域属性
 u32 curspeed;				///当前速度
 u32 speed=0xFFFFFFFF;		///最高速度
 u32 speedtime=0xFFFFFFFF;	///超速时间
 if( Check_CooisInRect( pCoo, AreaInfo->area_data) == 0)
	{
 	goto AREA_OUT;
 	}
 attri	= buf_to_data(AreaInfo->area_data->Data, 2);
 datalen = 18;
 if(attri & BIT(0))		///根据时间
 	{
 	if(Check_Area_Time(&AreaInfo->area_data->Data[datalen],&AreaInfo->area_data->Data[6+datalen]) == 0)
 		{
 		goto AREA_OUT;
 		}
	datalen += 12;
 	}
 
 if(attri & BIT(1))		///限速
 	{
 	speed = buf_to_data(AreaInfo->area_data->Data+datalen, 2);
 	speedtime = buf_to_data(AreaInfo->area_data->Data+datalen+2, 1);
	speedtime *= RT_TICK_PER_SECOND;
	datalen += 3;
 	}
 
 //AREA_IN:				///进区域
 //curspeed	= gps_speed;
 curspeed	= test_speed;
 if( AreaInfo->in_area == 0)
 	{
 	AreaInfo->speed	  = 0;
 	AreaInfo->in_area = 1;
 	AreaInfo->in_tick = rt_tick_get();
 	AreaInfo->speed_tick = rt_tick_get();
 	area_alarm_enter(AreaInfo);
 	}
 if(attri & BIT(1))		///限速
	{
	if( curspeed < speed )
	 	{
	 	AreaInfo->speed_tick = rt_tick_get();
	 	}
	if(rt_tick_get() - AreaInfo->speed_tick > speedtime)
	 	{
	 	area_alarm_speed(AreaInfo);
	 	}
	else
		{
	 	AreaInfo->speed		= 0;
		}
	}
 return 1;
 AREA_OUT:				///出区域
 if( AreaInfo->in_area == 1)
	{
	AreaInfo->speed		= 0;
	AreaInfo->in_area = 0;
	AreaInfo->in_tick = rt_tick_get();
	AreaInfo->speed_tick = rt_tick_get();
	area_alarm_leave(AreaInfo);
	}
 return 0;
}


/*********************************************************************************
*函数名称:u8 area_process_polygon(TypeStruct_Coor *pCoo, Type_AreaInfo *AreaInfo )
*功能描述:判断当前位置是否在该多边形区域里面
*输	入:	pCoo	:当前位置坐标
		AreaInfo:当前多边形区域
*输	出:none
*返 回 值:u8	1:表示在该区域，	0:表示不在该区域
*作	者:白养民
*创建日期:2013-07-01
*数学原形:	1、在区域内部的所有的点都会满足一个条件，该点到所有顶点的角度是可以覆盖360度区域所有角度
			2、计算时可以根据该点和所有连续的两点之间的角度小于180度的方法进行遍历，找出最大角度和最小角度
			当所有的点都遍历之后发现区域不是闭合的则表示该点不在多边形区域里面。
*---------------------------------------------------------------------------------
*修 改 人:
*修改日期:
*修改描述:
*********************************************************************************/
u8 area_process_polygon(TypeStruct_Coor *pCoo, Type_AreaInfo *AreaInfo )
{
 u16 i;
 u16 point_num;
 u16 datalen;
 u16 attri;					///区域属性
 u32 lati,longi;			///区域中心点坐标
 u32 curspeed;				///当前速度
 u32 speed=0xFFFFFFFF;		///最高速度
 u32 speedtime=0xFFFFFFFF;	///超速时间
 u16 angle_first=0,angle_last=0,angle_start=0,angle_end=0;///第一次的角度，最后一次的角度，开始角度，接收角度，
 //u16 angle_total=0;	///总角度
 u16 angle_1,angle_2,angle;				///计算当前点和多边形多点之间的角度
 u16 area_state = 0;			///该值为0x03表示在区域里面
 if( Check_CooisInRect( pCoo, AreaInfo->area_data) == 0)
	{
 	goto AREA_OUT;
 	}
 attri	= buf_to_data(AreaInfo->area_data->Data, 2);
 datalen = 2;
 if(attri & BIT(0))		///根据时间
 	{
 	if(Check_Area_Time(&AreaInfo->area_data->Data[datalen],&AreaInfo->area_data->Data[6+datalen]) == 0)
 		{
 		goto AREA_OUT;
 		}
	datalen += 12;
 	}
 if(attri & BIT(1))		///限速
 	{
 	speed = buf_to_data(AreaInfo->area_data->Data+datalen, 2);
 	speedtime = buf_to_data(AreaInfo->area_data->Data+datalen+2, 1);
	speedtime *= RT_TICK_PER_SECOND;
	datalen += 3;
 	}
 
 point_num	= buf_to_data(AreaInfo->area_data->Data+datalen, 2);
 datalen	+= 2;
 ///遍历一遍所有的点，计算出当前坐标是不是在该区域中
 for( i = 0; i <= point_num; i++)		///多遍历一次，计算最后的点和第一个点之间的区域角度
 	{
 	area_state	= 0;
 	if(i < point_num)
 		{
	 	lati 	= buf_to_data(AreaInfo->area_data->Data+2, 4);
		longi	= buf_to_data(AreaInfo->area_data->Data+6, 4);
		datalen	+= 8;
		angle	= AnglPoint2Point(pCoo->Lati, pCoo->Longi,lati, longi);
 		}
	else
		{
		angle	= angle_first;
		}
	if(i == 0)					///如果是第一个点，则将该点的值设置为当前值
		{
		angle_start	= angle;
		angle_end	= angle;
		angle_first	= angle;
		angle_last	= angle;
		}
	else
		{
		///查找对比当前角度和上次角度的大小，从而得出哪个点是旋转角度的开始点，哪个点是结束点
		///对比之前的点和当前的点，得出当前扫描的角度范围
		if((angle > angle_last)&&( angle - angle_last <= 180))
			{
			angle_1	= angle_last;
			angle_2	= angle;
			}
		else if((angle > angle_last)&&( angle - angle_last > 180))
			{
			angle_1	= angle;
			angle_2	= angle_last;
			}
		else if((angle <= angle_last)&&( angle_last - angle <= 180))
			{
			angle_1	= angle;
			angle_2	= angle_last;
			}
		else ///if((angle <= angle_last)&&( angle_last - angle > 180))
			{
			angle_1	= angle_last;
			angle_2	= angle;
			}
		///将当前扫描的角度范围放入总的角度范围中
		if(Check_Angle(angle_start,angle_1,angle_2))
			{
			area_state	|= BIT(0);
			angle_start	= angle_1;
			}
		if(Check_Angle(angle_end,angle_1,angle_2))
			{
			area_state	|= BIT(1);
			angle_end	= angle_2;
			}
		angle_last	= angle;
		if(area_state == 0x03)		///检查到了当前点在多边形区域中
			{
			break;
			}
		}
 	}
 if(area_state == 0x03)		///检查到了当前点在多边形区域中
	{
	goto AREA_IN;
	}
 else
 	{
 	goto AREA_OUT;
 	}
 AREA_IN:				///进区域
 curspeed	= gps_speed;
 if( AreaInfo->in_area == 0)
 	{
 	AreaInfo->speed	  = 0;
 	AreaInfo->in_area = 1;
 	AreaInfo->in_tick = rt_tick_get();
 	AreaInfo->speed_tick = rt_tick_get();
 	area_alarm_enter(AreaInfo);
 	}
 if(attri & BIT(1))		///限速
	{
	if( curspeed < speed )
	 	{
	 	AreaInfo->speed_tick = rt_tick_get();
	 	}
	if(rt_tick_get() - AreaInfo->speed_tick > speedtime)
	 	{
	 	area_alarm_speed(AreaInfo);
	 	}
	else
		{
	 	AreaInfo->speed		= 0;
		}
	}
 return 1;
 AREA_OUT:				///出区域
 if( AreaInfo->in_area == 1)
	{
	AreaInfo->speed		= 0;
	AreaInfo->in_area = 0;
	AreaInfo->in_tick = rt_tick_get();
	AreaInfo->speed_tick = rt_tick_get();
	area_alarm_leave(AreaInfo);
	}
 return 0;
}


/*********************************************************************************
*函数名称:u8 area_process_line(TypeStruct_Coor *pCoo, Type_LineInfo *AreaInfo )
*功能描述:判断当前位置是否在该线路上并且计算和线路相关报警及其他操作
*输	入:	pCoo	:当前位置坐标
		AreaInfo:当前线路
*输	出:none
*返 回 值:u8	1:表示在该区域，	0:表示不在该区域
*作	者:白养民
*创建日期:2013-07-02
*---------------------------------------------------------------------------------
*修 改 人:
*修改日期:
*修改描述:
*********************************************************************************/
u8 area_process_line(TypeStruct_Coor *pCoo, Type_LineInfo *AreaInfo )
{
 u16 i;
 u16 point_num;
 u16 datalen;
 u16 attri;						///线路属性
 u8  road_attri;				///路段属性
 u32 road_id;					///路段ID
 u16 road_width;				///路段宽度
 u32 road_timer_max=0xFFFFFFFF;	///路段行驶过长阈值
 u32 road_timer_min=0;			///路段行驶不足阈值
 u32 road_dis;				///当前位置坐标和路段的距离
 u32 road_dis_min=0xFFFFFFFF;	///计算的所有路段中，当前点距离最近的距离
 u32 road_dis_min_id=0;			///计算的所有路段中，当前点距离最近的路段ID
//u32 road_dis_min_width=0;		///计算的所有路段中，当前点距离最近的路段的宽度
 u32 lati,longi;				///坐标
 u32 lati_last=0,longi_last=0;		///上次处理的拐点坐标
 u32 curspeed;					///当前速度
 u32 speed=0xFFFFFFFF;			///最高速度
 u32 speedtime=0xFFFFFFFF;		///超速时间
	
 if( Check_CooisInRect( pCoo, &AreaInfo->line_head) == 0)
	{
 	goto AREA_OUT;
 	}
 ///检查当前处理线路数据是不是在RAM 的 buf中，为RT_NULL表示不在，需要从flash中获取数据
 if(AreaInfo->line_data == RT_NULL)
 	{
 	///从flash中获取当前处理的线路数据
 	if(area_flash_get_line_data(line_buffer, sizeof(line_buffer), AreaInfo) == 0)
 		{
 		goto AREA_OUT;
 		}
 	}
 attri	= buf_to_data(AreaInfo->line_data+4, 2);
 datalen = 6;
 if(attri & BIT(0))		///根据时间
 	{
 	if(Check_Area_Time(&AreaInfo->line_data[datalen],&AreaInfo->line_data[6+datalen]) == 0)
 		{
 		goto AREA_OUT;
 		}
	datalen += 12;
 	}
 
 point_num	= buf_to_data(AreaInfo->line_data+datalen, 2);
 datalen	+= 2;
 ///遍历一遍所有的点，计算出当前坐标是不是在该线路
 for( i = 0; i < point_num; i++)
 	{
	road_timer_max	= 0xFFFFFFFF;	///路段行驶过长阈值
	road_timer_min	= 0x00000000;	///路段行驶不足阈值
	speed			= 0xFFFFFFFF;	///最高速度
	speedtime		= 0xFFFFFFFF;	///超速时间
 	///拐点 ID 4BYTE
 	datalen += 4;
	///路段 ID 4BYTE
	road_id	= buf_to_data(AreaInfo->line_data+datalen, 4);
	datalen += 4;
	///拐点纬度  4BYTE
	lati 	= buf_to_data(AreaInfo->line_data+datalen, 4);
	datalen += 4;
	///拐点经度 4BYTE
	longi 	= buf_to_data(AreaInfo->line_data+datalen, 4);
	datalen += 4;
	///路段宽度 1BYTE
	road_width = AreaInfo->line_data[datalen++];
	///路段属性 1BYTE
	road_attri = AreaInfo->line_data[datalen++];
	if(road_attri & BIT(0))
		{
		///路段行驶过长阈值 2BYTE
		road_timer_max = buf_to_data(AreaInfo->line_data+datalen, 2);
		road_timer_max 	*= RT_TICK_PER_SECOND;
		datalen += 2;
		///路段行驶不足阈值 2BYTE
		road_timer_min = buf_to_data(AreaInfo->line_data+datalen, 2);
		road_timer_min 	*= RT_TICK_PER_SECOND;
		datalen += 2;
		}
	if(road_attri & BIT(0))
		{
		///路段最高速度 2BYTE
		speed 		= buf_to_data(AreaInfo->line_data+datalen, 2);
		datalen += 2;
		///路段超速持续时间 1BYTE
		speedtime	= buf_to_data(AreaInfo->line_data+datalen, 2);
		speedtime 	*= RT_TICK_PER_SECOND;
		datalen += 2;
		}
	if( i )		///第一个拐点没有线路，第二个拐点以后才有线路。
		{
		road_dis = dis_Point2Line(pCoo->Lati, pCoo->Longi,lati_last, longi_last, lati, longi);
		if( road_dis <= road_width)			///在该路段
			{
			road_dis_min	= 0;
			road_dis_min_id	= road_id;
			goto AREA_IN;
			}
		else if( road_dis != 0xFFFFFFFF )	///不在线路宽度范围内
			{
			road_dis -= road_width;
			if(road_dis < road_dis_min)
				{
				road_dis_min	= road_dis;
				road_dis_min_id	= road_id;
				}
			}
		else								///不在线路范围，什么都不做
			{
			}
		}
	lati_last	= lati;
	longi_last	=  longi;
 	}
 if(road_dis_min <= 300)		///路线偏离(当前点距离最近的线路如果小于300米，但是大于线路宽度，认为偏离线路)
 	{
 	area_alarm_road_deviate(AreaInfo,road_dis_min_id);
 	}
 goto AREA_OUT;
 
 AREA_IN:				///进区域
 curspeed	= gps_speed;
 if( AreaInfo->in_area == 0)
 	{
 	AreaInfo->speed				= 0;
 	AreaInfo->in_area 			= 1;
 	AreaInfo->in_tick 			= rt_tick_get();
	AreaInfo->road_id			= road_id;
	AreaInfo->road_in_tick		= rt_tick_get();
	AreaInfo->road_speed_tick	= rt_tick_get();
	AreaInfo->road_timer_min	= road_timer_min;
	AreaInfo->road_timer_max	= road_timer_max;
 	area_alarm_enter_line(AreaInfo);
 	}
 if(road_id != AreaInfo->road_id)		///行驶路段发生改变
 	{
 	if(rt_tick_get() - AreaInfo->road_in_tick < AreaInfo->road_timer_min)		///路段行驶不足
 		{
		area_alarm_road_timer((rt_tick_get() - AreaInfo->road_in_tick)/RT_TICK_PER_SECOND,AreaInfo->road_id,0);
 		}
	else if(rt_tick_get() - AreaInfo->road_in_tick > AreaInfo->road_timer_max)///路段行驶过长
 		{
		area_alarm_road_timer((rt_tick_get() - AreaInfo->road_in_tick)/RT_TICK_PER_SECOND,AreaInfo->road_id,1);
 		}
	AreaInfo->road_timer_min	= road_timer_min;
	AreaInfo->road_timer_max	= road_timer_max;
	AreaInfo->road_id			= road_id;
	AreaInfo->road_in_tick		= rt_tick_get();
	AreaInfo->road_speed_tick	= rt_tick_get();
 	}
 if(road_attri & BIT(0))		///路段行驶时间
	{
	}
 if(road_attri & BIT(1))		///路段限速
 	{
	if( curspeed < speed )
	 	{
	 	AreaInfo->road_speed_tick 	= rt_tick_get();
	 	}
	if(rt_tick_get() - AreaInfo->road_speed_tick > speedtime)
	 	{
	 	area_alarm_road_speed(AreaInfo,road_id);
	 	}
	else
		{
	 	AreaInfo->speed				= 0;
		}
 	}
 return 1;
 AREA_OUT:				///出区域
 if( AreaInfo->in_area == 1)
	{
	if(rt_tick_get() - AreaInfo->road_in_tick < AreaInfo->road_timer_min)		///路段行驶不足
 		{
		area_alarm_road_timer((rt_tick_get() - AreaInfo->road_in_tick)/RT_TICK_PER_SECOND,AreaInfo->road_id,0);
 		}
	else if(rt_tick_get() - AreaInfo->road_in_tick > AreaInfo->road_timer_max)///路段行驶过长
 		{
		area_alarm_road_timer((rt_tick_get() - AreaInfo->road_in_tick)/RT_TICK_PER_SECOND,AreaInfo->road_id,1);
 		}
	AreaInfo->speed				= 0;
 	AreaInfo->in_area 			= 0;
 	AreaInfo->in_tick 			= rt_tick_get();
	AreaInfo->road_id			= 0;
	AreaInfo->road_in_tick		= rt_tick_get();
	AreaInfo->road_speed_tick	= rt_tick_get();
 	area_alarm_leave_line(AreaInfo);
	}
 return 0;
}


/*********************************************************************************
*函数名称:void area_init(void)
*功能描述:电子围栏参数初始化，该函数在终端初始化时调用
*输	入:	none
*输	出:	none
*返 回 值:none
*作	者:白养民
*创建日期:2013-06-25
*---------------------------------------------------------------------------------
*修 改 人:
*修改日期:
*修改描述:
*********************************************************************************/
void area_init(void)
{
 ///初始化消息队列
 rt_mq_init( &mq_area, "mq_area_alarm", &msgpool_area[0], sizeof(Type_AREA_ALARM) + sizeof(void*), sizeof(msgpool_area), RT_IPC_FLAG_FIFO );
 
 //rt_kprintf("enmu len=%d,area_info len=%d \r\n",sizeof(ENUM_AREA),sizeof(area_info));
 memset((void *)&Area_Para,0,sizeof(Area_Para));
 rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
 area_flash_read_area(area_buffer,AREA_BUF_SIZE);
 area_flash_read_line(line_buffer,LINE_BUF_SIZE);
 rt_sem_release(&sem_dataflash);
}


void area_read(void)
{
 u16 i;
 area_init();
 for(i=0;i<Area_Para.area_num;i++)
 	{
 	rt_kprintf("\n 电子围栏 ID=%5d, 类型=%d",Area_Para.area_info[i].area_data->ID,Area_Para.area_info[i].area_data->State);
	rt_kprintf("矩形区域为:经度1=%9d,纬度1=%9d,经度2=%9d,纬度2=%9d",\
						Area_Para.area_info[i].area_data->Rect_left.Longi,\
						Area_Para.area_info[i].area_data->Rect_left.Lati,\
						Area_Para.area_info[i].area_data->Rect_right.Longi,\
						Area_Para.area_info[i].area_data->Rect_right.Lati);
	rt_kprintf("\n   DATA=");
	printer_data_hex((u8 *)Area_Para.area_info[i].area_data,Area_Para.area_info[i].area_data->Len);
 	}
 ///线路处理，如果当前位置在一个线路中，则跳出线路扫描
 for(i=0;i<Area_Para.line_num;i++)
 	{
 	rt_kprintf("\n 线    路 ID=%5d, 类型=%d",Area_Para.line_info[i].line_head.ID,Area_Para.line_info[i].line_head.State);
	rt_kprintf("矩形区域为:经度1=%9d,纬度1=%9d,经度2=%9d,纬度2=%9d",\
						Area_Para.area_info[i].area_data->Rect_left.Longi,\
						Area_Para.area_info[i].area_data->Rect_left.Lati,\
						Area_Para.area_info[i].area_data->Rect_right.Longi,\
						Area_Para.area_info[i].area_data->Rect_right.Lati);
 	}
}
FINSH_FUNCTION_EXPORT( area_read, area_read);

/*********************************************************************************
*函数名称:void area_process(void)
*功能描述:电子围栏和线路报警处理函数，该函数调用前需要首先调用一次函数"area_init",否则会异常。
*输	入:	none
*输	出:	none
*返 回 值:none
*作	者:白养民
*创建日期:2013-07-01
*---------------------------------------------------------------------------------
*修 改 人:
*修改日期:
*修改描述:
*********************************************************************************/
void area_process(void)
{
 u16 i;
 TypeStruct_Coor cur_Coo;
 static u16 cur_line = 0;
 //static u16 cur_area = 0;
	
 ///获取当前位置
 cur_Coo.Lati = gps_baseinfo.latitude;
 cur_Coo.Longi= gps_baseinfo.longitude;

 ///电子围栏处理，需要将所有围栏都扫描一遍，可能有重叠的区域
 for(i=0;i<Area_Para.area_num;i++)
 	{
 	if( Area_Para.area_info[i].area_data->State == AREA_Circular )
		{
		area_process_circular(&cur_Coo,  &Area_Para.area_info[i]);
		}
	else if(Area_Para.area_info[i].area_data->State == AREA_Rectangle)
		{
		area_process_rectangle(&cur_Coo,  &Area_Para.area_info[i]);
		}
	else if(Area_Para.area_info[i].area_data->State == AREA_Polygon)
		{
		area_process_polygon(&cur_Coo,  &Area_Para.area_info[i]);
		}
	else
		{
		continue;
		}
 	}
 ///线路处理，如果当前位置在一个线路中，则跳出线路扫描
 for(i=0;i<Area_Para.line_num;i++)
 	{
 	if(area_process_line(&cur_Coo, &Area_Para.line_info[cur_line]))
		break;
	cur_line++;
	cur_line %= Area_Para.line_num;
 	}
}


/*********************************************************************************
*函数名称:void area_process_ex(void)
*功能描述:电子围栏和线路报警处理函数，该函数调用前需要首先调用一次函数"area_init",否则会异常。
*输	入:	none
*输	出:	none
*返 回 值:none
*作	者:白养民
*创建日期:2013-07-01
*---------------------------------------------------------------------------------
*修 改 人:
*修改日期:
*修改描述:
*********************************************************************************/
void area_proces_ex(u32 Longi,u32 Lati,u16 speed)
{
 u16 i;
 TypeStruct_Coor cur_Coo;
 static u16 cur_line = 0;
 //static u16 cur_area = 0;
 //return;
 ///获取当前位置
 cur_Coo.Lati = Lati;
 cur_Coo.Longi= Longi;
 test_speed	= speed;

 ///电子围栏处理，需要将所有围栏都扫描一遍，可能有重叠的区域
 for(i=0;i<Area_Para.area_num;i++)
 	{
 	if( Area_Para.area_info[i].area_data->State == AREA_Circular )
		{
		area_process_circular(&cur_Coo,  &Area_Para.area_info[i]);
		}
	else if(Area_Para.area_info[i].area_data->State == AREA_Rectangle)
		{
		area_process_rectangle(&cur_Coo,  &Area_Para.area_info[i]);
		}
	else if(Area_Para.area_info[i].area_data->State == AREA_Polygon)
		{
		area_process_polygon(&cur_Coo,  &Area_Para.area_info[i]);
		}
	else
		{
		continue;
		}
 	}
 ///线路处理，如果当前位置在一个线路中，则跳出线路扫描
 for(i=0;i<Area_Para.line_num;i++)
 	{
 	if(area_process_line(&cur_Coo, &Area_Para.line_info[cur_line]))
		break;
	cur_line++;
	cur_line %= Area_Para.line_num;
 	}
}

FINSH_FUNCTION_EXPORT( area_proces_ex, area_proces_ex);


/*********************************************************************************
*函数名称:u32 area_get_alarm(u8 *pdestbuf,u16* len)
*功能描述:电子围栏及线路报警获取，需要注意的是如果该函数的返回值不等于0，用户调用该
			函数后需要调用函数"rt_free()"将资源释放，防止内存泄露。
*输	入:	none
*输	出:		pdestbuf	:输出808协议标准命令0x0200的附加消息体部分
			len			:附加消息体的长度
*返 回 值:	u32			:为0表示无报警，非0表示有报警，报警值对应808协议0x0200报警标记位
*作	者:白养民
*创建日期:2013-06-25
*---------------------------------------------------------------------------------
*修 改 人:
*修改日期:
*修改描述:
*********************************************************************************/
u32 area_get_alarm(u8 *pdestbuf,u16* destbuflen)
{
 u16 i;
 u16 datalen	= 0;
 u32 retdata	= 0;
 u8 alarem_num	= 0;
 static u8 this_buf[256];
 Type_AREA_ALARM area_alarm;

 *destbuflen = 0;

 pdestbuf	= this_buf;
 
 ///其它报警
 for(;;)
 	{
 	if( RT_EOK == rt_mq_recv(&mq_area,(void *)&area_alarm,sizeof(Type_AREA_ALARM),RT_WAITING_NO) )
 		{
 		retdata |= BIT(area_alarm.alarm_bit);
		memcpy(pdestbuf+datalen,&area_alarm.attri_id,area_alarm.len+2);
		datalen += area_alarm.len + 2;
 		}
	else
		{
		break;
		}
	if(datalen >= 240)
		{
		goto FUNC_RET;
		}
 	}
 ///电子围栏超速报警
 for(i=0; i<Area_Para.area_num; i++)
 	{
 	if(Area_Para.area_info[i].speed)
 		{
 		retdata	|= BIT(13);
		///附加信息 ID  1BYTE
		pdestbuf[datalen++]	= 0x11;
		///附加信息长度  1BYTE 
		pdestbuf[datalen++]	= 5;
		////位置类型  1BYTE 
		pdestbuf[datalen++]	=  Area_Para.area_info[i].area_data->State;
		///区域或路段ID  DWORD
		data_to_buf(pdestbuf+datalen, Area_Para.area_info[i].area_data->ID, 4);
		datalen	+= 4;
 		alarem_num++;
 		}
	if(datalen >= 240)
		{
		goto FUNC_RET;
		}
 	}
 ///线路超速报警
 for(i=0; i<Area_Para.line_num; i++)
 	{
 	if(Area_Para.line_info[i].speed)
 		{
 		retdata	|= BIT(13);
		///附加信息 ID  1BYTE
		pdestbuf[datalen++]	= 0x11;
		///附加信息长度  1BYTE 
		pdestbuf[datalen++]	= 5;
		////位置类型  1BYTE 
		pdestbuf[datalen++]	=  AREA_Line;
		///区域或路段ID  DWORD
		data_to_buf(pdestbuf+datalen, Area_Para.line_info[i].road_id, 4);
		datalen	+= 4;
 		alarem_num++;
 		}
	if(datalen >= 240)
		{
		goto FUNC_RET;
		}
 	}
 
 FUNC_RET:
 	*destbuflen = datalen;
	return retdata;
}
/************************************** The End Of File **************************************/
