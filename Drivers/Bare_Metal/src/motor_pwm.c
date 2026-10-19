#include "motor_pwm.h"

void motor_pwm_init(void) {
    // Enable GPIOA, TIM2 and TIM1 clocks
    RCC->AHB1ENR |= 0x00000001;
    RCC->APB1ENR |= 0x00000001;
    RCC->APB2ENR |= 0x00000001;

    // PA2, PA3, PA8, PA9: Alternate function
    GPIOA->MODER &= ~0x000F00F0;
    GPIOA->MODER |=  0x000A00A0;

    // AF1: TIM1 / TIM2
    GPIOA->AFR[0] &= ~0x0000FF00;
    GPIOA->AFR[0] |=  0x00001100;

    GPIOA->AFR[1] &= ~0x000000FF;
    GPIOA->AFR[1] |=  0x00000011;

    // LEFT MOTOR: TIM2, channels 3 and 4
    TIM2->PSC = 0;
    TIM2->ARR = 799;
    TIM2->CCR3 = 0;
    TIM2->CCR4 = 0;

    TIM2->CCMR2 = 0x6868;  // PWM mode 1, preload
    TIM2->CCER |= 0x1100;  // Enable CH3, CH4
    TIM2->EGR = 0x0001;
    TIM2->CR1 |= 0x0081;   // Start timer

    // RIGHT MOTOR: TIM1, channels 1 and 2
    TIM1->PSC = 0;
    TIM1->ARR = 799;
    TIM1->CCR1 = 0;
    TIM1->CCR2 = 0;

    TIM1->CCMR1 = 0x6868;  // PWM mode 1, preload
    TIM1->CCER |= 0x0011;  // Enable CH1, CH2
    TIM1->BDTR |= 0x8000;  // Main output enable
    TIM1->EGR = 0x0001;
    TIM1->CR1 |= 0x0081;   // Start timer
}
