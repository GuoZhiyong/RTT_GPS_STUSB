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

#define DECL_PARAM_DWORD( id, value )	int parma_ ## id	= value
#define DECL_PARAM_STRING( id, value )	char* parma_ ## id	= value

DECL_PARAM_DWORD( 0x0000, 0x13020100 );  /*0x0000 版本*/
DECL_PARAM_DWORD( 0x0001, 5 );                  /*0x0001 心跳发送间隔*/
DECL_PARAM_DWORD( 0x0002, 3 );                  /*0x0002 TCP应答超时时间*/
DECL_PARAM_DWORD( 0x0003, 15 );                 /*0x0003 TCP超时重传次数*/
DECL_PARAM_DWORD( 0x0004, 3 );                  /*0x0004	UDP应答超时时间*/
DECL_PARAM_DWORD( 0x0005, 5 );                  /*0x0005 UDP超时重传次数*/
DECL_PARAM_DWORD( 0x0006, 3 );                  /*0x0006 SMS消息应答超时时间*/
DECL_PARAM_DWORD( 0x0007, 5 );                  /*0x0007 SMS消息重传次数*/
DECL_PARAM_STRING( 0x0010, (char*)"CMNET" );     /*0x0010 主服务器APN*/
DECL_PARAM_STRING( 0x0011, "" );                 /*0x0011 用户名*/
DECL_PARAM_STRING( 0x0012, "" );                 /*0x0012 密码*/
DECL_PARAM_STRING( 0x0013, "" );                 /*0x0013 主服务器地址*/
DECL_PARAM_STRING( 0x0014, "" );                 /*0x0014 备份APN*/
DECL_PARAM_STRING( 0x0015, "" );                 /*0x0015 备份用户名*/
DECL_PARAM_STRING( 0x0016, "" );                 /*0x0016 备份密码*/
DECL_PARAM_STRING( 0x0017, "" );                 /*0x0017 备份服务器地址，ip或域名*/
DECL_PARAM_DWORD( 0x0018, 1234 );               /*0x0018 TCP端口*/
DECL_PARAM_DWORD( 0x0019, 5678 );               /*0x0019 UDP端口*/
DECL_PARAM_DWORD( 0x0020, 0 );                  /*0x0020 位置汇报策略*/
DECL_PARAM_DWORD( 0x0021, 1 );                  /*0x0021 位置汇报方案*/
DECL_PARAM_DWORD( 0x0022, 30 );                 /*0x0022 驾驶员未登录汇报时间间隔*/
DECL_PARAM_DWORD( 0x0027, 120 );                /*0x0027 休眠时汇报时间间隔*/
DECL_PARAM_DWORD( 0x0028, 5 );                  /*0x0028 紧急报警时汇报时间间隔*/
DECL_PARAM_DWORD( 0x0029, 30 );                 /*0x0029 缺省时间汇报间隔*/
DECL_PARAM_DWORD( 0x002C, 500 );                /*0x002c 缺省距离汇报间隔*/
DECL_PARAM_DWORD( 0x002D, 1000 );               /*0x002d 驾驶员未登录汇报距离间隔*/
DECL_PARAM_DWORD( 0x002E, 1000 );               /*0x002e 休眠时距离汇报间隔*/
DECL_PARAM_DWORD( 0x002F, 100 );                /*0x002f 紧急时距离汇报间隔*/
DECL_PARAM_DWORD( 0x0030, 5 );                  /*0x0030 拐点补传角度*/
DECL_PARAM_STRING( 0x0040, "1008611" );          /*0x0040 监控平台电话号码*/
DECL_PARAM_STRING( 0x0041, "" );                 /*0x0041 复位电话号码*/
DECL_PARAM_STRING( 0x0042, "" );                 /*0x0042 恢复出厂设置电话号码*/
DECL_PARAM_STRING( 0x0043, "" );                 /*0x0043 监控平台SMS号码*/
DECL_PARAM_STRING( 0x0044, "" );                 /*0x0044 接收终端SMS文本报警号码*/
DECL_PARAM_DWORD( 0x0045, 5 );                  /*0x0045 终端接听电话策略*/
DECL_PARAM_DWORD( 0x0046, 3 );                  /*0x0046 每次通话时长*/
DECL_PARAM_DWORD( 0x0047, 3 );                  /*0x0047 当月通话时长*/
DECL_PARAM_STRING( 0x0048, "" );                 /*0x0048 监听电话号码*/
DECL_PARAM_STRING( 0x0049, "" );                 /*0x0049 监管平台特权短信号码*/
DECL_PARAM_DWORD( 0x0050, 5 );                  /*0x0050 报警屏蔽字*/
DECL_PARAM_DWORD( 0x0051, 3 );                  /*0x0051 报警发送文本SMS开关*/
DECL_PARAM_DWORD( 0x0052, 5 );                  /*0x0052 报警拍照开关*/
DECL_PARAM_DWORD( 0x0053, 3 );                  /*0x0053 报警拍摄存储标志*/
DECL_PARAM_DWORD( 0x0054, 5 );                  /*0x0054 关键标志*/
DECL_PARAM_DWORD( 0x0055, 3 );                  /*0x0055 最高速度kmh*/
DECL_PARAM_DWORD( 0x0056, 5 );                  /*0x0056 超速持续时间*/
DECL_PARAM_DWORD( 0x0057, 3 );                  /*0x0057 连续驾驶时间门限*/
DECL_PARAM_DWORD( 0x0058, 5 );                  /*0x0058 当天累计驾驶时间门限*/
DECL_PARAM_DWORD( 0x0059, 3 );                  /*0x0059 最小休息时间*/
DECL_PARAM_DWORD( 0x005A, 5 );                  /*0x005A 最长停车时间*/
DECL_PARAM_DWORD( 0x0070, 3 );                  /*0x0070 图像视频质量(1-10)*/
DECL_PARAM_DWORD( 0x0071, 5 );                  /*0x0071 亮度*/
DECL_PARAM_DWORD( 0x0072, 3 );                  /*0x0072 对比度*/
DECL_PARAM_DWORD( 0x0073, 5 );                  /*0x0073 饱和度*/
DECL_PARAM_DWORD( 0x0074, 3 );                  /*0x0074 色度*/
DECL_PARAM_DWORD( 0x0080, 5 );                  /*0x0080 车辆里程表读数0.1km*/
DECL_PARAM_DWORD( 0x0081, 3 );                  /*0x0081 省域ID*/
DECL_PARAM_DWORD( 0x0082, 5 );                  /*0x0082 市域ID*/
DECL_PARAM_STRING( 0x0083, "津O-00001" );        /*0x0083 机动车号牌*/
DECL_PARAM_DWORD( 0x0084, 5 );                  /*0x0084 车牌颜色*/

