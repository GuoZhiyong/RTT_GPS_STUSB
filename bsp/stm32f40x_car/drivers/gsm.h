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


/*
����ʹ�õ�ģ���ͺ�
*/
#define MG323
//#define MC323
//#define EM310

#define GSM_UART_NAME	"uart2"

#define MAX_SOCKETS	3	//EM310����

typedef int (*ONDATA)(uint8_t *s,uint16_t len);
typedef int (*ONCMD)(uint8_t *s,uint16_t len);
typedef int (*ONSTATUS)(uint8_t *s,uint16_t len);

typedef struct
{
	uint8_t link_num;	//���Ӻ�
	uint8_t proto;		//Э������ TCP UDP
	char ip[20];		//peer ip �ַ���
	uint16_t port;		//peer port
	int	(*ondata)(uint8_t *s,uint16_t len);		//�յ����ݵĻص�����
}T_SOCKET;

T_SOCKET gsm_sockets[MAX_SOCKETS];


typedef struct 
{
	char	*cmd;		//����
	char	*resp;		//��Ӧ
	char	isok;		//�Ƿ񷵻�OK	
	int		timeout;	//�ȴ���ʱʱ��
}T_GSM_AT_CMD;


typedef struct 
{
	char	*apn;		//apn
	char	*resp;		//��Ӧ
	char	isok;		//�Ƿ񷵻�OK	
	int		timeout;	//�ȴ���ʱʱ��
}T_GSM_PPP;


typedef struct 
{
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
*/
typedef enum
{
	CMD_STATUS=1,		//��ѯGSM״̬
	CMD_AT_CMD,			//����AT����
	CMD_PPP,			//PPP����ά��
	CMD_SOCKET,			//����socket
	CMD_DNS,			//����DNS����
	CMD_TXRX_COUNT,		//���ͽ��յ��ֽ���
}T_GSM_CONTROL_CMD;


typedef enum
{
	GSM_IDLE=0,			//����
	GSM_POWERON,		//�ϵ������
	GSM_POWEROFF,		//�ϵ������
	GSM_AT,				//����AT�����շ�״̬
	GSM_PPP,			//����PPP����״̬
	GSM_DATA,			//��������״̬
}T_GSM_STATE;


#endif
