#ifndef _GSM_H_
#define _GSM_H_

/*
1.��gsm��װ��һ���豸���ڲ�gsm״̬����һ���̴߳���
  Ӧ�ó���ֻ��Ҫ���豸�����в������ɡ�

2.ͨ��AT�����ȡģ������? ��������ģ��Ͳ���Ҫˢ��
  ����Ҳ���ڱȽ�ģ�顣�ж��ʵ������?

3.ģ���״̬����Ϣ�����������·���Ϣ�����պ���/����
  ���߿���·��socket�Ͽ��� (��ͬ��ģ�鲻ͬ,Ҫ�����
  ��)�����֪ͨAPP? ʹ�ûص�������

4.��Ч��socket����
*/


#define GSM_UART_NAME	"uart4"

//#define MAX_SOCKETS	6	//EM310����3  MG323����6

/*
����ʹ�õ�ģ���ͺ�
*/
//#define MG323
#define M66
//#define MC323
//#define EM310



/*
GSM֧�ֲ������ܵ��б�
��ͬ��ģ��֧�ֵ����ͬ������¼������ TTS����
�˴�����ͳһ��һ��gsm.h,��Ӧ����ģ��Ĳ�ͬ����ͬ��
���ϲ������ο���?

���ƽӿ�Ӧ��Ӧ�ðѹ��ֵܷ��㹻��ϸ
���ʵ��һ������:һ������/�������ƽ�������
*/
typedef enum
{
	CTL_STATUS=1,		//��ѯGSM״̬
	CTL_AT_CMD, 		//����AT����
	CTL_PPP,			//PPP����ά��
	CTL_SOCKET, 		//����socket
	CTL_DNS,			//����DNS����
	CTL_TXRX_COUNT, 	//���ͽ��յ��ֽ���
	CTL_CONNECT,		//ֱ�ӽ�������

}T_GSM_CONTROL_CMD;




typedef enum
{
	GSM_IDLE=1,			//����
	GSM_POWERON,		//�ϵ���̲����ģ���AT�����ʼ������
	GSM_POWERONING,
	GSM_POWEROFF,		//�Ҷ����ӣ��ϵ������
	GSM_AT,				//����AT�����շ�״̬,����socket�������շ�����
}T_GSM_STATE;


typedef enum
{
	SOCKET_IDLE = 1,            /*��������*/
	SOCKET_INIT,				/*���õ�������������*/
	SOCKET_START,				/*��������Զ��*/
	SOCKET_ETCPIP_ERROR,		/*����ETCPIP����*/
	SOCKET_DNS,                 /*DNS��ѯ��*/
	SOCKET_DNS_ERR,
	SOCKET_CONNECT,             /*������*/
	SOCKET_CONNECT_ERR,         /*���Ӵ��󣬶Է���Ӧ��*/
	SOCKET_READY,               /*����ɣ����Խ�������*/
	SOCKET_CLOSE,
}T_SOCKET_STATE;

/*���֧��4������*/
#define MAX_GSM_SOCKET 4

typedef struct
{
	T_SOCKET_STATE	state;          /*����״̬*/
	char			type;           /*�������� 'u':udp client 't':TCP client  'U' udp server*/
	char			* apn;
	char			* user;
	char			* psw;
	char			* ipstr;    /*�������ַ*/
	char			ip_addr[16];     /*dns���IP xxx.xxx.xxx.xxx*/
	uint16_t		port;           /*�˿�*/
	MsgList			* msglist_tx;
}GSM_SOCKET;


void gsm_init(void);
int gsm_send(uint8_t *pinfo,uint16_t len);


#endif
