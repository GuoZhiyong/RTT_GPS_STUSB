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

#include <board.h>
#include <rtthread.h>
#include <finsh.h>

#include "stm32f4xx.h"
#include "jt808.h"
#include "msglist.h"
#include "jt808_sprintf.h"
#include "sst25.h"

#include "gsm.h"
#include "m66.h"

#define ByteSwap2( val )    \
    ( ( ( val & 0xff ) << 8 ) |   \
      ( ( val & 0xff00 ) >> 8 ) )

#define ByteSwap4( val )    \
    ( ( ( val & 0xff ) << 24 ) |   \
      ( ( val & 0xff00 ) << 8 ) |  \
      ( ( val & 0xff0000 ) >> 8 ) |  \
      ( ( val & 0xff000000 ) >> 24 ) )

typedef struct
{
	uint16_t id;
	int ( *func )( uint8_t linkno, uint8_t *pmsg );
}HANDLE_JT808_RX_MSG;

static struct rt_mailbox	mb_gprsdata;
#define MB_GPRSDATA_POOL_SIZE 32
static uint8_t				mb_gprsdata_pool[MB_GPRSDATA_POOL_SIZE];

static struct rt_mailbox	mb_gpsdata;
#define MB_GPSDATA_POOL_SIZE 32
static uint8_t				mb_gpsdata_pool[MB_GPSDATA_POOL_SIZE];

static uint16_t				tx_seq = 0; /*发送序号*/

static rt_device_t			pdev_gsm = RT_NULL;


/*发送信息列表*/
MsgList* list_jt808_tx;

/*接收信息列表*/
MsgList * list_jt808_rx;


/*
   同时准备好可用的四个连接，根据要求选择处理,依次为
   实际中并不会同时对多个连接建立，只能依次分组来处理
   主808服务器
   备份808服务器
   主IC卡鉴权服务器
   备份IC卡鉴权服务器

 */
GSM_SOCKET	gsm_socket[MAX_GSM_SOCKET];

GSM_SOCKET	* psocket = RT_NULL;

JT808_PARAM jt808_param =
{
	0x13022200,         /*0x0000 版本*/
	5,                  /*0x0001 心跳发送间隔*/
	5,                  /*0x0002 TCP应答超时时间*/
	3,                  /*0x0003 TCP超时重传次数*/
	3,                  /*0x0004 UDP应答超时时间*/
	5,                  /*0x0005 UDP超时重传次数*/
	3,                  /*0x0006 SMS消息应答超时时间*/
	5,                  /*0x0007 SMS消息重传次数*/
	"CMNET",            /*0x0010 主服务器APN*/
	"",                 /*0x0011 用户名*/
	"",                 /*0x0012 密码*/
	"60.28.50.210",     /*0x0013 主服务器地址*/
	"CMNET",            /*0x0014 备份APN*/
	"",                 /*0x0015 备份用户名*/
	"",                 /*0x0016 备份密码*/
	"www.google.com",   /*0x0017 备份服务器地址，ip或域名*/
	9131,               /*0x0018 TCP端口*/
	5678,               /*0x0019 UDP端口*/
	"",                 /*0x001A ic卡主服务器地址，ip或域名*/
	0,                  /*0x001B ic卡服务器TCP端口*/
	0,                  /*0x001C ic卡服务器UDP端口*/
	"",                 /*0x001D ic卡备份服务器地址，ip或域名*/
	0,                  /*0x0020 位置汇报策略*/
	1,                  /*0x0021 位置汇报方案*/
	30,                 /*0x0022 驾驶员未登录汇报时间间隔*/
	120,                /*0x0027 休眠时汇报时间间隔*/
	5,                  /*0x0028 紧急报警时汇报时间间隔*/
	30,                 /*0x0029 缺省时间汇报间隔*/
	500,                /*0x002c 缺省距离汇报间隔*/
	1000,               /*0x002d 驾驶员未登录汇报距离间隔*/
	1000,               /*0x002e 休眠时距离汇报间隔*/
	100,                /*0x002f 紧急时距离汇报间隔*/
	270,                /*0x0030 拐点补传角度*/
	500,                /*0x0031 电子围栏半径（非法位移阈值），单位为米*/
	"1008611",          /*0x0040 监控平台电话号码*/
	"",                 /*0x0041 复位电话号码*/
	"",                 /*0x0042 恢复出厂设置电话号码*/
	"",                 /*0x0043 监控平台SMS号码*/
	"",                 /*0x0044 接收终端SMS文本报警号码*/
	5,                  /*0x0045 终端接听电话策略*/
	3,                  /*0x0046 每次通话时长*/
	3,                  /*0x0047 当月通话时长*/
	"",                 /*0x0048 监听电话号码*/
	"",                 /*0x0049 监管平台特权短信号码*/
	5,                  /*0x0050 报警屏蔽字*/
	3,                  /*0x0051 报警发送文本SMS开关*/
	5,                  /*0x0052 报警拍照开关*/
	3,                  /*0x0053 报警拍摄存储标志*/
	5,                  /*0x0054 关键标志*/
	3,                  /*0x0055 最高速度kmh*/
	5,                  /*0x0056 超速持续时间*/
	3,                  /*0x0057 连续驾驶时间门限*/
	5,                  /*0x0058 当天累计驾驶时间门限*/
	3,                  /*0x0059 最小休息时间*/
	5,                  /*0x005A 最长停车时间*/
	900,                /*0x0005B 超速报警预警差值，单位为 1/10Km/h */
	90,                 /*0x005C 疲劳驾驶预警差值，单位为秒（s），>0*/
	0x200a,             /*0x005D 碰撞报警参数设置:*/
	30,                 /*0x005E 侧翻报警参数设置： 侧翻角度，单位 1 度，默认为 30 度*/
	0,                  /*0x0064 定时拍照控制*/
	0,                  /*0x0065 定距拍照控制*/
	3,                  /*0x0070 图像视频质量(1-10)*/
	5,                  /*0x0071 亮度*/
	3,                  /*0x0072 对比度*/
	5,                  /*0x0073 饱和度*/
	3,                  /*0x0074 色度*/
	5,                  /*0x0080 车辆里程表读数0.1km*/
	3,                  /*0x0081 省域ID*/
	5,                  /*0x0082 市域ID*/
	"津O-00001",        /*0x0083 机动车号牌*/
	1,                  /*0x0084 车牌颜色  1蓝色 2黄色 3黑色 4白色 9其他*/
	0x0f,               /*0x0090 GNSS 定位模式*/
	0x01,               /*0x0091 GNSS 波特率*/
	0x01,               /*0x0092 GNSS 模块详细定位数据输出频率*/
	0x01,               /*0x0093    GNSS 模块详细定位数据采集频率*/
	0x01,               /*0x0094    GNSS 模块详细定位数据上传方式*/
	0x01,               /*0x0095 GNSS 模块详细定位数据上传设置*/
	0,                  /*0x0100 CAN 总线通道 1 采集时间间隔(ms)，0 表示不采集*/
	0,                  /*0x0101 CAN 总线通道 1 上传时间间隔(s)，0 表示不上传*/
	0,                  /*0x0102 CAN 总线通道 2 采集时间间隔(ms)，0 表示不采集*/
	0,                  /*0x0103 CAN 总线通道 2 上传时间间隔(s)，0 表示不上传*/
	{ 0, 0 },           /*0x0110 CAN 总线 ID 单独采集设置*/
	{ 0, 0 },           /*0x0111 其他CAN 总线 ID 单独采集设置*/
	{ 0, 0 },           /*0x0112 其他CAN 总线 ID 单独采集设置*/
	{ 0, 0 },           /*0x0113 其他CAN 总线 ID 单独采集设置*/
	{ 0, 0 },           /*0x0114 其他CAN 总线 ID 单独采集设置*/
	{ 0, 0 },           /*0x0115 其他CAN 总线 ID 单独采集设置*/
	{ 0, 0 },           /*0x0116 其他CAN 总线 ID 单独采集设置*/
	{ 0, 0 },           /*0x0117 其他CAN 总线 ID 单独采集设置*/
	{ 0, 0 },           /*0x0118 其他CAN 总线 ID 单独采集设置*/
	{ 0, 0 },           /*0x0119 其他CAN 总线 ID 单独采集设置*/
};

TERM_PARAM term_param =
{
	0x07,
	{ 0x11,0x22,	0x33, 0x44, 0x55, 0x66 },
	{ "TCBBD" },
	{ "TW701-BD" },
	{ 0x00,0x99,	0xaa, 0xbb, 0xcc, 0xdd, 0xee},
};

