#ifndef PID_H
#define PID_H

typedef struct {
    float Kp, Ki, Kd;
    float setpoint;
    float integral_sum;
    float prev_error;
    float out_max, out_min;
} PID_Controller_t;

void PID_Init(PID_Controller_t *pid, float p, float i, float d, float max, float min);
float PID_Compute(PID_Controller_t *pid, float current_value, float dt);

#endif
