/************************************************************************************
*  Copyright (c), 2015, HelTec Automatic Technology co.,LTD.
*            All rights reserved.
*
* Http:    www.heltec.cn
* Email:   cn.heltec@gmail.com
* WebShop: heltec.taobao.com
*
* File name: OLED.h
* Project  : OLED
* Processor: STC89C52
* Compiler : Keil C51 Compiler
* 
* Author : Aaron Lee
* Version: 1.00
* Date   : 2014.3.24
* Email  : hello14blog@gmail.com
* Modification: none
* 
* Description:128*64点整OLED模块驱动文件，仅适用heltec.taobao.com所售产品
*
* Others: none;
*
* Function List:
*
* 1. void OLED_DLY_ms(unsigned int ms) -- OLED驱动程序用的延时程序,建议主函数中不要调用此程序
* 2. void OLED_WrDat(unsigned char dat) -- 向OLED屏写数据
* 3. void OLED_WrCmd(unsigned char cmd) -- 向OLED屏写命令
* 4. void OLED_SetPos(unsigned char x, unsigned char y) -- 设置显示坐标
* 5. void OLED_Fill(unsigned char bmp_dat) -- 全屏显示(显示BMP图片时才会用到此功能)
* 6. void OLED_CLS(void) -- 复位/清屏
* 7. void OLED_Init(void) -- OLED屏初始化程序，此函数应在操作屏幕之前最先调用
* 8. void OLED_DrawPoint(unsigned char x,unsigned char y,unsigned char t) -- 画点
* 9. void OLED_ShowChar(unsigned char x, unsigned char y, unsigned char ch[], unsigned char TextSize) -- 8x16点整，用于显示ASCII码，非常清晰
* 10.void OLED_ShowCN(unsigned char x, unsigned char y, unsigned char N) -- 16x16点整，用于显示汉字的最小阵列
* 11.void OLED_DrawBMP(unsigned char x0,unsigned char y0,unsigned char x1,unsigned char y1,unsigned char BMP[]) -- 将128x64像素的BMP位图在取字软件中算出字表，然后复制到codetab中，此函数调用即可
*
* History: none;
*
*************************************************************************************/

#include "OLED.h"
#include "Delay.h"
#include "codetab.h"
#include "Config.h"

void IIC_Start()
{
   SCL = 1;		
   SDA = 1;
   SDA = 0;
   SCL = 0;
}

void IIC_Stop()
{
   SCL = 0;
   SDA = 0;
   SCL = 1;
   SDA = 1;
}

void Write_IIC_Byte(unsigned char IIC_Byte)
{
	unsigned char i;
	for(i=0;i<8;i++)
	{
		if(IIC_Byte & 0x80)
			SDA=1;
		else
			SDA=0;
		SCL=1;
		SCL=0;
		IIC_Byte<<=1;
	}
	SDA=1;
	SCL=1;
	SCL=0;
}

void OLED_WrDat(unsigned char IIC_Data)
{
	IIC_Start();
	Write_IIC_Byte(0x78);
	Write_IIC_Byte(0x40);			//write data
	Write_IIC_Byte(IIC_Data);
	IIC_Stop();
}
/*********************OLED???************************************/
void OLED_WrCmd(unsigned char IIC_Command)
{
	IIC_Start();
	Write_IIC_Byte(0x78);            //Slave address,SA0=0
	Write_IIC_Byte(0x00);			//write command
	Write_IIC_Byte(IIC_Command);
	IIC_Stop();
}

void OLED_Set_Pos(unsigned char x, unsigned char y)
{ 
	OLED_WrCmd(0xb0+y);
	OLED_WrCmd(((x&0xf0)>>4)|0x10);
	OLED_WrCmd((x&0x0f)|0x01);
} 
/*********************OLED??************************************/
void OLED_Fill(unsigned char bmp_dat) 
{
	unsigned char y,x;
	for(y=0;y<8;y++)
	{
		OLED_WrCmd(0xb0+y);
		OLED_WrCmd(0x01);
		OLED_WrCmd(0x10);
		for(x=0;x<X_WIDTH;x++)
		OLED_WrDat(bmp_dat);
	}
}

