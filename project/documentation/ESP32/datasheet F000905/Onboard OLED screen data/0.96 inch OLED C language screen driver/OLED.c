/************************************************************************************
*  Copyright (c), 2015, HelTec Automatic Technology co.,LTD.
*            All rights reserved.
*
* Http:    www.heltec.cn
* Email:   heltec@heltec.cn
* WebShop: heltec.taobao.com
*
* File name: OLED.c
* Project  : OLED
* Processor: STC89C52
* Compiler : Keil C51 Compiler
* 
* Author : Aaron Lee
* Version: 1.01
* Date   : 2016.5.25
* Email  : support@heltec.com
* Modification: none
* 
* Description:128*64点阵OLED模块驱动文件，仅适用heltec.taobao.com所售产品
*
* Others: none;
*
* Function List:
*
* 1. void OLED_WrDat(unsigned char dat) -- 向OLED屏写数据
* 2. void OLED_WrCmd(unsigned char cmd) -- 向OLED屏写命令
* 3. void OLED_SetPos(unsigned char x, unsigned char y) -- 设置显示坐标
* 4. void OLED_Fill(unsigned char bmp_dat) -- 全屏显示(显示BMP图片时才会用到此功能)
* 5. void OLED_CLS(void) -- 复位/清屏
* 6. void OLED_Init(void) -- OLED屏初始化程序，此函数应在操作屏幕之前最先调用
* 7. void OLED_DrawPoint(unsigned char x,unsigned char y,unsigned char t) -- 画点
* 8. void OLED_ShowChar(unsigned char x, unsigned char y, unsigned char ch[], unsigned char TextSize) -- 8x16点整，用于显示ASCII码，非常清晰
* 9.void OLED_ShowCN(unsigned char x, unsigned char y, unsigned char N) -- 16x16点整，用于显示汉字的最小阵列
* 10.void OLED_DrawBMP(unsigned char x0,unsigned char y0,unsigned char x1,unsigned char y1,unsigned char BMP[]) -- 将128x64像素的BMP位图在取字软件中算出字表，然后复制到codetab中，此函数调用即可
*
* History: 将IIC方式、SPI方式两种通信方式整合到了同一个文件中;
*          将SH1106和SSD1306的初始化代码整合到了同一个文件中;
*          使用之前，先确定自己屏的通信方式和型号，再到OLED.h文件中修改宏定义
*
*************************************************************************************/

#include "OLED.h"
#include "Delay.h"
#include "codetab.h"

#if IIC_SPI_TYPE //SPI引脚定义
	sbit OLED_SCL=P1^3; //时钟CLK
	sbit OLED_SDA=P1^2; //MISO
	sbit OLED_RST=P1^1; //复位
	sbit OLED_DC =P1^0; //数据/命令控制
#else //IIC引脚定义
	sbit SCL=P1^3; //SCL
	sbit SDA=P1^2; //SDA
#endif

#if !IIC_SPI_TYPE
void IIC_Start(void)
{
   SCL = 1;		
   SDA = 1;
   SDA = 0;
   SCL = 0;
}

/**********************************************
//IIC Stop
**********************************************/
void IIC_Stop(void)
{
   SCL = 0;
   SDA = 0;
   SCL = 1;
   SDA = 1;
}

/**********************************************
// IIC Write byte
**********************************************/
void Write_IIC_Byte(unsigned char IIC_Byte)
{
	unsigned char i;
	for(i=0;i<8;i++)
	{
		if(IIC_Byte & 0x80)
			SDA = 1;
		else
			SDA = 0;
		SCL = 1;
		SCL = 0;
		IIC_Byte<<=1;
	}
	SDA = 1;
	SCL = 1;
	SCL = 0;
}
#endif

/*********************OLED写数据************************************/ 
void OLED_WrDat(unsigned char dat)
{
#if IIC_SPI_TYPE //SPI方式写数据
	
	unsigned char i;
	OLED_DC=1;  
	for(i=0;i<8;i++) //发送一个八位数据 
	{
		if((dat << i) & 0x80)
		{
			OLED_SDA  = 1;
		}
		else  OLED_SDA  = 0;
		OLED_SCL = 0;
		OLED_SCL = 1;
	}
	
#else // IIC方式写数据
	
	IIC_Start();
	Write_IIC_Byte(0x78);
	Write_IIC_Byte(0x40);			//write data
	Write_IIC_Byte(dat);
	IIC_Stop();
	
#endif
}

