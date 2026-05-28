#include "stm32f411xe.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "system.h"
#include "motor.h"
#include "encoder.h"
#include "pid.h"
#include "uart.h"


#define ENCODER_CPR     1562.0f
#define SAMPLE_TIME_MS  10
#define SAMPLE_TIME_SEC 0.01f


PID_Controller_t speed_pid;
float current_rpm  = 0.0f;
float target_rpm   = 100.0f;
int16_t pwm_output = 0;

// ham xu ly lenh nhan duoc (goi tu main, khong goi trong ISR)
static void Process_RX_Command(char *cmd)
{
    float value = 0.0f;
    char *eq = strchr(cmd, '=');
    if (eq == NULL) return;

    value = strtof(eq + 1, NULL);

    if      (strncmp(cmd, "KP", 2) == 0) { speed_pid.Kp = value; }
    else if (strncmp(cmd, "KI", 2) == 0) { speed_pid.Ki = value; }
    else if (strncmp(cmd, "KD", 2) == 0) { speed_pid.Kd = value; }
    else if (strncmp(cmd, "SP", 2) == 0) { target_rpm   = value; }
}

int main(void)
{
    SystemClock_Config_100MHz();
    SysTick_Init();

    Motor_Init();
    Encoder_Init();
    UART_Init();
    UART_RX_Interrupt_Enable();

    // sau khi UART_Init xong moi bat ngat RX
    UART_RX_Interrupt_Enable();

    PID_Init(&speed_pid, 1.5f, 0.8f, 0.05f, 999.0f, -999.0f);
    speed_pid.setpoint = target_rpm;

    UART_SendString("\r\n STM32 PID MOTOR CONTROL BOOTED \r\n");

    while (1)
        {
            // Kích hoạt khi UART nhận xong 1 chuỗi lệnh
            if (UART_IsCommandReady())
            {
                char cmd_copy[50];
                UART_GetCommand(cmd_copy); // Lấy data ra an toàn
                Process_RX_Command(cmd_copy);
            }

            // Kích hoạt chuẩn xác mỗi 10ms
            if (Check_Sample_Time(SAMPLE_TIME_MS))
            {
                speed_pid.setpoint = target_rpm;

                int32_t delta = Encoder_GetDelta();
                current_rpm = ((float)delta / ENCODER_CPR) * (60.0f / SAMPLE_TIME_SEC);

                pwm_output = (int16_t)PID_Compute(&speed_pid, current_rpm, SAMPLE_TIME_SEC);
                Motor_SetSpeed(pwm_output);

                UART_SendNumber((int32_t)target_rpm);
                UART_SendChar(',');
                UART_SendNumber((int32_t)current_rpm);
                UART_SendChar(',');
                UART_SendNumber((int32_t)pwm_output);
                UART_SendString("\r\n");
            }
        }
    }

