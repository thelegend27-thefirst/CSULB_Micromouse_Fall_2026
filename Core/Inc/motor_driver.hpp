#pragma once
#include "main.h"
#include <stdint.h>
#include "motor_pwm.h"
#include "cmsis_os2.h"

//for speed pwm use brake not coast
void motor_init(void);

void motor_forward(uint32_t speed);

void motor_backward(uint32_t speed);

void motor_coast();

void motor_brake();

void motor_right(uint32_t speed);

void motor_left(uint32_t speed);

/*
A strategy we could possibly implement is to use 
the encoders to verify RPM and create a feedback
loop to dynamically adjust PWM to get consistant RPM
*/