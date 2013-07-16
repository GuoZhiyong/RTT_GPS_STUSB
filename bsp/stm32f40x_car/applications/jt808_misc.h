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
#ifndef _H_JT808_MISC_
#define _H_JT808_MISC_

#include "stm32f4xx.h"
#include "jt808.h"

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

extern TEXT_INFO	TextInfo;
//-------文本信息-------
extern MSG_TEXT		TEXT_Obj;
extern MSG_TEXT		TEXT_Obj_8[8], TEXT_Obj_8bak[8];

//------ 提问  --------
extern CENTRE_ASK ASK_Centre;                       // 中心提问

//------- 事件 ----
//extern EVENT	EventObj;                           // 事件
//extern EVENT	EventObj_8[8];                      // 事件

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

typedef __packed struct
{
	uint32_t	id;                     /*单增序号*/
	MYTIME		datetime;               /*收到的时间*/
	uint8_t		flag;					/*已读，未读*/
	uint8_t		len;                    /*长度，人为截短到最大256-9*/
	uint8_t		body[256 - 10];          /*截短后收到的信息*/
}TEXTMSG;



typedef __packed struct
{
	uint32_t	id;                     /*单增序号*/
	MYTIME		datetime;               /*收到的时间*/
	uint8_t     flag;					/*已回答，未回答*/
	uint8_t		len;                    /*长度，人为截短到最大256-9*/
	uint8_t		body[256 - 10];          /*截短后收到的信息*/
}CENTER_ASK;




/*
   事件:设置在4k的buffer中
 */
typedef __packed struct
{
	uint8_t flag;                       /*标志*/
	uint8_t id;                         /*事件ID*/
	uint8_t len;                        /*事件长度*/
	uint8_t body[64-3];                   /*事件内容*/
}EVENT;





extern uint8_t textmsg_count;
extern uint8_t center_ask_count;

void jt808_misc_0x8300( uint8_t *pmsg );
void jt808_misc_0x8301( uint8_t *pmsg );
void jt808_misc_0x8302( uint8_t *pmsg );
void jt808_misc_0x8303( uint8_t *pmsg );
void jt808_misc_0x8304( uint8_t *pmsg );
void jt808_misc_0x8400( uint8_t *pmsg );
void jt808_misc_0x8401( uint8_t *pmsg );
void jt808_misc_0x8500( uint8_t *pmsg );


void jt808_textmsg_get( uint8_t index, TEXTMSG* pout );

uint8_t jt808_event_get(void);

extern uint8_t* event_buf;




void jt808_misc_init( void );


#endif
/************************************** The End Of File **************************************/

