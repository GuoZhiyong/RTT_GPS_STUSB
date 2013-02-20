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
#ifndef _H_JT808_H_
#define _H_JT808_H_

#include <stm32f4xx.h>
#include <rtthread.h>
#define   MsgQ_Timeout 3

/*
存储区域分配,采用绝对地址,以4K(0x1000)为一个扇区
*/

#define ADDR_PARAM	0x000000000




/*字节顺序的定义网络顺序*/
typedef struct
{
	uint32_t	latitude;       /*纬度 1/10000分 */
	uint32_t	longitude;      /*经度 1/10000分 */
	uint16_t	altitude;       /*高程 m*/
	uint16_t	speed;          /*速度 1/10KMH*/
	uint8_t		direction;      /*方向 0-178 刻度为2度*/
	uint8_t		datetime[6];    /*YY-MM-DD hh-mm-ss BCD编码*/
}T_GPSINFO;

//------- 文本信息 --------
typedef struct _TEXT_INFO
{
	uint8_t TEXT_FLAG;          //  文本标志
	uint8_t TEXT_SD_FLAG;       // 发送标志位
	uint8_t TEXT_Content[100];  // 文本内容
}TEXT_INFO;

//----- 信息 ----
typedef struct _MSG_TEXT
{
	uint8_t TEXT_mOld;          //  最新的一条信息  写为1代表是最新的一条信息
	uint8_t TEXT_TYPE;          //  信息类型   1-8  中第几条
	uint8_t TEXT_LEN;           //  信息长度
	uint8_t TEXT_STR[150];      //  信息内容
}MSG_TEXT;

//-----  提问 ------
typedef struct _CENTER_ASK
{
	uint8_t		ASK_SdFlag;     //  标志位           发给 TTS  1  ；   TTS 回来  2
	uint16_t	ASK_floatID;    // 提问流水号
	uint8_t		ASK_infolen;    // 信息长度
	uint8_t		ASK_answerID;   // 回复ID
	uint8_t		ASK_info[30];   //  信息内容
	uint8_t		ASK_answer[30]; // 候选答案
}CENTRE_ASK;

//------ 事件  -------
typedef struct _EVENT           //  name: event
{
	uint8_t Event_ID;           //  事件ID
	uint8_t Event_Len;          //  事件长度
	uint8_t Event_Effective;    //  事件是否有效，   1 为要显示  0
	uint8_t Event_Str[20];      //  事件内容
}EVENT;

//----- 信息 ----
typedef struct _MSG_BROADCAST   // name: msg_broadcast
{
	uint8_t		INFO_TYPE;      //  信息类型
	uint16_t	INFO_LEN;       //  信息长度
	uint8_t		INFO_PlyCancel; // 点播/取消标志      0 取消  1  点播
	uint8_t		INFO_SDFlag;    //  发送标志位
	uint8_t		INFO_Effective; //  显示是否有效   1 显示有效    0  显示无效
	uint8_t		INFO_STR[30];   //  信息内容
}MSG_BRODCAST;

//------ 电话本 -----
typedef struct _PHONE_BOOK      // name: phonebook
{
	uint8_t CALL_TYPE;          // 呼入类型  1 呼入 2 呼出 3 呼入/呼出
	uint8_t NumLen;             // 号码长度
	uint8_t UserLen;            // 联系人长度
	uint8_t Effective_Flag;     // 有效标志位   无效 0 ，有效  1
	uint8_t NumberStr[20];      // 电话号码
	uint8_t UserStr[10];        // 联系人名称  GBK 编码
}PHONE_BOOK;

typedef struct _MULTIMEDIA
{
	u32 Media_ID;               //   多媒体数据ID
	u8	Media_Type;             //   0:   图像    1 : 音频    2:  视频
	u8	Media_CodeType;         //   编码格式  0 : JPEG  1:TIF  2:MP3  3:WAV  4: WMV
	u8	Event_Code;             //   事件编码  0: 平台下发指令  1: 定时动作  2 : 抢劫报警触发 3: 碰撞侧翻报警触发 其他保留
	u8	Media_Channel;          //   通道ID
	//----------------------
	u8	SD_Eventstate;          // 发送事件信息上传状态    0 表示空闲   1  表示处于发送状态
	u8	SD_media_Flag;          // 发送没提事件信息标志位
	u8	SD_Data_Flag;           // 发送数据标志位
	u8	SD_timer;               // 发送定时器
	u8	MaxSd_counter;          // 最大发送次数
	u8	Media_transmittingFlag; // 多媒体传输数据状态  1: 多媒体传输前发送1包定位信息    2 :多媒体数据传输中  0:  未进行多媒体数据传输
	u16 Media_totalPacketNum;   // 多媒体总包数
	u16 Media_currentPacketNum; // 多媒体当前报数
	//----------------------
	u8	RSD_State;              //  重传状态   0 : 重传没有启用   1 :  重传开始    2  : 表示顺序传完但是还没收到中心的重传命令
	u8	RSD_Timer;              //  传状态下的计数器
	u8	RSD_Reader;             //  重传计数器当前数值
	u8	RSD_total;              //  重传选项数目

	u8 Media_ReSdList[10];      //  多媒体重传消息列表
}MULTIMEDIA;

