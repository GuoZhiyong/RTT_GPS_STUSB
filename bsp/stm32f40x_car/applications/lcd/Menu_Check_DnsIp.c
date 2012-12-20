#include "Menu_Include.h"



void Disp_DnsIP(u8 par)
{
u8 dns1[50]={"DNS1:"},dns2[50]={"DNS2:"},apn[20]={"APN:"};
u8 len1=0,len2=0,len3=0;
u8 ip1[30]={"IP_1:   .   .   .   :      "},ip2[30]={"IP_2:   .   .   .   :      "};
if(par==1)
	{
	len1=strlen(DomainNameStr_1);
	memcpy(dns1+5,DomainNameStr_1,len1);
	len2=strlen(DomainNameStr_2);
	memcpy(dns2+5,DomainNameStr_2,len2);
	len3=strlen(APN_String);
	memcpy(apn+4,APN_String,len2);
	lcd_fill(0);
	lcd_text12(0,5,dns1,sizeof(dns1),LCD_MODE_SET);
	lcd_text12(0,14,dns2,sizeof(dns2),LCD_MODE_SET);
	lcd_text12(0,23,apn,sizeof(apn),LCD_MODE_SET);
	lcd_update_all();
	}
else if(par==2)
	{
	ip1[5]=RemoteIP_1[0]/100+'0';
	ip1[6]=RemoteIP_1[0]%100/10+'0';
	ip1[7]=RemoteIP_1[0]%10+'0';
	
	ip1[9]=RemoteIP_1[1]/100+'0';
	ip1[10]=RemoteIP_1[1]%100/10+'0';
	ip1[11]=RemoteIP_1[1]%10+'0';
	
	ip1[13]=RemoteIP_1[2]/100+'0';
	ip1[14]=RemoteIP_1[2]%100/10+'0';
	ip1[15]=RemoteIP_1[2]%10+'0';
	
	ip1[17]=RemoteIP_1[3]/100+'0';
	ip1[18]=RemoteIP_1[3]%100/10+'0';
	ip1[19]=RemoteIP_1[3]%10+'0';

	ip1[21]=RemotePort_1/10000+'0';
	ip1[22]=RemotePort_1%10000/1000+'0';
	ip1[23]=RemotePort_1%1000/100+'0';
	ip1[24]=RemotePort_1%100/10+'0';
	ip1[25]=RemotePort_1%10+'0';

	ip2[5]=RemoteIP_2[0]/100+'0';
	ip2[6]=RemoteIP_2[0]%100/10+'0';
	ip2[7]=RemoteIP_2[0]%10+'0';
	
	ip2[9]=RemoteIP_2[1]/100+'0';
	ip2[10]=RemoteIP_2[1]%100/10+'0';
	ip2[11]=RemoteIP_2[1]%10+'0';
	
	ip2[13]=RemoteIP_2[2]/100+'0';
	ip2[14]=RemoteIP_2[2]%100/10+'0';
	ip2[15]=RemoteIP_2[2]%10+'0';
	
	ip2[17]=RemoteIP_2[3]/100+'0';
	ip2[18]=RemoteIP_2[3]%100/10+'0';
	ip2[19]=RemoteIP_2[3]%10+'0';

	ip2[21]=RemotePort_2/10000+'0';
	ip2[22]=RemotePort_2%10000/1000+'0';
	ip2[23]=RemotePort_2%1000/100+'0';
	ip2[24]=RemotePort_2%100/10+'0';
	ip2[25]=RemotePort_2%10+'0';
	
	lcd_fill(0);
	lcd_text12(0,8,ip1,sizeof(ip1),LCD_MODE_SET);
	lcd_text12(0,20,ip2,sizeof(ip2),LCD_MODE_SET);
	lcd_update_all();
	}
}

static void show(void)
	{
	lcd_fill(0);
	lcd_text12(24,3,"查看设置信息",12,LCD_MODE_SET);
	lcd_text12(30,18,"请按确认键",10,LCD_MODE_SET);
	lcd_update_all();
	}
static void keypress(unsigned int key)
{

	switch(KeyValue)
		{
		case KeyValueMenu:
			pMenuItem=&Menu_1_Idle;
			pMenuItem->show();
			CounterBack=0;
			break;
		case KeyValueOk:
			Disp_DnsIP(1);
			break;
		case KeyValueUP:
			Disp_DnsIP(1);
			break;
		case KeyValueDown:
			Disp_DnsIP(2);
			break;
		}
 KeyValue=0;
}


static void timetick(unsigned int systick)
{
       Cent_To_Disp();

	CounterBack++;
	if(CounterBack!=MaxBankIdleTime)
		return;
	CounterBack=0;
	pMenuItem=&Menu_1_Idle;
	pMenuItem->show();

}


MENUITEM	Menu_Check_DnsIp=
{
"DNS显示",
	7,
	&show,
	&keypress,
	&timetick,
	(void*)0
};

