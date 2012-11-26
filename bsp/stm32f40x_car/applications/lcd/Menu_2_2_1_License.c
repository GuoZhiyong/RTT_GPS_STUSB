#include  <string.h>
#include "menu.h"
#include "Lcd_init.h"

#define width_hz   12
#define width_zf   9
unsigned char Car_license[10];//��ų��ƺ���

unsigned char screen_flag=0;//==1��ʼ���뺺�ֽ���
unsigned char screen_in_sel=1,screen_in_sel2=1;
unsigned char zifu_counter=0;


struct IMG_DEF test_dis_license={12,12,test_00};


unsigned char select[]={0x0C,0x06,0xFF,0x06,0x0C};
unsigned char select_kong[]={0x00,0x00,0x00,0x00,0x00};




DECL_BMP(8,5,select); DECL_BMP(8,5,select_kong); 

/*
���򼽻���ԥ����
������³�������
����ʽ����¼���������ش�����
*/
void license_input(unsigned char par)
{
switch(par)
	{
	case 1:
		lcd_fill(0);
		DisAddRead_ZK(0,20,"���򼽻���ԥ���ɺ���",10,&test_dis_license,0,0);
		lcd_update_all();
		break;	
	case 2:
		lcd_fill(0);
		DisAddRead_ZK(0,20,"��³������Ӷ���ʽ�",10,&test_dis_license,0,0);
		lcd_update_all();
		break;	
	case 3:
		lcd_fill(0);
		DisAddRead_ZK(0,20,"���¼���������ش���",10,&test_dis_license,0,0);
		lcd_update_all();
		break;	
	case 4:
		lcd_fill(0);
		DisAddRead_ZK(0,20,"��",1,&test_dis_license,0,0);
		lcd_update_all();
		break;
	case 5:
		lcd_fill(0);
		lcd_text(14,0,FONT_TEN_DOT,(char *)Car_license+2);
		lcd_text(0,20,FONT_TEN_DOT,"0123456789");
		lcd_update_all();
		break;	
	case 6:
		lcd_fill(0);
		lcd_text(14,0,FONT_TEN_DOT,(char *)Car_license+2);
		lcd_text(0,20,FONT_TEN_DOT,"ABCDEFGHIJKLM");
		lcd_update_all();
		break;
	case 7:
		lcd_fill(0);
		lcd_text(14,0,FONT_TEN_DOT,(char *)Car_license+2);
		lcd_text(0,20,FONT_TEN_DOT,"NOPQRSTUVWXYZ");
		lcd_update_all();
		break;
	default:
		break;
	}
}

