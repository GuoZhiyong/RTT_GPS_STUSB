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


#define GSM_UART_NAME	"uart4"

//#define MAX_SOCKETS	6	//EM310定义3  MG323定义6

/*
定义使用的模块型号
*/
//#define MG323
#define M66
//#define MC323
//#define EM310



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
	CTL_CONNECT,		//直接建立连接

}T_GSM_CONTROL_CMD;




typedef enum
{
	GSM_IDLE=1,			//空闲
	GSM_POWERON,		//上电过程并完成模块的AT命令初始化过程
	GSM_POWERONING,
	GSM_POWEROFF,		//挂断链接，断电过程中
	GSM_AT,				//处于AT命令收发状态,设置socket参数，收发短信
}T_GSM_STATE;


typedef enum
{
	SOCKET_IDLE = 1,            /*无需启动*/
	SOCKET_INIT,				/*配置登网参数，登网*/
	SOCKET_START,				/*启动连接远程*/
	SOCKET_ETCPIP_ERROR,		/*进行ETCPIP错误*/
	SOCKET_DNS,                 /*DNS查询中*/
	SOCKET_DNS_ERR,
	SOCKET_CONNECT,             /*连接中*/
	SOCKET_CONNECT_ERR,         /*连接错误，对方不应答*/
	SOCKET_READY,               /*已完成，可以建立链接*/
	SOCKET_CLOSE,
}T_SOCKET_STATE;

/*最大支持4个链接*/
#define MAX_GSM_SOCKET 4

typedef struct
{
	T_SOCKET_STATE	state;          /*连接状态*/
	char			type;           /*连接类型 'u':udp client 't':TCP client  'U' udp server*/
	char			* apn;
	char			* user;
	char			* psw;
	char			* ipstr;    /*域名或地址*/
	char			ip_addr[16];     /*dns后的IP xxx.xxx.xxx.xxx*/
	uint16_t		port;           /*端口*/
	MsgList			* msglist_tx;
}GSM_SOCKET;


void gsm_init(void);
int gsm_send(uint8_t *pinfo,uint16_t len);


#endif