#define FLAG_DISABLE_REPORT_INVALID 1       /*设备非法*/
#define FLAG_DISABLE_REPORT_AREA	2       /*区域内禁止上报*/

static uint32_t flag_disable_report = 0;    /*禁止上报的标志位*/

/*保存参数到serialflash*/
void param_save( void )
{
	rt_kprintf( "parma_save size=%d\r\n", sizeof( jt808_param ) );
	sst25_write_back( ADDR_PARAM, (uint8_t*)&jt808_param, sizeof( jt808_param ) );
}

/*加载参数从serialflash*/
void param_load( void )
{
	/*预读一部分数据*/
	uint8_t		ver8[4];
	uint32_t	ver32;
	sst25_read( ADDR_PARAM, ver8, 4 );
	ver32 = ( ver8[0] ) | ( ver8[1] << 8 ) | ( ver8[2] << 16 ) | ( ver8[3] << 24 );
	rt_kprintf( "param_load ver=%08x\r\n", ver32 );
	if( jt808_param.id_0x0000 != ver32 ) /*不管是不是未初始化*/
	{
		rt_kprintf( "%s(%d)param_save\r\n", __func__, __LINE__ );
		param_save( );
	}
	sst25_read( ADDR_PARAM, (uint8_t*)&jt808_param, sizeof( jt808_param ) );
}

#define TYPE_BYTE	0x01    /*固定为1字节,小端对齐*/
#define TYPE_WORD	0x02    /*固定为2字节,小端对齐*/
#define TYPE_DWORD	0x04    /*固定为4字节,小端对齐*/
#define TYPE_STR	0x80    /*固定为32字节,网络顺序*/
#define TYPE_CAN_ID 0x48    /*固定为8字节,当前存储CAN_ID参数*/

