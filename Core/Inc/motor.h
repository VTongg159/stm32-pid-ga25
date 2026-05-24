#ifndef MOTOR_H
#define MOTOR_H

#include "stm32f411xe.h"
#include <stdint.h>

void Motor_Init(void);
void Motor_SetSpeed(int16_t speed);
void Motor_Brake(void);

#endif