/*********************OLED写命令************************************/										
void OLED_WrCmd(unsigned char cmd)
{
#if IIC_SPI_TYPE//SPI方式写命令
	
	unsigned char i;
	OLED_DC=0;
	for(i=0;i<8;i++) //发送一个八位数据
	{
		if((cmd << i) & 0x80)
		{
			OLED_SDA  = 1;
		}
		else
		{
			OLED_SDA  = 0;
		}
		OLED_SCL = 0;
		OLED_SCL = 1;
	}
	
#else //IIC方式写命令
	
	IIC_Start();
	Write_IIC_Byte(0x78);            //Slave address,SA0=0
	Write_IIC_Byte(0x00);			//write command
	Write_IIC_Byte(cmd);
	IIC_Stop();
	
#endif
}

/*********************OLED 设置坐标************************************/
void OLED_SetPos(unsigned char x, unsigned char y)
{ 
	OLED_WrCmd(0xb0+y);
	OLED_WrCmd(((x&0xf0)>>4)|0x10);
	OLED_WrCmd((x&0x0f)|0x01); 
}

/*********************OLED全屏************************************/
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

/*********************OLED清屏************************/
void OLED_CLS(void)
{
	OLED_Fill(0x00);
}

/*************开启OLED显示*************/
void OLED_ON(void)
{
	OLED_WrCmd(0X8D);  //设置电荷泵
	OLED_WrCmd(0X14);  //开启电荷泵
	OLED_WrCmd(0XAF);  //OLED唤醒       
}

/*************关闭OLED显示 -- 使OLED进入休眠模式*************/
void OLED_OFF(void)
{
	OLED_WrCmd(0X8D);  //设置电荷泵
	OLED_WrCmd(0X10);  //关闭电荷泵
	OLED_WrCmd(0XAE);  //OLED休眠
}

/*********************OLED初始化************************************/
void OLED_Init(void)
{
	DelayMs(500);
	
#if CHIP_IS_SSD1306
	
	OLED_WrCmd(0xae);//--turn off oled panel
	OLED_WrCmd(0x00);//---set low column address
	OLED_WrCmd(0x10);//---set high column address
	OLED_WrCmd(0x40);//--set start line address  Set Mapping RAM Display Start Line (0x00~0x3F)
	OLED_WrCmd(0x81);//--set contrast control register
	OLED_WrCmd(Brightness); // Set SEG Output Current Brightness
	OLED_WrCmd(0xa1);//--Set SEG/Column Mapping     0xa0左右反置 0xa1正常
	OLED_WrCmd(0xc8);//Set COM/Row Scan Direction   0xc0上下反置 0xc8正常
	OLED_WrCmd(0xa6);//--set normal display
	OLED_WrCmd(0xa8);//--set multiplex ratio(1 to 64)
	OLED_WrCmd(0x3f);//--1/64 duty
	OLED_WrCmd(0xd3);//-set display offset	Shift Mapping RAM Counter (0x00~0x3F)
	OLED_WrCmd(0x00);//-not offset
	OLED_WrCmd(0xd5);//--set display clock divide ratio/oscillator frequency
	OLED_WrCmd(0x80);//--set divide ratio, Set Clock as 100 Frames/Sec
	OLED_WrCmd(0xd9);//--set pre-charge period
	OLED_WrCmd(0xf1);//Set Pre-Charge as 15 Clocks & Discharge as 1 Clock
	OLED_WrCmd(0xda);//--set com pins hardware configuration
	OLED_WrCmd(0x12);
	OLED_WrCmd(0xdb);//--set vcomh
	OLED_WrCmd(0x40);//Set VCOM Deselect Level
	OLED_WrCmd(0x20);//-Set Page Addressing Mode (0x00/0x01/0x02)
	OLED_WrCmd(0x02);//
	OLED_WrCmd(0x8d);//--set Charge Pump enable/disable
	OLED_WrCmd(0x14);//--set(0x10) disable
	OLED_WrCmd(0xa4);// Disable Entire Display On (0xa4/0xa5)
	OLED_WrCmd(0xa6);// Disable Inverse Display On (0xa6/a7) 
	OLED_WrCmd(0xaf);//--turn on oled panel
	
#else //1106的初始化
	
	OLED_WrCmd(0xAE);    /*display off*/
	OLED_WrCmd(0x02);    /*set lower column address*/
	OLED_WrCmd(0x10);    /*set higher column address*/
	OLED_WrCmd(0x40);    /*set display start line*/
	OLED_WrCmd(0xB0);    /*set page address*/
	OLED_WrCmd(0x81);    /*contract control*/
	OLED_WrCmd(Brightness);    /*128*/
	OLED_WrCmd(0xA1);    /*set segment remap*/
	OLED_WrCmd(0xA6);    /*normal / reverse*/
	OLED_WrCmd(0xA8);    /*multiplex ratio*/
	OLED_WrCmd(0x3F);    /*duty = 1/32*/
	OLED_WrCmd(0xad);    /*set charge pump enable*/
	OLED_WrCmd(0x8b);    /*    0x8a    ??VCC   */
	OLED_WrCmd(0x30);    /*0X30---0X33  set VPP   9V liangdu!!!!*/
	OLED_WrCmd(0xC8);    /*Com scan direction*/
	OLED_WrCmd(0xD3);    /*set display offset*/
	OLED_WrCmd(0x00);    /*   0x20  */
	OLED_WrCmd(0xD5);    /*set osc division*/
	OLED_WrCmd(0x80);    
	OLED_WrCmd(0xD9);    /*set pre-charge period*/
	OLED_WrCmd(0x1f);    /*0x22*/
	OLED_WrCmd(0xDA);    /*set COM pins*/
	OLED_WrCmd(0x12);		 //0x02 -- duanhang xianshi,0x12 -- lianxuhang xianshi!!!!!!!!!
	OLED_WrCmd(0xdb);    /*set vcomh*/
	OLED_WrCmd(0x40);     
	OLED_WrCmd(0xAF);    /*display ON*/
	
#endif
	
	OLED_Fill(0x00);  //初始清屏
	OLED_SetPos(0,0); 	
} 

