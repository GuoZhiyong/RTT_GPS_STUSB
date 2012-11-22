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

#define GSM_UART_NAME	"uart2"

#define MAX_SOCKETS	3	//EM310����

/*
����ʹ�õ�ģ���ͺ�
*/
#define MG323

//#define MC323
//#define EM310

/*
����GSM�豸�Ĳ�����������Ҫ����֪ͨ
��Ϊ����
ondata �յ�������������Ϣ
oncmd  ���������Ӧ��
onstatus ����״̬�ϱ���״̬�л�����urc����
*/
typedef struct
{
	int	(*ondata)(uint8_t *pInfo,uint16_t len);
	int	(*oncmd)(uint8_t *pInfo,uint16_t len);
	int (*onstatus)(uint32_t *urc);
}T_GSM_OPS;





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
	
}T_GSM_CONTROL_CMD;




typedef enum
{
	GSM_IDLE=0,			//����
	GSM_POWERON,		//�ϵ������
	GSM_POWEROFF,		//�ϵ������
	GSM_AT_INIT,		//ģ���AT�����ʼ��������
	GSM_AT,				//����AT�����շ�״̬
	GSM_PPP,			//����PPP����״̬
	GSM_DATA,			//��������״̬
}T_GSM_STATE;



#endif