//--------------每分钟的平均速度
typedef  struct AvrgMintSpeed
{
	uint8_t datetime[6];        //current
	uint8_t datetime_Bak[6];    //Bak
	uint8_t avgrspd[60];
	uint8_t saveFlag;
}Avrg_MintSpeed;

extern uint32_t		jt808_alarm;
extern uint32_t		jt808_status;

extern TEXT_INFO	TextInfo;
//-------文本信息-------
extern MSG_TEXT		TEXT_Obj;
extern MSG_TEXT		TEXT_Obj_8[8], TEXT_Obj_8bak[8];

//------ 提问  --------
extern CENTRE_ASK ASK_Centre;                       // 中心提问

//------- 事件 ----
extern EVENT	EventObj;                           // 事件
extern EVENT	EventObj_8[8];                      // 事件

//------  信息点播  ---
extern MSG_BRODCAST MSG_BroadCast_Obj;              // 信息点播
extern MSG_BRODCAST MSG_Obj_8[8];                   // 信息点播

//------  电话本  -----
extern PHONE_BOOK		PhoneBook, Rx_PhoneBOOK;    //  电话本
extern PHONE_BOOK		PhoneBook_8[8];

extern MULTIMEDIA		MediaObj;                   // 多媒体信息

extern uint8_t			CarLoadState_Flag;          //选中车辆状态的标志   1:空车   2:半空   3:重车
extern uint8_t			Warn_Status[4];

extern u16				ISP_total_packnum;          // ISP  总包数
extern u16				ISP_current_packnum;        // ISP  当前包数

extern u8				APN_String[30];

extern u8				Camera_Number;
extern u8				Duomeiti_sdFlag;

extern Avrg_MintSpeed	Avrgspd_Mint;
extern u8				avgspd_Mint_Wr; // 填写每分钟平均速度记录下标

/*for new use*/

typedef struct
{
	int		id;
	short	attr;
	int		latitute;   /*以度位单位的纬度值乘以10的6次方，精确到百万分之一度*/
	int		longitute;
	int		radius;     /*单位为米m，路段为该拐点到下一拐点*/
	char	start[6];
	char	end[6];
	short	speed;
	char	interval;   /*持续时间,秒*/
}GPS_AREA_CIRCLE;

typedef enum
{
	T_NODEF = 1,
	T_BYTE,
	T_WORD,
	T_DWORD,
	T_STRING,
}PARAM_TYPE;

/*终端参数类型*/
typedef  struct
{
	uint8_t		id;
	PARAM_TYPE	type;
	void		* pvalue;
}PARAM;

/*终端参数类型*/
typedef __packed struct
{
	PARAM_TYPE	type;
	void		* pvalue;
}PARAM_BODY;

#if 0
typedef struct
{
	uint32_t ver;    /*版本信息四个字节yy_mm_dd_build,比较大小*/
/*车辆注册信息*/
	uint16_t	id0100_1_w;
	uint16_t	id0100_2_w;
	uint8_t		id0100_3_s[5];
	uint8_t		id0100_4_s[8];
	uint8_t		id0100_5_s[7];
	uint8_t		id0100_6_b;
	uint8_t		id0100_7_s[12];

/*网络有关*/
	char	apn[32];
	char	user[32];
	char	psw[32];
	char	mobile[6];
/*传输相关*/
	uint32_t	timeout_udp;    /*udp传输超时时间*/
	uint32_t	retry_udp;      /*udp传输重传次数*/
	uint32_t	timeout_tcp;    /*udp传输超时时间*/
	uint32_t	retry_tcp;      /*udp传输重传次数*/
}JT808_PARAM;

#endif