struct _tbl_id_lookup
{
	uint16_t	id;
	uint8_t		type;
	uint8_t		* val;
} tbl_id_lookup[] = {
	{ 0x0000, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0000 ) },    //uint32_t	id_0x0000;      /*0x0000 版本*/
	{ 0x0001, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0001 ) },    //uint32_t	id_0x0001;      /*0x0001 心跳发送间隔*/
	{ 0x0002, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0002 ) },    //uint32_t	id_0x0002;      /*0x0002 TCP应答超时时间*/
	{ 0x0003, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0003 ) },    //uint32_t	id_0x0003;      /*0x0003 TCP超时重传次数*/
	{ 0x0004, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0004 ) },    //uint32_t	id_0x0004;      /*0x0004 UDP应答超时时间*/
	{ 0x0005, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0005 ) },    //uint32_t	id_0x0005;      /*0x0005 UDP超时重传次数*/
	{ 0x0006, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0006 ) },    //uint32_t	id_0x0006;      /*0x0006 SMS消息应答超时时间*/
	{ 0x0007, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0007 ) },    //uint32_t	id_0x0007;      /*0x0007 SMS消息重传次数*/
	{ 0x0010, TYPE_STR,		 (uint8_t*)&( jt808_param.id_0x0010 ) },    //char		id_0x0010[32];  /*0x0010 主服务器APN*/
	{ 0x0011, TYPE_STR,		 (uint8_t*)&( jt808_param.id_0x0011 ) },    //char		id_0x0011[32];  /*0x0011 用户名*/
	{ 0x0012, TYPE_STR,		 (uint8_t*)&( jt808_param.id_0x0012 ) },    //char		id_0x0012[32];  /*0x0012 密码*/
	{ 0x0013, TYPE_STR,		 (uint8_t*)&( jt808_param.id_0x0013 ) },    //char		id_0x0013[32];  /*0x0013 主服务器地址*/
	{ 0x0014, TYPE_STR,		 (uint8_t*)&( jt808_param.id_0x0014 ) },    //char		id_0x0014[32];  /*0x0014 备份APN*/
	{ 0x0015, TYPE_STR,		 (uint8_t*)&( jt808_param.id_0x0015 ) },    //char		id_0x0015[32];  /*0x0015 备份用户名*/
	{ 0x0016, TYPE_STR,		 (uint8_t*)&( jt808_param.id_0x0016 ) },    //char		id_0x0016[32];  /*0x0016 备份密码*/
	{ 0x0017, TYPE_STR,		 (uint8_t*)&( jt808_param.id_0x0017 ) },    //char		id_0x0017[32];  /*0x0017 备份服务器地址，ip或域名*/
	{ 0x0018, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0018 ) },    //uint32_t	id_0x0018;      /*0x0018 TCP端口*/
	{ 0x0019, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0019 ) },    //uint32_t	id_0x0019;      /*0x0019 UDP端口*/
	{ 0x001A, TYPE_STR,		 (uint8_t*)&( jt808_param.id_0x001A ) },    //char		id_0x001A[32];  /*0x001A ic卡主服务器地址，ip或域名*/
	{ 0x001B, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x001B ) },    //uint32_t	id_0x001B;      /*0x001B ic卡服务器TCP端口*/
	{ 0x001C, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x001C ) },    //uint32_t	id_0x001C;      /*0x001C ic卡服务器UDP端口*/
	{ 0x001D, TYPE_STR,		 (uint8_t*)&( jt808_param.id_0x001D ) },    //char		id_0x001D[32];  /*0x001D ic卡备份服务器地址，ip或域名*/
	{ 0x0020, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0020 ) },    //uint32_t	id_0x0020;      /*0x0020 位置汇报策略*/
	{ 0x0021, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0021 ) },    //uint32_t	id_0x0021;      /*0x0021 位置汇报方案*/
	{ 0x0022, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0022 ) },    //uint32_t	id_0x0022;      /*0x0022 驾驶员未登录汇报时间间隔*/
	{ 0x0027, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0027 ) },    //uint32_t	id_0x0027;      /*0x0027 休眠时汇报时间间隔*/
	{ 0x0028, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0028 ) },    //uint32_t	id_0x0028;      /*0x0028 紧急报警时汇报时间间隔*/
	{ 0x0029, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0029 ) },    //uint32_t	id_0x0029;      /*0x0029 缺省时间汇报间隔*/
	{ 0x002C, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x002C ) },    //uint32_t	id_0x002C;      /*0x002c 缺省距离汇报间隔*/
	{ 0x002D, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x002D ) },    //uint32_t	id_0x002D;      /*0x002d 驾驶员未登录汇报距离间隔*/
	{ 0x002E, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x002E ) },    //uint32_t	id_0x002E;      /*0x002e 休眠时距离汇报间隔*/
	{ 0x002F, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x002F ) },    //uint32_t	id_0x002F;      /*0x002f 紧急时距离汇报间隔*/
	{ 0x0030, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0030 ) },    //uint32_t	id_0x0030;      /*0x0030 拐点补传角度*/
	{ 0x0031, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0031 ) },    //uint16_t	id_0x0031;      /*0x0031 电子围栏半径（非法位移阈值），单位为米*/
	{ 0x0040, TYPE_STR,		 (uint8_t*)&( jt808_param.id_0x0040 ) },    //char		id_0x0040[32];  /*0x0040 监控平台电话号码*/
	{ 0x0041, TYPE_STR,		 (uint8_t*)&( jt808_param.id_0x0041 ) },    //char		id_0x0041[32];  /*0x0041 复位电话号码*/
	{ 0x0042, TYPE_STR,		 (uint8_t*)&( jt808_param.id_0x0042 ) },    //char		id_0x0042[32];  /*0x0042 恢复出厂设置电话号码*/
	{ 0x0043, TYPE_STR,		 (uint8_t*)&( jt808_param.id_0x0043 ) },    //char		id_0x0043[32];  /*0x0043 监控平台SMS号码*/
	{ 0x0044, TYPE_STR,		 (uint8_t*)&( jt808_param.id_0x0044 ) },    //char		id_0x0044[32];  /*0x0044 接收终端SMS文本报警号码*/
	{ 0x0045, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0045 ) },    //uint32_t	id_0x0045;      /*0x0045 终端接听电话策略*/
	{ 0x0046, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0046 ) },    //uint32_t	id_0x0046;      /*0x0046 每次通话时长*/
	{ 0x0047, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0047 ) },    //uint32_t	id_0x0047;      /*0x0047 当月通话时长*/
	{ 0x0048, TYPE_STR,		 (uint8_t*)&( jt808_param.id_0x0048 ) },    //char		id_0x0048[32];  /*0x0048 监听电话号码*/
	{ 0x0049, TYPE_STR,		 (uint8_t*)&( jt808_param.id_0x0049 ) },    //char		id_0x0049[32];  /*0x0049 监管平台特权短信号码*/
	{ 0x0050, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0050 ) },    //uint32_t	id_0x0050;      /*0x0050 报警屏蔽字*/
	{ 0x0051, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0051 ) },    //uint32_t	id_0x0051;      /*0x0051 报警发送文本SMS开关*/
	{ 0x0052, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0052 ) },    //uint32_t	id_0x0052;      /*0x0052 报警拍照开关*/
	{ 0x0053, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0053 ) },    //uint32_t	id_0x0053;      /*0x0053 报警拍摄存储标志*/
	{ 0x0054, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0054 ) },    //uint32_t	id_0x0054;      /*0x0054 关键标志*/
	{ 0x0055, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0055 ) },    //uint32_t	id_0x0055;      /*0x0055 最高速度kmh*/
	{ 0x0056, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0056 ) },    //uint32_t	id_0x0056;      /*0x0056 超速持续时间*/
	{ 0x0057, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0057 ) },    //uint32_t	id_0x0057;      /*0x0057 连续驾驶时间门限*/
	{ 0x0058, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0058 ) },    //uint32_t	id_0x0058;      /*0x0058 当天累计驾驶时间门限*/
	{ 0x0059, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0059 ) },    //uint32_t	id_0x0059;      /*0x0059 最小休息时间*/
	{ 0x005A, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x005A ) },    //uint32_t	id_0x005A;      /*0x005A 最长停车时间*/
	{ 0x005B, TYPE_WORD,	 (uint8_t*)&( jt808_param.id_0x005B ) },    //uint16_t	id_0x005B;      /*0x005B 超速报警预警差值，单位为 1/10Km/h */
	{ 0x005C, TYPE_WORD,	 (uint8_t*)&( jt808_param.id_0x005C ) },    //uint16_t	id_0x005C;      /*0x005C 疲劳驾驶预警差值，单位为秒（s），>0*/
	{ 0x005D, TYPE_WORD,	 (uint8_t*)&( jt808_param.id_0x005D ) },    //uint16_t	id_0x005D;      /*0x005D 碰撞报警参数设置:b7..0：碰撞时间(4ms) b15..8：碰撞加速度(0.1g) 0-79 之间，默认为10 */
	{ 0x005E, TYPE_WORD,	 (uint8_t*)&( jt808_param.id_0x005E ) },    //uint16_t	id_0x005E;      /*0x005E 侧翻报警参数设置： 侧翻角度，单位 1 度，默认为 30 度*/
	{ 0x0064, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0064 ) },    //uint32_t	id_0x0064;      /*0x0064 定时拍照控制*/
	{ 0x0065, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0065 ) },    //uint32_t	id_0x0065;      /*0x0065 定距拍照控制*/
	{ 0x0070, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0070 ) },    //uint32_t	id_0x0070;      /*0x0070 图像视频质量(1-10)*/
	{ 0x0071, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0071 ) },    //uint32_t	id_0x0071;      /*0x0071 亮度*/
	{ 0x0072, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0072 ) },    //uint32_t	id_0x0072;      /*0x0072 对比度*/
	{ 0x0073, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0073 ) },    //uint32_t	id_0x0073;      /*0x0073 饱和度*/
	{ 0x0074, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0074 ) },    //uint32_t	id_0x0074;      /*0x0074 色度*/
	{ 0x0080, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0080 ) },    //uint32_t	id_0x0080;      /*0x0080 车辆里程表读数0.1km*/
	{ 0x0081, TYPE_WORD,	 (uint8_t*)&( jt808_param.id_0x0081 ) },    //uint16_t	id_0x0081;      /*0x0081 省域ID*/
	{ 0x0082, TYPE_WORD,	 (uint8_t*)&( jt808_param.id_0x0082 ) },    //uint16_t	id_0x0082;      /*0x0082 市域ID*/
	{ 0x0083, TYPE_STR,		 (uint8_t*)&( jt808_param.id_0x0083 ) },    //char		id_0x0083[32];  /*0x0083 机动车号牌*/
	{ 0x0084, TYPE_BYTE,	 (uint8_t*)&( jt808_param.id_0x0084 ) },    //uint8_t		id_0x0084;      /*0x0084 车牌颜色  1蓝色 2黄色 3黑色 4白色 9其他*/
	{ 0x0090, TYPE_BYTE,	 (uint8_t*)&( jt808_param.id_0x0090 ) },    //uint8_t		id_0x0090;      /*0x0090 GNSS 定位模式*/
	{ 0x0091, TYPE_BYTE,	 (uint8_t*)&( jt808_param.id_0x0091 ) },    //uint8_t		id_0x0091;      /*0x0091 GNSS 波特率*/
	{ 0x0092, TYPE_BYTE,	 (uint8_t*)&( jt808_param.id_0x0092 ) },    //uint8_t		id_0x0092;      /*0x0092 GNSS 模块详细定位数据输出频率*/
	{ 0x0093, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0093 ) },    //uint32_t	id_0x0093;      /*0x0093 GNSS 模块详细定位数据采集频率*/
	{ 0x0094, TYPE_BYTE,	 (uint8_t*)&( jt808_param.id_0x0094 ) },    //uint8_t		id_0x0094;      /*0x0094 GNSS 模块详细定位数据上传方式*/
	{ 0x0095, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0095 ) },    //uint32_t	id_0x0095;      /*0x0095 GNSS 模块详细定位数据上传设置*/
	{ 0x0100, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0100 ) },    //uint32_t	id_0x0100;      /*0x0100 CAN 总线通道 1 采集时间间隔(ms)，0 表示不采集*/
	{ 0x0101, TYPE_WORD,	 (uint8_t*)&( jt808_param.id_0x0101 ) },    //uint16_t	id_0x0101;      /*0x0101 CAN 总线通道 1 上传时间间隔(s)，0 表示不上传*/
	{ 0x0102, TYPE_DWORD,	 (uint8_t*)&( jt808_param.id_0x0102 ) },    //uint32_t	id_0x0102;      /*0x0102 CAN 总线通道 2 采集时间间隔(ms)，0 表示不采集*/
	{ 0x0103, TYPE_WORD,	 (uint8_t*)&( jt808_param.id_0x0103 ) },    //uint16_t	id_0x0103;      /*0x0103 CAN 总线通道 2 上传时间间隔(s)，0 表示不上传*/
	{ 0x0110, TYPE_BYTE | 8, (uint8_t*)&( jt808_param.id_0x0110 ) },    //uint8_t		id_0x0110[8];   /*0x0110 CAN 总线 ID 单独采集设置*/
	{ 0x0111, TYPE_BYTE | 8, (uint8_t*)&( jt808_param.id_0x0111 ) },    //uint8_t		id_0x0111[8];   /*0x0111 其他CAN 总线 ID 单独采集设置*/
	{ 0x0112, TYPE_BYTE | 8, (uint8_t*)&( jt808_param.id_0x0112 ) },    //uint8_t		id_0x0112[8];   /*0x0112 其他CAN 总线 ID 单独采集设置*/
	{ 0x0113, TYPE_BYTE | 8, (uint8_t*)&( jt808_param.id_0x0113 ) },    //uint8_t		id_0x0113[8];   /*0x0113 其他CAN 总线 ID 单独采集设置*/
	{ 0x0114, TYPE_BYTE | 8, (uint8_t*)&( jt808_param.id_0x0114 ) },    //uint8_t		id_0x0114[8];   /*0x0114 其他CAN 总线 ID 单独采集设置*/
	{ 0x0115, TYPE_BYTE | 8, (uint8_t*)&( jt808_param.id_0x0115 ) },    //uint8_t		id_0x0115[8];   /*0x0115 其他CAN 总线 ID 单独采集设置*/
	{ 0x0116, TYPE_BYTE | 8, (uint8_t*)&( jt808_param.id_0x0116 ) },    //uint8_t		id_0x0116[8];   /*0x0116 其他CAN 总线 ID 单独采集设置*/
	{ 0x0117, TYPE_BYTE | 8, (uint8_t*)&( jt808_param.id_0x0117 ) },    //uint8_t		id_0x0117[8];   /*0x0117 其他CAN 总线 ID 单独采集设置*/
	{ 0x0118, TYPE_BYTE | 8, (uint8_t*)&( jt808_param.id_0x0118 ) },    //uint8_t		id_0x0118[8];   /*0x0118 其他CAN 总线 ID 单独采集设置*/
	{ 0x0119, TYPE_BYTE | 8, (uint8_t*)&( jt808_param.id_0x0119 ) } //uint8_t		id_0x0119[8];   /*0x0119 其他CAN 总线 ID 单独采集设置*/
};


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
uint8_t param_put( uint16_t id, uint8_t len, uint8_t* value )
{
	int		i, j;
	uint8_t *psrc, *pdst;

	for( i = 0; i < sizeof( tbl_id_lookup ) / sizeof( struct _tbl_id_lookup ); i++ )
	{
		if( id == tbl_id_lookup[i].id )
		{
			if( ( tbl_id_lookup[i].type == TYPE_DWORD ) && ( len == 4 ) )
			{
				psrc = value;
				//rt_kprintf("psrc=%02x %02x %02x %02x \r\n",*(psrc+3),*(psrc+2),*(psrc+1),*(psrc+0));
				pdst	= tbl_id_lookup[i].val;
				*pdst++ = *( psrc + 3 );
				*pdst++ = *( psrc + 2 );
				*pdst++ = *( psrc + 1 );
				*pdst	= *( psrc + 0 );
				return 0;
			}
			if( ( tbl_id_lookup[i].type == TYPE_WORD ) && ( len == 2 ) )
			{
				psrc	= value;
				pdst	= tbl_id_lookup[i].val;
				*pdst++ = *( psrc + 1 );
				*pdst	= *psrc;
				return 0;
			}
			if( ( tbl_id_lookup[i].type == TYPE_BYTE ) && ( len == 1 ) )
			{
				psrc	= value;
				pdst	= tbl_id_lookup[i].val;
				*pdst	= *psrc;
				return 0;
			}
			if( tbl_id_lookup[i].type == TYPE_CAN_ID )
			{
				psrc	= value;
				pdst	= tbl_id_lookup[i].val;
				*pdst++ = *( psrc + 3 );
				*pdst++ = *( psrc + 2 );
				*pdst++ = *( psrc + 1 );
				*pdst++ = *psrc;
				*pdst++ = *( psrc + 7 );
				*pdst++ = *( psrc + 6 );
				*pdst++ = *( psrc + 5 );
				*pdst	= *( psrc + 4 );
				return 0;
			}
			if( tbl_id_lookup[i].type == TYPE_STR )
			{
				psrc = tbl_id_lookup[i].val;
				strncpy( psrc, value, 32 );
				*( psrc + 31 ) = 0;
				return 0;
			}
		}
	}
	return 1;
}

