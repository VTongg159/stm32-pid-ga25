#include "hmi_gpio.h"
#include "stm32f411xe.h"
#include "globals.h"

void HMI_GPIO_Init(void) {
    // Cấp clock cho Port A (Bit 0 của AHB1ENR)
    RCC->AHB1ENR |= (1U << 0);

    // Cấu hình Output cho PA4, PA5, PA6, PA7
    GPIOA->MODER &= ~((3U << (4 * 2)) | (3U << (5 * 2)) | (3U << (6 * 2)) | (3U << (7 * 2)));
    GPIOA->MODER |=  ((1U << (4 * 2)) | (1U << (5 * 2)) | (1U << (6 * 2)) | (1U << (7 * 2)));

    // Push-Pull
    GPIOA->OTYPER &= ~((1U << 4) | (1U << 5) | (1U << 6) | (1U << 7));

    // Low speed
    GPIOA->OSPEEDR &= ~((3U << (4 * 2)) | (3U << (5 * 2)) | (3U << (6 * 2)) | (3U << (7 * 2)));

    // Tắt tất cả (Reset kéo xuống mức 0)
    GPIOA->BSRR = (1U << (4 + 16)) | (1U << (5 + 16)) | (1U << (6 + 16)) | (1U << (7 + 16));
}

void LED_Update_State(void) {
    if (error_flag != 0) {
        // Bật Đỏ (PA6), Tắt Xanh (PA4), Tắt Vàng (PA5)
        GPIOA->BSRR = (1U << 6) | (1U << (4 + 16)) | (1U << (5 + 16));
    }
    else if (system_run == 1) {
        // Bật Xanh (PA4), Tắt Vàng (PA5), Tắt Đỏ (PA6)
        GPIOA->BSRR = (1U << 4) | (1U << (5 + 16)) | (1U << (6 + 16));
    }
    else {
        // Bật Vàng (PA5), Tắt Xanh (PA4), Tắt Đỏ (PA6)
        GPIOA->BSRR = (1U << 5) | (1U << (4 + 16)) | (1U << (6 + 16));
    }
}

void Buzzer_Set(uint8_t state) {
    if(state) GPIOA->BSRR = (1U << 7);
    else      GPIOA->BSRR = (1U << (7 + 16));
}