typedef struct _jt808_param
	{
		uint32_t	id_0x0000;		/*0x0000 版本*/
		uint32_t	id_0x0001;		/*0x0001 心跳发送间隔*/
		uint32_t	id_0x0002;		/*0x0002 TCP应答超时时间*/
		uint32_t	id_0x0003;		/*0x0003 TCP超时重传次数*/
		uint32_t	id_0x0004;		/*0x0004 UDP应答超时时间*/
		uint32_t	id_0x0005;		/*0x0005 UDP超时重传次数*/
		uint32_t	id_0x0006;		/*0x0006 SMS消息应答超时时间*/
		uint32_t	id_0x0007;		/*0x0007 SMS消息重传次数*/
		char		id_0x0010[32];	/*0x0010 主服务器APN*/
		char		id_0x0011[32];	/*0x0011 用户名*/
		char		id_0x0012[32];	/*0x0012 密码*/
		char		id_0x0013[32];	/*0x0013 主服务器地址*/
		char		id_0x0014[32];	/*0x0014 备份APN*/
		char		id_0x0015[32];	/*0x0015 备份用户名*/
		char		id_0x0016[32];	/*0x0016 备份密码*/
		char		id_0x0017[32];	/*0x0017 备份服务器地址，ip或域名*/
		uint32_t	id_0x0018;		/*0x0018 TCP端口*/
		uint32_t	id_0x0019;		/*0x0019 UDP端口*/
		uint32_t	id_0x0020;		/*0x0020 位置汇报策略*/
		uint32_t	id_0x0021;		/*0x0021 位置汇报方案*/
		uint32_t	id_0x0022;		/*0x0022 驾驶员未登录汇报时间间隔*/
		uint32_t	id_0x0027;		/*0x0027 休眠时汇报时间间隔*/
		uint32_t	id_0x0028;		/*0x0028 紧急报警时汇报时间间隔*/
		uint32_t	id_0x0029;		/*0x0029 缺省时间汇报间隔*/
		uint32_t	id_0x002C;		/*0x002c 缺省距离汇报间隔*/
		uint32_t	id_0x002D;		/*0x002d 驾驶员未登录汇报距离间隔*/
		uint32_t	id_0x002E;		/*0x002e 休眠时距离汇报间隔*/
		uint32_t	id_0x002F;		/*0x002f 紧急时距离汇报间隔*/
		uint32_t	id_0x0030;		/*0x0030 拐点补传角度*/
		char		id_0x0040[16];	/*0x0040 监控平台电话号码*/
		char		id_0x0041[16];	/*0x0041 复位电话号码*/
		char		id_0x0042[16];	/*0x0042 恢复出厂设置电话号码*/
		char		id_0x0043[16];	/*0x0043 监控平台SMS号码*/
		char		id_0x0044[16];	/*0x0044 接收终端SMS文本报警号码*/
		uint32_t	id_0x0045;		/*0x0045 终端接听电话策略*/
		uint32_t	id_0x0046;		/*0x0046 每次通话时长*/
		uint32_t	id_0x0047;		/*0x0047 当月通话时长*/
		char		id_0x0048[16];	/*0x0048 监听电话号码*/
		char		id_0x0049[16];	/*0x0049 监管平台特权短信号码*/
		uint32_t	id_0x0050;		/*0x0050 报警屏蔽字*/
		uint32_t	id_0x0051;		/*0x0051 报警发送文本SMS开关*/
		uint32_t	id_0x0052;		/*0x0052 报警拍照开关*/
		uint32_t	id_0x0053;		/*0x0053 报警拍摄存储标志*/
		uint32_t	id_0x0054;		/*0x0054 关键标志*/
		uint32_t	id_0x0055;		/*0x0055 最高速度kmh*/
		uint32_t	id_0x0056;		/*0x0056 超速持续时间*/
		uint32_t	id_0x0057;		/*0x0057 连续驾驶时间门限*/
		uint32_t	id_0x0058;		/*0x0058 当天累计驾驶时间门限*/
		uint32_t	id_0x0059;		/*0x0059 最小休息时间*/
		uint32_t	id_0x005A;		/*0x005A 最长停车时间*/
		uint32_t	id_0x0070;		/*0x0070 图像视频质量(1-10)*/
		uint32_t	id_0x0071;		/*0x0071 亮度*/
		uint32_t	id_0x0072;		/*0x0072 对比度*/
		uint32_t	id_0x0073;		/*0x0073 饱和度*/
		uint32_t	id_0x0074;		/*0x0074 色度*/
		uint32_t	id_0x0080;		/*0x0080 车辆里程表读数0.1km*/
		uint32_t	id_0x0081;		/*0x0081 省域ID*/
		uint32_t	id_0x0082;		/*0x0082 市域ID*/
		char		id_0x0083[16];	/*0x0083 机动车号牌*/
		uint32_t	id_0x0084;		/*0x0084 车牌颜色  1蓝色 2黄色 3黑色 4白色 9其他*/
}JT808_PARAM;


