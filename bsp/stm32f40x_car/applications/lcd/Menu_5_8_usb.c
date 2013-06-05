#include "Menu_Include.h"
#include "stm32f4xx.h"
#include <stdio.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <dfs_posix.h>
#include "App_moduleConfig.h"

unsigned char in_out=1;   //DataOutInFlag=1ʱ    1:���ݵ���    2:�̼�����
unsigned char DataOutInFlag=0;//  1:���뵼������  2:�����Ӳ˵�
unsigned char DynamicDisFlag=0;
unsigned char ZiKu_jindu[10]={"   0/   0"};
unsigned char USB_data_flag=0;//  0�������������ݣ�������ɺ�Ϊ1���Ƴ�Ϊ0
u8 TarName[64];

static rt_uint8_t Exit_updata=0;
static rt_uint8_t check_error=0;
static rt_uint8_t data_fetch_comp=0;

static rt_uint8_t	*ptr_write_packet = RT_NULL;
static rt_thread_t  tid_write_file = RT_NULL;

static rt_uint8_t   *ptr_read_packet=RT_NULL;
static rt_thread_t  tid_file_update=RT_NULL;
static rt_uint8_t   fetch_data_514[514];
static rt_uint8_t   read_data_514[514];
static rt_uint16_t  Updata_PacketNum=0;


/*���±����̣߳����յ��Դ����������ݣ�͸����gps������*/
void thread_usb_update( void* parameter )
{
rt_uint16_t i=0;

#define READ_PACKET_SIZE 1012

	void		( *msg )( void *p );
	int			fd = -1, count = 0;
	//rt_uint8_t	*pdata;             /*����*/
	rt_err_t	res;
	rt_uint8_t  file_infor[20];
	rt_uint32_t file_datalen;       /*�����ļ�����*/
	rt_uint16_t page,j;

	msg =(void (*)(void*)) parameter;

	ptr_read_packet= rt_malloc( 520 );
	if( ptr_read_packet == RT_NULL )
	{
	Exit_updata=0;
	check_error=1;
		msg( "E�ڴ治��" );
		return;
	}
  //   1.  Lock DF and  Erase  File  Area
           DF_LOCK=1;
	    rt_thread_delay(2);
	    DF_EraseAppFile_Area();

	
/*����U��*/
      //--------------------------------------------------------------------------------
	while( 1 )
	{
		if( rt_device_find( "udisk" ) == RT_NULL ) /*û���ҵ�*/
		{
			count++;
			if( count <= 10 )
			{
				msg( "I�ȴ�U�̲���" );
				
			}else
			{
				msg( "EU�̲�����" ); /*ָʾU�̲�����*/
				check_error=1;
				goto end_upgrade_usb_0;
			}
			rt_thread_delay( RT_TICK_PER_SECOND );
		}else
		{
			msg( "I���������ļ�" );
			break;
		}
	}
       
/*����ָ���ļ�*/
	fd = open( "/udisk/UPDATE.TCB", O_RDONLY, 0 );
	if( fd >= 0 )
		msg( "I��ȡ�ļ�����" );
	else
		{
		msg( "E�����ļ�������" );
		check_error=1;
		goto end_upgrade_usb_0;
		}
	file_datalen=0;//�ļ�����ͳ��
	Updata_PacketNum=0;//��������ָʾ 
	
      //-------------------------------------------------------------------------------
	while( 1 )
	{	
	    	rt_thread_delay(RT_TICK_PER_SECOND/2);
	res=0;
    res = read( fd, ptr_read_packet,514 );
	memcpy(fetch_data_514,ptr_read_packet,514);
	if(res<0)
		{
		msg("E��ȡ�ļ�����");
		check_error=1;
		goto end_upgrade_usb_1;
		}
	else if( res == 514 )               /*�ж��Ƿ�Ϊ���һ��*/
		{
		Updata_PacketNum++;
		page=(((u16)fetch_data_514[0])<<8)+((u16)fetch_data_514[1]);
		file_datalen+=res;
           rt_kprintf("\r\n��ȡ������page=%d,file_datalen=%d   ",page,file_datalen);
	    for(j=0;j<10;j++)
			rt_kprintf(" %2X",fetch_data_514[j]);
		rt_kprintf("\r\n");
		
		WatchDog_Feed();
		
		if(page==50)
			{
			  //-------- Version Type judge------
                     if(Update_HardSoft_Version_Judge(fetch_data_514+2)==false)   // +2 ������
                      {
                         msg( "E�̼����Ͳ�ƥ��" );
                        goto end_upgrade_usb_1;
			 }
			
			DF_WriteFlashDirect(50,0,&fetch_data_514[2],512);
			DF_ReadFlash(50,0,read_data_514,512);
			for(i=0;i<512;i++)
				{
				if(read_data_514[i]!=fetch_data_514[i+2])
					rt_kprintf("\r\n page=%d,i=%d,write=%X,read=%X",page,i,fetch_data_514[i+2],read_data_514[i]);
				}
			
			msg( "I�ļ��������" );
		    rt_kprintf("�ļ����� = %d",file_datalen);
			DF_LOCK=0;
			goto end_upgrade_usb_1;
			}
		else
			{
			if(page%8==0)
				DF_WriteFlashSector(page,0,&fetch_data_514[2],512);
			else
				DF_WriteFlashDirect(page,0,&fetch_data_514[2],512);
			rt_thread_delay(3);
	       //---------- �������ݱȽ��ж� -----------------------------------------------		
			DF_ReadFlash(page,0,read_data_514,512);
			for(i=0;i<512;i++)
				{
				if(read_data_514[i]!=fetch_data_514[i+2])
					{
					      rt_kprintf("\r\n page=%d,i=%d,write=%X,read=%X",page,i,fetch_data_514[i+2],read_data_514[i]);
                    msg( "Eд���ļ�����" );
					check_error=1;
					      goto end_upgrade_usb_1; 
				       } 
				}
		//--------------------------------------------------------------------------	
			sprintf((char *)file_infor,"I������%d��",Updata_PacketNum);
			msg(file_infor);
			}
		}
	else
		{
		msg( "EU�̶�ȡ����" );
		check_error=1;
		goto end_upgrade_usb_1;
		}	
	}
	//--------------------------------------------------------------------
end_upgrade_usb_1:
	if( fd >= 0 )
	{
		close( fd );
	}
end_upgrade_usb_0:
	Exit_updata=0;
	rt_free( ptr_read_packet);
	ptr_read_packet= RT_NULL; 
}

