#include "stm32f411xe.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// Module cua ban
#include "system.h"
#include "motor.h"
#include "encoder.h"
#include "pid.h"
#include "uart.h"

// Module cua Thanh vien 2
#include "globals.h"
#include "hmi_gpio.h"
#include "buttons.h"
#include "i2c_lcd.h"

#define ENCODER_CPR     1562.0f
#define SAMPLE_TIME_MS  10
#define SAMPLE_TIME_SEC 0.01f

// Khai bao bien toan cuc (Dung chung qua globals.h)
volatile uint8_t system_run  = 0;
volatile float   target_rpm  = 100.0f;
volatile uint8_t buzzer_flag = 0;
volatile uint8_t error_flag  = 0;

PID_Controller_t speed_pid;
float current_rpm  = 0.0f;
int16_t pwm_output = 0;

static void Process_RX_Command(char *cmd) {
    float value = 0.0f;
    char *eq = strchr(cmd, '=');
    if (eq == NULL) return;

    value = strtof(eq + 1, NULL);

    if      (strncmp(cmd, "KP", 2) == 0) { speed_pid.Kp = value; }
    else if (strncmp(cmd, "KI", 2) == 0) { speed_pid.Ki = value; }
    else if (strncmp(cmd, "KD", 2) == 0) { speed_pid.Kd = value; }
    else if (strncmp(cmd, "SP", 2) == 0) {
        target_rpm = value;
        if(value > 0) system_run = 1; else system_run = 0; // C# auto start/stop
    }
}

int main(void)
{
    // Khoi tao
    SystemClock_Config_100MHz();
    SysTick_Init();
    Motor_Init();
    Encoder_Init();
    UART_Init();
    UART_RX_Interrupt_Enable();

    HMI_GPIO_Init();
    Buttons_EXTI_Init();
    I2C1_Init();
    LCD_Init();

    PID_Init(&speed_pid, 1.5f, 0.8f, 0.05f, 999.0f, -999.0f);

    LCD_SetCursor(0, 0);
    LCD_Print("STM32 PID MOTOR ");
    Delay_ms(1000);
    LCD_SendCommand(0x01);

    UART_SendString("\r\n STM32 PID MOTOR CONTROL BOOTED \r\n");

    uint32_t last_lcd_tick = Get_Tick();
    uint32_t buzzer_start  = 0;
    uint8_t  buzzer_active = 0;

    while (1)
    {
        // 1. Kiem tra lenh C#
        if (UART_IsCommandReady()) {
            char cmd_copy[50];
            UART_GetCommand(cmd_copy);
            Process_RX_Command(cmd_copy);
        }

        // 2. Coi Buzzer khong giam long CPU
        if (buzzer_flag && !buzzer_active) {
            Buzzer_Set(1);
            buzzer_start = Get_Tick();
            buzzer_active = 1;
            buzzer_flag = 0;
        }
        if (buzzer_active && (Get_Tick() - buzzer_start) >= 50) {
            Buzzer_Set(0);
            buzzer_active = 0;
        }

        // 3. Vong lap loi: Dieu khien PID dung 10ms
        // (Ham nay cua ban van duoc giu ton ton tuyet doi)
        if (Check_Sample_Time(SAMPLE_TIME_MS)) // Trong system.c da ep 10ms
        {
            LED_Update_State();

            int32_t delta = Encoder_GetDelta();
            current_rpm = ((float)delta / ENCODER_CPR) * (60.0f / SAMPLE_TIME_SEC);

            if(system_run == 1 && error_flag == 0) {
                speed_pid.setpoint = target_rpm;
                pwm_output = (int16_t)PID_Compute(&speed_pid, current_rpm, SAMPLE_TIME_SEC);
                Motor_SetSpeed(pwm_output);
            } else {
                Motor_SetSpeed(0);
                speed_pid.integral_sum = 0.0f; // Reset cho PID truyen thong
                pwm_output = 0;
            }

            UART_SendNumber((int32_t)target_rpm);
            UART_SendChar(',');
            UART_SendNumber((int32_t)current_rpm);
            UART_SendChar(',');
            UART_SendNumber((int32_t)pwm_output);
            UART_SendString("\r\n");
        }

        // 4. Man hinh LCD doc lap chay moi 200ms
        if ((Get_Tick() - last_lcd_tick) >= 200) {
            last_lcd_tick += 200;

            char lcd_buf[17];
            LCD_SetCursor(0, 0);
            sprintf(lcd_buf, "SP:%4d %s", (int)target_rpm, (system_run ? "RUN " : "STOP"));
            LCD_Print(lcd_buf);

            LCD_SetCursor(1, 0);
            sprintf(lcd_buf, "AC:%4d P:%4d", (int)current_rpm, pwm_output);
            LCD_Print(lcd_buf);
        }
    }
}