#if 0

PARAM param_list[] =
{
	{ 0x0000, T_DWORD,	(void*)0x13020100 },    /*0x0000 版本*/
	{ 0x0001, T_DWORD,	5				  },    /*0x0001 心跳发送间隔*/
	{ 0x0002, T_DWORD,	3				  },    /*0x0002 TCP应答超时时间*/
	{ 0x0003, T_DWORD,	15				  },    /*0x0003 TCP超时重传次数*/
	{ 0x0004, T_DWORD,	3				  },    /*0x0004	UDP应答超时时间*/
	{ 0x0005, T_DWORD,	5				  },    /*0x0005 UDP超时重传次数*/
	{ 0x0006, T_DWORD,	3				  },    /*0x0006 SMS消息应答超时时间*/
	{ 0x0007, T_DWORD,	5				  },    /*0x0007 SMS消息重传次数*/
	{ 0x0010, T_STRING, (char*)"CMNET"	  },    /*0x0010 主服务器APN*/
	{ 0x0011, T_STRING, ""				  },    /*0x0011 用户名*/
	{ 0x0012, T_STRING, ""				  },    /*0x0012 密码*/
	{ 0x0013, T_STRING, ""				  },    /*0x0013 主服务器地址*/
	{ 0x0014, T_STRING, ""				  },    /*0x0014 备份APN*/
	{ 0x0015, T_STRING, ""				  },    /*0x0015 备份用户名*/
	{ 0x0016, T_STRING, ""				  },    /*0x0016 备份密码*/
	{ 0x0017, T_STRING, ""				  },    /*0x0017 备份服务器地址，ip或域名*/
	{ 0x0018, T_DWORD,	1234			  },    /*0x0018 TCP端口*/
	{ 0x0019, T_DWORD,	5678			  },    /*0x0019 UDP端口*/
	{ 0x0020, T_DWORD,	0				  },    /*0x0020 位置汇报策略*/
	{ 0x0021, T_DWORD,	1				  },    /*0x0021 位置汇报方案*/
	{ 0x0022, T_DWORD,	30				  },    /*0x0022 驾驶员未登录汇报时间间隔*/
	{ 0x0027, T_DWORD,	120				  },    /*0x0027 休眠时汇报时间间隔*/
	{ 0x0028, T_DWORD,	5				  },    /*0x0028 紧急报警时汇报时间间隔*/
	{ 0x0029, T_DWORD,	30				  },    /*0x0029 缺省时间汇报间隔*/
	{ 0x002C, T_DWORD,	500				  },    /*0x002c 缺省距离汇报间隔*/
	{ 0x002D, T_DWORD,	1000			  },    /*0x002d 驾驶员未登录汇报距离间隔*/
	{ 0x002E, T_DWORD,	1000			  },    /*0x002e 休眠时距离汇报间隔*/
	{ 0x002F, T_DWORD,	100				  },    /*0x002f 紧急时距离汇报间隔*/
	{ 0x0030, T_DWORD,	5				  },    /*0x0030 拐点补传角度*/
	{ 0x0040, T_STRING, "1008611"		  },    /*0x0040 监控平台电话号码*/
	{ 0x0041, T_STRING, ""				  },    /*0x0041 复位电话号码*/
	{ 0x0042, T_STRING, ""				  },    /*0x0042 恢复出厂设置电话号码*/
	{ 0x0043, T_STRING, ""				  },    /*0x0043 监控平台SMS号码*/
	{ 0x0044, T_STRING, ""				  },    /*0x0044 接收终端SMS文本报警号码*/
	{ 0x0045, T_DWORD,	5				  },    /*0x0045 终端接听电话策略*/
	{ 0x0046, T_DWORD,	3				  },    /*0x0046 每次通话时长*/
	{ 0x0047, T_DWORD,	3				  },    /*0x0047 当月通话时长*/
	{ 0x0048, T_STRING, ""				  },    /*0x0048 监听电话号码*/
	{ 0x0049, T_STRING, ""				  },    /*0x0049 监管平台特权短信号码*/
	{ 0x0050, T_DWORD,	5				  },    /*0x0050 报警屏蔽字*/
	{ 0x0051, T_DWORD,	3				  },    /*0x0051 报警发送文本SMS开关*/
	{ 0x0052, T_DWORD,	5				  },    /*0x0052 报警拍照开关*/
	{ 0x0053, T_DWORD,	3				  },    /*0x0053 报警拍摄存储标志*/
	{ 0x0054, T_DWORD,	5				  },    /*0x0054 关键标志*/
	{ 0x0055, T_DWORD,	3				  },    /*0x0055 最高速度kmh*/
	{ 0x0056, T_DWORD,	5				  },    /*0x0056 超速持续时间*/
	{ 0x0057, T_DWORD,	3				  },    /*0x0057 连续驾驶时间门限*/
	{ 0x0058, T_DWORD,	5				  },    /*0x0058 当天累计驾驶时间门限*/
	{ 0x0059, T_DWORD,	3				  },    /*0x0059 最小休息时间*/
	{ 0x005A, T_DWORD,	5				  },    /*0x005A 最长停车时间*/
	{ 0x0070, T_DWORD,	3				  },    /*0x0070 图像视频质量(1-10)*/
	{ 0x0071, T_DWORD,	5				  },    /*0x0071 亮度*/
	{ 0x0072, T_DWORD,	3				  },    /*0x0072 对比度*/
	{ 0x0073, T_DWORD,	5				  },    /*0x0073 饱和度*/
	{ 0x0074, T_DWORD,	3				  },    /*0x0074 色度*/
	{ 0x0080, T_DWORD,	5				  },    /*0x0080 车辆里程表读数0.1km*/
	{ 0x0081, T_DWORD,	3				  },    /*0x0081 省域ID*/
	{ 0x0082, T_DWORD,	5				  },    /*0x0082 市域ID*/
	{ 0x0083, T_STRING, "津O-00001"		  },    /*0x0083 机动车号牌*/
	{ 0x0084, T_DWORD,	5				  },    /*0x0084 车牌颜色*/
};
#endif

/************************************** The End Of File **************************************/