/*
���򼽻���ԥ����
������³�������
����ʽ����¼���������ش�����

offset:  type==2ʱ����    ==1ʱ����
*/
void license_input_az09(unsigned char type,unsigned char offset,unsigned char par)
{
if(type==1)
	{
	switch(par)
		{
		case 1:
			DisAddRead_ZK(0,0,"��",1,&test_dis_license,0,0);
			memcpy(Car_license,"��",2);
			break;
		case 2:
			DisAddRead_ZK(0,0,"��",1,&test_dis_license,0,0);
			memcpy(Car_license,"��",2);
			break;
		case 3:
			DisAddRead_ZK(0,0,"��",1,&test_dis_license,0,0);
			memcpy(Car_license,"��",2);
			break;
		case 4:
			DisAddRead_ZK(0,0,"��",1,&test_dis_license,0,0);
			memcpy(Car_license,"��",2);
			break;
		case 5:
			DisAddRead_ZK(0,0,"��",1,&test_dis_license,0,0);
			memcpy(Car_license,"��",2);
			lcd_update_all();
			break;
		case 6:
			DisAddRead_ZK(0,0,"ԥ",1,&test_dis_license,0,0);
			memcpy(Car_license,"ԥ",2);
			break;
		case 7:
			DisAddRead_ZK(0,0,"��",1,&test_dis_license,0,0);
			memcpy(Car_license,"��",2);
			break;
		case 8:
			DisAddRead_ZK(0,0,"��",1,&test_dis_license,0,0);
			memcpy(Car_license,"��",2);
			break;
		case 9:
			DisAddRead_ZK(0,0,"��",1,&test_dis_license,0,0);
			memcpy(Car_license,"��",2);
			break;
		case 10:
			DisAddRead_ZK(0,0,"��",1,&test_dis_license,0,0);
			memcpy(Car_license,"��",2);
			break;
		case 11:
			DisAddRead_ZK(0,0,"��",1,&test_dis_license,0,0);
			memcpy(Car_license,"��",2);
			break;
		case 12:
			DisAddRead_ZK(0,0,"³",1,&test_dis_license,0,0);
			memcpy(Car_license,"³",2);
			break;
		case 13:
			DisAddRead_ZK(0,0,"��",1,&test_dis_license,0,0);
			memcpy(Car_license,"��",2);
			break;
		case 14:
			DisAddRead_ZK(0,0,"��",1,&test_dis_license,0,0);
			memcpy(Car_license,"��",2);
			break;
		case 15:
			DisAddRead_ZK(0,0,"��",1,&test_dis_license,0,0);
			memcpy(Car_license,"��",2);
			break;
		case 16:
			DisAddRead_ZK(0,0,"��",1,&test_dis_license,0,0);
			memcpy(Car_license,"��",2);
			break;
		case 17:
			DisAddRead_ZK(0,0,"��",1,&test_dis_license,0,0);
			memcpy(Car_license,"��",2);
			break;
		case 18:
			DisAddRead_ZK(0,0,"��",1,&test_dis_license,0,0);
			memcpy(Car_license,"��",2);
			break;
		case 19:
			DisAddRead_ZK(0,0,"��",1,&test_dis_license,0,0);
			memcpy(Car_license,"��",2);
			break;
		case 20:
			DisAddRead_ZK(0,0,"��",1,&test_dis_license,0,0);
			memcpy(Car_license,"��",2);
			break;
		case 21:
			DisAddRead_ZK(0,0,"��",1,&test_dis_license,0,0);
			memcpy(Car_license,"��",2);
			break;
		case 22:
			DisAddRead_ZK(0,0,"��",1,&test_dis_license,0,0);
			memcpy(Car_license,"��",2);
			break;
		case 23:
			DisAddRead_ZK(0,0,"��",1,&test_dis_license,0,0);
			memcpy(Car_license,"��",2);
			break;
		case 24:
			DisAddRead_ZK(0,0,"��",1,&test_dis_license,0,0);
			memcpy(Car_license,"��",2);
			break;
		case 25:
			DisAddRead_ZK(0,0,"��",1,&test_dis_license,0,0);
			memcpy(Car_license,"��",2);
			break;
		case 26:
			DisAddRead_ZK(0,0,"��",1,&test_dis_license,0,0);
			memcpy(Car_license,"��",2);
			break;
		case 27:
			DisAddRead_ZK(0,0,"��",1,&test_dis_license,0,0);
			memcpy(Car_license,"��",2);
			break;
		case 28:
			DisAddRead_ZK(0,0,"��",1,&test_dis_license,0,0);
			memcpy(Car_license,"��",2);
			break;
		case 29:
			DisAddRead_ZK(0,0,"��",1,&test_dis_license,0,0);
			memcpy(Car_license,"��",2);
			break;
		case 30:
			DisAddRead_ZK(0,0,"��",1,&test_dis_license,0,0);
			memcpy(Car_license,"��",2);
			break;
		case 31:
			DisAddRead_ZK(0,0,"��",1,&test_dis_license,0,0);
			memcpy(Car_license,"��",2);
			break;
		default:
			break;
		}
	}
else if(type==2)
	{
	switch(par)
		{
		case 1:
			lcd_text(12+(offset-3)*10,0,FONT_TEN_DOT,"0");//Car_license+2+(offset-3)=Car_license+offset-1
			memcpy(Car_license+offset-1,"0",1);
			break;
		case 2:
			lcd_text(12+(offset-3)*10,0,FONT_TEN_DOT,"1");
			memcpy(Car_license+offset-1,"1",1);
			break;
		case 3:
			lcd_text(12+(offset-3)*10,0,FONT_TEN_DOT,"2");
			memcpy(Car_license+offset-1,"2",1);
			break;
		case 4:
			lcd_text(12+(offset-3)*10,0,FONT_TEN_DOT,"3");
			memcpy(Car_license+offset-1,"3",1);
			break;
		case 5:
			lcd_text(12+(offset-3)*10,0,FONT_TEN_DOT,"4");
			memcpy(Car_license+offset-1,"4",1);
			break;
		case 6:
			lcd_text(12+(offset-3)*10,0,FONT_TEN_DOT,"5");
			memcpy(Car_license+offset-1,"5",1);
			break;
		case 7:
			lcd_text(12+(offset-3)*10,0,FONT_TEN_DOT,"6");
			memcpy(Car_license+offset-1,"6",1);
			break;
		case 8:
			lcd_text(12+(offset-3)*10,0,FONT_TEN_DOT,"7");
			memcpy(Car_license+offset-1,"7",1);
			break;
		case 9:
			lcd_text(12+(offset-3)*10,0,FONT_TEN_DOT,"8");
			memcpy(Car_license+offset-1,"8",1);
			break;
		case 10:
			lcd_text(12+(offset-3)*10,0,FONT_TEN_DOT,"9");
			memcpy(Car_license+offset-1,"9",1);
			break;
		case 11:
			lcd_text(12+(offset-3)*10,0,FONT_TEN_DOT,"A");
			memcpy(Car_license+offset-1,"A",1);
			break;
		case 12:
			lcd_text(12+(offset-3)*10,0,FONT_TEN_DOT,"B");
			memcpy(Car_license+offset-1,"B",1);
			break;
		case 13:
			lcd_text(12+(offset-3)*10,0,FONT_TEN_DOT,"C");
			memcpy(Car_license+offset-1,"C",1);
			break;
		case 14:
			lcd_text(12+(offset-3)*10,0,FONT_TEN_DOT,"D");
			memcpy(Car_license+offset-1,"D",1);
			break;
		case 15:
			lcd_text(12+(offset-3)*10,0,FONT_TEN_DOT,"E");
			memcpy(Car_license+offset-1,"E",1);
			break;
		case 16:
			lcd_text(12+(offset-3)*10,0,FONT_TEN_DOT,"F");
			memcpy(Car_license+offset-1,"F",1);
			break;
		case 17:
			lcd_text(12+(offset-3)*10,0,FONT_TEN_DOT,"G");
			memcpy(Car_license+offset-1,"G",1);
			break;
		case 18:
			lcd_text(12+(offset-3)*10,0,FONT_TEN_DOT,"H");
			memcpy(Car_license+offset-1,"H",1);
			break;
		case 19:
			lcd_text(12+(offset-3)*10,0,FONT_TEN_DOT,"I");
			memcpy(Car_license+offset-1,"I",1);
			break;
		case 20:
			lcd_text(12+(offset-3)*10,0,FONT_TEN_DOT,"J");
			memcpy(Car_license+offset-1,"J",1);
			break;
		case 21:
			lcd_text(12+(offset-3)*10,0,FONT_TEN_DOT,"K");
			memcpy(Car_license+offset-1,"K",1);
			break;
		case 22:
			lcd_text(12+(offset-3)*10,0,FONT_TEN_DOT,"L");
			memcpy(Car_license+offset-1,"L",1);
			break;
		case 23:
			lcd_text(12+(offset-3)*10,0,FONT_TEN_DOT,"M");
			memcpy(Car_license+offset-1,"M",1);
			break;
		case 24:
			lcd_text(12+(offset-3)*10,0,FONT_TEN_DOT,"N");
			memcpy(Car_license+offset-1,"N",1);
			break;
		case 25:
			lcd_text(12+(offset-3)*10,0,FONT_TEN_DOT,"O");
			memcpy(Car_license+offset-1,"O",1);
			break;
		case 26:
			lcd_text(12+(offset-3)*10,0,FONT_TEN_DOT,"P");
			memcpy(Car_license+offset-1,"P",1);
			break;
		case 27:
			lcd_text(12+(offset-3)*10,0,FONT_TEN_DOT,"Q");
			memcpy(Car_license+offset-1,"Q",1);
			break;
		case 28:
			lcd_text(12+(offset-3)*10,0,FONT_TEN_DOT,"R");
			memcpy(Car_license+offset-1,"R",1);
			break;
		case 29:
			lcd_text(12+(offset-3)*10,0,FONT_TEN_DOT,"S");
			memcpy(Car_license+offset-1,"S",1);
			break;
		case 30:
			lcd_text(12+(offset-3)*10,0,FONT_TEN_DOT,"T");
			memcpy(Car_license+offset-1,"T",1);
			break;
		case 31:
			lcd_text(12+(offset-3)*10,0,FONT_TEN_DOT,"U");
			memcpy(Car_license+offset-1,"U",1);
			break;
		case 32:
			lcd_text(12+(offset-3)*10,0,FONT_TEN_DOT,"V");
			memcpy(Car_license+offset-1,"V",1);
			break;
		case 33:
			lcd_text(12+(offset-3)*10,0,FONT_TEN_DOT,"W");
			memcpy(Car_license+offset-1,"W",1);
			break;
		case 34:
			lcd_text(12+(offset-3)*10,0,FONT_TEN_DOT,"X");
			memcpy(Car_license+offset-1,"X",1);
			break;
		case 35:
			lcd_text(12+(offset-3)*10,0,FONT_TEN_DOT,"Y");
			memcpy(Car_license+offset-1,"Y",1);
			break;
		case 36:
			lcd_text(12+(offset-3)*10,0,FONT_TEN_DOT,"Z");
			memcpy(Car_license+offset-1,"Z",1);
			break;
		}
	}
}

