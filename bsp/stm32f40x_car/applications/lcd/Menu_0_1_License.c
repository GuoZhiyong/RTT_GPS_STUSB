#include  <string.h>
#include "Menu_Include.h"
#include "Lcd.h"

#define width_hz   12
#define width_zf   6
#define top_line   14

unsigned char screen_flag=0;//==1��ʼ���뺺�ֽ���
unsigned char screen_in_sel=1,screen_in_sel2=1;
unsigned char zifu_counter=0;



//���򼽻���ԥ���ɺ���  ��³������Ӷ���ʽ�  ���¼���������ش���  ��
unsigned char Car_HZ_code[31][2]={"��","��","��","��","��","ԥ","��","��","��","��",\
"��","³","��","��","��","��","��","��","��","��","��","��","��","��","��","��","��","��","��","��","��"};
//
/*unsigned char Car_num_code[36]={'0','1','2','3','4','5','6','7','8','9',\
'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R',\
'S','T','U','V','W','X','Y','Z'};*/
unsigned char Car_num_code[36][1]={"0","1","2","3","4","5","6","7","8","9",\
"A","B","C","D","E","F","G","H","I","J","K","L","M","N","O","P","Q","R","S","T","U","V","W","X","Y","Z"};


unsigned char select[]={0x0C,0x06,0xFF,0x06,0x0C};
unsigned char select_kong[]={0x00,0x00,0x00,0x00,0x00};
DECL_BMP(8,5,select); 
DECL_BMP(8,5,select_kong); 

/*
���򼽻���ԥ����
������³�������
����ʽ����¼���������ش�����
*/
void license_input(unsigned char par)
{
lcd_fill(0);
switch(par)
	{
	case 1:
		lcd_text12(0,20,"���򼽻���ԥ���ɺ���",20,LCD_MODE_SET);
		break;	
	case 2:
		lcd_text12(0,20,"��³������Ӷ���ʽ�",20,LCD_MODE_SET);
		break;	
	case 3:
		lcd_text12(0,20,"���¼���������ش���",20,LCD_MODE_SET);
		break;	
	case 4:
		lcd_text12(0,20,"��",2,LCD_MODE_SET);
		break;
	case 5:
		lcd_text12(0,20,"0123456789ABCDEFGHIJ",20,LCD_MODE_SET);
		break;	
	case 6:
		lcd_text12(0,20,"KLMNOPQRSTUVWXYZ",16,LCD_MODE_SET);
		break;
	default:
		break;
	}
lcd_update_all();
}

/*
���򼽻���ԥ����
������³�������
����ʽ����¼���������ش�����

offset:  type==2ʱ����    ==1ʱ����
*/
void license_input_az09(unsigned char type,unsigned char offset,unsigned char par)
{
if((type==1)&&(par>=1)&&(par<=31))
	{
	memcpy(Menu_Car_license,(char *)Car_HZ_code[par-1],2);
	lcd_text12(0,0,(char *)Menu_Car_license,2,LCD_MODE_SET);
	}
else if((type==2)&&(par>=1)&&(par<=36))
	{
	 memcpy(Menu_Car_license+offset-1,(char *)Car_num_code[par-1],1);
	 lcd_text12(0,0,(char *)Menu_Car_license,offset,LCD_MODE_SET);//Car_license+2+(offset-3)=Car_license+offset-1
 	}
}

static void msg( void *p)
{

}

static void show(void)
{
       memset(Menu_Car_license,0,sizeof(Menu_Car_license));
	CounterBack=0;
	license_input(1);
	lcd_bitmap((screen_in_sel-1)*12, top_line, &BMP_select, LCD_MODE_SET);
	lcd_update_all();

	screen_flag=1;
    screen_in_sel=1;
	screen_in_sel2=1;
    zifu_counter=0;
}


