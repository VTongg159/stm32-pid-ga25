#ifndef UART_H
#define UART_H

#include "stm32f411xe.h"
#include <stdint.h>

// Khoi tao USART2
void UART_Init(void);

// gui 1 ky tu
void UART_SendChar(char c);

// Gui 1 chuoi ki tu
void UART_SendString(const char* str);

// Gửi mot so nguyen 32 bit
void UART_SendNumber(int32_t num);

#endif