void OLED_Init(void)
{
	RST = 1;
	DelayMs(200);
	RST = 0;
	DelayMs(200);
	RST = 1;
	
	OLED_WrCmd(0xae);
	OLED_WrCmd(0x00);
	OLED_WrCmd(0x10);
	OLED_WrCmd(0x40);
	OLED_WrCmd(0xB0);
	OLED_WrCmd(0x81);
	OLED_WrCmd(Brightness);
	OLED_WrCmd(0xa1);
	OLED_WrCmd(0xa6);//正显
	OLED_WrCmd(0xa8);
	OLED_WrCmd(0x1f);
	OLED_WrCmd(0xC8);
	OLED_WrCmd(0xd3);
	OLED_WrCmd(0x00);
	OLED_WrCmd(0xd5);
	OLED_WrCmd(0x80);
	OLED_WrCmd(0xd9);
	OLED_WrCmd(0x11);
	OLED_WrCmd(0xda);
	OLED_WrCmd(0x02);
	OLED_WrCmd(0x8d);
	OLED_WrCmd(0x14);
	OLED_WrCmd(0xdb);
	OLED_WrCmd(0x20);
	OLED_WrCmd(0xaf);
	OLED_Fill(0x00);
	OLED_Set_Pos(0,0);
}

void OLED_ShowInt(unsigned char x, unsigned char y, unsigned int Num)
{
	unsigned char c = 0,i = 0,j = 0,ch[3];
	
	ch[0] = Num/100 + 0x30;
	ch[1] = (Num%100)/10 + 0x30;
	ch[2] = Num%10 + 0x30;
	
	while(ch[j] != '\0')
	{
		c = ch[j] - 32;
		if(x > 120)
		{
			x = 0;
			y++;
		}
		OLED_Set_Pos(x,y);
		for(i=0;i<8;i++)
			OLED_WrDat(F8X16[c*16+i]);
		OLED_Set_Pos(x,y+1);
		for(i=0;i<8;i++)
			OLED_WrDat(F8X16[c*16+i+8]);
		x += 8;
		j++;
	}
}

void OLED_ShowChar(unsigned char x, unsigned char y, unsigned char ch[], unsigned char TextSize)
{
	unsigned char c = 0,i = 0,j = 0;
	switch(TextSize)
	{
		case 1:
		{
			while(ch[j] != '\0')
			{
				c = ch[j] - 32;
				if(x > 126)
				{
					x = 0;
					y++;
				}
				OLED_Set_Pos(x,y);
				for(i=0;i<6;i++)
					OLED_WrDat(F6x8[c][i]);
				x += 6;
				j++;
			}
		}break;
		case 2:
		{
			while(ch[j] != '\0')
			{
				c = ch[j] - 32;
				if(x > 120)
				{
					x = 0;
					y++;
				}
				OLED_Set_Pos(x,y);
				for(i=0;i<8;i++)
					OLED_WrDat(F8X16[c*16+i]);
				OLED_Set_Pos(x,y+1);
				for(i=0;i<8;i++)
					OLED_WrDat(F8X16[c*16+i+8]);
				x += 8;
				j++;
			}
		}break;
	}
}

void OLED_ShowCN(unsigned char x, unsigned char y, unsigned char N)
{
	unsigned char wm=0;
	unsigned int  adder=32*N;
	OLED_Set_Pos(x,y);
	for(wm = 0;wm < 16;wm++)
	{
		OLED_WrDat(F16x16[adder]);
		adder += 1;
	}
	OLED_Set_Pos(x,y + 1);
	for(wm = 0;wm < 16;wm++)
	{
		OLED_WrDat(F16x16[adder]);
		adder += 1;
	}
}
