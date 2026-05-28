#ifndef SYSTEM_H
#define SYSTEM_H

#include <stdint.h>

// cau hinh clock he thong len 100MHz tu nguon noi HSI
void SystemClock_Config_100MHz(void);

// khoi tao ngat dinh thoi SysTick 1ms
void SysTick_Init(void);

// ham kiem tra chu ky lay mau (don vi: ms)
uint8_t Check_Sample_Time(uint32_t sample_time_ms);

#endif /* SYSTEM_H */
