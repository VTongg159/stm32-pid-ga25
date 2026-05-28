#ifndef GLOBALS_H
#define GLOBALS_H
#include <stdint.h>

// Cac bien toan cuc dung chung giua cac module
extern volatile uint8_t  system_run;
extern volatile float    target_rpm;   // Kieu float de khop voi PID
extern volatile uint8_t  buzzer_flag;
extern volatile uint8_t  error_flag;

#endif // GLOBALS_H
