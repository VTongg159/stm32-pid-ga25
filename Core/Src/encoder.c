#include "encoder.h"

void Encoder_Init(void) {
    // 1. Bật Clock cho GPIOA và Timer 2
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    // 2. Cấu hình PA0, PA1 sang chức năng thay thế (AF1 - TIM2_CH1/TIM2_CH2)
    GPIOA->MODER &= ~((3U << (0 * 2)) | (3U << (1 * 2)));
    GPIOA->MODER |=  ((2U << (0 * 2)) | (2U << (1 * 2))); // AF Mode

    GPIOA->AFR[0] &= ~((0xFU << (0 * 4)) | (0xFU << (1 * 4))); // AFR[0] (Low Register)
    GPIOA->AFR[0] |=  ((1U << (0 * 4)) | (1U << (1 * 4)));  // AF1

    // 3. Cấu hình Timer ở Chế độ Encoder Mode 3 (Đếm trên cả 2 cạnh kênh A và B)
    // Thanh ghi SMCR (Slave Mode Control Register): SMS = 011 (Encoder mode 3)
    TIM2->SMCR &= ~TIM_SMCR_SMS;
    TIM2->SMCR |=  (3U << TIM_SMCR_SMS_Pos);

    // 4. Cấu hình phân cực và bộ lọc nhiễu (tùy chọn) trong CCMR1
    // CC1S = 01 (Kênh TI1 kết nối vào IC1), CC2S = 01 (Kênh TI2 kết nối vào IC2)
    TIM2->CCMR1 &= ~(TIM_CCMR1_CC1S | TIM_CCMR1_CC2S);
    TIM2->CCMR1 |=  ((1U << TIM_CCMR1_CC1S_Pos) | (1U << TIM_CCMR1_CC2S_Pos));

    // 5. Thiết lập giá trị Auto-reload tối đa (Do là Timer 32-bit, ta dùng tối đa để tránh tràn xung ngắn hạn)
    TIM2->ARR = 0xFFFFFFFF;
    TIM2->CNT = 0; // Reset bộ đếm về 0

    // 6. Kích hoạt Timer 2
    TIM2->CR1 |= TIM_CR1_CEN;
}

int16_t Encoder_GetDelta(void) {
    // Đọc giá trị counter hiện tại
    uint32_t current_cnt = TIM2->CNT;

    // Ép kiểu sang int16_t để tự động xử lý hiện tượng tràn/giảm số đếm âm một cách chính xác
    int16_t delta = (int16_t)(current_cnt);

    // Reset bộ đếm về 0 để phục vụ cho chu kỳ lấy mẫu tiếp theo
    TIM2->CNT = 0;

    return delta;
}