void thread_usbout_udisk( void* parameter )
{

	void		( *msg )( void *p );
	int			fd = -1, size;
	u8 write_da[300];//={"1234567890"};
	u8 write_data_record[50];
	u8 i=0;
	u16 len=0;

	msg = (void (*)(void*))parameter;

	ptr_write_packet = rt_malloc(100);
	if( ptr_write_packet == RT_NULL )
	{
		msg( "E�ڴ治��" );
		return;
	}
	ptr_write_packet=write_da;
/*����U��*/
	while( 1 )
	{
	if( rt_device_find( "udisk" ) == RT_NULL ) /*û���ҵ�*/
		{
		msg( "EU�̲�����" ); /*ָʾU�̲�����*/
		rt_thread_delay( RT_TICK_PER_SECOND );		
		USB_insertFlag=1;
		}
	else
		{
	    USB_insertFlag=0;
		
        if(!USB_data_flag)
        	{
		    if(DataOutStartFlag==1)
		        strcpy( (char *)TarName, "/udisk/ACCIDENT.TXT" );
			else if(DataOutStartFlag==2)
				strcpy( (char *)TarName, "/udisk/TIRED.TXT" );
			else if(DataOutStartFlag==3)
				strcpy( (char *)TarName, "/udisk/EXSPEED.TXT" );
			
		    if(DataOutStartFlag)
				fd = open((const char*)TarName,(O_CREAT|O_WRONLY|O_TRUNC), 0 );			
			if( fd >= 0 )
				{
				//msg( "I���ļ��ɹ�" );
				rt_kprintf("\r\n ���ļ��ɹ�");
				//������Ϣ��  VIN 17 +  ���ƺ�12  +  ���Ʒ��� 12 ��
				memcpy(write_da,JT808Conf_struct.Vechicle_Info.Vech_sim,12);
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
				
			    if(DataOutStartFlag==1)//�ɵ�
			    	{
			    	write_da[64]=1;
					Api_DFdirectory_Read(doubt_data,write_da+65,206,0,1);
					len=271;//65+206
					size = write( fd,write_da,len);
					rt_kprintf("\r\n �����ɵ����ݼ�¼sizeof= %d ",size); 
					data_fetch_comp=1;
					msg("I���ݵ������");
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
			    	}
				
				}
			else
				msg( "E���ļ�������" );

			//size = write( fd,"0123456789",10);
			//rt_kprintf("\r\n sizeof= %d ",size);
			close(fd);
			DataOutOK=1;   //�������
			USB_data_flag=1;
        	}
		}
	rt_thread_delay(RT_TICK_PER_SECOND/2);
	}
}
void DataInOutSe(unsigned char num)
{
	lcd_fill(0);
	lcd_text12(0,11,"USB",3,LCD_MODE_SET);
	if(num==1)
		{
		lcd_text12(25, 3,"1.���ݵ���",10,LCD_MODE_INVERT);
		lcd_text12(25,19,"2.�̼�����",10,LCD_MODE_SET);
		}
	else
		{
		lcd_text12(25, 3,"1.���ݵ���",10,LCD_MODE_SET);
		lcd_text12(25,19,"2.�̼�����",10,LCD_MODE_INVERT);
		}
	lcd_update_all();
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
		tid_file_update=RT_NULL;
		}
	lcd_update_all( );
}
static void show(void)
{
CounterBack=0;
    DataInOutSe(1);
	in_out=1;
	DataOutOK=0;
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
				USB_data_flag=0;
                if(Exit_updata==1)
                	{
                	Exit_updata=2;
					lcd_fill(0);
					lcd_text12(0,3,"�Ƿ��˳�U�̱��ظ���",19,LCD_MODE_SET);
					lcd_text12(24,18,"��ȷ�ϼ��˳�",12,LCD_MODE_SET);
					lcd_update_all();
                	}
				else if((check_error==1)||(data_fetch_comp==1))
					{
					check_error=0;
					data_fetch_comp=0;
					pMenuItem=&Menu_5_other;
					pMenuItem->show();

					if(tid_write_file)
						rt_thread_delete(tid_write_file);
					if(tid_file_update)
						rt_thread_delete(tid_file_update);
					}
				
                DataOutStartFlag=0;
				DataOutStartFlag=0;
				OUT_DataCounter=0;
				DataOutInFlag=0;
                in_out=1;
				break;
			case KeyValueOk:
				//�˳�����U������
				if(Exit_updata==2)
                	{
                	Exit_updata=0;
					pMenuItem=&Menu_5_other;
					pMenuItem->show();

					if(tid_write_file)
						rt_thread_delete(tid_write_file);
					if(tid_file_update)
						rt_thread_delete(tid_file_update);
                	}
				//��������
				if(DataOutInFlag==1)
					{
					if(in_out==1)
						{
						DataOutInFlag=2;//������������ѡ���־
						USB_OUTFileSe(OUT_DataCounter);
						}
					else if(in_out==2)
						{
						
						tid_file_update= rt_thread_create( "fileupdate", thread_usb_update, (void*)msg, 1024, 9, 5 );
						if( tid_file_update != RT_NULL )
							{
							Exit_updata=1;
							msg("I�ļ�����");							
							rt_thread_startup(tid_file_update);
							}
						else
							{
							Exit_updata=0;
							msg("E�����̴߳���ʧ��");
							}
						}
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
						msg("E�̴߳���ʧ��");
						}
					}
				break;
			case KeyValueUP:
				if(!Exit_updata)
					{
					if(DataOutInFlag==1)
						{
						in_out=1;
						DataInOutSe(1);
						}
					else if(DataOutInFlag==2)
						{
						OUT_DataCounter--;
						if(OUT_DataCounter<=1)
							OUT_DataCounter=1;
						USB_OUTFileSe(OUT_DataCounter);
						}
					}
				break;
			case KeyValueDown:
				if(!Exit_updata)
					{
					if(DataOutInFlag==1)
						{
						in_out=2;
						DataInOutSe(2);
						}
					else if(DataOutInFlag==2)
						{
						OUT_DataCounter++;
						if(OUT_DataCounter>=3)
							OUT_DataCounter=3;
						USB_OUTFileSe(OUT_DataCounter);
						}
					}
				break;
			}
	KeyValue=0;
	}
	

static void timetick(unsigned int systick)
{   
	CounterBack++;
	if(CounterBack!=MaxBankIdleTime*10)
		return;
	pMenuItem=&Menu_1_Idle;
	pMenuItem->show();
    CounterBack=0;

	DataOutStartFlag=0;
	DataOutStartFlag=0;
	OUT_DataCounter=0;
	DataOutInFlag=0;
	in_out=1;
}

ALIGN(RT_ALIGN_SIZE)
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

