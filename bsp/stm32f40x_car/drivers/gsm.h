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
	GSM_POWEROFF,		//�Ҷ����ӣ��ϵ������
	GSM_AT,				//����AT�����շ�״̬,����socket�������շ�����
}T_GSM_STATE;



void gsm_init(void);
int gsm_send(uint8_t *pinfo,uint16_t len);


#endif
