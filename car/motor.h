#ifndef MOTOR_H
#define MOTOR_H

void init_motor();

void left_front_go(int pwm);
void left_front_back(int pwm);
void left_back_go(int pwm);
void left_back_back(int pwm);
void right_front_go(int pwm);
void right_front_back(int pwm);
void right_back_go(int pwm);
void right_back_back(int pwm);

//差速转弯 前进转弯
void front_turn(int left_pwm, int right_pwm);
//差速转弯 后退转弯
void back_turn(int left_pwm, int right_pwm);
//坦克 左调头
void left_tank_turn(int pwm);
//坦克 右调头
void right_tank_turn(int pwm);

#endif