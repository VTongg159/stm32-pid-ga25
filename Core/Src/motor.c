#include "motor.h"

void Motor_Init(void) {
    // cap xung clock cho GPIOA va timer1
	RCC->AHB1ENR |= (1 << 0);
	RCC->APB2ENR |= (1 << 0);

    // cau hinh PA8 o che do alternate
	GPIOA->MODER &= ~(1<<16);
	GPIOA->MODER |= (1 << 17);
	GPIOA->AFR[1] &= ~(0xF << 0);
	GPIOA->AFR[1] |= (1 << 0);    //AF1:0001


    // cau hinh output cho PA9 va PA10

	// PA9
	GPIOA->MODER |= (1 << 18);
	GPIOA->MODER &= ~(1 << 19);
	//PA10
	GPIOA->MODER |= (1 << 20);
	GPIOA->MODER &= ~(1 << 21);


    // cau hinh timer1 xuat xung pwm tan so 10khz
    TIM1->PSC = 9;
    TIM1->ARR = 999;   // f = 10MHz / (999 + 1) = 10kHz
    TIM1->CCR1 = 0;

    //cau hinh output compare mode cho channel 1
    TIM1->CCMR1 &= ~(0x7 << 4);
    // pwm1: 110
    TIM1->CCMR1 &= ~(1 << 4);
    TIM1->CCMR1 |= (1 << 5);
    TIM1->CCMR1 |= (1 << 6);

    //bat bo dem Preload cho CCR1
    TIM1->CCMR1 |= (1 << 3);

    // cho phep xuat dau ra  channel 1 (CCER)
    TIM1->CCER |= (1 << 0);

    //bat ngo ra chinh (MOE - Main Output Enable)
    TIM1->BDTR |= (1 << 15);

    // Auto-reload preload
  	TIM1->CR1 |= (1 << 7);

    // counter enable
    TIM1->CR1 |= (1 << 0);

    //force update event
    TIM1->EGR |= (1 << 0);
}

void Motor_SetSpeed(int16_t speed) {
    // gioi han bam xung pwm trong khoang ARR
    if (speed > 999)  speed = 999;
    if (speed < -999) speed = -999;

    if (speed >= 0) {
        // Quay thuan: IN1 = 1, IN2 = 0
        GPIOA->ODR |=  (1 << 9);
        GPIOA->ODR &= ~(1 << 10);
        TIM1->CCR1 = speed;
    } else {
        // Quay nghich: IN1 = 0, IN2 = 1
        GPIOA->ODR &= ~(1 << 9);
        GPIOA->ODR |=  (1 << 10);
        TIM1->CCR1 = -speed;
    }
}

void Motor_Brake(void) {
    // dung khan cap: IN1 = 0, IN2 = 0
    GPIOA->ODR &= ~(1 << 9);
    GPIOA->ODR &= ~(1 << 10);
    TIM1->CCR1 = 0;
}