static void show(void)
{
	lcd_fill(0);
	DisAddRead_ZK(18,3,"�����복�ƺ���",7,&test_dis_license,0,0);
	DisAddRead_ZK(24,19,"��ȷ�ϼ���ʼ",6,&test_dis_license,0,0);
	lcd_update_all();

	screen_flag=0;//==1��ʼ���뺺�ֽ���
    screen_in_sel=1;
	screen_in_sel2=1;
    zifu_counter=0;
}


static void keypress(unsigned int key)
{
	switch(KeyValue)
		{
		case KeyValueMenu:
			pMenuItem=&Menu_1_CarSet;
			pMenuItem->show();

			screen_in_sel=1;
			screen_in_sel2=1;
			screen_flag=0;
			zifu_counter=0;
			memset(Car_license,0,sizeof(Car_license));
			break;
		case KeyValueOk:
			if(screen_flag==0)
				{
				screen_flag=1;
				license_input(1);
				lcd_bitmap((screen_in_sel-1)*12, 12, &BMP_select, LCD_MODE_SET);
				lcd_update_all();
				}
			else if(screen_flag==1)
				{							
				if(zifu_counter==0)
					{
					zifu_counter=2;//1������=2���ַ�
					//printf("\r\n���ƺ������ַ�λ�� = %d",zifu_counter);
					}
					
				screen_flag=5;//��һ������ѡ�ã�ѡ�ַ�
				license_input(5);//��ĸѡ��
				license_input_az09(1,0,screen_in_sel);//д��ѡ���ĺ���
				/////lcd_bitmap((screen_in_sel-1)*12, 12, &BMP_select, LCD_MODE_SET);
				lcd_bitmap(0, 12, &BMP_select, LCD_MODE_SET);
				lcd_update_all();

				//screen_in_sel=1;//���ָܻ�Ϊ��ʼֵ��Ҫ������ʾ������
				screen_in_sel2=1;
				}
			else if((screen_flag==5)&&(zifu_counter<8))
				{
				zifu_counter++;


                //printf("\r\n���ƺ������ַ�λ�� = %d",zifu_counter);
				license_input(5);//��ĸѡ��
				license_input_az09(2,zifu_counter,screen_in_sel2);//
				license_input_az09(1,0,screen_in_sel);//д��ѡ���ĺ���

				//screen_in_sel=1;//���ָܻ�Ϊ��ʼֵ��Ҫ������ʾ������
				screen_in_sel2=1;
				/////lcd_bitmap((screen_in_sel2-1)*12,12, &BMP_select, LCD_MODE_SET);
				lcd_bitmap(0, 12, &BMP_select, LCD_MODE_SET);
				lcd_update_all();

				}
			else if((screen_flag==5)&&(zifu_counter==8))
				{
				screen_flag=10;//���ƺ��������
				
				zifu_counter=0;
				lcd_fill(0);
				lcd_text(15,1,FONT_TEN_DOT,(char *)Car_license+2);
				DisAddRead_ZK(30,20,"���ƺ��������",7,&test_dis_license,1,0);
				license_input_az09(1,0,screen_in_sel);//д��ѡ���ĺ���
				lcd_update_all();

                //д�복�ƺ���
				//memcpy(Vechicle_Info.Vech_Num,Car_license,8); 
				//Vehicleinfo_Write();

				screen_in_sel2=1;
				}
			else if(screen_flag==10)
				{
				pMenuItem=&Menu_1_CarSet;
				pMenuItem->show();

				screen_in_sel=1;
				screen_in_sel2=1;
				screen_flag=0;
				zifu_counter=0;
				memset(Car_license,0,sizeof(Car_license));
				}
			break;
		case KeyValueUP:
			if(screen_flag==1)//����ѡ��
				{
				if(screen_in_sel>=2)
					screen_in_sel--;
				if(screen_in_sel<=10)
					{
					license_input(1);
					lcd_bitmap((screen_in_sel-1)*width_hz, 12, &BMP_select, LCD_MODE_SET);
					lcd_update_all();
					}
				else if((screen_in_sel>10)&&(screen_in_sel<=20))
					{
					license_input(2);
					lcd_bitmap((screen_in_sel-11)*width_hz, 12, &BMP_select, LCD_MODE_SET);
					lcd_update_all();
					}
				else if((screen_in_sel>20)&&(screen_in_sel<=30))
					{
					license_input(3);
					lcd_bitmap((screen_in_sel-21)*width_hz, 12, &BMP_select, LCD_MODE_SET);
					lcd_update_all();
					}
                else if(screen_in_sel==31)
                	{
                	license_input(4);
					lcd_bitmap((screen_in_sel-31)*width_hz, 12, &BMP_select, LCD_MODE_SET);
					lcd_update_all();
                	}
				}
			else if(screen_flag==5)//��ĸ/���� ѡ��
				{
				if(screen_in_sel2>=2)
					screen_in_sel2--;
				if(screen_in_sel2<=10)
					{
					license_input(5);
					lcd_bitmap((screen_in_sel2-1)*width_zf,12, &BMP_select, LCD_MODE_SET);
					
					license_input_az09(1,0,screen_in_sel);//д��ѡ���ĺ���
					lcd_update_all();
					}
				else if((screen_in_sel2>10)&&(screen_in_sel2<=23))
					{
					license_input(6);
					lcd_bitmap((screen_in_sel2-11)*width_zf,12, &BMP_select, LCD_MODE_SET);
					
					license_input_az09(1,0,screen_in_sel);//д��ѡ���ĺ���
					lcd_update_all();
					}
				else if((screen_in_sel2>23)&&(screen_in_sel2<=36))
					{
					license_input(7);
					lcd_bitmap((screen_in_sel2-24)*width_zf,12, &BMP_select, LCD_MODE_SET);
					
					license_input_az09(1,0,screen_in_sel);//д��ѡ���ĺ���
					lcd_update_all();
					}
				}
			break;
		case KeyValueDown:
			if(screen_flag==1)//����ѡ��
				{
				if(screen_in_sel<31)
					screen_in_sel++;
				if(screen_in_sel<=10)
					{
					license_input(1);
					lcd_bitmap((screen_in_sel-1)*width_hz,12, &BMP_select, LCD_MODE_SET);
					lcd_update_all();
					}
				else if((screen_in_sel>10)&&(screen_in_sel<=20))
					{
					license_input(2);
					lcd_bitmap((screen_in_sel-11)*width_hz,12, &BMP_select, LCD_MODE_SET);
					lcd_update_all();
					}
				else if((screen_in_sel>20)&&(screen_in_sel<=30))
					{
					license_input(3);
					lcd_bitmap((screen_in_sel-21)*width_hz,12, &BMP_select, LCD_MODE_SET);
					lcd_update_all();
					}
                else if(screen_in_sel==31)
                	{
                	license_input(4);
					lcd_bitmap((screen_in_sel-31)*width_hz,12, &BMP_select, LCD_MODE_SET);
					lcd_update_all();
                	}
				}
			else if(screen_flag==5)//��ĸ/���� ѡ��
				{
				if(screen_in_sel2<36)
					screen_in_sel2++;
				if(screen_in_sel2<=10)
					{
					license_input(5);
					lcd_bitmap((screen_in_sel2-1)*width_zf,12, &BMP_select, LCD_MODE_SET);
					
					license_input_az09(1,0,screen_in_sel);//д��ѡ���ĺ���
					lcd_update_all();
					}
				else if((screen_in_sel2>10)&&(screen_in_sel2<=23))
					{
					license_input(6);
					lcd_bitmap((screen_in_sel2-11)*width_zf,12, &BMP_select, LCD_MODE_SET);
					
					license_input_az09(1,0,screen_in_sel);//д��ѡ���ĺ���
					lcd_update_all();
					}
				else if((screen_in_sel2>23)&&(screen_in_sel2<=36))
					{
					license_input(7);
					lcd_bitmap((screen_in_sel2-24)*width_zf,12, &BMP_select, LCD_MODE_SET);
					
					license_input_az09(1,0,screen_in_sel);//д��ѡ���ĺ���
					lcd_update_all();
					}
				}

			break;
		}
	KeyValue=0;
}


static void timetick(unsigned int systick)
{
	CounterBack++;
	if(CounterBack!=MaxBankIdleTime)
		return;
	CounterBack=0;
	pMenuItem=&Menu_1_Idle;
	pMenuItem->show();

	
	screen_flag=0;//==1��ʼ���뺺�ֽ���
	screen_in_sel=1;
	screen_in_sel2=1;
	zifu_counter=0;
}


MENUITEM	Menu_2_2_1_license=
{
	"���ƺ�����",
	&show,
	&keypress,
	&timetick,
	(void*)0
};




