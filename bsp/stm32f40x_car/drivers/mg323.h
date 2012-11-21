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

typedef struct
{




}T_SOCKET;

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
	GSM_AT_CMD=1,		//����AT����
	GSM_PPP,			//PPP����ά��
	GSM_SOCKET,			//����socket
	GSM_DNS,			//����DNS����
	GSM_TXRX_COUNT,		//���ͽ��յ��ֽ���
}GSM_CONTROL_CMD;






#endif
