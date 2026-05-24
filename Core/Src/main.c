#include "stm32f411xe.h"
#include <stdint.h>

// include thu vien
#include "motor.h"
#include "encoder.h"
#include "pid.h"
#include "uart.h"

// dinh nghia tham so cua he thong
#define ENCODER_CPR     1496.0f //so xung trong 1 vong encoder
#define SAMPLE_TIME_MS  10      // chu ky lay mau ms
#define SAMPLE_TIME_SEC 0.01f   // chu ky lay mau s

// bien global
volatile uint32_t tick_ms = 0; // dem thoi gian thuc


// struct pid
PID_Controller_t speed_pid;
float current_rpm = 0.0f;
float target_rpm = 100.0f; // set point
int16_t pwm_output = 0;

// khai bao ham
void SystemClock_Config_100MHz(void);
void SysTick_Init(void);
uint8_t Check_Sample_Time(void);

int main(void)
{
    // khoi tao clock he thong
    SystemClock_Config_100MHz();

    // khoi tao ngat dinh thoi
    SysTick_Init();

    // khoi tao ngoai vi
    Motor_Init();
    Encoder_Init();
    UART_Init();

    // khoi tao tham so pid
    PID_Init(&speed_pid, 1.5f, 0.8f, 0.05f, 999.0f, -999.0f);
    speed_pid.setpoint = target_rpm;

    //gui thong bao cho he thong
    UART_SendString("\r\n STM32 PID MOTOR CONTROL BOOTED \r\n");

    while (1)
    {
        // vong lap thuc thi moiI 10ms (100Hz)
        if (Check_Sample_Time())
        {
            // doc cam bien
            int32_t delta = Encoder_GetDelta();

            // tinh rpm
            current_rpm = ((float)delta / ENCODER_CPR) * (60.0f / SAMPLE_TIME_SEC);

            // tinh toan pid
            pwm_output = (int16_t)PID_Compute(&speed_pid, current_rpm, SAMPLE_TIME_SEC);

            // xuat tin hieu ra cau H
            Motor_SetSpeed(pwm_output);

            // gui du lieu ra uart
            UART_SendNumber((int32_t)target_rpm);
            UART_SendChar(',');
            UART_SendNumber((int32_t)current_rpm);
            UART_SendChar(',');
            UART_SendNumber((int32_t)pwm_output);
            UART_SendString("\r\n");
        }
    }
}

// cau hinh clock 100MHZ
void SystemClock_Config_100MHz(void)
{
    //  Internal high-speed clock enable (16MHz)
    RCC->CR |= (1 << 0);
    while (!(RCC->CR & (1 << 1))); // HSI oscillator ready

    // Power interface clock enable
    RCC->APB1ENR |= (1 << 28);

    // cau hinh Flash Latency: 100MHz o 3.3V can 3 Wait States
    FLASH->ACR = (1 << 8) | (1 << 9) | (1 << 10) | (3 << 0);

    // cau hinh Prescalers: AHB = /1 (100MHz); APB1 = /2 (50MHz); APB2 = /1 (100MHz)
    RCC->CFGR |= (0 << 4) | (4 << 10) | (0 << 13);

    // cau hinh PLL
    RCC->PLLCFGR = (8 << 0) | (100 << 6) | (0 << 16) | (0 << 22);

    // bat PLL
    RCC->CR |= (1 << 24);
    // doi den khi Main PLL on dinh
    while (!(RCC->CR & (1 << 25)));

    // chuyen nguon System Clock sang PLL
    RCC->CFGR &= ~(3 << 0);
    RCC->CFGR |= (2 << 0);

    //doi den khi phan cung xac nhan chuyen sang PLL thanh cong
    while ((RCC->CFGR & (3 << 2)) != (2 << 2));
}

//cau hinh systick
void SysTick_Init(void)
{
    // Clock = 100MHz
    SysTick->LOAD = 100000 - 1;
    SysTick->VAL = 0;
    // bat SysTick, dung System Clock va cho phep ngat
    SysTick->CTRL = (1 << 2) | (1 << 1) | (1 << 0);
}

// ham phuc vu ngat
void SysTick_Handler(void)
{
    tick_ms++;
}

// ham kiem tra dinh thoi
uint8_t Check_Sample_Time(void)
{
    static uint32_t last_tick = 0;
    if ((tick_ms - last_tick) >= SAMPLE_TIME_MS)
    {
        last_tick += SAMPLE_TIME_MS;
        return 1;
    }
    return 0;
}
