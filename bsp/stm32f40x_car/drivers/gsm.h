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


#define TEST_GSM

#define GSM_UART_NAME	"uart2"

#define MAX_SOCKETS	3	//EM310定义

/*
定义使用的模块型号
*/
#define MG323

//#define MC323
//#define EM310

/*
定义GSM设备的操作函数，主要用来通知
分为三类
ondata 收到来自网络侧的信息
oncmd  发送命令的应答
onstatus 主动状态上报，状态切换，或urc出现
*/
typedef struct
{
	int	(*ondata)(uint8_t *pInfo,uint16_t len);
	int	(*oncmd)(uint8_t *pInfo,uint16_t len);
	int (*onstatus)(uint32_t *urc);
}T_GSM_OPS;





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

控制接口应不应该把功能分的足够详细
如何实现一键拨号:一个命令/函数控制建立连接
*/
typedef enum
{
	CTL_STATUS=1,		//查询GSM状态
	CTL_AT_CMD, 		//发送AT命令
	CTL_PPP,			//PPP链接维护
	CTL_SOCKET, 		//建立socket
	CTL_DNS,			//进行DNS解析
	CTL_TXRX_COUNT, 	//发送接收的字节数
	
}T_GSM_CONTROL_CMD;




typedef enum
{
	GSM_IDLE=0,			//空闲
	GSM_POWERON,		//上电过程中
	GSM_POWEROFF,		//断电过程中
	GSM_AT_INIT,		//模块的AT命令初始化过程中
	GSM_AT,				//处于AT命令收发状态
	GSM_PPP,			//处于PPP连接状态
	GSM_DATA,			//处于数据状态
}T_GSM_STATE;



#endif
