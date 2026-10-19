#include "motor_driver.hpp"

void motor_init(void){
    motor_pwm_init();
}

//always high on pwm vlaue is 800 determined by TIM1->ARR = 799 & TIM2->ARR = 799;
void motor_forward(uint32_t speed){
    if(speed > 800){
        speed = 800;
    }
    TIM2->CCR3 = 800;          // PA2: Left IN1
    TIM2->CCR4 = 800-speed;    // PA3: Left IN2
    TIM1->CCR1 = 800;          // PA8: Right IN1
    TIM1->CCR2 = 800-speed;    // PA9: Right IN2
}

void motor_backward(uint32_t speed){
    if(speed > 800){
        speed = 800;
    }
    TIM2->CCR3 = 800-speed;          // PA2: Left IN1
    TIM2->CCR4 = 800;                // PA3: Left IN2
    TIM1->CCR1 = 800-speed;          // PA8: Right IN1
    TIM1->CCR2 = 800;                // PA9: Right IN2
}

void motor_brake(){
    TIM2->CCR3 = 800;          // PA2: Left IN1
    TIM2->CCR4 = 800;                // PA3: Left IN2
    TIM1->CCR1 = 800;          // PA8: Right IN1
    TIM1->CCR2 = 800;                // PA9: Right IN2
}

void motor_coast(){
    TIM2->CCR3 = 0;          // PA2: Left IN1
    TIM2->CCR4 = 0;                // PA3: Left IN2
    TIM1->CCR1 = 0;          // PA8: Right IN1
    TIM1->CCR2 = 0;                // PA9: Right IN2    
}

void motor_right(uint32_t speed){
    if(speed > 800){
        speed = 800;
    }
    TIM2->CCR3 = 800;          // PA2: Left IN1
    TIM2->CCR4 = 800-speed;    // PA3: Left IN2
    TIM1->CCR1 = 800-speed;          // PA8: Right IN1
    TIM1->CCR2 = 800;                // PA9: Right IN2
}

void motor_left(uint32_t speed){
    if(speed > 800){
        speed = 800;
    }
    TIM2->CCR3 = 800-speed;          // PA2: Left IN1
    TIM2->CCR4 = 800;                // PA3: Left IN2
    TIM1->CCR1 = 800;                // PA8: Right IN1
    TIM1->CCR2 = 800-speed;          // PA9: Right IN2
}
