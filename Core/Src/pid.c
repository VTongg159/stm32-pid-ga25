#include "pid.h"

void PID_Init(PID_Controller_t *pid, float p, float i, float d, float max, float min) {
    pid->Kp = p;
    pid->Ki = i;
    pid->Kd = d;
    pid->out_max = max;
    pid->out_min = min;
    pid->setpoint = 0.0f;
    pid->integral_sum = 0.0f;
    pid->prev_error = 0.0f;
}

float PID_Compute(PID_Controller_t *pid, float current_value, float dt) {
    // 1. Tính toán sai số hiện tại (Error)
    float error = pid->setpoint - current_value;

    // 2. Thành phần Tỷ lệ (Proportional)
    float p_term = pid->Kp * error;

    // 3. Thành phần Tích phân (Integral) kết hợp tích lũy
    pid->integral_sum += error * dt;
    float i_term = pid->Ki * pid->integral_sum;

    // --- Giải thuật ANTI-WINDUP (Chống bão hòa tích phân) ---
    if (i_term > pid->out_max) {
        i_term = pid->out_max;
        pid->integral_sum = pid->out_max / pid->Ki; // Giới hạn ngược bộ nhớ tích phân
    } else if (i_term < pid->out_min) {
        i_term = pid->out_min;
        pid->integral_sum = pid->out_min / pid->Ki;
    }

    // 4. Thành phần Vi phân (Derivative)
    float d_term = pid->Kd * ((error - pid->prev_error) / dt);

    // 5. Tổng hợp tín hiệu điều khiển đầu ra
    float output = p_term + i_term + d_term;

    // 6. Giới hạn cơ cấu chấp hành đầu ra tổng thể (Khống chế PWM)
    if (output > pid->out_max) output = pid->out_max;
    if (output < pid->out_min) output = pid->out_min;

    // 7. Lưu lại sai số cho chu kỳ sau
    pid->prev_error = error;

    return output;
}