static void keypress(unsigned int key)
{
	switch(KeyValue)
		{
		case KeyValueMenu:
			pMenuItem=&Menu_0_loggingin;
			pMenuItem->show();

			screen_in_sel=1;
			screen_in_sel2=1;
			screen_flag=0;
			zifu_counter=0;
			
			break;
		case KeyValueOk:
			if(screen_flag==1)
				{							
				if(zifu_counter==0)
					zifu_counter=2;//1������=2���ַ�
					
				screen_flag=5;//��һ������ѡ�ã�ѡ�ַ�
				lcd_fill(0);
				//lcd_text12(0,0,(char *)Car_license,zifu_counter,LCD_MODE_SET);
				
				license_input(5);//��ĸѡ��
				license_input_az09(1,0,screen_in_sel);//д��ѡ���ĺ���
				lcd_bitmap(0, top_line, &BMP_select, LCD_MODE_SET);
				lcd_update_all();

				screen_in_sel2=1;
				}
			else if((screen_flag==5)&&(zifu_counter<8))
				{
				zifu_counter++;
				
				lcd_fill(0);
				license_input(5);//��ĸѡ��
				license_input_az09(2,zifu_counter,screen_in_sel2);//
				//license_input_az09(1,0,screen_in_sel);//д��ѡ���ĺ���
				screen_in_sel2=1;
				lcd_bitmap(0, top_line, &BMP_select, LCD_MODE_SET);
				lcd_text12(0,0,(char *)Menu_Car_license,zifu_counter,LCD_MODE_SET);
				lcd_update_all();
                screen_in_sel2=1;
				}
			else if((screen_flag==5)&&(zifu_counter==8))
				{
				screen_flag=10;//���ƺ��������
				
				zifu_counter=0;
				lcd_fill(0);
				lcd_text12(0,0,(char *)Menu_Car_license,8,LCD_MODE_SET);
				//lcd_text12(15,1,(char *)Car_license+2,sizeof(Car_license)-2,LCD_MODE_SET);
				lcd_text12(18,20,"���ƺ��������",14,LCD_MODE_INVERT);
				license_input_az09(1,0,screen_in_sel);//д��ѡ���ĺ���
				lcd_update_all();

				screen_in_sel2=1;
				}
			else if(screen_flag==10)
				{
				CarSet_0_counter=2;//   ���õ�2��
				pMenuItem=&Menu_0_loggingin;
				pMenuItem->show();

				screen_in_sel=1;
				screen_in_sel2=1;
				screen_flag=0;
				zifu_counter=0;
				}
			break;
		case KeyValueUP:
			if(screen_flag==1)//����ѡ��
				{
				if(screen_in_sel>=2)
					screen_in_sel--;
				else if(screen_in_sel==1)
					screen_in_sel=31;
				if(screen_in_sel<=10)
					{
					license_input(1);
					lcd_bitmap((screen_in_sel-1)*width_hz, top_line, &BMP_select, LCD_MODE_SET);
					}
				else if((screen_in_sel>10)&&(screen_in_sel<=20))
					{
					license_input(2);
					lcd_bitmap((screen_in_sel-11)*width_hz, top_line, &BMP_select, LCD_MODE_SET);
					}
				else if((screen_in_sel>20)&&(screen_in_sel<=30))
					{
					license_input(3);
					lcd_bitmap((screen_in_sel-21)*width_hz, top_line, &BMP_select, LCD_MODE_SET);
					}
                else if(screen_in_sel==31)
                	{
                	license_input(4);
					lcd_bitmap((screen_in_sel-31)*width_hz, top_line, &BMP_select, LCD_MODE_SET);
                	}
				lcd_update_all();
				}
			else if(screen_flag==5)//��ĸ/���� ѡ��
				{
				if(screen_in_sel2>=2)
					screen_in_sel2--;
				else if(screen_in_sel2==1)
					screen_in_sel2=36;
				if(screen_in_sel2<=20)
					{
					license_input(5);
					lcd_bitmap((screen_in_sel2-1)*width_zf,top_line, &BMP_select, LCD_MODE_SET);		
					}
				else if((screen_in_sel2>20)&&(screen_in_sel2<=36))
					{
					license_input(6);
					lcd_bitmap((screen_in_sel2-21)*width_zf,top_line, &BMP_select, LCD_MODE_SET);					
					}
				//license_input_az09(1,0,screen_in_sel);//д��ѡ������ĸ������
				lcd_text12(0,0,(char *)Menu_Car_license,zifu_counter,LCD_MODE_SET);
				lcd_update_all();
				}
			break;
		case KeyValueDown:
			if(screen_flag==1)//����ѡ��
				{
				if(screen_in_sel<31)
					screen_in_sel++;
				else if(screen_in_sel==31)
					screen_in_sel=1;
				if(screen_in_sel<=10)
					{
					license_input(1);
					lcd_bitmap((screen_in_sel-1)*width_hz,top_line, &BMP_select, LCD_MODE_SET);
					}
				else if((screen_in_sel>10)&&(screen_in_sel<=20))
					{
					license_input(2);
					lcd_bitmap((screen_in_sel-11)*width_hz,top_line, &BMP_select, LCD_MODE_SET);
					}
				else if((screen_in_sel>20)&&(screen_in_sel<=30))
					{
					license_input(3);
					lcd_bitmap((screen_in_sel-21)*width_hz,top_line, &BMP_select, LCD_MODE_SET);
					}
                else if(screen_in_sel==31)
                	{
                	license_input(4);
					lcd_bitmap((screen_in_sel-31)*width_hz,top_line, &BMP_select, LCD_MODE_SET);
                	}
				lcd_update_all();
				}
			else if(screen_flag==5)//��ĸ/���� ѡ��
				{
				if(screen_in_sel2<36)
					screen_in_sel2++;
				else if(screen_in_sel2==36)
					screen_in_sel2=1;
				if(screen_in_sel2<=20)
					{
					license_input(5);
					lcd_bitmap((screen_in_sel2-1)*width_zf,top_line, &BMP_select, LCD_MODE_SET);
					}
				else if((screen_in_sel2>20)&&(screen_in_sel2<=36))
					{
					license_input(6);
					lcd_bitmap((screen_in_sel2-21)*width_zf,top_line, &BMP_select, LCD_MODE_SET);
					}
				//license_input_az09(1,0,screen_in_sel);//д��ѡ�������֡���ĸ
				lcd_text12(0,0,(char *)Menu_Car_license,zifu_counter,LCD_MODE_SET);
				lcd_update_all();
				}

			break;
		}
	KeyValue=0;
}


static void timetick(unsigned int systick)
{
	/*CounterBack++;
	if(CounterBack!=MaxBankIdleTime*5)
		return;
	CounterBack=0;
	pMenuItem=&Menu_0_loggingin;
	pMenuItem->show();

	
	screen_flag=0;//==1��ʼ���뺺�ֽ���
	screen_in_sel=1;
	screen_in_sel2=1;
	zifu_counter=0;*/
}


MENUITEM	Menu_0_1_license=
{
	"���ƺ�",
	6,
	&show,
	&keypress,
	&timetick,
	&msg,
	(void*)0
};




