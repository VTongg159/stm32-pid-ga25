#include "buttons.h"
#include "stm32f411xe.h"
#include "globals.h"
#include "system.h" // Dung system.h thay vi systick.h

volatile uint32_t last_button_press = 0;

void EXTI1_IRQHandler(void) {
    if (EXTI->PR & (1U << 1)) {
        EXTI->PR |= (1U << 1);
        uint32_t current_time = Get_Tick();
        if ((current_time - last_button_press) > 50) {
            system_run = 1;
            buzzer_flag = 1;
            last_button_press = current_time;
        }
    }
}

void EXTI2_IRQHandler(void) {
    if (EXTI->PR & (1U << 2)) {
        EXTI->PR |= (1U << 2);
        uint32_t current_time = Get_Tick();
        if ((current_time - last_button_press) > 50) {
            system_run = 0;
            buzzer_flag = 1;
            last_button_press = current_time;
        }
    }
}

void EXTI4_IRQHandler(void) {
    if (EXTI->PR & (1U << 4)) {
        EXTI->PR |= (1U << 4);
        uint32_t current_time = Get_Tick();
        if ((current_time - last_button_press) > 50) {
            if (system_run) target_rpm += 10.0f; // Cong 10 RPM
            buzzer_flag = 1;
            last_button_press = current_time;
        }
    }
}

void EXTI9_5_IRQHandler(void) {
    if (EXTI->PR & (1U << 5)) {
        EXTI->PR |= (1U << 5);
        uint32_t current_time = Get_Tick();
        if ((current_time - last_button_press) > 50) {
            if (system_run && target_rpm >= 10.0f) target_rpm -= 10.0f;
            buzzer_flag = 1;
            last_button_press = current_time;
        }
    }
}

void Buttons_EXTI_Init(void) {
    RCC->AHB1ENR |= (1U << 1);
    RCC->APB2ENR |= (1U << 14);

    GPIOB->MODER &= ~((3U << (1 * 2)) | (3U << (2 * 2)) | (3U << (4 * 2)) | (3U << (5 * 2)));
    GPIOB->PUPDR &= ~((3U << (1 * 2)) | (3U << (2 * 2)) | (3U << (4 * 2)) | (3U << (5 * 2)));
    GPIOB->PUPDR |=  ((1U << (1 * 2)) | (1U << (2 * 2)) | (1U << (4 * 2)) | (1U << (5 * 2)));

    SYSCFG->EXTICR[0] &= ~((0xFU << 4) | (0xFU << 8));
    SYSCFG->EXTICR[0] |=  ((1U << 4) | (1U << 8));
    SYSCFG->EXTICR[1] &= ~((0xFU << 0) | (0xFU << 4));
    SYSCFG->EXTICR[1] |=  ((1U << 0) | (1U << 4));

    EXTI->IMR |= (1U << 1) | (1U << 2) | (1U << 4) | (1U << 5);
    EXTI->FTSR |= (1U << 1) | (1U << 2) | (1U << 4) | (1U << 5);
    EXTI->RTSR &= ~((1U << 1) | (1U << 2) | (1U << 4) | (1U << 5));

    // DAT MUC UU TIEN MUC 2 (De UART MUC 1 duoc uu tien cao nhat)
    NVIC_SetPriority(EXTI1_IRQn, 2);
    NVIC_SetPriority(EXTI2_IRQn, 2);
    NVIC_SetPriority(EXTI4_IRQn, 2);
    NVIC_SetPriority(EXTI9_5_IRQn, 2);

    NVIC->ISER[0] |= (1U << 7) | (1U << 8) | (1U << 10) | (1U << 23);
}
