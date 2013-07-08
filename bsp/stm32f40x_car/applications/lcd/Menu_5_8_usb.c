#include "Menu_Include.h"
#include "stm32f4xx.h"
#include <stdio.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <dfs_posix.h>
#include "App_moduleConfig.h"

unsigned char DataOutInFlag=0;//  1:���뵼������  2:�����Ӳ˵�
unsigned char USB_data_flag=0;//  0�������������ݣ�������ɺ�Ϊ1���Ƴ�Ϊ0
u8 TarName[64];
static rt_uint8_t	*ptr_write_packet = RT_NULL;
static rt_thread_t  tid_write_file = RT_NULL;

static rt_uint8_t data_fetch_comp=0;
static rt_uint8_t usb_error=0;
void thread_usbout_udisk( void* parameter )
{

	void		( *msg )( void *p );
	int			fd = -1, size;
	u8 write_da[300];//={"1234567890"};
	u8 write_data_record[50];
	u8 i=0;
	u16 len=0;
	u32 count=0;
	

	//������Ϣ��  VIN 17 +  ���ƺ�12  +  ���Ʒ��� 12 ��
	memcpy(write_da,IMSI_CODE+3,12);
	memcpy(write_da+12,"00000",5);//17λvin
	memcpy(write_da+17,JT808Conf_struct.Vechicle_Info.Vech_Num,12);//���ƺ�12
	memcpy(write_da+29,JT808Conf_struct.Vechicle_Info.Vech_Type,12);//���Ʒ���12
	//��ʻԱ��Ϣ ��  ��ʻԱ���� 3 +��ʻ֤���� 18��
	memcpy(write_da+41,JT808Conf_struct.Driver_Info.DriveCode,3);//  ��ʻԱ����3
	memcpy(write_da+44,JT808Conf_struct.Driver_Info.DriverCard_ID,18);//��ʻ֤����18
	// ��������Ϣ  1
	write_da[62]=Vehicle_sensor;
	//��Ϣ����    ����2+����
	write_da[63]=0;

	msg = (void (*)(void*))parameter;

	ptr_write_packet = rt_malloc(256);
	if( ptr_write_packet == RT_NULL )
	{
		msg( "E�ڴ治��" );
		return;
	}
	//ptr_write_packet=write_da;
/*����U��*/
/*����U��*/
	while( 1 )
	{
	if( rt_device_find( "udisk" ) == RT_NULL ) /*û���ҵ�*/
		{
		count++;
		if( count <= 5 )
			{
			msg( "I�ȴ�U�̲���" );
			}
		else
			{
			msg( "EU�̲�����" ); /*ָʾU�̲�����*/
			usb_error=1;
			goto end_usbdata_0;
			}
		rt_thread_delay( RT_TICK_PER_SECOND );
		}
	else
		{
		usb_error=0;
		break;
		}
	WatchDog_Feed();
	}
	count=0;
	
	while( 1 )
		{
		count++;
		rt_thread_delay(RT_TICK_PER_SECOND/2);
		usb_error=0;		
		// if(!USB_data_flag)
		//{
		if(DataOutStartFlag==1)
			strcpy( (char *)TarName, "/udisk/ACCIDENT.TXT" );
		else if(DataOutStartFlag==2)
			strcpy( (char *)TarName, "/udisk/TIRED.TXT" );
		else if(DataOutStartFlag==3)
			strcpy( (char *)TarName, "/udisk/EXSPEED.TXT" );
		else
			break;
		WatchDog_Feed(); 	
		if(DataOutStartFlag)
			fd = open((const char*)TarName,(O_CREAT|O_WRONLY|O_TRUNC), 0 );			
		if( fd >= 0 )
			{
			//msg( "I���ļ��ɹ�" );
			rt_kprintf("\r\n ���ļ��ɹ�");

			if(DataOutStartFlag==1)//�ɵ�
				{
				write_da[64]=1;
				Api_DFdirectory_Read(doubt_data,write_da+65,206,0,1);
				len=271;//65+206
				size = write( fd,write_da,len);
				rt_kprintf("\r\n �����ɵ����ݼ�¼sizeof= %d ",size);
				data_fetch_comp=1;
				msg("I���ݵ������");
				//goto end_usbdata_0;
				}
			else if(DataOutStartFlag==2)//ƣ��
				{
				write_da[64]=TiredDrv_write;
				if(TiredDrv_write>=1)
					{
					Api_DFdirectory_Read(tired_warn,write_data_record,30,0,TiredDrv_write);
					for(i=0;i<TiredDrv_write;i++)
						memcpy(write_da+65+i*12,write_data_record+18+i*30,12);
					//Api_DFdirectory_Read(tired_warn,write_da+65,30,0,TiredDrv_write);
					}
				len=65+TiredDrv_write*12;
				size = write( fd,write_da,len);
				rt_kprintf("\r\n ����ƣ�ͼ�¼sizeof= %d ",size);
				data_fetch_comp=1;
				msg("I���ݵ������");
				//goto end_usbdata_0;
				}
			else if(DataOutStartFlag==3)//����
				{
				write_da[64]=ExpSpdRec_write;
				if(ExpSpdRec_write>=1)
					{
					Api_DFdirectory_Read(spd_warn,write_data_record,31,0,ExpSpdRec_write);
					for(i=0;i<ExpSpdRec_write;i++)
						memcpy(write_da+65+i*13,write_data_record+18+i*31,12);
					//Api_DFdirectory_Read(spd_warn,write_da+65,31,0,TiredDrv_write);
					}
				len=65+ExpSpdRec_write*13;
				size = write( fd,write_da,len);
				rt_kprintf("\r\n �������ټ�¼sizeof= %d ",size);
				data_fetch_comp=1;
				msg("I���ݵ������");
				//goto end_usbdata_0;
				}
			close(fd);
			goto end_usbdata_0;
			}
		else
			{
			if(count>=3)
				{
				usb_error=1;
				msg("EU�̴����ļ�ʧ��");
				rt_kprintf( "\r\n�����ļ�ʧ��" );
				break;
				}
			}
		}
	end_usbdata_0:
	WatchDog_Feed();
	 rt_free( ptr_write_packet );
	 ptr_write_packet = RT_NULL;
}


