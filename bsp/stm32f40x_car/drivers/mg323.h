#ifndef _GSM_H_
#define _GSM_H_

/*
1.将gsm封装成一个设备，内部gsm状态机由一个线程处理
  应用程序只需要打开设备，进行操作即可。

2.通过AT命令读取模块类型? 这样更换模块就不需要刷新
  程序，也便于比较模块。有多大实际意义?

3.模块的状态或信息，比如中心下发信息、接收呼叫/短信
  天线开短路、socket断开等 (不同的模块不同,要抽象出
  来)，如何通知APP? 使用回调函数。

4.有效的socket管理
*/


/*
定义使用的模块型号
*/
#define MG323
//#define MC323
//#define EM310

typedef struct
{




}T_SOCKET;

typedef struct 
{
	char	*cmd;		//命令
	char	*resp;		//响应
	char	isok;		//是否返回OK	
	int		timeout;	//等待超时时间
}T_GSM_AT_CMD;


typedef struct 
{
	char	*apn;		//apn
	char	*resp;		//响应
	char	isok;		//是否返回OK	
	int		timeout;	//等待超时时间
}T_GSM_PPP;


typedef struct 
{
	char	op;				//操作
	char	type;			//scoket类型 UDPorTCP
	char	*ip;			//ip地址
	unsigned short	*port;	//端口
}T_GSM_SOCKET;



/*
GSM支持操作功能的列表
不同的模块支持的命令不同，比如录音命令 TTS命令
此处不能统一成一个gsm.h,而应依据模块的不同而不同。
但上层软件如何控制?
*/
typedef enum
{
	GSM_AT_CMD=1,		//发送AT命令
	GSM_PPP,			//PPP链接维护
	GSM_SOCKET,			//建立socket
	GSM_DNS,			//进行DNS解析
	GSM_TXRX_COUNT,		//发送接收的字节数
}GSM_CONTROL_CMD;






#endif