//--------------------------------------------------------------
// Prototype      : void OLED_DrawPoint(unsigned char x,unsigned char y,unsigned char t)
// Calls          : 
// Parameters     : x:0~127,y:0~63,t:1 填充 0,清空
// Description    : 画点
//--------------------------------------------------------------
/*void OLED_DrawPoint(unsigned char x,unsigned char y,unsigned char t)
{
	unsigned char pos,bx,temp=0;
	unsigned char OLED_GRAM[128][8];
	
	if(x>127||y>63)
		return;//超出范围了
	pos = y/8;
	bx = y%8;
	temp = 1<<bx;
	if(t)
		OLED_GRAM[x][pos] |= temp;
	else
		OLED_GRAM[x][pos] &= ~temp;
}*/

//--------------------------------------------------------------
// Prototype      : void OLED_ShowChar(unsigned char x, unsigned char y, unsigned char ch[], unsigned char TextSize)
// Calls          : 
// Parameters     : x,y -- 起始点坐标(x:0~127, y:0~7); ch[]:要显示的字符串; TextSize:1 -- 6*8点阵,2 -- 8*16点阵
// Description    : 显示不同字体的ASCII字符
//--------------------------------------------------------------
/*void OLED_Char(unsigned char x, unsigned char y, unsigned char ch)
{
	unsigned char c = 0,i = 0;
	while(ch != '\0')
	{
		c = ch - 32;
		OLED_SetPos(x,y);
		for(i=0;i<6;i++)
			OLED_WrDat(F6x8[c][i]);
	}
}*/

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
				OLED_SetPos(x,y);
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
				OLED_SetPos(x,y);
				for(i=0;i<8;i++)
					OLED_WrDat(F8X16[c*16+i]);
				OLED_SetPos(x,y+1);
				for(i=0;i<8;i++)
					OLED_WrDat(F8X16[c*16+i+8]);
				x += 8;
				j++;
			}
		}break;
	}
}

//--------------------------------------------------------------
// Prototype      : void OLED_ShowCN(unsigned char x, unsigned char y, unsigned char N)
// Calls          : 
// Parameters     : x,y -- 起始点坐标(x:0~127, y:0~7); N:汉字在codetab.h中的索引
// Description    : 显示codetab.h中的汉字,16*16点阵
//--------------------------------------------------------------
void OLED_ShowCN(unsigned char x, unsigned char y, unsigned char N)
{
	unsigned char wm=0;
	unsigned int  adder=32*N;
	OLED_SetPos(x , y);
	for(wm = 0;wm < 16;wm++)
	{
		OLED_WrDat(F16x16[adder]);
		adder += 1;
	}
	OLED_SetPos(x,y + 1);
	for(wm = 0;wm < 16;wm++)
	{
		OLED_WrDat(F16x16[adder]);
		adder += 1;
	}
}

//--------------------------------------------------------------
// Prototype      : void OLED_DrawBMP(unsigned char x0,unsigned char y0,unsigned char x1,unsigned char y1,unsigned char BMP[]);
// Calls          : 
// Parameters     : x0,y0 -- 起始点坐标(x0:0~127, y0:0~7); x1,y1 -- 起点对角线(结束点)的坐标(x1:1~128,y1:1~8)
// Description    : 显示BMP位图
//--------------------------------------------------------------
void OLED_DrawBMP(unsigned char x0,unsigned char y0,unsigned char x1,unsigned char y1,unsigned char BMP[])
{
	unsigned int j=0;
	unsigned char x,y;

  if(y1%8==0)
		y = y1/8;
  else
		y = y1/8 + 1;
	for(y=y0;y<y1;y++)
	{
		OLED_SetPos(x0,y);
    for(x=x0;x<x1;x++)
		{
			OLED_WrDat(BMP[j++]);
		}
	}
}
