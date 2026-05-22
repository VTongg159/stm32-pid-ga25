#ifndef ENCODER_H
#define ENCODER_H
#include "stm32f411xe.h"

void Encoder_Init(void);
int16_t Encoder_GetDelta(void);

#endif
