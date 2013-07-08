#include "Menu_Include.h"
#include "stm32f4xx.h"
#include <stdio.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <dfs_posix.h>
#include "App_moduleConfig.h"

unsigned char DataOutInFlag=0;//  1:导入导出界面  2:导出子菜单
unsigned char USB_data_flag=0;//  0创建并导出数据，导出完成后为1，推出为0
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
	

	//车辆信息（  VIN 17 +  车牌号12  +  车牌分类 12 ）
	memcpy(write_da,IMSI_CODE+3,12);
	memcpy(write_da+12,"00000",5);//17位vin
	memcpy(write_da+17,JT808Conf_struct.Vechicle_Info.Vech_Num,12);//车牌号12
	memcpy(write_da+29,JT808Conf_struct.Vechicle_Info.Vech_Type,12);//车牌分类12
	//驾驶员信息 （  驾驶员代码 3 +驾驶证号码 18）
	memcpy(write_da+41,JT808Conf_struct.Driver_Info.DriveCode,3);//  驾驶员代码3
	memcpy(write_da+44,JT808Conf_struct.Driver_Info.DriverCard_ID,18);//驾驶证号码18
	// 开关量信息  1
	write_da[62]=Vehicle_sensor;
	//信息内容    条数2+内容
	write_da[63]=0;

	msg = (void (*)(void*))parameter;

	ptr_write_packet = rt_malloc(256);
	if( ptr_write_packet == RT_NULL )
	{
		msg( "E内存不足" );
		return;
	}
	//ptr_write_packet=write_da;
/*查找U盘*/
/*查找U盘*/
	while( 1 )
	{
	if( rt_device_find( "udisk" ) == RT_NULL ) /*没有找到*/
		{
		count++;
		if( count <= 5 )
			{
			msg( "I等待U盘插入" );
			}
		else
			{
			msg( "EU盘不存在" ); /*指示U盘不存在*/
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
			//msg( "I打开文件成功" );
			rt_kprintf("\r\n 打开文件成功");

			if(DataOutStartFlag==1)//疑点
				{
				write_da[64]=1;
				Api_DFdirectory_Read(doubt_data,write_da+65,206,0,1);
				len=271;//65+206
				size = write( fd,write_da,len);
				rt_kprintf("\r\n 导出疑点数据记录sizeof= %d ",size);
				data_fetch_comp=1;
				msg("I数据导出完成");
				//goto end_usbdata_0;
				}
			else if(DataOutStartFlag==2)//疲劳
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
				rt_kprintf("\r\n 导出疲劳记录sizeof= %d ",size);
				data_fetch_comp=1;
				msg("I数据导出完成");
				//goto end_usbdata_0;
				}
			else if(DataOutStartFlag==3)//超速
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
				rt_kprintf("\r\n 导出超速记录sizeof= %d ",size);
				data_fetch_comp=1;
				msg("I数据导出完成");
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
				msg("EU盘创建文件失败");
				rt_kprintf( "\r\n创建文件失败" );
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
		lcd_text12(0,3,"1.指定的事故疑点记录",20,LCD_MODE_SET);
	else if(OutType==2)
		lcd_text12(0,3,"2.指定的疲劳驾驶记录",20,LCD_MODE_SET);
	else if(OutType==3)
	    lcd_text12(0,3,"3.指定的超速驾驶记录",20,LCD_MODE_SET);
	lcd_update_all();
}



static void msg( void *p)
{
	unsigned int len;
	char *pinfo;
	lcd_fill( 0 );
	//lcd_text12( 0, 3, "数据", 4, LCD_MODE_SET );
	//lcd_text12( 0, 17, "导出", 4, LCD_MODE_SET );
	pinfo=(char *)p;
	len=strlen(pinfo);
	lcd_text12( 20, 10,pinfo+1,len-1, LCD_MODE_SET );
	if(*pinfo=='E')	/*出错或结束*/
		{
		tid_write_file=RT_NULL;
		}
	lcd_update_all( );
}
static void show(void)
{
	CounterBack=0;
	lcd_fill(0);
	lcd_text12(6,11,"选择导出的文件类型",18,LCD_MODE_SET);
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

				//if(tid_write_file)						///测试会复位
				//if(tid_write_file != RT_NULL)			///测试OK
				/* 调度器上锁，上锁后，将不再切换到其他线程，仅响应中断*/
				rt_enter_critical();
				if (tid_write_file != RT_NULL && tid_write_file->stat != RT_THREAD_CLOSE)	///测试OK
					{
					rt_thread_delete(tid_write_file);
					//WatchDog_Feed();
					}
				/* 调度器解锁*/
				rt_exit_critical();
				}
			
		       DataOutStartFlag=0;
			DataOutStartFlag=0;
			OUT_DataCounter=0;
			DataOutInFlag=0;
			break;
		case KeyValueOk:
			//本地升级
			if(DataOutInFlag==1)
				{
				DataOutInFlag=2;//导出数据类型选择标志
				USB_OUTFileSe(OUT_DataCounter);
				}
			//文件导出
			else if(DataOutInFlag==2)
				{
				DataOutInFlag=0;

				if((OUT_DataCounter>=1)&&(OUT_DataCounter<=3))
					DataOutStartFlag=OUT_DataCounter;//在timetick中显示导出过程

				tid_write_file= rt_thread_create( "writefile", thread_usbout_udisk, (void*)msg, 1024, 8, 5 );
				if( tid_write_file != RT_NULL )
					{
					msg("I导出存储的数据");
					rt_thread_startup( tid_write_file);
					}
				else
					{
					msg("E错误按菜单键");
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
	"USB数据",
	7,
	&show,
	&keypress,
	&timetick,
	&msg,
	(void*)0
};

