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


#define TEST_GSM

#define GSM_UART_NAME	"uart4"

#define MAX_SOCKETS	6	//EM310����3  MG323����6

/*
����ʹ�õ�ģ���ͺ�
*/
//#define MG323
#define M66
//#define MC323
//#define EM310




typedef struct 
{
	char	*cmd;		//����
	char	*resp;		//��Ӧ
	char	isok;		//�Ƿ񷵻�OK	
	int		timeout;	//�ȴ���ʱʱ��
}T_GSM_AT_CMD;



/*
�������

	MG323��Profile

*/

typedef struct 
{
	char	*apn;		//apn
	char	*user;		//�û���
	char	*password;	//����
}T_GSM_APN;


typedef struct 
{
	char	active;			/*�Ƿ񼤻�*/
	char	status;			/*��ǰ״̬*/
	char	op;				//����
	char	type;			//scoket���� UDPorTCP
	char	*ip;			//ip��ַ
	unsigned short	*port;	//�˿�
}T_GSM_SOCKET;



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
	GSM_IDLE=0,			//����
	GSM_POWERON,		//�ϵ���̲����ģ���AT�����ʼ������
	GSM_POWEROFF,		//�Ҷ����ӣ��ϵ������
	GSM_AT,				//����AT�����շ�״̬,����socket�������շ�����
}T_GSM_STATE;


typedef enum
{
	SOCKET_IDLE=0,	/*��������*/
	SOCKET_TCPIP_INIT,	/*tcpip��ʼ��*/
	SOCKET_READY,	/*����ɣ����Խ�������*/
}T_SOCKET_STATE;

extern void gsm_init(void);
extern int gsm_send(uint8_t *pinfo,uint16_t len);


#endif
