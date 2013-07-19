#ifndef _GPS_H_
#define _GPS_H_


#define CTL_GPS_BAUD	0x30
#define CTL_GPS_OUTMODE	0x31	/*gps��Ϣ�����ģʽ*/


#define GPS_OUTMODE_TRIGGER	0x01	/*�봥�����*/
#define GPS_OUTMODE_GPRMC	0x02
#define GPS_OUTMODE_GPGSV	0x04
#define GPS_OUTMODE_BD		0x08

#define GPS_OUTMODE_ALL		0xFFFFFFFF	/*ȫ�������*/


/*
�������µĲ���״̬����룬��Ϊ��������

0000 0000-0FFF FFFF  �����


*/


#define BDUPG_RES_UART_OK		(0x10000000)	/*û�д��������ɹ�-�����ɹ�*/
#define BDUPG_RES_UART_READY	(0x1000FFFF)	/*���ڸ��¾���*/


#define BDUPG_RES_USB_OK		(0x20000000)	/*û�д��������ɹ�-�����ɹ�*/

#define BDUPG_RES_USB_FILE_ERR	(0x2000FFFC)	/*�����ļ���ʽ����*/
#define BDUPG_RES_USB_NOFILE	(0x2000FFFD)	/*�����ļ�������*/
#define BDUPG_RES_USB_WAITUSB	(0x2000FFFD)	/*�����ļ�������*/
#define BDUPG_RES_USB_NOEXIST	(0x2000FFFE)	/*u�̲�����*/
#define BDUPG_RES_USB_READY		(0x2000FFFF)	/*u�̾���*/


#define BDUPG_RES_USB_MODULE_H	(0x20010000)	/*ģ���ͺŸ�16bit*/
#define BDUPG_RES_USB_MODULE_L	(0x20020000)	/*ģ���ͺŵ�16bit*/
#define BDUPG_RES_USB_FILE_VER	(0x20030000)	/*����汾*/


#define BDUPG_RES_THREAD	(0xFFFFFFFE)	/*���������߳�ʧ��-��������ʧ��*/
#define BDUPG_RES_RAM		(0xFFFFFFFD)	/*����RAMʧ��*/
#define BDUPG_RES_TIMEOUT	(0xFFFFFFFC)	/*��ʱʧ��*/








void thread_gps_upgrade_uart( void* parameter );
void thread_gps_upgrade_udisk( void* parameter );

rt_size_t gps_mode( uint8_t mode );

#endif

