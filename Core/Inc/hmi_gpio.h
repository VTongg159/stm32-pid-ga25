#ifndef HMI_GPIO_H
#define HMI_GPIO_H
#include <stdint.h>

void HMI_GPIO_Init(void);
void LED_Update_State(void);
void Buzzer_Set(uint8_t state);

#endif // HMI_GPIO_H