/*设置参数*/
static void param_put_int( uint16_t id, uint32_t value )
{
	uint32_t	i, j;
	int			len;
	uint8_t		*p;

	for( i = 0; i < sizeof( tbl_id_lookup ) / sizeof( struct _tbl_id_lookup ); i++ )
	{
		if( id == tbl_id_lookup[i].id )
		{
			p		= tbl_id_lookup[i].val;
			j		= value;
			*p++	= ( j & 0xff );
			*p++	= j >> 8;
			*p++	= j >> 16;
			*p		= j >> 24;
			break;
		}
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
static void param_put_str( uint16_t id, uint8_t* value )
{
	int		i, j;
	int		len;
	uint8_t *p;

	for( i = 0; i < sizeof( tbl_id_lookup ) / sizeof( struct _tbl_id_lookup ); i++ )
	{
		if( id == tbl_id_lookup[i].id )
		{
			p = tbl_id_lookup[i].val;
			strncpy( p, (char*)value, 32 );
			break;
		}
	}
}

/*读取参数,返回参数类型参数*/
uint8_t param_get( uint16_t id, uint8_t* value )
{
	int			i;
	uint8_t		*p;
	uint32_t	val;
	for( i = 0; i < sizeof( tbl_id_lookup ) / sizeof( struct _tbl_id_lookup ); i++ )
	{
		if( id == tbl_id_lookup[i].id )
		{
			if( tbl_id_lookup[i].type == TYPE_DWORD )
			{
				p = tbl_id_lookup[i].val;
				memcpy( value, p, 4 );
				return 4;
			}

			if( tbl_id_lookup[i].type == TYPE_WORD )
			{
				p = tbl_id_lookup[i].val;
				memcpy( value, p, 2 );
				return 2;
			}

			if( tbl_id_lookup[i].type == TYPE_BYTE )
			{
				p		= tbl_id_lookup[i].val;
				*value	= *p;
				return 1;
			}
			if( tbl_id_lookup[i].type == TYPE_STR )
			{
				p = tbl_id_lookup[i].val;
				memcpy( value, p, strlen( p ) );
				return strlen( p );
			}
			if( tbl_id_lookup[i].type == TYPE_CAN_ID )
			{
				p = tbl_id_lookup[i].val;
				memcpy( value, p, 8 );
				return 8;
			}
		}
	}
	return 0;
}

/*读取参数*/
uint32_t param_get_int( uint16_t id )
{
	int			i;
	uint8_t		*p;
	uint32_t	val = 0;
	for( i = 0; i < sizeof( tbl_id_lookup ) / sizeof( struct _tbl_id_lookup ); i++ )
	{
		if( id == tbl_id_lookup[i].id )
		{
			p	= tbl_id_lookup[i].val;
			val |= ( *p++ );
			val |= ( ( *p++ ) << 8 );
			val |= ( ( *p++ ) << 16 );
			val |= ( ( *p ) << 24 );
			return val;
		}
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
void param_print( void )
{
	int			i, j, id;
	int			type;
	uint8_t		*p;
	uint32_t	val = 0;

	for( i = 0; i < sizeof( tbl_id_lookup ) / sizeof( struct _tbl_id_lookup ); i++ )
	{
		id	= tbl_id_lookup[i].id;
		p	= tbl_id_lookup[i].val;
		val = 0;
		switch( tbl_id_lookup[i].type )
		{
			case TYPE_DWORD: /*字节对齐方式 little_endian*/
				val |= ( *p++ );
				val |= ( ( *p++ ) << 8 );
				val |= ( ( *p++ ) << 16 );
				val |= ( ( *p ) << 24 );
				rt_kprintf( "\r\nid=%04x value=%08x\r\n", id, val );
				break;
			case TYPE_CAN_ID:
				val |= ( *p++ );
				val |= ( ( *p++ ) << 8 );
				val |= ( ( *p++ ) << 16 );
				val |= ( ( *p++ ) << 24 );
				rt_kprintf( "\r\nid=%04x value=%08x", id, val );
				val = 0;
				val |= ( *p++ );
				val |= ( ( *p++ ) << 8 );
				val |= ( ( *p++ ) << 16 );
				val |= ( ( *p ) << 24 );
				rt_kprintf( " %08x\r\n", val );
				break;

				break;
			case TYPE_STR:
				rt_kprintf( "\r\nid=%04x value=%s\r\n", id, p );
				break;
		}
	}
}

FINSH_FUNCTION_EXPORT( param_print, print param );

/*打印参数信息*/
void param_dump( void )
{
	uint8_t tbl[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
	int		i, count = 0;
	uint8_t c;
	uint8_t *p = (uint8_t*)&jt808_param;
	uint8_t printbuf[70];
	int32_t len = sizeof( jt808_param );

	while( len > 0 )
	{
		count = ( len < 16 ) ? len : 16;
		memset( printbuf, 0x20, 70 );
		for( i = 0; i < count; i++ )
		{
			c					= *p;
			printbuf[i * 3]		= tbl[c >> 4];
			printbuf[i * 3 + 1] = tbl[c & 0x0f];
			if( c < 0x20 )
			{
				c = '.';
			}
			if( c > 0x7f )
			{
				c = '.';
			}
			printbuf[50 + i] = c;
			p++;
		}
		printbuf[69] = 0;
		rt_kprintf( "%s\r\n", printbuf );
		len -= count;
	}
}

FINSH_FUNCTION_EXPORT( param_dump, dump param );


/*
   jt808格式数据解码判断
   <标识0x7e><消息头><消息体><校验码><标识0x7e>

   返回有效的数据长度,为0 表明有错

 */
static uint16_t jt808_decode_fcs( uint8_t * pinfo, uint16_t length )
{
	uint8_t		* psrc, * pdst;
	uint16_t	count, len;
	uint8_t		fstuff	= 0;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        /*是否字节填充*/
	uint8_t		fcs		= 0;

	if( length < 5 )
	{
		return 0;
	}
	if( *pinfo != 0x7e )
	{
		return 0;
	}
	if( *( pinfo + length - 1 ) != 0x7e )
	{
		return 0;
	}
	psrc	= pinfo + 1;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            /*1byte标识后为正式信息*/
	pdst	= pinfo;
	count	= 0;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    /*转义后的长度*/
	len		= length - 2;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           /*去掉标识位的数据长度*/

	while( len )
	{
		if( fstuff )
		{
			*pdst	= *psrc + 0x7c;
			fstuff	= 0;
			count++;
			fcs ^= *pdst;
		} else
		{
			if( *psrc == 0x7d )
			{
				fstuff = 1;
			} else
			{
				*pdst	= *psrc;
				fcs		^= *pdst;
				count++;
			}
		}
		psrc++;
		pdst++;
		len--;
	}
	if( fcs != 0 )
	{
		rt_kprintf( "%s>fcs error\r\n", __func__ );
		return 0;
	}
	rt_kprintf( "count=%d\r\n", count );
	return count;
}

/**添加一个字节**/
static uint16_t jt808_pack_byte( uint8_t * buf, uint8_t * fcs, uint8_t data )
{
	uint8_t * p = buf;
	*fcs ^= data;
	if( ( data == 0x7d ) || ( data == 0x7e ) )
	{
		*p++	= 0x7d;
		*p		= ( data - 0x7c );
		return 2;
	} else
	{
		*p = data;
		return 1;
	}
}

/*传递进长度，便于计算*/
static uint16_t jt808_pack_int( uint8_t * buf, uint8_t * fcs, uint32_t data, uint8_t width )
{
	uint16_t count = 0;
	switch( width )
	{
		case 1:
			count += jt808_pack_byte( buf + count, fcs, ( data & 0xff ) );
			break;
		case 2:
			count	+= jt808_pack_byte( buf + count, fcs, ( data >> 8 ) );
			count	+= jt808_pack_byte( buf + count, fcs, ( data & 0xff ) );
			break;
		case 4:
			count	+= jt808_pack_byte( buf + count, fcs, ( data >> 24 ) );
			count	+= jt808_pack_byte( buf + count, fcs, ( data >> 16 ) );
			count	+= jt808_pack_byte( buf + count, fcs, ( data >> 8 ) );
			count	+= jt808_pack_byte( buf + count, fcs, ( data & 0xff ) );
			break;
	}
	return count;
}

/*添加字符串***/
static uint16_t jt808_pack_string( uint8_t * buf, uint8_t * fcs, char * str )
{
	uint16_t	count	= 0;
	char		* p		= str;
	while( *p )
	{
		count += jt808_pack_byte( buf + count, fcs, *p++ );
	}
	return count;
}

/**添加数组**/
static uint16_t jt808_pack_array( uint8_t * buf, uint8_t * fcs, uint8_t * src, uint16_t len )
{
	uint16_t	count = 0;
	int			i;
	char		* p = src;
	for( i = 0; i < len; i++ )
	{
		count += jt808_pack_byte( buf + count, fcs, *p++ );
	}
	return count;
}

/*
   jt808终端发送信息
   并将相关信息注册到接收信息的处理线程中
   需要传递消息ID,和消息体，由jt808_send线程完成
    消息的填充
    发送和重发机制
    流水号
    已发信息的回收free
   传递进来的格式
   <msgid 2bytes><msg_len 2bytes><msgbody nbytes>

 */
static void jt808_send( void * parameter )
{
}

/*发送后收到应答处理*/
void jt808_tx_response( uint8_t linkno, uint8_t *pmsg )
{
	uint8_t		* msg = pmsg + 12;
	uint16_t	id;
	uint16_t	seq;
	uint8_t		res;

	seq = ( *msg << 8 ) | *( msg + 1 );
	id	= ( *( msg + 2 ) << 8 ) | *( msg + 3 );
	res = *( msg + 4 );

	switch( id )                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                // 判断对应终端消息的ID做区分处理
	{
		case 0x0200:                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            //	对应位置消息的应答
			rt_kprintf( "\r\nCentre ACK!\r\n" );
			break;
		case 0x0002:                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            //	心跳包的应答
			rt_kprintf( "\r\n  Centre  Heart ACK!\r\n" );
			break;
		case 0x0101:                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            //	终端注销应答
			break;
		case 0x0102:                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            //	终端鉴权
			break;
		case 0x0800:                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            // 多媒体事件信息上传
			break;
		case 0x0702:
			rt_kprintf( "\r\n  驾驶员信息上报---中心应答!  \r\n" );
			break;
		case 0x0701:
			rt_kprintf( "\r\n	电子运单上报---中心应答!  \r\n");
			break;
		default:
			rt_kprintf( "\r\nunknown id=%04x\r\n", id );
			break;
	}
}

/*
   消息发送超时
 */
static rt_err_t jt808_tx_timeout( JT808_TX_MSG_NODEDATA * nodedata )
{
	rt_kprintf( "tx timeout\r\n" );
}

/*
   添加一个信息到发送列表中
 */
static rt_err_t jt808_add_tx_data( uint8_t linkno, JT808_MSG_TYPE type, uint16_t id, uint8_t *pinfo, uint16_t len )
{
	uint8_t					* pdata;
	JT808_TX_MSG_NODEDATA	* pnodedata;

	pnodedata = rt_malloc( sizeof( JT808_TX_MSG_NODEDATA ) );
	if( pnodedata == NULL )
	{
		return -RT_ERROR;
	}
	pnodedata->type				= type;
	pnodedata->state			= IDLE;
	pnodedata->retry			= 0;
	pnodedata->cb_tx_timeout	= jt808_tx_timeout;
	pnodedata->cb_tx_response	= jt808_tx_response;
//在此可以存储在上报
	pdata = rt_malloc( len );
	if( pdata == NULL )
	{
		rt_free( pnodedata );
		return -RT_ERROR;
	}
	memcpy( pdata, pinfo, len );
	pnodedata->msg_len	= len;
	pnodedata->pmsg		= pdata;
	pnodedata->head_sn	= tx_seq;
	pnodedata->head_id	= id;
	msglist_append( list_jt808_tx, pnodedata );
	tx_seq++;
}

/*
   终端通用应答
 */
static rt_err_t jt808_tx_0x0001( uint8_t linkno, uint16_t seq, uint16_t id, uint8_t res )
{
	uint8_t					* pdata;
	JT808_TX_MSG_NODEDATA	* pnodedata;
	uint8_t					buf [256];
	uint8_t					* p;
	uint16_t				len;

	pnodedata = rt_malloc( sizeof( JT808_TX_MSG_NODEDATA ) );
	if( pnodedata == NULL )
	{
		return -RT_ERROR;
	}
	pnodedata->type				= TERMINAL_ACK;
	pnodedata->state			= IDLE;
	pnodedata->retry			= 0;
	pnodedata->cb_tx_timeout	= jt808_tx_timeout;
	pnodedata->cb_tx_response	= jt808_tx_response;

	len		= jt808_pack( buf, "%w%6s%w%w%w%b", 0x0001, term_param.mobile, tx_seq, seq, id, res );
	pdata	= rt_malloc( len );
	if( pdata == NULL )
	{
		rt_free( pnodedata );
		return -RT_ERROR;
	}
	memcpy( pdata, buf, len );
	pnodedata->msg_len	= len;
	pnodedata->pmsg		= pdata;
	msglist_append( list_jt808_tx, pnodedata );
	tx_seq++;
}

/*平台通用应答,收到信息，停止发送*/
static int handle_rx_0x8001( uint8_t linkno, uint8_t *pmsg )
{
	MsgListNode				* iter;
	JT808_TX_MSG_NODEDATA	* iterdata;

	uint16_t				id;
	uint16_t				seq;
	uint8_t					res;
/*跳过消息头12byte*/
	seq = ( *( pmsg + 12 ) << 8 ) | *( pmsg + 13 );
	id	= ( *( pmsg + 14 ) << 8 ) | *( pmsg + 15 );
	res = *( pmsg + 16 );

	/*单条处理*/
	iter		= list_jt808_tx->first;
	iterdata	= (JT808_TX_MSG_NODEDATA*)iter->data;
	if( ( iterdata->head_id == id ) && ( iterdata->head_sn == seq ) )
	{
		iterdata->cb_tx_response( linkno, pmsg ); /*应答处理函数*/
		iterdata->state = ACK_OK;
	}
}

/*补传分包请求*/
static int handle_rx_0x8003( uint8_t linkno, uint8_t *pmsg )
{
}

/* 监控中心对终端注册消息的应答*/
static int handle_rx_0x8100( uint8_t linkno, uint8_t *pmsg )
{
	MsgListNode				* iter;
	JT808_TX_MSG_NODEDATA	* iterdata;

	uint16_t				body_len; /*消息体长度*/
	uint16_t				ack_seq;
	uint8_t					res;
	uint8_t					* msg;

	body_len	= ( ( *( pmsg + 2 ) << 8 ) | ( *( pmsg + 3 ) ) ) & 0x3FF;
	msg			= pmsg + 12;

	ack_seq = ( *msg << 8 ) | *( msg + 1 );
	res		= *( msg + 2 );

	iter		= list_jt808_tx->first;
	iterdata	= iter->data;
	if( ( iterdata->head_id == 0x0100 ) && ( iterdata->head_sn == ack_seq ) )
	{
		if( res == 0 )
		{
			strncpy( term_param.register_code, msg + 3, body_len - 3 );
			iterdata->state = ACK_OK;
		}
	}
	return 1;
}

/*设置终端参数*/
static int handle_rx_0x8103( uint8_t linkno, uint8_t *pmsg )
{
	uint8_t		* p;
	uint8_t		res;

	uint16_t	msg_len, count = 0;
	uint32_t	param_id;
	uint8_t		param_len;

	uint16_t	seq, id;

	if( *( pmsg + 2 ) >= 0x20 ) /*如果是多包的设置参数*/
	{
		rt_kprintf( "\r\n>%s multi packet no support!", __func__ );
		return 1;
	}

	id	= ( pmsg[0] << 8 ) | pmsg[1];
	seq = ( pmsg[10] << 8 ) | pmsg[11];

	msg_len = ( ( pmsg[2] << 8 ) | pmsg[3] ) & 0x3FF - 1;
	p		= pmsg + 13;

	/*使用数据长度,判断数据是否结束，没有使用参数总数*/
	while( count < msg_len )
	{
		param_id	= ( ( *p++ ) << 24 ) | ( ( *p++ ) << 16 ) | ( ( *p++ ) << 8 ) | ( *p++ );
		param_len	= *p++;
		count		+= ( 5 + param_len );
		res			|= param_put( param_id, param_len, p );
		if( res )
		{
			rt_kprintf( "\r\n%s>res=%d\r\n", __func__, __LINE__ );
			break;
		}
	}
	/*返回通用应答*/
	jt808_tx_0x0001( linkno, seq, id, res );
	return 1;
}

/*查询全部终端参数，有可能会超出单包最大字节*/
static int handle_rx_0x8104( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/*终端控制*/
static int handle_rx_0x8105( uint8_t linkno, uint8_t *pmsg )
{
	uint8_t cmd;
	uint8_t * cmd_arg;

	cmd = *( pmsg + 12 );
	switch( cmd )
	{
		case 1: /*无线升级*/
			break;
		case 2: /*终端控制链接指定服务器*/
			break;
		case 3: /*终端关机*/
			break;
		case 4: /*终端复位*/
			break;
		case 5: /*恢复出厂设置*/
			break;
		case 6: /*关闭数据通讯*/
			break;
		case 7: /*关闭所有无线通讯*/
			break;
	}
	return 1;
}

/*查询指定终端参数,返回应答0x0104*/
static int handle_rx_0x8106( uint8_t linkno, uint8_t *pmsg )
{
	int			i;
	uint8_t		*p;
	uint8_t		fcs = 0;
	uint8_t		value[8];
	uint32_t	id;
	uint16_t	len;
	uint16_t	pos;
	uint16_t	info_len	= 0;
	uint16_t	head_len	= 0;
	uint8_t		param_count, return_param_count;

	uint8_t		buf[1500];

	pos					= 100;              /*先空出100byte*/
	param_count			= *( pmsg + 12 );   /*总的参数个数*/
	return_param_count	= 0;
	p					= pmsg + 13;
	/*填充要返回消息的数据，并记录长度*/
	for( i = 0; i < param_count; i++ )      /*如果有未知的id怎么办，忽略,这样参数个数就改变了*/
	{
		id	= *p++;
		id	|= ( *p++ ) << 8;
		id	|= ( *p++ ) << 16;
		id	|= ( *p++ ) << 24;
		len = param_get( id, value );       /*得到参数的长度，未转义*/
		if( len )
		{
			return_param_count++;           /*找到有效的id*/
			pos += jt808_pack_int( buf + pos, &fcs, id, 2 );
			pos + jt808_pack_int( buf + pos, &fcs, len, 1 );
			pos			+= jt808_pack_array( buf + pos, &fcs, value, len );
			info_len	+= ( len + 3 );     /*id+长度+数据*/
		}
	}

	head_len	= 1;                        /*空出开始的0x7e*/
	head_len	+= jt808_pack_int( buf + head_len, &fcs, 0x0104, 2 );
	head_len	+= jt808_pack_int( buf + head_len, &fcs, info_len + 3, 2 );
	head_len	+= jt808_pack_array( buf + head_len, &fcs, pmsg + 4, 6 );
	head_len	+= jt808_pack_int( buf + head_len, &fcs, tx_seq, 2 );

	head_len	+= jt808_pack_array( buf + head_len, &fcs, pmsg + 10, 2 );
	head_len	+= jt808_pack_int( buf + head_len, &fcs, return_param_count, 1 );

	memcpy( buf + head_len, buf + 100, pos - 100 ); /*拼接数据*/
	len = head_len + pos - 100;                     /*当前数据0x7e,<head><msg>*/

	len			+= jt808_pack_byte( buf + len, &fcs, fcs );
	buf [0]		= 0x7e;
	buf [len]	= 0x7e;

	jt808_add_tx_data( linkno, TERMINAL_ACK, 0x0104, buf, len + 1 );
	return 1;
}

/*查询终端属性,应答 0x0107*/
static int handle_rx_0x8107( uint8_t linkno, uint8_t *pmsg )
{
	uint8_t		buf[100];
	uint8_t		fcs			= 0;
	uint16_t	len			= 1;
	uint16_t	info_len	= 0;
	uint16_t	head_len	= 1;

	len += jt808_pack_int( buf + len, &fcs, 0x0107, 2 );
	len += jt808_pack_int( buf + len, &fcs, 0x0107, 2 );
	len += jt808_pack_int( buf + len, &fcs, 0x0107, 2 );

	jt808_add_tx_data( linkno, TERMINAL_ACK, 0x0107, buf, len + 1 );
	return 1;
}

/**/
static int handle_rx_0x8201( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8202( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8300( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8301( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8302( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8303( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8304( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8400( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8401( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8500( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8600( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8601( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8602( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8603( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8604( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8605( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8606( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8607( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/*行驶记录仪数据采集*/
static int handle_rx_0x8700( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/*行驶记录仪参数下传*/
static int handle_rx_0x8701( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8800( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/*摄像头立即拍摄命令*/
static int handle_rx_0x8801( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8802( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8803( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8804( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8805( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8900( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8A00( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
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
static int handle_rx_default( uint8_t linkno, uint8_t *pmsg )
{
	rt_kprintf( "\r\nunknown!\r\n" );
	return 1;
}

#define DECL_JT808_RX_HANDLE( a )	{ a, handle_rx_ ## a }
#define DECL_JT808_TX_HANDLE( a )	{ a, handle_jt808_tx_ ## a }

HANDLE_JT808_RX_MSG handle_rx_msg[] =
{
	DECL_JT808_RX_HANDLE( 0x8001 ), //	通用应答
	DECL_JT808_RX_HANDLE( 0x8003 ), //	补传分包请求
	DECL_JT808_RX_HANDLE( 0x8100 ), //  监控中心对终端注册消息的应答
	DECL_JT808_RX_HANDLE( 0x8103 ), //	设置终端参数
	DECL_JT808_RX_HANDLE( 0x8104 ), //	查询终端参数
	DECL_JT808_RX_HANDLE( 0x8105 ), // 终端控制
	DECL_JT808_RX_HANDLE( 0x8106 ), // 查询指定终端参数
	DECL_JT808_RX_HANDLE( 0x8201 ), // 位置信息查询    位置信息查询消息体为空
	DECL_JT808_RX_HANDLE( 0x8202 ), // 临时位置跟踪控制
	DECL_JT808_RX_HANDLE( 0x8300 ), //	文本信息下发
	DECL_JT808_RX_HANDLE( 0x8301 ), //	事件设置
	DECL_JT808_RX_HANDLE( 0x8302 ), // 提问下发
	DECL_JT808_RX_HANDLE( 0x8303 ), //	信息点播菜单设置
	DECL_JT808_RX_HANDLE( 0x8304 ), //	信息服务
	DECL_JT808_RX_HANDLE( 0x8400 ), //	电话回拨
	DECL_JT808_RX_HANDLE( 0x8401 ), //	设置电话本
	DECL_JT808_RX_HANDLE( 0x8500 ), //	车辆控制
	DECL_JT808_RX_HANDLE( 0x8600 ), //	设置圆形区域
	DECL_JT808_RX_HANDLE( 0x8601 ), //	删除圆形区域
	DECL_JT808_RX_HANDLE( 0x8602 ), //	设置矩形区域
	DECL_JT808_RX_HANDLE( 0x8603 ), //	删除矩形区域
	DECL_JT808_RX_HANDLE( 0x8604 ), //	多边形区域
	DECL_JT808_RX_HANDLE( 0x8605 ), //	删除多边区域
	DECL_JT808_RX_HANDLE( 0x8606 ), //	设置路线
	DECL_JT808_RX_HANDLE( 0x8607 ), //	删除路线
	DECL_JT808_RX_HANDLE( 0x8700 ), //	行车记录仪数据采集命令
	DECL_JT808_RX_HANDLE( 0x8701 ), //	行驶记录仪参数下传命令
	DECL_JT808_RX_HANDLE( 0x8800 ), //	多媒体数据上传应答
	DECL_JT808_RX_HANDLE( 0x8801 ), //	摄像头立即拍照
	DECL_JT808_RX_HANDLE( 0x8802 ), //	存储多媒体数据检索
	DECL_JT808_RX_HANDLE( 0x8803 ), //	存储多媒体数据上传命令
	DECL_JT808_RX_HANDLE( 0x8804 ), //	录音开始命令
	DECL_JT808_RX_HANDLE( 0x8805 ), //	单条存储多媒体数据检索上传命令 ---- 补充协议要求
	DECL_JT808_RX_HANDLE( 0x8900 ), //	数据下行透传
	DECL_JT808_RX_HANDLE( 0x8A00 ), //	平台RSA公钥
};


/*
   接收处理
   分析jt808格式的数据
   <linkno><长度2byte><标识0x7e><消息头><消息体><校验码><标识0x7e>

 */
uint16_t jt808_rx_proc( uint8_t * pinfo )
{
	uint8_t		* psrc;
	uint16_t	len;
	uint8_t		linkno;
	uint16_t	i, id;
	uint8_t		flag_find	= 0;
	uint8_t		fcs			= 0;
	uint16_t	ret;

	linkno	= pinfo [0];
	len		= ( pinfo [1] << 8 ) | pinfo [2];
	rt_kprintf( ">dump start len=%d\r\n", len );

/*去转义，还是直接在pinfo上操作*/
	len = jt808_decode_fcs( pinfo + 3, len );
	if( len == 0 )
	{
		rt_kprintf( ">len=0\r\n" );
		return 1;
	}
/*显示解码后的信息*/
	rt_kprintf( "\r\n>dump start" );
	psrc = pinfo + 3;
	for( i = 0; i < len; i++ )
	{
		if( i % 16 == 0 )
		{
			rt_kprintf( "\r\n" );
		}
		rt_kprintf( "%02x ", *psrc++ );
	}
	rt_kprintf( "\r\n>dump end\r\n" );
/*fcs计算*/
	psrc = pinfo + 3;
	for( i = 0; i < len - 1; i++ )
	{
		fcs ^= *psrc++;
	}
	if( fcs != *( psrc + len - 1 ) )
	{
		rt_kprintf( "\r\n%d>%s fcs error!", rt_tick_get( ), __func__ );
		return 1;
	}

/*直接处理收到的信息，根据ID分发，直接分发消息*/

	psrc	= pinfo + 3;
	id		= ( *psrc << 8 ) | *( psrc + 1 );

	for( i = 0; i < sizeof( handle_rx_msg ) / sizeof( HANDLE_JT808_RX_MSG ); i++ )
	{
		if( id == handle_rx_msg [i].id )
		{
			handle_rx_msg [i].func( linkno, psrc );
			flag_find = 1;
		}
	}
	if( !flag_find )
	{
		handle_rx_default( linkno, psrc );
	}
}

/*
   处理每个要发送信息的状态
   现在允许并行处理吗?
 */

static MsgListRet jt808_tx_proc( MsgListNode * node )
{
	MsgListNode				* pnode		= ( MsgListNode* )node;
	JT808_TX_MSG_NODEDATA	* pnodedata = ( JT808_TX_MSG_NODEDATA* )( pnode->data );
	int						i;

	if( node == RT_NULL )
	{
		return MSGLIST_RET_OK;
	}

	if( pnodedata->state == IDLE )                      /*空闲，发送信息或超时后没有数据*/
	{
		if( pnodedata->retry >= jt808_param.id_0x0003 ) /*超过了最大重传次数*/                                                                     /*已经达到重试次数*/
		{
			/*表示发送失败*/
			pnodedata->cb_tx_timeout( pnodedata );      /*调用发送失败处理函数*/
			return MSGLIST_RET_DELETE_NODE;
		} else
		{
			socket_write( pnodedata->linkno, pnodedata->pmsg, pnodedata->msg_len, jt808_param.id_0x0002 * RT_TICK_PER_SECOND );
			pnodedata->tick = rt_tick_get( );
			pnodedata->retry++;
			pnodedata->timeout	= pnodedata->retry * jt808_param.id_0x0002 * RT_TICK_PER_SECOND;
			pnodedata->state	= WAIT_ACK;
			rt_kprintf( "send retry=%d,timeout=%d\r\n", pnodedata->retry, pnodedata->timeout * 10 );
		}
		return MSGLIST_RET_OK;
	}

	if( pnodedata->state == WAIT_ACK )
	{
		if( rt_tick_get( ) - pnodedata->tick > pnodedata->timeout )
		{
			pnodedata->state = IDLE;
		}
		return MSGLIST_RET_OK;
	}

	if( pnodedata->state == ACK_OK )
	{
		return MSGLIST_RET_DELETE_NODE;
	}

	return MSGLIST_RET_OK;
}

/*jt808的socket管理

   维护链路。会有不同的原因
   上报状态的维护
   1.尚未登网
   2.中心连接，DNS,超时不应答
   3.禁止上报，关闭模块的区域
   4.当前正在进行空中更新，多媒体上报等不需要打断的工作

 */

typedef enum
{
	CONNECT_IDLE = 0,
	CONNECTING,
	CONNECTED
}CONN_STATE;

struct
{
	uint8_t		disable_connect; /*禁止链接标志，协议控制 0:允许链接*/
	CONN_STATE	server_state;
	uint8_t		server_index;
	CONN_STATE	auth_state;
	uint8_t		auth_index;
} connect_state =
{ 0, CONNECT_IDLE, 0, CONNECT_IDLE, 0 };


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static void jt808_socket_proc( void )
{
	T_SOCKET_STATE	state;
	rt_tick_t		lasttick;

	if( flag_disable_report )
	{
		return;
	}
/*检查GSM状态*/
	state = gsmstate( 0 );
	if( state == GSM_IDLE )
	{
		if( connect_state.disable_connect == 0 )
		{
			gsmstate( GSM_POWERON ); /*开机登网*/
		}
		return;
	}
	if( state != GSM_AT )
	{
		return;
	}

/*在GSM_AT的情况下，依次检查连接的状态*/
/*检查808服务器的连接状态*/
	if( connect_state.server_state == CONNECT_IDLE )                                    /*没有连接*/
	{
		psocket						= &gsm_socket[connect_state.server_index % 2];      /*单数连接备用,双数连接主服*/
		psocket->state				= SOCKET_INIT;
		connect_state.server_state	= CONNECTING;
	}

	if( connect_state.server_state == CONNECTED )                                       /*808服务器已连接,连接IC卡服务器*/
	{
		if( connect_state.auth_state == CONNECT_IDLE )                                  /*没有连接*/
		{
			psocket						= &gsm_socket[connect_state.auth_index % 2];    /*单数连接备用,双数连接主服*/
			psocket->state				= SOCKET_INIT;
			connect_state.auth_state	= CONNECTING;
		}
		if( connect_state.auth_state == CONNECTING )
		{
		}
	}

/*检查socket状态,判断用不用DNS*/
	switch( psocket->state )
	{
		case SOCKET_IDLE:
			break;
		case SOCKET_CONNECT_ERR:
		case SOCKET_DNS_ERR:
			gsmstate( GSM_POWEROFF );   /*关闭gsm等待重连*/
			break;
	}
}

/*
   连接状态维护
   jt808协议处理

 */
ALIGN( RT_ALIGN_SIZE )
static char thread_jt808_stack [4096];
struct rt_thread thread_jt808;

/***/
static void rt_thread_entry_jt808( void * parameter )
{
	rt_err_t				ret;
	int						i;
	uint8_t					* pstr;

	MsgListNode				* iter;
	MsgListNode				* iter_next;
	JT808_TX_MSG_NODEDATA	* pnodedata;

	int						j = 0xaabbccdd;

	rt_kprintf( "\r\n1.id0=%08x\r\n", param_get_int( 0x0000 ) );

	param_put_int( 0x000, j );
	rt_kprintf( "\r\nwrite j=%08x read=%08x\r\n", j, param_get_int( 0x0000 ) );

	param_put( 0x000, 4, (uint8_t*)&j );
	rt_kprintf( "\r\nid0=%08x\r\n", param_get_int( 0x0000 ) );


/*读取参数，并配置*/
	param_load( );

	list_jt808_tx	= msglist_create( );
	list_jt808_rx	= msglist_create( );

	pdev_gsm = rt_device_find( "gsm" );

	while( 1 )
	{
/*接收gps信息*/
		ret = rt_mb_recv( &mb_gpsdata, ( rt_uint32_t* )&pstr, 5 );
		if( ret == RT_EOK )
		{
			gps_analy( pstr );
			rt_free( pstr );
		}
/*接收gprs信息*/
		ret = rt_mb_recv( &mb_gprsdata, ( rt_uint32_t* )&pstr, 5 );
		if( ret == RT_EOK )
		{
			jt808_rx_proc( pstr );
			rt_free( pstr );
		}
/*jt808 socket处理*/
		jt808_socket_proc( );

/*发送信息逐条处理*/
		iter = list_jt808_tx->first;

		if( jt808_tx_proc( iter ) == MSGLIST_RET_DELETE_NODE )  /*删除该节点*/
		{
			pnodedata = ( JT808_TX_MSG_NODEDATA* )( iter->data );
			rt_free( pnodedata->pmsg );                         /*删除用户数据*/
			rt_free( pnodedata );                               /*删除节点数据*/
			list_jt808_tx->first = iter->next;                  /*指向下一个*/
			rt_free( iter );
		}
		rt_thread_delay( RT_TICK_PER_SECOND / 20 );
	}

	msglist_destroy( list_jt808_tx );
}

/*jt808处理线程初始化*/
void jt808_init( void )
{
	rt_mb_init( &mb_gprsdata, "gprsdata", &mb_gprsdata_pool, MB_GPRSDATA_POOL_SIZE / 4, RT_IPC_FLAG_FIFO );
	rt_mb_init( &mb_gpsdata, "gpsdata", &mb_gpsdata_pool, MB_GPSDATA_POOL_SIZE / 4, RT_IPC_FLAG_FIFO );

	rt_thread_init( &thread_jt808,
	                "jt808",
	                rt_thread_entry_jt808,
	                RT_NULL,
	                &thread_jt808_stack [0],
	                sizeof( thread_jt808_stack ), 10, 5 );
	rt_thread_startup( &thread_jt808 );
}

/*gps接收处理*/
void gps_rx( uint8_t * pinfo, uint16_t length )
{
	uint8_t * pmsg;
	pmsg = rt_malloc( length + 2 );
	if( pmsg != RT_NULL )
	{
		pmsg [0]	= length >> 8;
		pmsg [1]	= length & 0xff;
		memcpy( pmsg + 2, pinfo, length );
		rt_mb_send( &mb_gpsdata, ( rt_uint32_t )pmsg );
	}
}

/*gprs接收处理,收到数据要尽快处理*/
rt_err_t gprs_rx( uint8_t linkno, uint8_t * pinfo, uint16_t length )
{
	uint8_t * pmsg;
	pmsg = rt_malloc( length + 3 );                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           /*包含长度信息*/
	if( pmsg != RT_NULL )
	{
		pmsg [0]	= linkno;
		pmsg [1]	= length >> 8;
		pmsg [2]	= length & 0xff;
		memcpy( pmsg + 3, pinfo, length );
		rt_mb_send( &mb_gprsdata, ( rt_uint32_t )pmsg );
		return 0;
	}
	return 1;
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
rt_err_t server( uint8_t cmd, char * apn, char * name, char * psw )
{
	if( cmd == 0 )
	{
		rt_kprintf( "\r\napn=%s user=%s psw=%s ip_domain=%s tcp_port=%d udp_port=%d",
		            jt808_param.id_0x0010,
		            jt808_param.id_0x0011,
		            jt808_param.id_0x0012,
		            jt808_param.id_0x0013,
		            jt808_param.id_0x0018,
		            jt808_param.id_0x0019
		            );
		rt_kprintf( "\r\napn=%s user=%s psw=%s ip_domain=%s tcp_port=%d udp_port=%d\r\n",
		            jt808_param.id_0x0014,
		            jt808_param.id_0x0015,
		            jt808_param.id_0x0016,
		            jt808_param.id_0x0017,
		            jt808_param.id_0x0018,
		            jt808_param.id_0x0019
		            );
	}
	if( cmd == 1 )                                                                                                                                          /*设置主server*/
	{
	}
	if( cmd == 2 )                                                                                                                                          /*设置备份server*/
	{
	}
	return RT_EOK;
}

FINSH_FUNCTION_EXPORT( server, config server );


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static rt_err_t jt808_tx( void )
{
	uint8_t					* pdata;
	JT808_TX_MSG_NODEDATA	* pnodedata;
	uint8_t					buf [256];
	uint8_t					* p;
	uint16_t				len;
	uint8_t					fcs = 0;
	int						i;

	pnodedata = rt_malloc( sizeof( JT808_TX_MSG_NODEDATA ) );
	if( pnodedata == NULL )
	{
		return -1;
	}
	pnodedata->type				= TERMINAL_CMD;
	pnodedata->state			= IDLE;
	pnodedata->retry			= 0;
	pnodedata->cb_tx_timeout	= jt808_tx_timeout;
	pnodedata->cb_tx_response	= jt808_tx_response;

	len = jt808_pack( buf, "%w%w%6s%w%w%w%5s%20s%7s%b%s",
	                  0x0100,
	                  37 + strlen( jt808_param.id_0x0083 ),
	                  term_param.mobile,
	                  tx_seq,
	                  jt808_param.id_0x0081,
	                  jt808_param.id_0x0082,
	                  term_param.producer_id,
	                  term_param.model,
	                  term_param.terminal_id,
	                  jt808_param.id_0x0084,
	                  jt808_param.id_0x0083 );

	rt_kprintf( "\r\n*********************\r\n" );
	for( i = 0; i < len; i++ )
	{
		rt_kprintf( "%02x ", buf [i] );
	}
	rt_kprintf( "\r\n*********************\r\n" );

	len = 1;
	len += jt808_pack_int( buf + len, &fcs, 0x0100, 2 );
	len += jt808_pack_int( buf + len, &fcs, 37 + strlen( jt808_param.id_0x0083 ), 2 );
	len += jt808_pack_array( buf + len, &fcs, term_param.mobile, 6 );
	len += jt808_pack_int( buf + len, &fcs, tx_seq, 2 );

	len			+= jt808_pack_int( buf + len, &fcs, jt808_param.id_0x0081, 2 );
	len			+= jt808_pack_int( buf + len, &fcs, jt808_param.id_0x0082, 2 );
	len			+= jt808_pack_array( buf + len, &fcs, term_param.producer_id, 5 );
	len			+= jt808_pack_array( buf + len, &fcs, term_param.model, 20 );
	len			+= jt808_pack_array( buf + len, &fcs, term_param.terminal_id, 7 );
	len			+= jt808_pack_int( buf + len, &fcs, jt808_param.id_0x0084, 1 );
	len			+= jt808_pack_string( buf + len, &fcs, jt808_param.id_0x0083 );
	len			+= jt808_pack_byte( buf + len, &fcs, fcs );
	buf [0]		= 0x7e;
	buf [len]	= 0x7e;
	pdata		= rt_malloc( len + 1 );
	if( pdata == NULL )
	{
		rt_free( pnodedata );
		return;
	}
	rt_kprintf( "\r\n--------------------\r\n" );
	for( i = 0; i < len + 1; i++ )
	{
		rt_kprintf( "%02x ", buf [i] );
	}
	rt_kprintf( "\r\n--------------------\r\n" );
	memcpy( pdata, buf, len + 1 );
	pnodedata->msg_len	= len + 1;
	pnodedata->pmsg		= pdata;
	pnodedata->head_sn	= tx_seq;
	pnodedata->head_id	= 0x0100;
	msglist_append( list_jt808_tx, pnodedata );
	tx_seq++;
}

FINSH_FUNCTION_EXPORT( jt808_tx, jt808_tx test );

/************************************** The End Of File **************************************/

