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


#define	DF_LineAddress_Start	0xF2000			///·�����ݴ洢��ʼλ��
#define DF_LineAddress_End		0x106000			///·�����ݴ洢����λ��
#define DF_LINESaveSect			0x1000			///·�����ݴ洢��С���


#define	DF_AreaAddress_Start	0x106000			///����Χ�����ݴ洢��ʼλ��
#define DF_AreaAddress_End		0x108000			///����Χ�����ݴ洢����λ��
#define DF_AreaSaveSect			0x40			///����Χ�����ݴ洢��С���





#define AREANUM			24

#define LINENUM			20

#define AREA_BUF_SIZE	4096
#define LINE_BUF_SIZE	4096

#define WLENGH			111200			///ÿ��γ�ȵľ��룬��ֵΪ�̶�ֵ��λΪ��

#define JLENGH			111320			///�����ÿ�����ȵľ��룬��λΪ��



#ifndef BIT
#define BIT(i) ((unsigned long)(1<<i))
#endif


typedef enum
{
	AREA_None=0,				///������
	AREA_Circular,				///Բ��
	AREA_Rectangle,				///����
	AREA_Polygon,				///�����
	AREA_Line,					///��·
}ENUM_AREA;


typedef __packed struct				///������һ������
{
	u32				Lati;				///��������γ��
	u32 			Longi;				///�������꾭��
}TypeStruct_Coor;					


typedef __packed struct
{
	u8			alarm_bit;				///������־λȡֵΪ0~31
	u8			attri_id;				///������Ϣ ID 
	u8 			len;					///������Ϣ����
	u8			data[7];				///����������Ϣ��
}Type_AREA_ALARM;


typedef __packed struct
{
	char 		Head[4];			///�������֣���ʾ��ǰ��������Ϊĳ�̶����ݿ�ʼ
	u16			Len;				///���ݳ��ȣ���������ͷ��������
	ENUM_AREA	State;				///�������ͣ����ΪAREA_None��ʾ������
	TypeStruct_Coor	Rect_left;		///������Χ����->���ϽǾ�γ��
	TypeStruct_Coor	Rect_right;		///������Χ����->���½Ǿ�γ��
	u32			ID;					///����ID
	u8			Data[1];			///����
}TypeDF_AreaHead;

typedef __packed struct
{
	TypeDF_AreaHead *area_data;		///����Χ������ָ��
	u8 			in_area;			///������������ڲ����,0��ʾ���ڸ�����,1��ʾ��
	u32			in_tick;			///����������ʱ�䣬��λΪtick
	u32			speed_tick;			///���ٵ�ʱ�䣬��λΪtick
	u8			speed;				///���ٱ�ǣ�Ϊ0��ʾû�г��٣�Ϊ1��ʾ����
}Type_AreaInfo;

typedef __packed struct
{
	TypeDF_AreaHead line_head;		///��·���ݰ�ͷ
	u8			*line_data;			///��·����ָ��
	u8 			in_area;			///������������ڲ����0��ʾ���ڸ�����
	u32			in_tick;			///����������ʱ�䣬��λΪtick
	u32			road_speed_tick;	///��ʱ��ʱ�䣬��λΪtick
	u32			road_id;			///��ǰ���ڵ�·��ID
	u32			road_in_tick;		///�����·�ε�ʱ�䣬��λΪtick
	u32 		road_timer_max;		///·����ʻ������ֵ
	u32 		road_timer_min;		///·����ʻ������ֵ
	u8			deviate;			///·��ƫ�룬Ϊ0��ʾû��ƫ�룬Ϊ1��ʾƫ��
	u8			speed;				///���ٱ�ǣ�Ϊ0��ʾû�г��٣�Ϊ1��ʾ����
}Type_LineInfo;

typedef __packed struct
{

 u16				area_len;			///����Χ������ʹ�ó���
 u16				line_len;			///��·����ʹ�ó���
 u8 				area_num;			///����Χ������
 u8 				line_num;			///��·����
 Type_AreaInfo	 	area_info[AREANUM];	///����Χ������������
 Type_LineInfo		line_info[LINENUM];	///��·����������
}Type_AreaPara;

static u8	area_buffer[AREA_BUF_SIZE];    ///�洢����Χ������������
static u8	line_buffer[LINE_BUF_SIZE];    ///�洢��·����������

static const char	AREA_HEAD[]={"AREA"};
Type_AreaPara		Area_Para;

/* ��Ϣ���п��ƿ飬���ڵ���Χ������·�����¼��Ĵ���*/
struct rt_messagequeue mq_area;
/* ��Ϣ�������õ��ķ�����Ϣ���ڴ��*/
static char msgpool_area[256];

static u16 test_speed;

//*********************************************************************************
//�����ⲿ����
//*********************************************************************************
extern u32 Times_To_LongInt(T_TIMES *T);
extern uint32_t buf_to_data( uint8_t * psrc, uint8_t width );
extern uint16_t data_to_buf( uint8_t * pdest, uint32_t data, uint8_t width );
extern void printer_data_hex(u8 *pSrc,u16 nSrcLength);