typedef struct
{
	char mobile[6];		/*终端号码*/
	uint8_t producer_id[5];
	uint8_t model[20];
	uint8_t terminal_id[7];
}TERM_PARAM;



typedef enum
{
	IDLE = 1,                   /*空闲等待发送*/
	WAIT_ACK,                   /*等待ACK中*/
	ACK_OK,                     /*已收到ACK应答*/
} JT808_MSG_STATE;

typedef enum
{
	TERMINAL_CMD = 1,
	TERMINAL_ACK,
	CENTER_CMD,
	CENTER_ACK
}JT808_MSG_TYPE;

typedef __packed struct
{
	uint16_t	id;
	uint16_t	attr;
	uint8_t		mobile[6];
	uint16_t	seq;
}JT808_MSG_HEAD;

typedef __packed struct
{
	uint32_t	tick;           /*收到的时刻,多包接收的超时判断*/
	uint8_t		linkno;         /*使用的linkno*/
	uint16_t	id;             /*消息ID*/
	uint16_t	attr;           /*消息体属性*/
	uint8_t		mobileno[6];    /*终端手机号*/
	uint16_t	seq;            /*消息流水号*/
	uint16_t	packetcount;    /*多包的总包数，如果有*/
	uint16_t	packetno;       /*当前包序号，从1开始，如果有*/
	uint16_t	msg_len;        /*消息长度*/
	uint8_t		*pmsg;          /*收到消息体*/
}JT808_RX_MSG_NODEDATA;

typedef __packed struct _jt808_tx_msg_nodedata
{
/*发送机制相关*/
//	uint8_t			linkno;     /*传输使用的link,包括了协议和远端socket*/
	JT808_MSG_TYPE	type;
	JT808_MSG_STATE state;      /*发送状态*/
	uint32_t		retry;      /*重传次数,递增，递减找不到*/
	uint32_t		max_retry;  /*最大重传次数*/
	uint32_t		timeout;    /*超时时间*/
	uint32_t		tick;       /*发送时间*/
/*接收的处理判断相关*/
	void ( *cb_tx_timeout )( struct _jt808_tx_msg_nodedata *pnodedata );
	void ( *cb_tx_response )( JT808_RX_MSG_NODEDATA* pnodedata );
	uint16_t	head_id;        /*消息ID*/
	uint16_t	head_sn;        /*消息流水号*/
/*真实的发送数据*/
	uint16_t	msg_len;        /*消息长度*/
	uint8_t		*pmsg;          /*发送消息体,真实的要发送的数据格式，经过转义和FCS后的<7e>为标志*/
}JT808_TX_MSG_NODEDATA;



typedef enum
{
	SOCKET_IDLE=1,	/*无需启动*/
//	SOCKET_INIT,
	SOCKET_DNS,	/*DNS查询中*/
	SOCKET_DNS_ERR,
	SOCKET_CONNECT, /*连接中*/
	SOCKET_CONNECT_ERR, /*连接错误，对方不应答*/
	SOCKET_READY,	/*已完成，可以建立链接*/
}T_SOCKET_STATE;


/*最大支持4个链接*/
#define MAX_GSM_SOCKET 4

typedef struct 
{
	T_SOCKET_STATE	state; 		     /*连接状态*/
	char			type;			/*连接类型 'u':udp client 't':TCP client  'U' udp server*/
	char*			apn;
	char*			user;
	char*			psw;
	char*			ip_domain;    /*域名*/
	char			ip_str[16];     /*dns后的IP xxx.xxx.xxx.xxx*/
	uint16_t		port;           /*端口*/
//	MsgList*		msglist_tx;
//	MsgList*		msglist_rx;
}GSM_SOCKET;



extern JT808_PARAM jt808_param;

extern GSM_SOCKET curr_gsm_socket;

void gps_rx( uint8_t *pinfo, uint16_t length );
rt_err_t gprs_rx( uint8_t linkno, uint8_t *pinfo, uint16_t length );


#endif

/************************************** The End Of File **************************************/
