/**************************************************************************
 *                                                                         
 *   sed1520.c                                                             
 *   LCD display controller interface routines for graphics modules        
 *   with onboard SED1520 controller(s) in "write-only" setup              
 *                                                                         
 *   Version 1.02 (20051031)                                               
 *                                                                         
 *   For Atmel AVR controllers with avr-gcc/avr-libc                       
 *   Copyright (c) 2005                                                    
 *     Martin Thomas, Kaiserslautern, Germany                              
 *     <eversmith@heizung-thomas.de>                                       
 *     http://www.siwawi.arubi.uni-kl.de/avr_projects                      
 *
 *   Permission to use in NON-COMMERCIAL projects is herbey granted. For
 *   a commercial license contact the author.
 *
 *   The above copyright notice and this permission notice shall be included in all
 *   copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 *   FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 *   COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 *   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 *   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *                                                                         
 *
 *   partly based on code published by:                                    
 *   Michael J. Karas and Fabian "ape" Thiele
 *
 *
 ***************************************************************************/

/* 
   An Emerging Display EW12A03LY 122x32 Graphics module has been used
   for testing. This module only supports "write". There is no option
   to read data from the SED1520 RAM. The SED1520 R/W line on the 
   module is bound to GND according to the datasheet. Because of this 
   Read-Modify-Write using the LCD-RAM is not possible with the 12A03 
   LCD-Module. So this library uses a "framebuffer" which needs 
   ca. 500 bytes of the AVR's SRAM. The libray can of cause be used 
   with read/write modules too.
*/

/* tab-width: 4 */

//#include <LPC213x.H>      
//#include <includes.h>

#include <stdint.h>
#include "sed1520.h"
#include "fonts.h"
#include "bmp.h"
#include "stm32f4xx.h"

#include "Lcd_init.h"
#include "menu.h"

void Init_lcdkey(void)
{

    GPIO_InitTypeDef  GPIO_InitStructure;
	
	//����	  PE1:down	  PB7:up	PB8:ok	   PB9:mune
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOE, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7| GPIO_Pin_8| GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

    //OUT  (/MR	 SHCP	DS	 STCP	STCP)	
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12| GPIO_Pin_13| GPIO_Pin_14|GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOE, &GPIO_InitStructure);  
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_SetBits(GPIOA,GPIO_Pin_15);
}


void KeyCheckFun(void)
{
if((GPIO_ReadInputDataBit(GPIOE,GPIO_Pin_1)&0x01)==0)
		{
        KeyCheck_Flag[3]++;
		if(KeyCheck_Flag[3]==2)
			{
			KeyValue=1;
			}
		}
	else
		{
		KeyCheck_Flag[3]=0;
		}

if((GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_7)&0x01)==0)
	{
	KeyCheck_Flag[0]++;
	if(KeyCheck_Flag[0]==2)
		{
		KeyValue=2;
		}
	}
else
	{
	KeyCheck_Flag[0]=0;
	}

if((GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_8)&0x01)==0)
	{
	KeyCheck_Flag[1]++;
	if(KeyCheck_Flag[1]==2)
		{
		KeyValue=3;
		}
	}
else
	{
	KeyCheck_Flag[1]=0;
	}

if((GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_9)&0x01)==0)
	{
	KeyCheck_Flag[2]++;
	if(KeyCheck_Flag[2]==2)
		{
		KeyValue=4;
		}
	}
else
	{
	KeyCheck_Flag[2]=0;
	}
}

