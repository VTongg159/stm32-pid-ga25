#ifndef I2C_LCD_H
#define I2C_LCD_H
#include <stdint.h>

void I2C1_Init(void);
void LCD_Init(void);
void LCD_Print(char *str);
void LCD_SetCursor(uint8_t row, uint8_t col);

#endif // I2C_LCD_H