/*********************************************************************************
*��������:rt_err_t area_jt808_0x8600(uint16_t fram_num,uint8_t *pmsg,u16 msg_len)
*��������:����ָ����γ����ÿ�����ȵĳ���.
*��	��:	latitude:��ǰγ��ֵ����λΪ�����֮һ��
*��	��:none
*�� �� ֵ:double 	:����ʵ��ÿ�����ȵĳ��ȣ���λΪ��
*��	��:������
*��������:2013-06-16
*---------------------------------------------------------------------------------
*�� �� ��:
*�޸�����:
*�޸�����:
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
*��������:rt_err_t area_jt808_0x8600(uint16_t fram_num,uint8_t *pmsg,u16 msg_len)
*��������:����ָ����γ����ÿ�����ȵĳ��ȣ�Ȼ������û�����ľ�������Ӧ�ľ���������λΪ
			�Զ�Ϊ��λ�ľ���ֵ���� 10 �� 6 �η�����ȷ�������֮һ��
*��	��:	latitude:��ǰγ��ֵ����λΪ�����֮һ��
		distance:�û�����ľ���
*��	��:none
*�� �� ֵ:u32 	:����ʵ�ʵľ�����
*��	��:������
*��������:2013-06-16
*---------------------------------------------------------------------------------
*�� �� ��:
*�޸�����:
*�޸�����:
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
*��������:u32 AnglPoint2Point(s32 Lati_0,s32 Longi_0,s32 Lati_1,s32 Longi_1)
*��������:��������֮��ľ��룬
*��	��:	Lati_1	:��һ�����γ�ȣ���λΪ�����֮һ��
		Longi_1	:��һ����ľ��ȣ���λΪ�����֮һ��
		Lati_2	:�ڶ������γ�ȣ���λΪ�����֮һ��
		Longi_2	:�ڶ�����ľ��ȣ���λΪ�����֮һ��
*��	��:none
*�� �� ֵ:u32 	:����ʵ�ʵľ��룬��λΪ��
*��	��:������
*��������:2013-06-16
*---------------------------------------------------------------------------------
*�� �� ��:
*�޸�����:
*�޸�����:
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
*��������:u32 dis_Point2Point(s32 Lati_1,s32 Longi_1,s32 Lati_2,s32 Longi_2)
*��������:��������֮��ľ��룬
*��	��:	Lati_1	:��һ�����γ�ȣ���λΪ�����֮һ��
		Longi_1	:��һ����ľ��ȣ���λΪ�����֮һ��
		Lati_2	:�ڶ������γ�ȣ���λΪ�����֮һ��
		Longi_2	:�ڶ�����ľ��ȣ���λΪ�����֮һ��
*��	��:none
*�� �� ֵ:u32 	:����ʵ�ʵľ��룬��λΪ��
*��	��:������
*��������:2013-06-16
*---------------------------------------------------------------------------------
*�� �� ��:
*�޸�����:
*�޸�����:
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
*��������:u32 dis_Point2Line(u32 Cur_Lati, u32 Cur_Longi,u32 Lati_1,u32 Longi_1,u32 Lati_2,u32 Longi_2)
*��������:����㵽��֮��ľ��룬
*��	��:	Cur_Lati	:��ǰ���γ�ȣ���λΪ�����֮һ��
		Cur_Longi	:��ǰ��ľ��ȣ���λΪ�����֮һ��
		Lati_1		:��һ�����γ�ȣ���λΪ�����֮һ��
		Longti_1	:��һ����ľ��ȣ���λΪ�����֮һ��
		Lati_2		:�ڶ������γ�ȣ���λΪ�����֮һ��
		Longti_2	:�ڶ�����ľ��ȣ���λΪ�����֮һ��
*��	��:none
*�� �� ֵ:u32 	:����ʵ�ʵľ��룬��λΪ��
*��	��:������
*��������:2013-06-16
*��ѧԭ��:	1����ǰ����������ĺ���Ϊ		y = a*x;
			2�������������ֱ�������ĺ���Ϊ	y1= -(1/a)*x1 + b;
			3�����ȼ����:  a,b��1/a��Ȼ�����������ֵ��������������ཻ�ĵ��X������
				�й�ʽ	a*x	= -(1/a)*x + b;   ���  x2 = b / (a + 1/a),
			4������x2�������y2���꣬y = a * x,Ȼ���������㵽��ǰ��ľ��룬ͬʱ���������
				�Ƿ���������ɵ����м䣬���򷵻����������򷵻ظ���
			5��ע�⣬���еĵ�ļ��㶼��Lati_1��Longi_1��Ϊԭ��,����Ϊx�ᣬγ��Ϊy��
*---------------------------------------------------------------------------------
*�� �� ��:
*�޸�����:
*�޸�����:
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
*��������:u8 Check_CooisInRect(TypeStruct_Coor *pCoo, TypeDF_AreaHead *pHead )
*��������:�жϵ�ǰλ���Ƿ��ڸþ�����������
*��	��:	pCoo	:��ǰλ������
		pHead	:��ǰ��������
*��	��:none
*�� �� ֵ:u8	1:��ʾ�ڸ�����	0:��ʾ���ڸ�����
*��	��:������
*��������:2013-07-01
*---------------------------------------------------------------------------------
*�� �� ��:
*�޸�����:
*�޸�����:
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
*��������:u8 Check_Area_Time(u8 * StartTime,u8 *EndTime)
*��������:�жϵ�ǰʱ���Ƿ���ָ��ʱ������棬����808Э��ĵ���Χ������·ʱ��Ƚ�
*��	��:	StartTime	:��ʼʱ��
		EndTime		:����ʱ��
*��	��:none
*�� �� ֵ:u8	1:��ʾ�ڸ�ʱ��Σ�	0:��ʾ���ڸ�ʱ���
*��	��:������
*��������:2013-07-01
*---------------------------------------------------------------------------------
*�� �� ��:
*�޸�����:
*�޸�����:
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
*��������:u8 Check_Angle(u32 angle,u32 angle_start,u32 angle_end )
*��������:�жϵ�ǰ�Ƕ��Ƿ���ָ���ĽǶ����棬ָ���ĽǶ��ǰ�����ʱ�뷽ʽ��ת��
*��	��:	angle		:��ǰ�ĽǶ�
		StartTime	:��ʼ�Ƕ�ֵ
		EndTime		:�����Ƕ�ֵ
*��	��:none
*�� �� ֵ:u8	1:��ʾ�ڸýǶȷ�Χ	0:��ʾ���ڸýǶȷ�Χ
*��	��:������
*��������:2013-07-01
*---------------------------------------------------------------------------------
*�� �� ��:
*�޸�����:
*�޸�����:
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
*��������:u16 area_flash_read_area( u8 *pdest,u16 maxlen)
*��������:��ȡflash�еĵ���Χ����ȫ�ֱ������� Area_Para��
*��	��:	pdatabuf	:��Ҫ�������Χ�����ݵ�ram����
		maxlen		:pdatabuf�ĳ��ȣ����ߺ�����ֹ���
*��	��:	none
*�� �� ֵ:u16	0:���������	1:��������
*��	��:������
*��������:2013-06-25
*---------------------------------------------------------------------------------
*�� �� ��:
*�޸�����:
*�޸�����:
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
		///���buf���Ȳ��������Ѿ���ȡ������������������AREANUM����ֱ�ӷ���0����ʾ���
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
*��������:u16 area_flash_read_line( u8 *pdest,u16 maxlen)
*��������:��ȡflash�е���·������ȫ�ֱ������� Area_Para��
*��	��:	pdatabuf	:��Ҫ������·���ݵ�ram����
		maxlen		:pdatabuf�ĳ��ȣ����ߺ�����ֹ���
*��	��:	none
*�� �� ֵ:u16	0:���������	1:��������
*��	��:������
*��������:2013-06-25
*---------------------------------------------------------------------------------
*�� �� ��:
*�޸�����:
*�޸�����:
*********************************************************************************/
u16 area_flash_read_line( u8 *pdatabuf,u16 maxlen)
{
 u8 *ptempbuf = pdatabuf;
 u32 TempAddress;
 u16 sublen;		///flash�洢���ӵİ�ͷ���ֵĳ���
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
		///����Ѿ���ȡ������������������LINENUM����ֱ�ӷ���0����ʾ���
		if( Area_Para.line_num >= LINENUM )
			{
			ret = 0; goto FUNC_RET;
			}
		memcpy(&Area_Para.line_info[Area_Para.line_num].line_head,&TempAreaHead,sizeof(TypeDF_AreaHead));
		///���buf���Ȳ�����������ݵ�����buf��
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
*��������:u16 area_flash_get_line_data( u8 *pdatabuf, u16 maxlen, Type_LineInfo *AreaInfo)
*��������:��ȡflash�е���·���ݵ�AreaInfo�У��ú���Ҳ���޸�ȫ�ֲ��� Area_Para.line_info ������ָ��line_data
*��	��:	pdatabuf	:��Ҫ������·���ݵ�ram����
		maxlen		:pdatabuf�ĳ��ȣ����ߺ�����ֹ���
		AreaInfo	:
*��	��:	none
*�� �� ֵ:u16	0:���������	1:��������
*��	��:������
*��������:2013-06-25
*---------------------------------------------------------------------------------
*�� �� ��:
*�޸�����:
*�޸�����:
*********************************************************************************/
u16 area_flash_get_line_data( u8 *pdatabuf, u16 maxlen, Type_LineInfo *AreaInfo)
{
 u32 i;
 u8 *ptempbuf = pdatabuf;
 u32 TempAddress;
 u16 sublen;		///flash�洢���ӵİ�ͷ���ֵĳ���
 TypeDF_AreaHead TempAreaHead;
 u16 ret = 0;
 
 sublen	= sizeof(TypeDF_AreaHead) - 5;
 
 ///���buf���Ȳ�����buf��գ����ҽ�ȫ�ֲ���Area_Para.line_info[i].line_data ȫ�����
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
		///����Ѿ���ȡ������������������LINENUM����ֱ�ӷ���0����ʾ���
		if( Area_Para.line_num >= LINENUM )
			{
			ret = 0; goto FUNC_RET;
			}
		///�����ǰ��ȡ����ID��Ҫ�����ID��ͬ
		if(TempAreaHead.ID == AreaInfo->line_head.ID)
			{
			//���buf���ȹ���������ݵ�����buf��
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
*��������:u16 area_flash_write_area( u8 *pdatabuf,u16 maxlen,TypeDF_AreaHead *pData)
*��������:��ȡflash�еĵ���Χ����ȫ�ֱ������� Area_Para��
*��	��:	pdatabuf	:��Ҫ�������Χ�����ݵ�ram����
		maxlen		:pdatabuf�ĳ��ȣ����ߺ�����ֹ���
		pData		:��Ҫ����ĵ���Χ������
*��	��:	none
*�� �� ֵ:u16	0:���������	1:��������
*��	��:������
*��������:2013-06-25
*---------------------------------------------------------------------------------
*�� �� ��:
*�޸�����:
*�޸�����:
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
 ///�����޸ĵ���Χ����������
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
 ///�������ӵ���Χ������
 memcpy(ptempbuf + Area_Para.area_len,pData,pData->Len);
 Area_Para.area_info[Area_Para.area_num++].area_data = (TypeDF_AreaHead*)(ptempbuf + Area_Para.area_len);
 Area_Para.area_len += pData->Len;

 ///��������Χ������flash
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
*��������:u16 area_flash_write_line( u8 *pdatabuf,u16 maxlen,TypeDF_AreaHead *pData)
*��������:��flash�д�����·
*��	��:	pdatabuf	:��Ҫ�������Χ�����ݵ�ram����
		maxlen		:pdatabuf�ĳ��ȣ����ߺ�����ֹ���
		pData		:��Ҫ�������·����
*��	��:	none
*�� �� ֵ:u16	0:���������	1:��������
*��	��:������
*��������:2013-06-25
*---------------------------------------------------------------------------------
*�� �� ��:
*�޸�����:
*�޸�����:
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
*��������:void area_flash_del_area( u32 del_id , ENUM_AREA	del_State)
*��������:ɾ��ָ�����͵ĵ���Χ��
*��	��:	del_id		:��Ҫɾ���ĵ���Χ��ID�������ֵΪ0��ʾɾ������ָ�����͵ĵ���Χ��
		del_State	:��Ҫɾ���ĵ���Χ������
*��	��:	none
*�� �� ֵ:none
*��	��:������
*��������:2013-06-25
*---------------------------------------------------------------------------------
*�� �� ��:
*�޸�����:
*�޸�����:
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
*��������:void area_flash_del_line( u32 del_id )
*��������:ɾ����·
*��	��:	del_id	:��Ҫɾ������·ID�������ֵΪ0��ʾɾ��������·
*��	��:	none
*�� �� ֵ:none
*��	��:������
*��������:2013-06-25
*---------------------------------------------------------------------------------
*�� �� ��:
*�޸�����:
*�޸�����:
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
*��������:void area_jt808_commit_ack(u16 fram_num,u16 cmd_id,u8 isFalse)
*��������:ͨ��Ӧ�𣬵���OKӦ��
*��	��:	fram_num:Ӧ����ˮ��
		cmd_id	:����ʱ��808����
		statue 	:��ʾ״̬��0:��ʾOK	1:��ʾʧ��	2:��ʾ��Ϣ����	3:��ʾ��֧��
*��	��:	none
*�� �� ֵ:rt_err_t
*��	��:������
*��������:2013-06-24
*---------------------------------------------------------------------------------
*�� �� ��:
*�޸�����:
*�޸�����:
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
*��������:rt_err_t area_jt808_del(uint8_t linkno,uint8_t *pmsg, ENUM_AREA  del_State))
*��������:808ɾ��Բ����������
*��	��:	pmsg		:808��Ϣ������
		msg_len		:808��Ϣ�峤��
		del_State	:��ʾҪɾ��������
*��	��:none
*�� �� ֵ:rt_err_t
*��	��:������
*��������:2013-06-16
*---------------------------------------------------------------------------------
*�� �� ��:
*�޸�����:
*�޸�����:
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
*��������:rt_err_t area_jt808_0x8600(uint8_t linkno,uint8_t *pmsg)
*��������:808����Բ����������
*��	��:	pmsg	:808��Ϣ������
		msg_len	:808��Ϣ�峤��
*��	��:none
*�� �� ֵ:rt_err_t
*��	��:������
*��������:2013-06-16
*---------------------------------------------------------------------------------
*�� �� ��:
*�޸�����:
*�޸�����:
*********************************************************************************/
rt_err_t area_jt808_0x8600(uint8_t linkno,uint8_t *pmsg)
{
 u16 i;
 u16 datalen;
 u16 tempu16data;
 u32 tempu32data;
 u32 Longi,Lati;		///���Ⱥ�γ��
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
 ///808��Ϣ�岿��
 msg+=2;	///�������� 1byte,�������� 1byte
 
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

	///�������״��������
	Lati	= buf_to_data(msg+6, 4);
	Longi	= buf_to_data(msg+10, 4);
		///����Բ�뾶��Ӧ�ľ�����ֵ
	tempu32data=dis_to_Longi(Lati,buf_to_data(msg+14, 4));
	
	pTempHead->Rect_left.Longi	= Longi - tempu32data;
	pTempHead->Rect_right.Longi	= Longi + tempu32data;
		///����Բ�뾶��Ӧ��γ����ֵ
	temp_d = buf_to_data(msg+14, 4);
	tempu32data = temp_d*1000000/WLENGH;
	pTempHead->Rect_left.Lati	= Lati + tempu32data;
	pTempHead->Rect_right.Lati	= Lati - tempu32data;
	///���Ƶ���Χ�����ݵ���Ҫд��Ľṹ����
	memcpy((void *)&pTempHead->ID,msg,datalen);
	area_flash_write_area(area_buffer,sizeof(area_buffer),pTempHead);
 	}
 area_jt808_commit_ack(fram_num,0x8600,0);
 return RT_EOK;
}


/*********************************************************************************
*��������:rt_err_t area_jt808_0x8601(uint8_t linkno,uint8_t *pmsg)
*��������:808ɾ��Բ����������
*��	��:	pmsg	:808��Ϣ������
		msg_len	:808��Ϣ�峤��
*��	��:none
*�� �� ֵ:rt_err_t
*��	��:������
*��������:2013-06-16
*---------------------------------------------------------------------------------
*�� �� ��:
*�޸�����:
*�޸�����:
*********************************************************************************/
rt_err_t area_jt808_0x8601(uint8_t linkno,uint8_t *pmsg)
{
 return area_jt808_del(linkno, pmsg, AREA_Circular);
}


/*********************************************************************************
*��������:rt_err_t area_jt808_0x8602(uint8_t linkno,uint8_t *pmsg)
*��������:808���þ�����������
*��	��:	pmsg	:808��Ϣ������
		msg_len	:808��Ϣ�峤��
*��	��:none
*�� �� ֵ:rt_err_t
*��	��:������
*��������:2013-06-16
*---------------------------------------------------------------------------------
*�� �� ��:
*�޸�����:
*�޸�����:
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
 ///808��Ϣ�岿��
 msg+=2;	///�������� 1byte,�������� 1byte
 
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

	///�������״��������
	pTempHead->Rect_left.Longi	= buf_to_data(msg+10, 4);
	pTempHead->Rect_right.Longi	= buf_to_data(msg+18, 4);
	pTempHead->Rect_left.Lati	= buf_to_data(msg+6, 4);
	pTempHead->Rect_right.Lati	= buf_to_data(msg+14, 4);
	
	///���Ƶ���Χ�����ݵ���Ҫд��Ľṹ����
	memcpy((void *)&pTempHead->ID,msg,datalen);
	area_flash_write_area(area_buffer,sizeof(area_buffer),pTempHead);
 	}
 area_jt808_commit_ack(fram_num,0x8602,0);
 return RT_EOK;
}


/*********************************************************************************
*��������:rt_err_t area_jt808_0x8603(uint8_t linkno,uint8_t *pmsg)
*��������:808ɾ��������������
*��	��:	pmsg	:808��Ϣ������
		msg_len	:808��Ϣ�峤��
*��	��:none
*�� �� ֵ:rt_err_t
*��	��:������
*��������:2013-06-16
*---------------------------------------------------------------------------------
*�� �� ��:
*�޸�����:
*�޸�����:
*********************************************************************************/
rt_err_t area_jt808_0x8603(uint8_t linkno,uint8_t *pmsg)
{
 return area_jt808_del(linkno, pmsg, AREA_Rectangle);
}


/*********************************************************************************
*��������:rt_err_t area_jt808_0x8604(uint8_t linkno,uint8_t *pmsg)
*��������:808���ö������������
*��	��:	pmsg	:808��Ϣ������
		msg_len	:808��Ϣ�峤��
*��	��:none
*�� �� ֵ:rt_err_t
*��	��:������
*��������:2013-06-16
*---------------------------------------------------------------------------------
*�� �� ��:
*�޸�����:
*�޸�����:
*********************************************************************************/
rt_err_t area_jt808_0x8604(uint8_t linkno,uint8_t *pmsg)
{
 u16 i;
 u16 datalen;
 u16 tempu16data;
 u32 Longi,Lati;		///���Ⱥ�γ��
 TypeDF_AreaHead *pTempHead = NULL;
 
 u8 *msg;
// u16 msg_len;
 u16 fram_num;
 
 //msg_len	= buf_to_data( pmsg + 2, 2 ) & 0x3FF;
 fram_num	= buf_to_data( pmsg + 10, 2 );
 pmsg 		+= 12;
 msg	= pmsg;

 ///808��Ϣ�岿��
 
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
 ///��������
 tempu16data = buf_to_data(msg+datalen, 2);
 if(tempu16data > 64)			///�޶���󶥵���Ϊ64
 	tempu16data = 64;
 datalen += 2;
 
 pTempHead = (TypeDF_AreaHead *)rt_malloc( datalen + tempu16data * 8 );
 if( pTempHead )
 	{
 	///�������״��������
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

	///���Ƶ���Χ�����ݵ���Ҫд��Ľṹ����
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
*��������:rt_err_t area_jt808_0x8605(uint8_t linkno,uint8_t *pmsg)
*��������:808ɾ���������������
*��	��:	pmsg	:808��Ϣ������
		msg_len	:808��Ϣ�峤��
*��	��:none
*�� �� ֵ:rt_err_t
*��	��:������
*��������:2013-06-16
*---------------------------------------------------------------------------------
*�� �� ��:
*�޸�����:
*�޸�����:
*********************************************************************************/
rt_err_t area_jt808_0x8605(uint8_t linkno,uint8_t *pmsg)
{
 return area_jt808_del(linkno, pmsg, AREA_Polygon);
}


/*********************************************************************************
*��������:rt_err_t area_jt808_0x8606(uint8_t linkno,uint8_t *pmsg)
*��������:808������·������
*��	��:	pmsg	:808��Ϣ������
		msg_len	:808��Ϣ�峤��
*��	��:none
*�� �� ֵ:rt_err_t
*��	��:������
*��������:2013-06-16
*---------------------------------------------------------------------------------
*�� �� ��:
*�޸�����:
*�޸�����:
*********************************************************************************/
rt_err_t area_jt808_0x8606(uint8_t linkno,uint8_t *pmsg)
{
 u16 i;
 u16 datalen;
 u16 tempu16data;
 u32 Longi,Lati;		///���Ⱥ�γ��
 TypeDF_AreaHead *pTempHead = NULL;
 
 u8 *msg;
 u16 msg_len;
 u16 fram_num;
 
 msg_len	= buf_to_data( pmsg + 2, 2 ) & 0x3FF;
 fram_num	= buf_to_data( pmsg + 10, 2 );

 ///���������������ʱ��֧�ֶ��
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

 ///808��Ϣ�岿��
 datalen = 23;
 datalen += 6;
 tempu16data = buf_to_data(msg+4, 2);
 if(tempu16data & BIT(0))
	 {
	 datalen += 12;
	 }
 ///��������
 tempu16data = buf_to_data(msg+datalen, 2);
 if(tempu16data > 32)			///�޶���󶥵���Ϊ64
 	tempu16data = 32;
 datalen += 2;
 
 if( tempu16data > (msg_len - datalen) / 25 )
 	{
 	tempu16data = (msg_len - datalen) / 25 ;
 	}
 
 pTempHead = (TypeDF_AreaHead *)rt_malloc( datalen + tempu16data * 25 );
 if( pTempHead )
 	{
 	///�������״��������
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

	///���Ƶ���Χ�����ݵ���Ҫд��Ľṹ����
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
*��������:rt_err_t area_jt808_0x8607(uint8_t linkno,uint8_t *pmsg)
*��������:808ɾ����·��������
*��	��:	pmsg	:808��Ϣ������
		msg_len	:808��Ϣ�峤��
*��	��:none
*�� �� ֵ:rt_err_t
*��	��:������
*��������:2013-06-16
*---------------------------------------------------------------------------------
*�� �� ��:
*�޸�����:
*�޸�����:
*********************************************************************************/
rt_err_t area_jt808_0x8607(uint8_t linkno,uint8_t *pmsg)
{
 return area_jt808_del(linkno, pmsg, AREA_Line);
}


/*********************************************************************************
*��������:void area_alarm_enter( Type_AreaInfo *AreaInfo )
*��������:����Χ�����뱨��
*��	��:	AreaInfo	:��ǰ����Χ����Ϣ
*��	��:none
*�� �� ֵ:none
*��	��:������
*��������:2013-07-01
*---------------------------------------------------------------------------------
*�� �� ��:
*�޸�����:
*�޸�����:
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
 rt_kprintf("\n �������Χ��:ID = %d",AreaInfo->area_data->ID);
}


/*********************************************************************************
*��������:void area_alarm_leave( Type_AreaInfo *AreaInfo )
*��������:����Χ���뿪����
*��	��:	AreaInfo	:��ǰ����Χ����Ϣ
*��	��:none
*�� �� ֵ:none
*��	��:������
*��������:2013-07-01
*---------------------------------------------------------------------------------
*�� �� ��:
*�޸�����:
*�޸�����:
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
 rt_kprintf("\n �뿪����Χ��:ID = %d",AreaInfo->area_data->ID);
}


/*********************************************************************************
*��������:void area_alarm_speed( Type_AreaInfo *AreaInfo )
*��������:����Χ�����ٱ���
*��	��:	AreaInfo	:��ǰ����Χ����Ϣ
*��	��: none
*�� �� ֵ:none
*��	��:������
*��������:2013-07-01
*---------------------------------------------------------------------------------
*�� �� ��:
*�޸�����:
*�޸�����:
*********************************************************************************/
void area_alarm_speed( Type_AreaInfo *AreaInfo )
{
 AreaInfo->speed	= 1;
 rt_kprintf("\n ���ٱ���:ID = %d",AreaInfo->area_data->ID);
}


/*********************************************************************************
*��������:void area_alarm_enter_line( Type_LineInfo *AreaInfo )
*��������:·�߽��뱨��
*��	��:	AreaInfo	:��ǰ��·��Ϣ
*��	��:none
*�� �� ֵ:none
*��	��:������
*��������:2013-07-01
*---------------------------------------------------------------------------------
*�� �� ��:
*�޸�����:
*�޸�����:
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
 rt_kprintf("\n ������·:ID = %d",AreaInfo->line_head.ID);
}


/*********************************************************************************
*��������:void area_alarm_leave_line( Type_LineInfo *AreaInfo )
*��������:��·�뿪����
*��	��:	AreaInfo	:��ǰ��·��Ϣ
*��	��:none
*�� �� ֵ:none
*��	��:������
*��������:2013-07-01
*---------------------------------------------------------------------------------
*�� �� ��:
*�޸�����:
*�޸�����:
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
 rt_kprintf("\n �뿪��·:ID = %d",AreaInfo->line_head.ID);
}


/*********************************************************************************
*��������:void area_alarm_speed_road( Type_LineInfo *AreaInfo ,u32 road_id)
*��������:����Χ�����ٱ���
*��	��:	AreaInfo	:��ǰ��·��Ϣ
		road_id		:·��ID
*��	��:none
*�� �� ֵ:none
*��	��:������
*��������:2013-07-01
*---------------------------------------------------------------------------------
*�� �� ��:
*�޸�����:
*�޸�����:
*********************************************************************************/
void area_alarm_road_speed( Type_LineInfo *AreaInfo ,u32 road_id)
{
	AreaInfo->speed	= 1;
}

/*********************************************************************************
*��������:void area_alarm_road_deviate( Type_LineInfo *AreaInfo,u32 road_id )
*��������:��··��ƫ�뱨��
*��	��:	AreaInfo	:��ǰ��·��Ϣ
		road_id		:·��ID
*��	��:none
*�� �� ֵ:none
*��	��:������
*��������:2013-07-03
*---------------------------------------------------------------------------------
*�� �� ��:
*�޸�����:
*�޸�����:
*********************************************************************************/
void area_alarm_road_deviate( Type_LineInfo *AreaInfo,u32 road_id )
{
}


/*********************************************************************************
*��������:void area_alarm_road_timer( u16 road_time,u32 road_id, u8 state)
*��������:��··��ƫ�뱨��
*��	��:	road_time	:��ǰ·����ʻʱ��
		road_id		:·��ID
		state		:��ʻʱ��������㱨����0��ʾʱ�䲻�㣬1��ʾʱ��̫��
*��	��:none
*�� �� ֵ:none
*��	��:������
*��������:2013-07-03
*---------------------------------------------------------------------------------
*�� �� ��:
*�޸�����:
*�޸�����:
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
*��������:u8 area_process_circular(TypeStruct_Coor *pCoo, Type_AreaInfo *AreaInfo )
*��������:�жϵ�ǰλ���Ƿ��ڸ�Բ����������
*��	��:	pCoo	:��ǰλ������
		AreaInfo:��ǰԲ������
*��	��:none
*�� �� ֵ:u8	1:��ʾ�ڸ�����	0:��ʾ���ڸ�����
*��	��:������
*��������:2013-07-01
*---------------------------------------------------------------------------------
*�� �� ��:
*�޸�����:
*�޸�����:
*********************************************************************************/
u8 area_process_circular(TypeStruct_Coor *pCoo, Type_AreaInfo *AreaInfo )
{
 u16 datalen;
 u16 attri;					///��������
 u32 lati,longi;			///�������ĵ�����
 u32 r;						///�뾶
 u32 d;						///����
 u32 curspeed;				///��ǰ�ٶ�
 u32 speed=0xFFFFFFFF;		///����ٶ�
 u32 speedtime=0xFFFFFFFF;	///����ʱ��
 rt_kprintf("\n Բ�������ж�");
 if( Check_CooisInRect( pCoo, AreaInfo->area_data) == 0)
	{
	rt_kprintf("\n û�ھ�������");
 	goto AREA_OUT;
 	}
 attri	= buf_to_data(AreaInfo->area_data->Data, 2);
 datalen = 14;
 if(attri & BIT(0))		///����ʱ��
 	{
 	if(Check_Area_Time(&AreaInfo->area_data->Data[datalen],&AreaInfo->area_data->Data[6+datalen]) == 0)
 		{
 		rt_kprintf("\n û��ʱ�䷶Χ");
 		goto AREA_OUT;
 		}
	datalen += 12;
 	}
 if(attri & BIT(1))		///����
 	{
 	speed = buf_to_data(AreaInfo->area_data->Data+datalen, 2);
 	speedtime = buf_to_data(AreaInfo->area_data->Data+datalen+2, 1);
	speedtime *= RT_TICK_PER_SECOND;
	datalen += 3;
 	}
 lati 	= buf_to_data(AreaInfo->area_data->Data+2, 4);
 longi	= buf_to_data(AreaInfo->area_data->Data+6, 4);
 r		= buf_to_data(AreaInfo->area_data->Data+10, 4);
 d		= dis_Point2Point(lati, longi, pCoo->Lati, pCoo->Longi );	///��ǰ�㵽���ĵ�ľ���
 //rt_kprintf("\r\n circular_dis= %d,   r=%d",d,r);
 if( d <= r )
 	{
 	goto AREA_IN;		///������
 	}
 else
 	{
 	goto AREA_OUT;		///������
 	}
 
 AREA_IN:				///������
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
 if(attri & BIT(1))		///����
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
 AREA_OUT:				///������
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
*��������:u8 area_process_rectangle(TypeStruct_Coor *pCoo, Type_AreaInfo *AreaInfo )
*��������:�жϵ�ǰλ���Ƿ��ڸþ�����������
*��	��:	pCoo	:��ǰλ������
		AreaInfo:��ǰ��������
*��	��:none
*�� �� ֵ:u8	1:��ʾ�ڸ�����	0:��ʾ���ڸ�����
*��	��:������
*��������:2013-07-01
*---------------------------------------------------------------------------------
*�� �� ��:
*�޸�����:
*�޸�����:
*********************************************************************************/
u8 area_process_rectangle(TypeStruct_Coor *pCoo, Type_AreaInfo *AreaInfo )
{
 u16 datalen;
 u16 attri;					///��������
 u32 curspeed;				///��ǰ�ٶ�
 u32 speed=0xFFFFFFFF;		///����ٶ�
 u32 speedtime=0xFFFFFFFF;	///����ʱ��
 if( Check_CooisInRect( pCoo, AreaInfo->area_data) == 0)
	{
 	goto AREA_OUT;
 	}
 attri	= buf_to_data(AreaInfo->area_data->Data, 2);
 datalen = 18;
 if(attri & BIT(0))		///����ʱ��
 	{
 	if(Check_Area_Time(&AreaInfo->area_data->Data[datalen],&AreaInfo->area_data->Data[6+datalen]) == 0)
 		{
 		goto AREA_OUT;
 		}
	datalen += 12;
 	}
 
 if(attri & BIT(1))		///����
 	{
 	speed = buf_to_data(AreaInfo->area_data->Data+datalen, 2);
 	speedtime = buf_to_data(AreaInfo->area_data->Data+datalen+2, 1);
	speedtime *= RT_TICK_PER_SECOND;
	datalen += 3;
 	}
 
 //AREA_IN:				///������
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
 if(attri & BIT(1))		///����
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
 AREA_OUT:				///������
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
*��������:u8 area_process_polygon(TypeStruct_Coor *pCoo, Type_AreaInfo *AreaInfo )
*��������:�жϵ�ǰλ���Ƿ��ڸö������������
*��	��:	pCoo	:��ǰλ������
		AreaInfo:��ǰ���������
*��	��:none
*�� �� ֵ:u8	1:��ʾ�ڸ�����	0:��ʾ���ڸ�����
*��	��:������
*��������:2013-07-01
*��ѧԭ��:	1���������ڲ������еĵ㶼������һ���������õ㵽���ж���ĽǶ��ǿ��Ը���360���������нǶ�
			2������ʱ���Ը��ݸõ����������������֮��ĽǶ�С��180�ȵķ������б������ҳ����ǶȺ���С�Ƕ�
			�����еĵ㶼����֮���������Ǳպϵ����ʾ�õ㲻�ڶ�����������档
*---------------------------------------------------------------------------------
*�� �� ��:
*�޸�����:
*�޸�����:
*********************************************************************************/
u8 area_process_polygon(TypeStruct_Coor *pCoo, Type_AreaInfo *AreaInfo )
{
 u16 i;
 u16 point_num;
 u16 datalen;
 u16 attri;					///��������
 u32 lati,longi;			///�������ĵ�����
 u32 curspeed;				///��ǰ�ٶ�
 u32 speed=0xFFFFFFFF;		///����ٶ�
 u32 speedtime=0xFFFFFFFF;	///����ʱ��
 u16 angle_first=0,angle_last=0,angle_start=0,angle_end=0;///��һ�εĽǶȣ����һ�εĽǶȣ���ʼ�Ƕȣ����սǶȣ�
 //u16 angle_total=0;	///�ܽǶ�
 u16 angle_1,angle_2,angle;				///���㵱ǰ��Ͷ���ζ��֮��ĽǶ�
 u16 area_state = 0;			///��ֵΪ0x03��ʾ����������
 if( Check_CooisInRect( pCoo, AreaInfo->area_data) == 0)
	{
 	goto AREA_OUT;
 	}
 attri	= buf_to_data(AreaInfo->area_data->Data, 2);
 datalen = 2;
 if(attri & BIT(0))		///����ʱ��
 	{
 	if(Check_Area_Time(&AreaInfo->area_data->Data[datalen],&AreaInfo->area_data->Data[6+datalen]) == 0)
 		{
 		goto AREA_OUT;
 		}
	datalen += 12;
 	}
 if(attri & BIT(1))		///����
 	{
 	speed = buf_to_data(AreaInfo->area_data->Data+datalen, 2);
 	speedtime = buf_to_data(AreaInfo->area_data->Data+datalen+2, 1);
	speedtime *= RT_TICK_PER_SECOND;
	datalen += 3;
 	}
 
 point_num	= buf_to_data(AreaInfo->area_data->Data+datalen, 2);
 datalen	+= 2;
 ///����һ�����еĵ㣬�������ǰ�����ǲ����ڸ�������
 for( i = 0; i <= point_num; i++)		///�����һ�Σ��������ĵ�͵�һ����֮�������Ƕ�
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
	if(i == 0)					///����ǵ�һ���㣬�򽫸õ��ֵ����Ϊ��ǰֵ
		{
		angle_start	= angle;
		angle_end	= angle;
		angle_first	= angle;
		angle_last	= angle;
		}
	else
		{
		///���ҶԱȵ�ǰ�ǶȺ��ϴνǶȵĴ�С���Ӷ��ó��ĸ�������ת�ǶȵĿ�ʼ�㣬�ĸ����ǽ�����
		///�Ա�֮ǰ�ĵ�͵�ǰ�ĵ㣬�ó���ǰɨ��ĽǶȷ�Χ
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
		///����ǰɨ��ĽǶȷ�Χ�����ܵĽǶȷ�Χ��
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
		if(area_state == 0x03)		///��鵽�˵�ǰ���ڶ����������
			{
			break;
			}
		}
 	}
 if(area_state == 0x03)		///��鵽�˵�ǰ���ڶ����������
	{
	goto AREA_IN;
	}
 else
 	{
 	goto AREA_OUT;
 	}
 AREA_IN:				///������
 curspeed	= gps_speed;
 if( AreaInfo->in_area == 0)
 	{
 	AreaInfo->speed	  = 0;
 	AreaInfo->in_area = 1;
 	AreaInfo->in_tick = rt_tick_get();
 	AreaInfo->speed_tick = rt_tick_get();
 	area_alarm_enter(AreaInfo);
 	}
 if(attri & BIT(1))		///����
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
 AREA_OUT:				///������
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
*��������:u8 area_process_line(TypeStruct_Coor *pCoo, Type_LineInfo *AreaInfo )
*��������:�жϵ�ǰλ���Ƿ��ڸ���·�ϲ��Ҽ������·��ر�������������
*��	��:	pCoo	:��ǰλ������
		AreaInfo:��ǰ��·
*��	��:none
*�� �� ֵ:u8	1:��ʾ�ڸ�����	0:��ʾ���ڸ�����
*��	��:������
*��������:2013-07-02
*---------------------------------------------------------------------------------
*�� �� ��:
*�޸�����:
*�޸�����:
*********************************************************************************/
u8 area_process_line(TypeStruct_Coor *pCoo, Type_LineInfo *AreaInfo )
{
 u16 i;
 u16 point_num;
 u16 datalen;
 u16 attri;						///��·����
 u8  road_attri;				///·������
 u32 road_id;					///·��ID
 u16 road_width;				///·�ο��
 u32 road_timer_max=0xFFFFFFFF;	///·����ʻ������ֵ
 u32 road_timer_min=0;			///·����ʻ������ֵ
 u32 road_dis;				///��ǰλ�������·�εľ���
 u32 road_dis_min=0xFFFFFFFF;	///���������·���У���ǰ���������ľ���
 u32 road_dis_min_id=0;			///���������·���У���ǰ����������·��ID
//u32 road_dis_min_width=0;		///���������·���У���ǰ����������·�εĿ��
 u32 lati,longi;				///����
 u32 lati_last=0,longi_last=0;		///�ϴδ���Ĺյ�����
 u32 curspeed;					///��ǰ�ٶ�
 u32 speed=0xFFFFFFFF;			///����ٶ�
 u32 speedtime=0xFFFFFFFF;		///����ʱ��
	
 if( Check_CooisInRect( pCoo, &AreaInfo->line_head) == 0)
	{
 	goto AREA_OUT;
 	}
 ///��鵱ǰ������·�����ǲ�����RAM �� buf�У�ΪRT_NULL��ʾ���ڣ���Ҫ��flash�л�ȡ����
 if(AreaInfo->line_data == RT_NULL)
 	{
 	///��flash�л�ȡ��ǰ�������·����
 	if(area_flash_get_line_data(line_buffer, sizeof(line_buffer), AreaInfo) == 0)
 		{
 		goto AREA_OUT;
 		}
 	}
 attri	= buf_to_data(AreaInfo->line_data+4, 2);
 datalen = 6;
 if(attri & BIT(0))		///����ʱ��
 	{
 	if(Check_Area_Time(&AreaInfo->line_data[datalen],&AreaInfo->line_data[6+datalen]) == 0)
 		{
 		goto AREA_OUT;
 		}
	datalen += 12;
 	}
 
 point_num	= buf_to_data(AreaInfo->line_data+datalen, 2);
 datalen	+= 2;
 ///����һ�����еĵ㣬�������ǰ�����ǲ����ڸ���·
 for( i = 0; i < point_num; i++)
 	{
	road_timer_max	= 0xFFFFFFFF;	///·����ʻ������ֵ
	road_timer_min	= 0x00000000;	///·����ʻ������ֵ
	speed			= 0xFFFFFFFF;	///����ٶ�
	speedtime		= 0xFFFFFFFF;	///����ʱ��
 	///�յ� ID 4BYTE
 	datalen += 4;
	///·�� ID 4BYTE
	road_id	= buf_to_data(AreaInfo->line_data+datalen, 4);
	datalen += 4;
	///�յ�γ��  4BYTE
	lati 	= buf_to_data(AreaInfo->line_data+datalen, 4);
	datalen += 4;
	///�յ㾭�� 4BYTE
	longi 	= buf_to_data(AreaInfo->line_data+datalen, 4);
	datalen += 4;
	///·�ο�� 1BYTE
	road_width = AreaInfo->line_data[datalen++];
	///·������ 1BYTE
	road_attri = AreaInfo->line_data[datalen++];
	if(road_attri & BIT(0))
		{
		///·����ʻ������ֵ 2BYTE
		road_timer_max = buf_to_data(AreaInfo->line_data+datalen, 2);
		road_timer_max 	*= RT_TICK_PER_SECOND;
		datalen += 2;
		///·����ʻ������ֵ 2BYTE
		road_timer_min = buf_to_data(AreaInfo->line_data+datalen, 2);
		road_timer_min 	*= RT_TICK_PER_SECOND;
		datalen += 2;
		}
	if(road_attri & BIT(0))
		{
		///·������ٶ� 2BYTE
		speed 		= buf_to_data(AreaInfo->line_data+datalen, 2);
		datalen += 2;
		///·�γ��ٳ���ʱ�� 1BYTE
		speedtime	= buf_to_data(AreaInfo->line_data+datalen, 2);
		speedtime 	*= RT_TICK_PER_SECOND;
		datalen += 2;
		}
	if( i )		///��һ���յ�û����·���ڶ����յ��Ժ������·��
		{
		road_dis = dis_Point2Line(pCoo->Lati, pCoo->Longi,lati_last, longi_last, lati, longi);
		if( road_dis <= road_width)			///�ڸ�·��
			{
			road_dis_min	= 0;
			road_dis_min_id	= road_id;
			goto AREA_IN;
			}
		else if( road_dis != 0xFFFFFFFF )	///������·��ȷ�Χ��
			{
			road_dis -= road_width;
			if(road_dis < road_dis_min)
				{
				road_dis_min	= road_dis;
				road_dis_min_id	= road_id;
				}
			}
		else								///������·��Χ��ʲô������
			{
			}
		}
	lati_last	= lati;
	longi_last	=  longi;
 	}
 if(road_dis_min <= 300)		///·��ƫ��(��ǰ������������·���С��300�ף����Ǵ�����·��ȣ���Ϊƫ����·)
 	{
 	area_alarm_road_deviate(AreaInfo,road_dis_min_id);
 	}
 goto AREA_OUT;
 
 AREA_IN:				///������
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
 if(road_id != AreaInfo->road_id)		///��ʻ·�η����ı�
 	{
 	if(rt_tick_get() - AreaInfo->road_in_tick < AreaInfo->road_timer_min)		///·����ʻ����
 		{
		area_alarm_road_timer((rt_tick_get() - AreaInfo->road_in_tick)/RT_TICK_PER_SECOND,AreaInfo->road_id,0);
 		}
	else if(rt_tick_get() - AreaInfo->road_in_tick > AreaInfo->road_timer_max)///·����ʻ����
 		{
		area_alarm_road_timer((rt_tick_get() - AreaInfo->road_in_tick)/RT_TICK_PER_SECOND,AreaInfo->road_id,1);
 		}
	AreaInfo->road_timer_min	= road_timer_min;
	AreaInfo->road_timer_max	= road_timer_max;
	AreaInfo->road_id			= road_id;
	AreaInfo->road_in_tick		= rt_tick_get();
	AreaInfo->road_speed_tick	= rt_tick_get();
 	}
 if(road_attri & BIT(0))		///·����ʻʱ��
	{
	}
 if(road_attri & BIT(1))		///·������
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
 AREA_OUT:				///������
 if( AreaInfo->in_area == 1)
	{
	if(rt_tick_get() - AreaInfo->road_in_tick < AreaInfo->road_timer_min)		///·����ʻ����
 		{
		area_alarm_road_timer((rt_tick_get() - AreaInfo->road_in_tick)/RT_TICK_PER_SECOND,AreaInfo->road_id,0);
 		}
	else if(rt_tick_get() - AreaInfo->road_in_tick > AreaInfo->road_timer_max)///·����ʻ����
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
*��������:void area_init(void)
*��������:����Χ��������ʼ�����ú������ն˳�ʼ��ʱ����
*��	��:	none
*��	��:	none
*�� �� ֵ:none
*��	��:������
*��������:2013-06-25
*---------------------------------------------------------------------------------
*�� �� ��:
*�޸�����:
*�޸�����:
*********************************************************************************/
void area_init(void)
{
 ///��ʼ����Ϣ����
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
 	rt_kprintf("\n ����Χ�� ID=%5d, ����=%d",Area_Para.area_info[i].area_data->ID,Area_Para.area_info[i].area_data->State);
	rt_kprintf("��������Ϊ:����1=%9d,γ��1=%9d,����2=%9d,γ��2=%9d",\
						Area_Para.area_info[i].area_data->Rect_left.Longi,\
						Area_Para.area_info[i].area_data->Rect_left.Lati,\
						Area_Para.area_info[i].area_data->Rect_right.Longi,\
						Area_Para.area_info[i].area_data->Rect_right.Lati);
	rt_kprintf("\n   DATA=");
	printer_data_hex((u8 *)Area_Para.area_info[i].area_data,Area_Para.area_info[i].area_data->Len);
 	}
 ///��·���������ǰλ����һ����·�У���������·ɨ��
 for(i=0;i<Area_Para.line_num;i++)
 	{
 	rt_kprintf("\n ��    · ID=%5d, ����=%d",Area_Para.line_info[i].line_head.ID,Area_Para.line_info[i].line_head.State);
	rt_kprintf("��������Ϊ:����1=%9d,γ��1=%9d,����2=%9d,γ��2=%9d",\
						Area_Para.area_info[i].area_data->Rect_left.Longi,\
						Area_Para.area_info[i].area_data->Rect_left.Lati,\
						Area_Para.area_info[i].area_data->Rect_right.Longi,\
						Area_Para.area_info[i].area_data->Rect_right.Lati);
 	}
}
FINSH_FUNCTION_EXPORT( area_read, area_read);

/*********************************************************************************
*��������:void area_process(void)
*��������:����Χ������·�������������ú�������ǰ��Ҫ���ȵ���һ�κ���"area_init",������쳣��
*��	��:	none
*��	��:	none
*�� �� ֵ:none
*��	��:������
*��������:2013-07-01
*---------------------------------------------------------------------------------
*�� �� ��:
*�޸�����:
*�޸�����:
*********************************************************************************/
void area_process(void)
{
 u16 i;
 TypeStruct_Coor cur_Coo;
 static u16 cur_line = 0;
 //static u16 cur_area = 0;
	
 ///��ȡ��ǰλ��
 cur_Coo.Lati = gps_baseinfo.latitude;
 cur_Coo.Longi= gps_baseinfo.longitude;

 ///����Χ��������Ҫ������Χ����ɨ��һ�飬�������ص�������
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
 ///��·���������ǰλ����һ����·�У���������·ɨ��
 for(i=0;i<Area_Para.line_num;i++)
 	{
 	if(area_process_line(&cur_Coo, &Area_Para.line_info[cur_line]))
		break;
	cur_line++;
	cur_line %= Area_Para.line_num;
 	}
}


/*********************************************************************************
*��������:void area_process_ex(void)
*��������:����Χ������·�������������ú�������ǰ��Ҫ���ȵ���һ�κ���"area_init",������쳣��
*��	��:	none
*��	��:	none
*�� �� ֵ:none
*��	��:������
*��������:2013-07-01
*---------------------------------------------------------------------------------
*�� �� ��:
*�޸�����:
*�޸�����:
*********************************************************************************/
void area_proces_ex(u32 Longi,u32 Lati,u16 speed)
{
 u16 i;
 TypeStruct_Coor cur_Coo;
 static u16 cur_line = 0;
 //static u16 cur_area = 0;
 //return;
 ///��ȡ��ǰλ��
 cur_Coo.Lati = Lati;
 cur_Coo.Longi= Longi;
 test_speed	= speed;

 ///����Χ��������Ҫ������Χ����ɨ��һ�飬�������ص�������
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
 ///��·���������ǰλ����һ����·�У���������·ɨ��
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
*��������:u32 area_get_alarm(u8 *pdestbuf,u16* len)
*��������:����Χ������·������ȡ����Ҫע���������ú����ķ���ֵ������0���û����ø�
			��������Ҫ���ú���"rt_free()"����Դ�ͷţ���ֹ�ڴ�й¶��
*��	��:	none
*��	��:		pdestbuf	:���808Э���׼����0x0200�ĸ�����Ϣ�岿��
			len			:������Ϣ��ĳ���
*�� �� ֵ:	u32			:Ϊ0��ʾ�ޱ�������0��ʾ�б���������ֵ��Ӧ808Э��0x0200�������λ
*��	��:������
*��������:2013-06-25
*---------------------------------------------------------------------------------
*�� �� ��:
*�޸�����:
*�޸�����:
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
 
 ///��������
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
 ///����Χ�����ٱ���
 for(i=0; i<Area_Para.area_num; i++)
 	{
 	if(Area_Para.area_info[i].speed)
 		{
 		retdata	|= BIT(13);
		///������Ϣ ID  1BYTE
		pdestbuf[datalen++]	= 0x11;
		///������Ϣ����  1BYTE 
		pdestbuf[datalen++]	= 5;
		////λ������  1BYTE 
		pdestbuf[datalen++]	=  Area_Para.area_info[i].area_data->State;
		///�����·��ID  DWORD
		data_to_buf(pdestbuf+datalen, Area_Para.area_info[i].area_data->ID, 4);
		datalen	+= 4;
 		alarem_num++;
 		}
	if(datalen >= 240)
		{
		goto FUNC_RET;
		}
 	}
 ///��·���ٱ���
 for(i=0; i<Area_Para.line_num; i++)
 	{
 	if(Area_Para.line_info[i].speed)
 		{
 		retdata	|= BIT(13);
		///������Ϣ ID  1BYTE
		pdestbuf[datalen++]	= 0x11;
		///������Ϣ����  1BYTE 
		pdestbuf[datalen++]	= 5;
		////λ������  1BYTE 
		pdestbuf[datalen++]	=  AREA_Line;
		///�����·��ID  DWORD
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
