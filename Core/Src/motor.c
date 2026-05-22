#include "motor.h"

void Motor_Init(void) {
    // 1. Bật Clock cho GPIOA và Timer 1
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;

    // 2. Cấu hình PA8 thành chức năng thay thế (AF1 - TIM1_CH1)
    GPIOA->MODER &= ~(3U << (8 * 2));
    GPIOA->MODER |=  (2U << (8 * 2)); // AF mode
    GPIOA->AFR[1] &= ~(0xFU << (0 * 4)); // PA8 thuộc AFR[1] (High Register)
    GPIOA->AFR[1] |=  (1U << (0 * 4));  // AF1

    // 3. Cấu hình PA9 và PA10 thành Output thường (Điều khiển chiều IN1, IN2)
    GPIOA->MODER &= ~((3U << (9 * 2)) | (3U << (10 * 2)));
    GPIOA->MODER |=  ((1U << (9 * 2)) | (1U << (10 * 2))); // Output mode (01)

    // 4. Cấu hình Timer 1 sinh xung PWM Tần số 20 kHz
    TIM1->PSC = 0;             // Không chia tần (Prescaler = 0), Timer chạy ở 100MHz
    TIM1->ARR = 4999;          // Tần số = 100MHz / (4999 + 1) = 20kHz
    TIM1->CCR1 = 0;            // Mặc định Duty Cycle = 0%

    // 5. Cấu hình Output Compare Mode cho Channel 1: PWM Mode 1 (Thanh ghi CCMR1)
    TIM1->CCMR1 &= ~TIM_CCMR1_OC1M;
    TIM1->CCMR1 |=  (6U << TIM_CCMR1_OC1M_Pos); // 6: PWM mode 1 (Đầu ra tích cực khi CNT < CCR1)
    TIM1->CCMR1 |= TIM_CCMR1_OC1PE;            // Bật bộ đệm Preload cho CCR1

    // 6. Cho phép xuất đầu ra Channel 1 (CCER) và bật ngõ ra chính (MOE - Main Output Enable)
    TIM1->CCER |= TIM_CCER_CC1E;
    TIM1->BDTR |= TIM_BDTR_MOE;

    // 7. Kích hoạt Timer 1
    TIM1->CR1 |= TIM_CR1_ARPE | TIM_CR1_CEN; // Bật Auto-reload preload và Enable Counter
}

void Motor_SetSpeed(int16_t speed) {
    // Giới hạn dải băm xung trong khoảng giá trị ARR (-4999 đến 4999)
    if (speed > 4999)  speed = 4999;
    if (speed < -4999) speed = -4999;

    if (speed >= 0) {
        // Quay thuận: IN1 = 1, IN2 = 0
        GPIOA->ODR |=  (1U << 9);
        GPIOA->ODR &= ~(1U << 10);
        TIM1->CCR1 = speed;
    } else {
        // Quay nghịch: IN1 = 0, IN2 = 1
        GPIOA->ODR &= ~(1U << 9);
        GPIOA->ODR |=  (1U << 10);
        TIM1->CCR1 = -speed; // Duty cycle luôn dương
    }
}

void Motor_Brake(void) {
    // Dừng khẩn cấp: IN1 = 0, IN2 = 0
    GPIOA->ODR &= ~((1U << 9) | (1U << 10));
    TIM1->CCR1 = 0;
}