void USB_OUTFileSe(unsigned char OutType)
{
	lcd_fill(0);
	if(OutType==1)
		lcd_text12(0,3,"1.ָ�����¹��ɵ��¼",20,LCD_MODE_SET);
	else if(OutType==2)
		lcd_text12(0,3,"2.ָ����ƣ�ͼ�ʻ��¼",20,LCD_MODE_SET);
	else if(OutType==3)
	    lcd_text12(0,3,"3.ָ���ĳ��ټ�ʻ��¼",20,LCD_MODE_SET);
	lcd_update_all();
}



static void msg( void *p)
{
	unsigned int len;
	char *pinfo;
	lcd_fill( 0 );
	//lcd_text12( 0, 3, "����", 4, LCD_MODE_SET );
	//lcd_text12( 0, 17, "����", 4, LCD_MODE_SET );
	pinfo=(char *)p;
	len=strlen(pinfo);
	lcd_text12( 20, 10,pinfo+1,len-1, LCD_MODE_SET );
	if(*pinfo=='E')	/*��������*/
		{
		tid_write_file=RT_NULL;
		}
	lcd_update_all( );
}
static void show(void)
{
	CounterBack=0;
	lcd_fill(0);
	lcd_text12(6,11,"ѡ�񵼳����ļ�����",18,LCD_MODE_SET);
	lcd_update_all();
	OUT_DataCounter=1;
	DataOutInFlag=1;

}

static void keypress(unsigned int key)
{ 
	switch(KeyValue)
		{
		case KeyValueMenu:
			DF_LOCK=0;
			CounterBack=0;
			//USB_data_flag=0;
			if((data_fetch_comp==1)||(usb_error==1))
				{
				usb_error=0;
				data_fetch_comp=0;
				pMenuItem=&Menu_5_other;
				pMenuItem->show();		

				//if(tid_write_file)						///���ԻḴλ
				//if(tid_write_file != RT_NULL)			///����OK
				/* �����������������󣬽������л��������̣߳�����Ӧ�ж�*/
				rt_enter_critical();
				if (tid_write_file != RT_NULL && tid_write_file->stat != RT_THREAD_CLOSE)	///����OK
					{
					rt_thread_delete(tid_write_file);
					//WatchDog_Feed();
					}
				/* ����������*/
				rt_exit_critical();
				}
			
		       DataOutStartFlag=0;
			DataOutStartFlag=0;
			OUT_DataCounter=0;
			DataOutInFlag=0;
			break;
		case KeyValueOk:
			//��������
			if(DataOutInFlag==1)
				{
				DataOutInFlag=2;//������������ѡ���־
				USB_OUTFileSe(OUT_DataCounter);
				}
			//�ļ�����
			else if(DataOutInFlag==2)
				{
				DataOutInFlag=0;

				if((OUT_DataCounter>=1)&&(OUT_DataCounter<=3))
					DataOutStartFlag=OUT_DataCounter;//��timetick����ʾ��������

				tid_write_file= rt_thread_create( "writefile", thread_usbout_udisk, (void*)msg, 1024, 8, 5 );
				if( tid_write_file != RT_NULL )
					{
					msg("I�����洢������");
					rt_thread_startup( tid_write_file);
					}
				else
					{
					msg("E���󰴲˵���");
					usb_error=1;
					}
				}
			break;
		case KeyValueUP:
				if(DataOutInFlag==2)
					{
					if(OUT_DataCounter<=1)
						OUT_DataCounter=3;
					else
						OUT_DataCounter--;
					USB_OUTFileSe(OUT_DataCounter);
					}
			break;
		case KeyValueDown:
				if(DataOutInFlag==2)
					{
					if(OUT_DataCounter>=3)
						OUT_DataCounter=1;
					else
						OUT_DataCounter++;
					USB_OUTFileSe(OUT_DataCounter);
					}
			break;
		}
KeyValue=0;
}
	

static void timetick(unsigned int systick)
{   
	CounterBack++;
	if(CounterBack!=MaxBankIdleTime*5)
		return;
	pMenuItem=&Menu_1_Idle;
	pMenuItem->show();
    CounterBack=0;

	DataOutStartFlag=0;
	DataOutStartFlag=0;
	OUT_DataCounter=0;
	DataOutInFlag=0;
}

MYTIME
MENUITEM	Menu_5_8_Usb=
{
	"USB����",
	7,
	&show,
	&keypress,
	&timetick,
	&msg,
	(void*)0
};

