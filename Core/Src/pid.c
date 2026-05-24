#include "pid.h"

// khoi tao tham so pid
void PID_Init(PID_Controller_t *pid, float p, float i, float d, float max, float min)
{
    pid->Kp = p;
    pid->Ki = i;
    pid->Kd = d;
    pid->out_max = max;
    pid->out_min = min;
    pid->setpoint = 0.0f;
    pid->integral_sum = 0.0f;
    pid->prev_error = 0.0f;
}

// ham tinh toan pid
float PID_Compute(PID_Controller_t *pid, float current_value, float dt)
{
    if(dt <= 0.0f) return 0.0f;

    float error = pid->setpoint - current_value;

    // P
    float Proportional = pid->Kp * error;

    // I
    pid->integral_sum += error * dt;
    float Integral = pid->Ki * pid->integral_sum;

    // Anti-windup
    if(Integral > pid->out_max)
    {
        Integral = pid->out_max;

        if(pid->Ki != 0.0f)
        {
            pid->integral_sum = pid->out_max / pid->Ki;
        }
    }

    else if(Integral < pid->out_min)
    {
        Integral = pid->out_min;

        if(pid->Ki != 0.0f)
        {
            pid->integral_sum = pid->out_min / pid->Ki;
        }
    }

    // D
    float Derivative = pid->Kd * ((error - pid->prev_error) / dt);

    // PID output
    float output = Proportional + Integral + Derivative;

    // gioi han output pid
    if(output > pid->out_max) output = pid->out_max;
    if(output < pid->out_min) output = pid->out_min;

    // Update previous error
    pid->prev_error = error;
    return output;
}
