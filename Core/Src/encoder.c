#include "encoder.h"

void Encoder_Init(void) {
    // cap xung clock cho GPIOA va TIM2
    RCC->AHB1ENR |= (1 << 0);
    RCC->APB1ENR |= (1 << 0);

    //cau hinh che do alternate cho PA0, PA1: FA1
    //PA0
    GPIOA->MODER &= ~(1 << 0 );
    GPIOA->MODER |=  (1 << 1);
    GPIOA->AFR[0] &= ~(0xF << 0);
    GPIOA->AFR[0] |= (1 << 0);
    //PA1
    GPIOA->MODER &= ~(1 << 2 );
    GPIOA->MODER |=  (1 << 3);
    GPIOA->AFR[0] &= ~(0xF << 4);
    GPIOA->AFR[0] |= (1 << 4);


    // cau hinh encoder mode 3
    TIM2->SMCR &= ~(0x7 << 0);
    TIM2->SMCR |=  (3 << 0);

    //CC1S = 01
    TIM2->CCMR1 &= ~(3 << 0);
    TIM2->CCMR1 |= (1 << 0);
    // CC2S = 01
    TIM2->CCMR1 &= ~(3 << 8);
    TIM2->CCMR1 |= (1 << 8);



    // thiet lap gia tri auto-reload la max -> tranh tran xung
    TIM2->ARR = 0xFFFFFFFF;
    TIM2->CNT = 0;

    // kich hoat timer2
    TIM2->CR1 |= (1 << 0);
}

int32_t Encoder_GetDelta(void) {
    static uint32_t last_cnt = 0;          // Bien static de luu gia tri lan doc truoc
    uint32_t current_cnt = TIM2->CNT;      // Doc gia tri timer hien tai

    // tinh do lech (delta) giua 2 lan doc
    // ep kieu int32_t se tu dong xu ly dung ke ca khi current_cnt bi tran
    int32_t delta = (int32_t)(current_cnt - last_cnt);

    // Luu lai gia tri cho lan goi ham tiep theo
    last_cnt = current_cnt;

    return delta;
}
