#include "Arduino.h"
#include "motor.h"
// 前后四电机
#define LEFT_FRONT_MOTOR_1 12
#define LEFT_FRONT_MOTOR_2 13
#define RIGHT_FRONT_MOTOR_1 32
#define RIGHT_FRONT_MOTOR_2 33
#define LEFT_BACK_MOTOR_1 25
#define LEFT_BACK_MOTOR_2 26
#define RIGHT_BACK_MOTOR_1 23
#define RIGHT_BACK_MOTOR_2 22

#define PWM_Res 8
#define PWM_Freq 1000
#define LF_CH_1 1
#define LF_CH_2 2
#define RF_CH_1 3
#define RF_CH_2 4
#define LB_CH_1 5
#define LB_CH_2 6
#define RB_CH_1 7
#define RB_CH_2 8

void init_motor() {
  ledcSetup(LF_CH_1, PWM_Freq, PWM_Res);  // 5 kHz PWM, 8-bit resolution
  ledcSetup(LF_CH_2, PWM_Freq, PWM_Res);
  ledcSetup(RF_CH_1, PWM_Freq, PWM_Res);
  ledcSetup(RF_CH_2, PWM_Freq, PWM_Res);
  ledcSetup(LB_CH_1, PWM_Freq, PWM_Res);
  ledcSetup(LB_CH_2, PWM_Freq, PWM_Res);
  ledcSetup(RB_CH_1, PWM_Freq, PWM_Res);
  ledcSetup(RB_CH_2, PWM_Freq, PWM_Res);

  ledcAttachPin(LEFT_FRONT_MOTOR_1, LF_CH_1);
  ledcAttachPin(LEFT_FRONT_MOTOR_2, LF_CH_2);
  ledcAttachPin(RIGHT_FRONT_MOTOR_1, RF_CH_1);
  ledcAttachPin(RIGHT_FRONT_MOTOR_2, RF_CH_2);
  ledcAttachPin(LEFT_BACK_MOTOR_1, LB_CH_1);
  ledcAttachPin(LEFT_BACK_MOTOR_2, LB_CH_2);
  ledcAttachPin(RIGHT_BACK_MOTOR_1, RB_CH_1);
  ledcAttachPin(RIGHT_BACK_MOTOR_2, RB_CH_2);
}

void front_turn(int left_pwm, int right_pwm) {
  left_front_go(left_pwm);
  left_back_go(left_pwm);
  right_front_go(right_pwm);
  right_back_go(right_pwm);
}
void back_turn(int left_pwm, int right_pwm) {
  left_front_back(left_pwm);
  left_back_back(left_pwm);
  right_front_back(right_pwm);
  right_back_back(right_pwm);
}
void left_tank_turn(int pwm) {
  left_front_back(pwm);
  left_back_back(pwm);
  right_front_go(pwm);
  right_back_go(pwm);
}
void right_tank_turn(int pwm) {
  left_front_go(pwm);
  left_back_go(pwm);
  right_front_back(pwm);
  right_back_back(pwm);
}


void left_front_go(int pwm) {
  ledcWrite(LF_CH_1, pwm);
  ledcWrite(LF_CH_2, 0);
}
void left_front_back(int pwm) {
  ledcWrite(LF_CH_1, 0);
  ledcWrite(LF_CH_2, pwm);
}
void left_back_go(int pwm) {
  ledcWrite(LB_CH_1, pwm);
  ledcWrite(LB_CH_2, 0);
}
void left_back_back(int pwm) {
  ledcWrite(LB_CH_1, 0);
  ledcWrite(LB_CH_2, pwm);
}

void right_front_go(int pwm) {
  ledcWrite(RF_CH_1, pwm);
  ledcWrite(RF_CH_2, 0);
}
void right_front_back(int pwm) {
  ledcWrite(RF_CH_1, 0);
  ledcWrite(RF_CH_2, pwm);
}
void right_back_go(int pwm) {
  ledcWrite(RB_CH_1, pwm);
  ledcWrite(RB_CH_2, 0);
}
void right_back_back(int pwm) {
  ledcWrite(RB_CH_1, 0);
  ledcWrite(RB_CH_2, pwm);
}