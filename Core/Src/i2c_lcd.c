#include "i2c_lcd.h"
#include "stm32f411xe.h"
#include "system.h"

void I2C1_Init(void) {
    RCC->AHB1ENR |= (1U << 1);
    RCC->APB1ENR |= (1U << 21);

    GPIOB->MODER &= ~((3U << (6 * 2)) | (3U << (7 * 2)));
    GPIOB->MODER |=  ((2U << (6 * 2)) | (2U << (7 * 2)));
    GPIOB->OTYPER |= (1U << 6) | (1U << 7);
    GPIOB->OSPEEDR |= ((3U << (6 * 2)) | (3U << (7 * 2)));
    GPIOB->PUPDR &= ~((3U << (6 * 2)) | (3U << (7 * 2)));
    GPIOB->PUPDR |=  ((1U << (6 * 2)) | (1U << (7 * 2)));
    GPIOB->AFR[0] &= ~((0xFU << 24) | (0xFU << 28));
    GPIOB->AFR[0] |=  ((4U << 24) | (4U << 28));

    I2C1->CR1 |=  (1U << 15);
    I2C1->CR1 &= ~(1U << 15);

    I2C1->CR2 = (50U << 0);
    I2C1->CCR = (1U << 15) | 42; // Bit 15: Fast Mode, 42: Hệ số chia cho 400kHz
    I2C1->TRISE = 16;

    I2C1->CR1 |= (1U << 0);
}

void I2C1_Start(void) {
    I2C1->CR1 |= (1U << 8);
    uint32_t t = Get_Tick();
    while (!(I2C1->SR1 & (1U << 0))) if ((Get_Tick() - t) > 5) return;
}

void I2C1_WriteAddress(uint8_t address) {
    I2C1->DR = address;
    uint32_t t = Get_Tick();
    while (!(I2C1->SR1 & (1U << 1))) if ((Get_Tick() - t) > 5) return;
    volatile uint32_t temp = I2C1->SR1;
    temp = I2C1->SR2;
    (void)temp;
}

void I2C1_WriteData(uint8_t data) {
    uint32_t t = Get_Tick();
    while (!(I2C1->SR1 & (1U << 7))) if ((Get_Tick() - t) > 5) return;
    I2C1->DR = data;
    t = Get_Tick();
    while (!(I2C1->SR1 & (1U << 2))) if ((Get_Tick() - t) > 5) return;
}

void I2C1_Stop(void) {
    I2C1->CR1 |= (1U << 9);
}

void I2C_Send_PCF8574(uint8_t data) {
    I2C1_Start();
    I2C1_WriteAddress(0x4E);
    I2C1_WriteData(data);
    I2C1_Stop();
}

// XOA BO DELAY_MS TRONG NAY DE CHONG BLOCK PID 10MS CUA BAN
void LCD_Write_Nibble(uint8_t nibble, uint8_t rs) {
    uint8_t data = nibble | 0x08 | rs;

    I2C_Send_PCF8574(data | 0x04);
    I2C_Send_PCF8574(data & ~0x04);
}

void LCD_SendCommand(uint8_t cmd) {
    LCD_Write_Nibble(cmd & 0xF0, 0);
    LCD_Write_Nibble((cmd << 4) & 0xF0, 0);
}

void LCD_SendData(uint8_t data) {
    LCD_Write_Nibble(data & 0xF0, 1);
    LCD_Write_Nibble((data << 4) & 0xF0, 1);
}

void LCD_Init(void) {
    Delay_ms(50);
    LCD_Write_Nibble(0x30, 0); Delay_ms(5);
    LCD_Write_Nibble(0x30, 0); Delay_ms(1);
    LCD_Write_Nibble(0x30, 0); Delay_ms(1);
    LCD_Write_Nibble(0x20, 0); Delay_ms(5);
    LCD_SendCommand(0x28);
    LCD_SendCommand(0x0C);
    LCD_SendCommand(0x06);
    LCD_SendCommand(0x01);
    Delay_ms(5);
}

void LCD_Print(char *str) {
    while (*str) LCD_SendData(*str++);
}

void LCD_SetCursor(uint8_t row, uint8_t col) {
    uint8_t address = (row == 0) ? (0x80 + col) : (0xC0 + col);
    LCD_SendCommand(address);
}
