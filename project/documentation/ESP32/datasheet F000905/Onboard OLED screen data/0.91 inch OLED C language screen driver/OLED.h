#ifndef __OLED_H__
#define __OLED_H__

#include "reg52.h"

#define	Brightness	0xCF 
#define X_WIDTH 		128
#define Y_WIDTH 		64

void IIC_Start();
void IIC_Stop();
void Write_IIC_Byte(unsigned char IIC_Byte);
void OLED_WrDat(unsigned char IIC_Data);
void OLED_WrCmd(unsigned char IIC_Command);
void OLED_Set_Pos(unsigned char x, unsigned char y);
void OLED_Fill(unsigned char bmp_dat);
void OLED_Init(void);
void OLED_ShowInt(unsigned char x, unsigned char y, unsigned int Num);
void OLED_ShowChar(unsigned char x, unsigned char y, unsigned char ch[], unsigned char TextSize);
void OLED_ShowCN(unsigned char x, unsigned char y, unsigned char N);

#endif