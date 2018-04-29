#ifndef __OLED_H__
#define __OLED_H__

#include "reg52.h"

#define	Brightness	0xCF  //亮度 0x00~0xff
#define X_WIDTH 		132   //SH1106的芯片请将这里修改成132，SSD1306的将这里改成128
#define Y_WIDTH 		64
#define IIC_SPI_TYPE 		0    // 1 -- SPI方式; 0 -- IIC方式
#define CHIP_IS_SSD1306 1    //1 - SSD1306; 0 -- SH1106

void IIC_Start(void);
void IIC_Stop(void);
void Write_IIC_Byte(unsigned char IIC_Byte);
void OLED_WrDat(unsigned char dat);
void OLED_WrCmd(unsigned char cmd);
void OLED_SetPos(unsigned char x, unsigned char y);
void OLED_Fill(unsigned char bmp_dat);
void OLED_CLS(void);
void OLED_ON(void);
void OLED_OFF(void);
void OLED_Init(void);
//void OLED_DrawPoint(unsigned char x,unsigned char y,unsigned char t);
//void OLED_Char(unsigned char x, unsigned char y, unsigned char ch);
void OLED_ShowChar(unsigned char x, unsigned char y, unsigned char ch[], unsigned char TextSize);
void OLED_ShowCN(unsigned char x, unsigned char y, unsigned char N);
void OLED_DrawBMP(unsigned char x0,unsigned char y0,unsigned char x1,unsigned char y1,unsigned char BMP[]);

#endif